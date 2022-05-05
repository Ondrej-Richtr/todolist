#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <ctype.h>

#define TEXT_MAX_LEN 64
#define NUM_BUFFER_SIZE 64

//structures:
typedef struct
{
	uint_least8_t day;
	uint_least8_t month;
	uint_least16_t year;
} date_t;

inline int is_date_valid(const date_t date)
{	//checks if given date has correct month (way of validating dates here)
	return date.month && date.month <= 12;
}

int write_date(FILE *f, const date_t date);

typedef struct
{
	int status;							//signalizes if entry was completed
	date_t created_date;				//date of creation
	date_t deadline;					//deadline for this entry
	char text_buffer[TEXT_MAX_LEN + 1];	//description of entry, +1 for NULL char
} todo_entry_t;

//linked list
struct node
{
	struct node *next;
	todo_entry_t *val;
};

void node_destroy(struct node *n);

typedef struct linked_list
{
	struct node *first;
	struct node *last;
} llist;

//linkedlist.c
void llist_add_node_end(llist *list, struct node *n);

void llist_add_node_first(llist *list, struct node *n);

struct node* llist_pop_node_first(llist *list);

int llist_add_end(llist *list, todo_entry_t *val);

int llist_add_first(llist *list, todo_entry_t *val);

void llist_destroy_contents(llist *list);

struct node *llist_nth_node(llist *list, size_t n);

todo_entry_t *llist_nth_entry(llist *list, size_t n);

//todolist.c
//reading file
int isseparator(int c);

void skip_until(FILE *f, int *in_char, char until);

size_t copy_until_sep(size_t max_size, char buffer[max_size + 1], char* source);

size_t readline(FILE *f, size_t max_size, char buffer[max_size + 1]);

void skip_comment_blank_lines(FILE *f, int *in_char);

size_t load_num_8(FILE *f, uint_least8_t* num, int *in_char);

size_t load_num_16(FILE *f, uint_least16_t* num, int *in_char);

int load_num_tolerant_8(FILE *f, uint_least8_t* num, int *in_char);

int load_num_tolerant_16(FILE *f, uint_least16_t* num, int *in_char);

int load_date(FILE *f, date_t* d, int c);

size_t load_buffer(FILE *f, char buffer[TEXT_MAX_LEN], int *in_char);

int load_one_entry(FILE *f, todo_entry_t *entry);

int load_entries(llist *list, const char *path);

//writing to file
void write_buffer(FILE *f, char* buffer);

int write_one_entry(FILE *f, todo_entry_t *entry);

int write_entries(FILE *f, llist *list);

//cli functionality
int add_entry_string(llist *list, char* string);

int add_entry_splitted(llist *list, char status, char *orig_date, char *dead_date, char *text);

//outputting
int write_date(FILE *f, const date_t date);

void print_todoentry(todo_entry_t entry, int style);

void print_llist(llist *list);
