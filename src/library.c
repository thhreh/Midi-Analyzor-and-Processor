/* Wei Yan, library.c, CS 24000, Fall 2020
 * Last updated October 12, 2020
 */

/* Add any includes here */

#include "library.h"

#include<malloc.h>
#include<stdio.h>
#include<stdint.h>
#include<assert.h>
#include<string.h>
#include<ftw.h>


tree_node_t *g_song_library = {0};

int fn(const char *input_dic, const struct stat *fun_pointer, int flag);

/* this function link
 * small family to its parent
 */

void insert_family(tree_node_t **head, tree_node_t *node){
  if (!node){
    return;
  }
  tree_insert(head, node);
  insert_family(head, node->left_child);
  insert_family(head, node->right_child);
} /* insert_family() */


/* This function should locate the node in
 * the given tree having the given song_name
 */

tree_node_t **find_parent_pointer(tree_node_t **tree_input,
const char * input_name){
  assert(tree_input != NULL);
  assert(input_name != NULL);
  if (strcmp((*tree_input)->song_name, input_name) == 0) {
    return tree_input;
  }
  if ((*tree_input)->left_child != NULL){
    if (strcmp((*tree_input)->left_child->song_name, input_name) == 0){
      return &((*tree_input)->left_child);
    }
  }
  if ((*tree_input)->right_child != NULL){
    if (strcmp((*tree_input)->right_child->song_name, input_name) == 0){
      return &((*tree_input)->right_child);
    }
  }
  if (((*tree_input)->right_child == NULL) &&
    ((*tree_input)->left_child == NULL)){
    return NULL;
  }
  else{
    if ((*tree_input)->left_child != NULL){
      tree_node_t **song_node_left = NULL;
      tree_node_t **left_test_temp = &((*tree_input)->left_child);
      song_node_left = find_parent_pointer(left_test_temp, input_name);
      if (song_node_left != NULL){
        return song_node_left;
      }
    }
    if ((*tree_input)->right_child != NULL){
      tree_node_t **song_node_right = NULL;
      tree_node_t **right_test_temp = &((*tree_input)->right_child);
      song_node_right = find_parent_pointer(right_test_temp, input_name);
      if (song_node_right != NULL){
        return song_node_right;
      }
    }

  }
  return NULL;
} /* find_parent_pointer() */


/* The first argument to this function is a double
 * pointer to the root of the tree. The second
 * argument is a node to be inserted into
 * the tree it will return int
 */

int tree_insert(tree_node_t ** tree_root_input, tree_node_t * node_input){
  assert(node_input != NULL);

  /* head condition */

  if (*tree_root_input == NULL){
    *tree_root_input = node_input;
    return INSERT_SUCCESS;
  }
  if (strcmp((*tree_root_input)->song_name, node_input->song_name) == 0){
    return DUPLICATE_SONG;
  }

  /* if root dictionary after  node */

  if (strcmp((*tree_root_input)->song_name, node_input->song_name) > 0 ){
    tree_node_t **left_test_temp = &((*tree_root_input)->left_child);
    if (left_test_temp == NULL){
      *left_test_temp = node_input;
      return INSERT_SUCCESS;
    }
    return tree_insert(left_test_temp, node_input);
  }

  /* if root dictionary before node */

  if (strcmp((*tree_root_input)->song_name, node_input->song_name) < 0 ){
    tree_node_t **right_test_temp = &((*tree_root_input)->right_child);
    if (right_test_temp == NULL){
      *right_test_temp = node_input;
      return INSERT_SUCCESS;
    }
    return tree_insert(right_test_temp, node_input);
  }
  return DUPLICATE_SONG;

} /* tree_insert() */


/* This function should search the given
 * tree for a song with the given name
 */

int remove_song_from_tree(tree_node_t ** node_input,
const char *song_input){
  tree_node_t ** node_remove = NULL;
  tree_node_t * head = NULL;

  /*  first head is song situation */

  if ((*node_input) == NULL){
    return SONG_NOT_FOUND;
  }
  if (strcmp((*node_input)->song_name, song_input) == 0) {

    /* left 1 right 0 */

    if ((*node_input)->left_child != NULL){
      head = (*node_input)->left_child;
      if ((*node_input)->right_child == NULL){
        free_node(*node_input);
        *node_input = head;
        return DELETE_SUCCESS;
      }

      /*left 1 right 1*/

      if ((*node_input)->right_child != NULL){
        insert_family(&head, (*node_input)->right_child);
        free_node(*node_input);
        *node_input = head;
        return DELETE_SUCCESS;
      }
    }

    /* left 0 right 0*/

    if (((*node_input)->right_child == NULL) &&
      ((*node_input)->left_child == NULL)){
      if (*node_input == NULL){
        return SONG_NOT_FOUND;
      }
      free_node(*node_input);
      return DELETE_SUCCESS;
    }
    if ((*node_input)->right_child != NULL){
      head = (*node_input)->right_child;
      if ((*node_input)->left_child != NULL){
        insert_family(&head, (*node_input)->left_child);
      }
      free_node(*node_input);
      *node_input = head;
      return DELETE_SUCCESS;
    }
    free_node(*node_input);
    return DELETE_SUCCESS;
  }

  /* left of the head similar */

  if ((*node_input)->left_child != NULL) {
    if (strcmp((*node_input)->left_child->song_name, song_input) == 0){
      head = (*node_input)->left_child;
      (*node_input)->left_child = NULL;
      tree_node_t *left_head = head->left_child;
      tree_node_t *right_head = head->right_child;
      free_node(head);
      insert_family(node_input, left_head);
      insert_family(node_input, right_head);
      return DELETE_SUCCESS;
    }
  }

  /* right of head similar */

  if ((*node_input)->right_child != NULL) {
    if (strcmp((*node_input)->right_child->song_name, song_input) == 0){
      head = (*node_input)->right_child;
      (*node_input)->right_child = NULL;
      tree_node_t *right_head = head->right_child;
      tree_node_t *left_head = head->left_child;
      free_node(head);
      insert_family(node_input, left_head);
      insert_family(node_input, right_head);
      return DELETE_SUCCESS;
    }
  }

  /* other general condition */

  if (strcmp((*node_input)->song_name, song_input) < 0){
    tree_node_t **right_test_temp = &((*node_input)->right_child);
    if (right_test_temp == NULL){
      return SONG_NOT_FOUND;
    }
    assert(right_test_temp != NULL);
    int sec_checker = remove_song_from_tree(right_test_temp, song_input);
    return sec_checker;
  }

  if (strcmp((*node_input)->song_name, song_input) > 0) {
    tree_node_t **left_test_temp = &((*node_input)->left_child);
    if (left_test_temp == NULL){
      return SONG_NOT_FOUND;
    }
    assert(left_test_temp != NULL);
    int first_checker = remove_song_from_tree(left_test_temp, song_input);
    return first_checker;
  }
  return SONG_NOT_FOUND;
} /* remove_song_from_tree() */


/* This functions should free all
 * data associated with the given node
 */

void free_node(tree_node_t * input_tree){
  free_song(input_tree->song);
  input_tree->song = NULL;
  input_tree->left_child = NULL;
  input_tree->right_child = NULL;
  free(input_tree);
  input_tree = NULL;
} /* free_node() */

/* This function should print the song name
 * associated with the given tree node
 */

void print_node(tree_node_t * input_node, FILE *my_file){
  fprintf(my_file, "%s\n", input_node->song_name);
} /* print_node() */


/* This function accepts a tree pointer
 * a piece of arbitrary data, and a function pointer. This
 * function should perform a pre-order traversal of the tree
 */

void traverse_pre_order(tree_node_t * input_tree, void *data,
traversal_func_t function){
  if (input_tree == NULL){
    return;
  }
  function(input_tree, data);
  traverse_pre_order(input_tree->left_child, data, function);
  traverse_pre_order(input_tree->right_child, data, function);
} /* traverse_pre_order() */


/* This function is exactly the same as the prior
 * but should visit each node in an in-order fashion
 */

void traverse_in_order(tree_node_t *input_tree, void *data,
  traversal_func_t function){
  if (input_tree == NULL){
    return;
  }
  traverse_in_order(input_tree->left_child, data, function);
  function(input_tree, data);
  traverse_in_order(input_tree->right_child, data, function);
} /* traverse_in_order() */

/* This function is exactly the same as the prior
 * but should visit each node in a post-order fashion
 */

void traverse_post_order(tree_node_t *input_tree, void *data,
  traversal_func_t function){
  if (input_tree == NULL){
    return;
  }
  traverse_post_order(input_tree->left_child, data, function);
  traverse_post_order(input_tree->right_child, data, function);
  function(input_tree, data);
} /* traverse_post_order() */


/* This function should free all
 * data associated with the input tree
 */

void free_library(tree_node_t *head){
  if (head->left_child != NULL){
    free_library(head->left_child);
  }
  if (head->right_child != NULL){
    free_library(head->right_child);
  }
  free_node(head);
} /* free_library() */


/* This function should print the names of
 * all songs in the given tree to the file
 */

void write_song_list(FILE *my_file, tree_node_t *head){
  traversal_func_t print_file = (traversal_func_t) print_node;
  traverse_in_order(head, my_file, print_file);
} /* write_song_list() */


/* This function takes in the name of a directory
 * and should search through the directory structure
 * adding adding every MIDI file found in the structure
 */

void make_library(const char *directory_path){
  int function_feeback = ftw(directory_path, fn, 10);
} /* make_library() */


/* function for ftw callback
 * read file end with midi
 */

int fn(const char *input_dic, const struct stat *fun_pointer, int flag) {
  tree_node_t *head = malloc(sizeof(tree_node_t));
  assert(head != NULL);
  if (flag == FTW_F) {
    if (strrchr(input_dic, '.') == NULL){
      return 0;
    }

    char *file_name = strrchr(input_dic, '.');
    if (strcmp(file_name, ".mid") == 0) {
      head->song = parse_file(input_dic);
      char *input_path = strrchr(head->song->path, '/');
      head->song_name = input_path + 1;
      head->left_child = NULL;
      head->right_child = NULL;
      tree_insert(&g_song_library, head);
    }
  }
  return 0;
} /* fn() */
