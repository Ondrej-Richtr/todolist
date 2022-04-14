#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <ctype.h>

#define TEXT_MAX_LEN 64

//structures:
typedef struct
{
	uint_least8_t day;
	uint_least8_t month;
	uint_least16_t year;
} date_t;

//checks if given date has correct month (way of validating dates here)
inline int is_date_valid(const date_t date)
{
	return date.month && date.month <= 12;
}

void print_date(date_t date);

typedef struct
{
	int status;							//signalizes if entry was completed
	date_t created_date;				//date of creation
	date_t deadline;					//deadline for this entry
	char text_buffer[TEXT_MAX_LEN + 1];	//description of entry, +1 for NULL char
} todo_entry_t;

void print_todoentry(todo_entry_t entry, int style);

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

void llist_add_node_end(llist *list, struct node *n);

void llist_add_node_first(llist *list, struct node *n);

struct node* llist_pop_node_first(llist *list);

int llist_add_end(llist *list, todo_entry_t *val);

int llist_add_first(llist *list, todo_entry_t *val);

void llist_destroy_contents(llist *list);

//reading file
int isseparator(int c);

void skip_until(FILE *f, int *in_char, char until);

void skip_comment_lines(FILE *f, int *in_char);

int load_num_8(FILE *f, uint_least8_t* num, int *in_char);

int load_num_16(FILE *f, uint_least16_t* num, int *in_char);

int load_num_tolerant_8(FILE *f, uint_least8_t* num, int *in_char);

int load_num_tolerant_16(FILE *f, uint_least16_t* num, int *in_char);

int load_date(FILE *f, date_t* d, int c);

size_t load_buffer(FILE *f, char buffer[TEXT_MAX_LEN], int *in_char);

int load_one_entry(FILE *f, todo_entry_t *entry);

int load_entries(llist *list, const char *path);
