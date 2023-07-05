/* Wei Yan, parser.c, CS 24000, Fall 2020
 * Last updated October 12, 2020
 */

/* Add any includes here */

#include "parser.h"

#include <stdio.h>
#include <assert.h>
#include <malloc.h>
#include <string.h>


midi_event_t g_previous_midi = {0};

/* This function takes in the path of
 * a MIDI file and returns the parsed
 * representation of that
 * song
 */

song_data_t *parse_file(const char* file_name){
  g_previous_midi.status = 0;
  assert(file_name != NULL);
  FILE *my_file = fopen(file_name, "r");
  assert(my_file != NULL);
  song_data_t *output = malloc(sizeof(song_data_t));
  assert(output != NULL);
  output->track_list = NULL;
  output->path = malloc(sizeof(char) * (strlen(file_name) + 1));
  assert(output->path != NULL);
  strcpy(output->path, file_name);
  int file_length = 0;
  fseek(my_file, 0, SEEK_END);
  file_length = ftell(my_file);
  fseek(my_file, 0, SEEK_SET);
  parse_header(my_file, output);
  parse_track(my_file, output);
  assert(file_length == ftell(my_file));
  fclose(my_file);
  my_file = NULL;
  return output;
} /* parse_file() */

/* This function should read a MIDI
 * header chunk from the given file
 * pointer and update the pointer
 */

void parse_header(FILE *input_file, song_data_t *input_data){
  int scan_temp = 0;
  char type_input[4] = {0};
  uint32_t length_input = 0;
  uint16_t format_input = 0;
  uint16_t ntrks_input = 0;
  uint16_t division_input = 0;

  /* read input and check for assertion */

  scan_temp = fread(&type_input, sizeof(char), 4, input_file);
  assert(scan_temp == 4);
  scan_temp = fread(&length_input, sizeof(uint32_t) , 1, input_file);
  assert(scan_temp == 1);
  scan_temp = fread(&format_input, sizeof(uint16_t), 1, input_file);
  assert(scan_temp == 1);
  scan_temp = fread(&ntrks_input, sizeof(uint16_t), 1, input_file);
  assert(scan_temp == 1);
  scan_temp = fread(&division_input, sizeof(uint16_t), 1, input_file);
  assert(scan_temp == 1);
  assert(sizeof(ntrks_input == sizeof(uint16_t)));
  assert(strcmp(type_input, "MThd") == 0);
  length_input = end_swap_32((uint8_t *) &length_input);
  assert(length_input == 0x6);
  format_input = end_swap_16((uint8_t *) &format_input);
  assert(format_input <= 0x02);
  input_data->format = format_input;
  input_data->num_tracks = end_swap_16((uint8_t *) &ntrks_input);
  assert(input_data->num_tracks < 32768);
  division_input = end_swap_16((uint8_t *) &division_input);
  assert(division_input < 65536);
  if ((1 << 15) & division_input){
    input_data->division.uses_tpq = false;
    division_input = division_input >> 1;
    input_data->division.ticks_per_frame = (uint8_t) division_input;
    input_data->division.frames_per_sec = (uint8_t) division_input >> 8;
  }
  if (((uint8_t)division_input << 7) != 128){
    input_data->division.uses_tpq = true;
    input_data->division.ticks_per_qtr = division_input;
  }
} /* parse_header() */


/* This function read a MIDI track
 * chunk from the given file pointer
 * and update the track
 */

void parse_track(FILE * input_file, song_data_t *input_data){
  input_data->track_list = malloc(sizeof(track_node_t));
  assert(input_data->track_list != NULL);
  track_node_t *track_head = input_data->track_list;
  track_node_t *tracker_temp = {0};
  int scan_temp = 0;
  int tracks_amount = input_data->num_tracks;

  /* loop for num of tracks, read all in once */

  for (int i = 0; i < tracks_amount; i++) {
    track_head->next_track = NULL;
    char type_input[4] = {0};
    scan_temp = fread(&type_input, sizeof(char), 4, input_file);
    assert(scan_temp == 4);
    assert(!strcmp(type_input, "MTrk"));
    track_head->track = malloc(sizeof(track_t));
    assert(track_head->track != NULL);
    uint32_t length_input = 0;
    scan_temp = fread(&length_input, sizeof(uint32_t), 1, input_file);
    assert(scan_temp == 1);
    track_head->track->length = end_swap_32((uint8_t *)&length_input);
    track_head->track->event_list = malloc(sizeof(event_node_t));
    assert(track_head->track->event_list != NULL);
    event_node_t *event_head = NULL;
    event_node_t *event_temp = NULL;
    int byte_num_before = ftell(input_file);
    int byte_num_after = 0;
    int count = 0;
    event_head = track_head->track->event_list;
    assert(event_head != NULL);
    while (1){
      byte_num_after = ftell(input_file);
      if ((byte_num_after - byte_num_before) == track_head->track->length) {
        break;
      }
      if (count != 0) {
        track_head->track->event_list->next_event =
        malloc(sizeof(event_node_t));
        track_head->track->event_list =
        track_head->track->event_list->next_event;
      }
      count++;
      track_head->track->event_list->next_event = NULL;
      track_head->track->event_list->event = parse_event(input_file);
    }
    track_head->track->event_list = event_head;


    if ( i != (tracks_amount - 1)) {
      track_head->next_track = malloc(sizeof(track_node_t));
      tracker_temp = track_head;
      track_head = track_head->next_track;
    }
  }
  track_head->next_track = NULL;
} /* parse_track() */



/* This function should read and
 * return a pointer to an event_t
 * struct from the given MIDI file
 * pointer
 */

event_t *parse_event(FILE *input_file){
  event_t *output = malloc(sizeof(event_t));
  assert(output != NULL);
  output->delta_time = parse_var_len(input_file);
  uint8_t type_temp = 0;
  int scan_temp = 0;
  scan_temp = fread(&type_temp, sizeof(uint8_t), 1, input_file);
  assert(scan_temp == 1);
  if (type_temp == SYS_EVENT_1) {
    output->sys_event = parse_sys_event(input_file);
    output->type = SYS_EVENT_1;
  }
  else if (type_temp == SYS_EVENT_2){
    output->type = SYS_EVENT_2;
    output->sys_event = parse_sys_event(input_file);
  }
  else if (type_temp == META_EVENT) {
    output->type = META_EVENT;
    output->meta_event = parse_meta_event(input_file);
  }
  else {

    /* updata global variable when last one is midi */

    output->type = MIDI_EVENT_T;
    output->midi_event = parse_midi_event(input_file, type_temp);
    g_previous_midi = output->midi_event;
  }
  return output;
} /* parse_event() */


/* This function read and return
 * a sys_event_t struct
 */

sys_event_t parse_sys_event(FILE *input_file){
  sys_event_t output = {0};
  output.data_len = parse_var_len(input_file);
  int scan_temp = 0;
  if (output.data_len != 0) {
    output.data = malloc(output.data_len);
    assert(output.data != NULL);
    scan_temp = fread(output.data, sizeof(char), output.data_len, input_file);
    assert(scan_temp == output.data_len);
  }
  return output;
} /* parse_sys_event() */


/* This function read and return a
 * meta_event_t struct
 */

meta_event_t parse_meta_event(FILE *input_file){
  meta_event_t output = {0};
  int scan_temp = 0;
  uint8_t input_name = 0;
  scan_temp = fread(&input_name, sizeof(uint8_t), 1, input_file);
  assert(scan_temp == 1);
  assert(0 < input_name < 255);
  assert(META_TABLE[input_name].name != NULL);
  output.name = META_TABLE[input_name].name;
  output.data_len = parse_var_len(input_file);
  if (output.data_len > 0) {
    assert((output.data_len == META_TABLE[input_name].data_len) ||
          (META_TABLE[input_name].data_len == 0));
    output.data = malloc(output.data_len);
    scan_temp = fread(output.data, sizeof(char), output.data_len, input_file);
    assert(scan_temp == output.data_len);
  }
  return output;
} /* parse_meta_event() */


/* This function should read and
 * return a midi_event_t struct
 */

midi_event_t parse_midi_event(FILE *input_file, uint8_t input_status){
  midi_event_t output = {0};
  int scan_temp = 0;
  if ((input_status >> 7) != 1){
    output.status = g_previous_midi.status;
    output.name = MIDI_TABLE[output.status].name;
    assert(output.name != NULL);
    output.data_len = MIDI_TABLE[output.status].data_len;
    if (output.data_len > 0) {
      output.data = malloc(output.data_len);
      *output.data = input_status;
      scan_temp = fread(output.data + 1,
        sizeof(char), output.data_len - 1, input_file);
      scan_temp += 1;
      assert(scan_temp == output.data_len);
    }
    return output;
  }
  else {
    output.status = input_status;
    output.name = MIDI_TABLE[output.status].name;
    assert(output.name != NULL);
    output.data_len = MIDI_TABLE[output.status].data_len;
    scan_temp = 0;
    if (output.data_len > 0) {
      output.data = malloc(output.data_len);
      scan_temp = fread(output.data, sizeof(char), output.data_len, input_file);
      assert(scan_temp == output.data_len);
    }
    return output;
  }
} /* parse_midi_event() */


/* This function should read a variable
 * length integer from the given MIDI file
 * pointer and return it as a fixed-size
 * uint32_t.
 */

uint32_t parse_var_len(FILE *input_file){
  assert(input_file != NULL);
  uint32_t sum = 0;
  uint8_t read_input = 0;
  int scan_temp = 0;
  int count = 0;
  while (1){
    scan_temp = fread(&read_input, sizeof(char), 1, input_file);
    assert(scan_temp == 1);
    sum = (sum << 7) | (uint64_t)(read_input & 127);
    if (!(read_input >> 7 & 1)){
      break;
    }
  }
  return sum;
} /* parse_var_len() */


/* This function takes in a pointer to an
 * event, and should return either type
 */

uint8_t event_type(event_t * input_event){
  if (input_event->type == SYS_EVENT_1){
    return SYS_EVENT_T;
  }
  else if (input_event->type == SYS_EVENT_2) {
    return SYS_EVENT_T;
  }
  else if (input_event->type == META_EVENT) {
    return META_EVENT_T;
  }
  else {
    return MIDI_EVENT_T;
  }
} /* event_type() */


/* This function should free all memory
 * associated with the given song
 */

void free_song(song_data_t *song){
  free(song->path);
  song->path = NULL;
  track_node_t *head = song->track_list;
  track_node_t *temp_node = NULL;
  for (int i = 0; i < song->num_tracks; i++){
    temp_node = head;
    head = head->next_track;
    free_track_node(temp_node);
    temp_node = NULL;
  }
  song->track_list = NULL;
  free(song);
  song = NULL;
} /* free_song() */


/* This function should free all memory
 * associated with the given track node
 */

void free_track_node(track_node_t * input_track){
  input_track->next_track = NULL;
  event_node_t *head = input_track->track->event_list;
  event_node_t *temp_node = NULL;
  while (head != NULL){
    temp_node = head;
    head = head->next_event;
    free_event_node(temp_node);
    temp_node = NULL;
  }
  free(input_track->track);
  input_track->track = NULL;
  free(input_track);
  input_track = NULL;
} /* free_track_node() */


/* This function should free all
 * memory associated with the given
 * event node
 */

void free_event_node(event_node_t * input_event){
  if (event_type(input_event->event) == 1) {
    if (input_event->event->sys_event.data_len != 0){
      free(input_event->event->sys_event.data);
      input_event->event->sys_event.data = NULL;
    }
  }
  else if (event_type(input_event->event) == 2){
    if (input_event->event->meta_event.data_len != 0){
      free(input_event->event->meta_event.data);
      input_event->event->meta_event.data = NULL;
    }
  }
  else {
    if (input_event->event->midi_event.data_len != 0) {
      free(input_event->event->midi_event.data);
      input_event->event->midi_event.data = NULL;
    }
  }
  free(input_event->event);
  input_event->event = NULL;
  free(input_event);
  input_event = NULL;
} /* free_event_node() */


/* This function takes in a buffer of two
 * uint8_ts representing a uint16_t, and
 * should return a uint16_t with opposite
 * endianness
 */

uint16_t end_swap_16(uint8_t old[2]){
  uint16_t new= 0;
  new = ((uint16_t)old[0] << 8) | old[1];
  return new;
} /* end_swap_16() */


/* This function takes in a buffer of
 * four uint8_ts representing a uint32_t
 * and should return a uint32_t with opposite
 * endianness
 */

uint32_t end_swap_32(uint8_t old[4]){
  uint16_t new_1 = 0;
  uint16_t new_2 = 0;
  uint32_t final = 0;
  new_1 = ((uint16_t) old[0] << 8) | old[1];
  new_2 = ((uint16_t) old[2] << 8) | old[3];
  final = ((uint32_t) new_1 << 16) | new_2;
  return final;
} /* end_swap_32() */
