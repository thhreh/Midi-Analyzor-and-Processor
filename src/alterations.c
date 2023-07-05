/* Name, alterations.c, CS 24000, Fall 2020
 * Last updated October 12, 2020
 */

/* Add any includes here */

#include "alterations.h"

#include <string.h>
#include <assert.h>
#include <malloc.h>
#include <stdlib.h>


/* This function takes in an event pointer
 * and a pointer to a number of octaves
 */

int change_event_octave(event_t * song_event, int * input_octave){
  if (event_type(song_event) == MIDI_EVENT_T){

    /* find midi with given assumptions */

    if ((strcmp(song_event->midi_event.name, "Note On") == 0) ||
      (strcmp(song_event->midi_event.name, "Note Off") == 0) ||
      (strcmp(song_event->midi_event.name, "Polyphonic Key") == 0)){
      int boundary_check = song_event->midi_event.data[0] +
      (*input_octave * OCTAVE_STEP);
      if ((0 <= boundary_check) && (boundary_check <= 127)) {
        song_event->midi_event.data[0] += (*input_octave * OCTAVE_STEP);
        return 1;
      }
    }
  }
  return 0;
} /* change_event_octave() */

/* This function takes in an event pointer
 * and a pointer to a float multiplier
 */

int change_event_time(event_t * song_event, float * input_multiplier) {
  if (*input_multiplier == 1){
    return 0;
  }
  assert((*input_multiplier) > 0);
  uint32_t old_time = song_event->delta_time;
  song_event->delta_time = song_event->delta_time * (*input_multiplier);
  int old_bits = 0;
  int new_bits = 0;

  /* find num of bytes for both */

  if (song_event->delta_time > 2097151) {
    new_bits = 4;
  }
  if ((song_event->delta_time <= 2097151) && (song_event->delta_time > 16383)) {
    new_bits = 3;
  }
  if ((song_event->delta_time <= 16383) && (song_event->delta_time > 127)) {
    new_bits = 2;
  }
  if ((song_event->delta_time <= 127)){
    new_bits = 1;
  }
  if (old_time > 2097151){
    old_bits = 4;
  }
  if ((old_time <= 2097151) && (old_time > 16383)){
    old_bits = 3;
  }
  if ((old_time <= 16383) && (old_time > 127)) {
    old_bits = 2;
  }
  if ((old_time <= 127)){
    old_bits = 1;
  }
  return new_bits - old_bits;
} /* change_event_time() */

/* This function takes in an event
 * pointer and a table mapping from current instruments to the
 * desired new instruments
 */

int change_event_instrument(event_t * song_event, remapping_t input_map) {
  if (event_type(song_event) == MIDI_EVENT_T){
    if (strcmp(song_event->midi_event.name, "Program Change") == 0) {
      song_event->midi_event.data[0] =
      input_map[song_event->midi_event.data[0]];
      return 1;
    }
  }
  return 0;
} /* change_event_instrument() */

/* This function takes in an event pointer
 * and a table mapping from current notes
 * to the desired new notes
 */

int change_event_note(event_t * song_event, remapping_t input_map) {
  if (event_type(song_event) == MIDI_EVENT_T){

    /* find event with given assumptions */

    if ((strcmp(song_event->midi_event.name, "Note On") == 0) ||
      (strcmp(song_event->midi_event.name, "Note Off") == 0) ||
      (strcmp(song_event->midi_event.name, "Polyphonic Key") == 0)){
      song_event->midi_event.data[0] =
      input_map[song_event->midi_event.data[0]];
      return 1;
    }
  }
  return 0;
} /* change_event_note() */


/* This function takes in a song, a function pointer
 * and a piece of arbitrary data, and applies
 * that function with that piece of data to
 * every event in the given song
 */

int apply_to_events(song_data_t * song_event,
event_func_t input_func, void * data){
  int output_sum = 0;
  track_node_t *track_head = song_event->track_list;
  while (track_head != NULL){
    event_node_t *event_head = track_head->track->event_list;
    while (event_head != NULL){
      output_sum += input_func(event_head->event, data);
      event_head = event_head->next_event;
    }
    track_head = track_head->next_track;
  }
  return output_sum;
} /* apply_to_events() */

/* This function takes in a song and an integer number
 * of octaves. Each note in the song should
 * have its octave shifted by the given number of octaves.
 */

int change_octave(song_data_t * song_event, int input_octave) {
  int output_sum = apply_to_events(song_event,
  (event_func_t) change_event_octave, (void*) &input_octave);
  return output_sum;
} /* change_octave() */

/* This function takes in a song and a
 * float multiplier
 */

int warp_time(song_data_t * song_event, float input_float) {
  int output_sum = 0;
  track_node_t *track_head = song_event->track_list;
  event_node_t *event_head = {0};
  while (track_head != NULL){
    int track_sum = 0;
    event_head = track_head->track->event_list;
    while (event_head != NULL){
      track_sum += change_event_time(event_head->event, &input_float);
      event_head = event_head->next_event;
    }
    output_sum += track_sum;
    track_head->track->length += track_sum;
    track_head = track_head->next_track;
  }
  return output_sum;
} /* warp_time() */


/* This function takes in a song
 * and a table mapping from current
 * instruments to desired new
 * instruments
 */

int remap_instruments(song_data_t * song_event, remapping_t input_map){
  int output_sum = apply_to_events(song_event,
  (event_func_t) change_event_instrument, (void*) &input_map);
  return output_sum;
} /* remap_instruments() */


/* This function takes in a song and a
 * table mapping from current notes to new notes
 */

int remap_notes(song_data_t *song_event , remapping_t instrument_table){
  int output_sum = apply_to_events(song_event,
  (event_func_t) change_event_note, (void*) &instrument_table);
  return output_sum;
} /* remap_notes() */

/* This function takes in a song, a track index (int)
 * an octave differential (int), a time delay
 * (unsigned int), and an instrument (represented as a
 * uint8 t per appendix 1.4 of the website)
 * and uses them to turn the song into a round of sorts
 */

void add_round(song_data_t * song_event, int track_index, int input_octave,
  unsigned int input_delay, uint8_t input_instrument){

  assert(track_index < song_event->num_tracks);
  assert(song_event->format != 2);

  /* this empty track recieve information */

  track_node_t *added_node = malloc(sizeof(track_node_t));

  assert(added_node != NULL);
  added_node->next_track = NULL;
  added_node->track = malloc(sizeof(track_t));
  assert(added_node->track != NULL);
  track_node_t* track_found = song_event->track_list;
  track_node_t* channel_track = song_event->track_list;
  int channel[16] = {0};
  for (int i = 0; i < 16; i++) {
    channel[i] = 0;
  }
  int total_track = song_event->num_tracks;

  /* find channel that didnt use */

  for (int i = 0; i < total_track; i++){
    event_node_t *event_list_a = channel_track->track->event_list;
    while (event_list_a != NULL) {
      if (event_type(event_list_a->event) == MIDI_EVENT_T){
        int temp_channel = event_list_a->event->midi_event.status & (0x0F);
        channel[temp_channel] = 1;
      }
      event_list_a = event_list_a->next_event;
    }
    channel_track = channel_track->next_track;
  }
  int smallest_channel = -1;
  for (int i = 0; i < 16; i++) {
    if (channel[i] == 0) {
      smallest_channel = i;
      break;
    }
  }
  assert(smallest_channel != -1);
  if ( track_index != 0) {

    /* find the track we want */

    for (int i = 0; i < track_index; i++) {
      track_found = track_found->next_track;
      assert(track_found->track->event_list != NULL);
    }
  }
  assert(added_node->track->event_list != NULL);
  remapping_t map_table = {0};
  for (int i = 0; i <= 0xFF; i++) {
    map_table[i] = input_instrument;
  }
  assert(track_found->track->event_list != NULL);

  /* add in first meta event */

  added_node->track->length = track_found->track->length;
  event_node_t *head_event_added = malloc(sizeof(event_node_t));
  added_node->track->event_list = head_event_added;
  head_event_added->next_event = NULL;
  event_node_t *event_list = track_found->track->event_list;
  assert(event_list->next_event != NULL);
  head_event_added->event = malloc(sizeof(event_t));
  assert(head_event_added->event != NULL);
  assert(event_type(event_list->event) == 2);
  head_event_added->event->type = event_list->event->type;
  head_event_added->event->delta_time = event_list->event->delta_time;
  head_event_added->event->meta_event.name =
  event_list->event->meta_event.name;
  head_event_added->event->meta_event.data_len =
  event_list->event->meta_event.data_len;
  if (head_event_added->event->meta_event.data_len != 0){
    head_event_added->event->meta_event.data =
      malloc( head_event_added->event->meta_event.data_len);
    assert(head_event_added->event->meta_event.data != 0);
    for (int i = 0; i < head_event_added->event->meta_event.data_len; i++) {
      head_event_added->event->meta_event.data[i] =
        event_list->event->meta_event.data[i];
    }
  }
  event_node_t *temp_new_head = head_event_added;
  event_list = event_list->next_event;

  while (event_list != NULL) {

    /* this note is the next node of head */

    event_node_t *temp_next = malloc(sizeof(event_node_t));
    assert(temp_next != NULL);
    temp_next->next_event = NULL;
    temp_next->event = malloc(sizeof(event_t));
    assert(temp_next->event != NULL);
    assert(temp_next->event != NULL);
    if (event_type(event_list->event) == META_EVENT_T){
      temp_next->event->type = event_list->event->type;
      temp_next->event->delta_time = event_list->event->delta_time;
      temp_next->event->meta_event.name
      = event_list->event->meta_event.name;
      temp_next->event->meta_event.data_len
      = event_list->event->meta_event.data_len;
      if ( temp_next->event->meta_event.data_len != 0){
        temp_next->event->meta_event.data
        = malloc(temp_next->event->meta_event.data_len);
        assert(temp_next->event->meta_event.data != NULL);
        for (int i = 0; i < temp_next->event->meta_event.data_len; i++) {
          temp_next->event->meta_event.data[i]
            = event_list->event->meta_event.data[i];
        }
      }
    }
    else if (event_type(event_list->event) == SYS_EVENT_T){
      temp_next->event->type = event_list->event->type;
      temp_next->event->delta_time = event_list->event->delta_time;
      temp_next->event->sys_event.data_len
      = event_list->event->sys_event.data_len;
      if (temp_next->event->sys_event.data_len != 0){
        temp_next->event->sys_event.data
        = malloc(temp_next->event->sys_event.data_len);
        assert(temp_next->event->sys_event.data != NULL);
        for (int i = 0; i < temp_next->event->sys_event.data_len; i++) {
          temp_next->event->sys_event.data[i]
          = event_list->event->sys_event.data[i];
        }
      }
    }
    else {
      temp_next->event->delta_time = event_list->event->delta_time;
      temp_next->event->midi_event.name
      = event_list->event->midi_event.name;
      temp_next->event->midi_event.status
      = event_list->event->midi_event.status;
      temp_next->event->midi_event.data_len
      = event_list->event->midi_event.data_len;
      if (temp_next->event->midi_event.data_len != 0) {
        temp_next->event->midi_event.data
        = malloc(temp_next->event->midi_event.data_len);
        assert(temp_next->event->midi_event.data != NULL);
        for (int i = 0; i < temp_next->event->midi_event.data_len; i++) {
          temp_next->event->midi_event.data[i]
            = event_list->event->midi_event.data[i];
        }
      }
      temp_next->event->midi_event.status =
        (temp_next->event->midi_event.status & 0xF0) |
        smallest_channel;
      temp_next->event->type = temp_next->event->midi_event.status;
      change_event_instrument(temp_next->event, map_table);
      change_event_octave(temp_next->event, &input_octave);
    }

    temp_new_head->next_event = temp_next;
    temp_new_head = temp_new_head->next_event;
    event_list = event_list->next_event;
  }
  track_node_t* end_of_track = track_found;

  /* find the end of the list */

  while (end_of_track->next_track != NULL){
    end_of_track = end_of_track->next_track;
  }
  assert(end_of_track != NULL);
  end_of_track->next_track = added_node;
  song_event->format = 1;
  song_event->num_tracks += 1;
  int old_delta_time = added_node->track->event_list->event->delta_time;
  added_node->track->event_list->event->delta_time += input_delay;
  int new_delta_time = added_node->track->event_list->event->delta_time;
  int old_num_bits = 0;
  int new_num_bits = 0;

  /*length operator */

  if (old_delta_time > 2097151){
    old_num_bits = 4;
  }
  if ((old_delta_time <= 2097151) && (old_delta_time > 16383)){
    old_num_bits = 3;
  }
  if ((old_delta_time <= 16383) && (old_delta_time > 127)){
    old_num_bits = 2;
  }
  if (old_delta_time <= 127){
    old_num_bits = 1;
  }
  if (new_delta_time > 2097151){
    new_num_bits = 4;
  }
  if ((new_delta_time <= 2097151) && (new_delta_time > 16383)){
    new_num_bits = 3;
  }
  if ((new_delta_time <= 2097151) && (new_delta_time > 16383)){
    new_num_bits = 2;
  }
  if (new_delta_time <= 127){
    new_num_bits = 1;
  }
  added_node->track->length += (new_num_bits - old_num_bits);
} /* add_round() */


/*
 * Function called prior to main that sets up random mapping tables
 */

void build_mapping_tables()
{
  for (int i = 0; i <= 0xFF; i++) {
    I_BRASS_BAND[i] = 61;
  }

  for (int i = 0; i <= 0xFF; i++) {
    I_HELICOPTER[i] = 125;
  }

  for (int i = 0; i <= 0xFF; i++) {
    N_LOWER[i] = i;
  }
  //  Swap C# for C
  for (int i = 1; i <= 0xFF; i += 12) {
    N_LOWER[i] = i - 1;
  }
  //  Swap F# for G
  for (int i = 6; i <= 0xFF; i += 12) {
    N_LOWER[i] = i + 1;
  }
} /* build_mapping_tables() */
