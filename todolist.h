#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <ctype.h>
#include <string.h>
#include <time.h>

#define TEXT_MAX_LEN 255
#define NUM_BUFFER_SIZE 64
#define CLI_LINE_MAX_LEN 1024
//the maximum of (guaranteed) chars cmd names can be - print, add, delete...
//it can some cases commands of longer length might be recognised (in interactive mode)
#define CMD_NAME_MAX_LEN 23

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

inline void date_null(date_t *date)
{
	date->day = 0, date->month = 0, date->year = 0;
}

typedef struct
{
	uint_least8_t status;				//signalizes if entry was completed, 0 -> not done, 1 -> done, anything else should not happen
	date_t 	created_date;				//date of creation (currently date of last modification)
	date_t deadline;					//deadline for this entry
	char text_buffer[TEXT_MAX_LEN + 1];	//description of entry, +1 for NULL char
} todo_entry;

inline int is_todoentry_valid(todo_entry *entry)
{
	return entry->text_buffer[0] || is_date_valid(entry->deadline);
}

//linked list
struct node
{
	struct node *next;
	todo_entry *val;
};

void node_destroy(struct node *n);

typedef struct linked_list
{
	struct node *first;
	struct node *last;
} llist;

//linkedlist.c
size_t llist_length(llist *list);

void llist_add_node_end(llist *list, struct node *n);

void llist_add_node_first(llist *list, struct node *n);

struct node* llist_pop_node_first(llist *list);

int llist_add_end(llist *list, todo_entry *val);

int llist_add_first(llist *list, todo_entry *val);

void llist_delete_after(llist* list, struct node *prev);

void llist_destroy_contents(llist *list);

struct node *llist_nth_node(llist *list, size_t n);

todo_entry *llist_nth_entry(llist *list, size_t n);

int llist_delete_nth_entry(llist *list, size_t n);

int llist_delete_range(llist *list, size_t indexS, size_t indexE);

void llist_clear(llist *list, uint_least8_t status);

int llist_disconnect(llist *list, llist *into, size_t start, size_t end);

int llist_move(llist *list, size_t from, size_t to, size_t where);

int llist_swap(llist *list, size_t idx1, size_t idx2);

int llist_sort(llist *list, int(*comparator)(const todo_entry*, const todo_entry*));

//todolist_load.c
int isseparator(int c);

//void skip_until(FILE *f, int *in_char, char until);

size_t copy_until_delimiter(size_t max_size, char buffer[max_size + 1], const char* source, int(*delim)(int));

char* next_word_skip(char *string);

char* word_skip(char *string);

const char* word_skip_const(const char *string);

size_t readline(FILE *f, size_t max_size, char buffer[max_size + 1]);

//void skip_comment_blank_lines(FILE *f, int *in_char);

int str_to_num(const char *string, size_t *end_index);

size_t load_num_8(FILE *f, uint_least8_t* num, int *in_char);

size_t load_num_16(FILE *f, uint_least16_t* num, int *in_char);

int load_num_tolerant_8(FILE *f, uint_least8_t* num, int *in_char);

int load_num_tolerant_16(FILE *f, uint_least16_t* num, int *in_char);

int load_date(FILE *f, date_t* d, int c);

int load_date_string(date_t *d, const char *str_start);

void strcpy_buffer(size_t buffer_size, char *buffer, const char *source);

int load_one_entry(FILE *f, todo_entry *entry);

int load_entries(llist *list, const char *path);

//todolist_write.c
//void write_buffer(FILE *f, const char* buffer); //USELESS

int write_date(FILE *out, const date_t date);

int write_one_entry(FILE *f, todo_entry *entry);

int write_entries(FILE *f, llist *list);

int write_todofile(FILE *f, llist *list);

//todolist.c
enum CmdType{ help_c, print_c, add_c, del_c, mark_c, clear_c, change_c, move_c, swap_c, sort_c };
enum SpecType{ all_c, done_c, undone_c};

//time handling:
date_t get_today_date();

//parsing enums and specifiers:
int parse_direction(const char *string, size_t *end_index);

int parse_cmd_type(const char *cmd, enum CmdType *type_ptr);

int is_valid_cmd(const char *str, enum CmdType *type); //maybe move this somewhere else?

int parse_specifier_type(char *string, enum SpecType *spec_ptr);

int parse_opt_basichelp(const char *str);

int parse_opt_cmdhelp(const char *str);

//cli functionality
int generate_entry_splitted(todo_entry *entry, const char status, const date_t orig_date, char *dead_date);

int generate_entry_from_string(const char* string, todo_entry *entry);

int llist_asc_index_map(llist *list, const char *string, int(*func)(llist*, size_t, size_t, size_t));

void print_todoentry(FILE *out, const todo_entry *entry, const int style);

void print_todolist(const llist *list, const int style);

void print_basichelp(int isoption);

int cmd_print(llist *list, char *data_buffer);

int cmd_add(llist *list, char *data_buffer);

int cmd_mark(llist *list, const char *string);

int cmd_clear(llist *list, char *data_buffer);

int cmd_change(llist *list, char *data_buffer, int is_verbose, int noninter);

int cmd_move(llist *list, char *data_buffer);

int cmd_help(char *data_buffer, int isoption);

int cmd_help_noninter_parse(size_t argc, const char** argv);

int cmd_swap(llist *list, char *data_buffer);

int cmd_sort(llist *list, char *data_buffer);

int parse_range(char *string, size_t *start, size_t *end, char **range_end);
int parse_range_const(const char *string, size_t *start, size_t *end, const char **range_end);

int interactive_mode(FILE *input, const char *todo_file_path);

int noninteractive_mode(const size_t options_num, const char **options, const char *todo_file_path);
