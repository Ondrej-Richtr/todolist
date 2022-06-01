#include "todolist.h"


//declaratons:
int is_date_valid(const date_t date);

void date_null(date_t *date);


//definitions:

//cli funcionality
int add_entry_splitted(llist *list, char status, char *orig_date, char *dead_date, char *text)
{
	todo_entry_t *entry = malloc(sizeof(todo_entry_t));
	if (!entry) return 1;
	
	entry->status = 0;
	if (status == 'X') entry->status = 1;
	
	if (!dead_date) date_null(&entry->deadline);
	else if (load_date_string(&entry->deadline, dead_date))
	{
		free(entry);
		return 2;
	}
	
	if (!orig_date) date_null(&entry->created_date);
	else if (load_date_string(&entry->created_date, orig_date))
	{
		free(entry);
		return 3;
	}
	
	//can't fail, at worst it loads nothing
	strcpy_buffer(TEXT_MAX_LEN, (char*)entry->text_buffer, text);
	
	if (!llist_add_end(list, entry))
	{
		free(entry);
		return 4;
	}
	
	return 0;
}

int add_entry_string(llist *list, char* string)
{	/*adds entry described by C-style string, returns zero if success
	-1 if bad parameters and failure codes from add_splitted*/
	if (!list || !string) return -1;
	
	size_t index = 0;
	char status = ' ';
	//buffer for deadline number and for text
	char num_buffer[NUM_BUFFER_SIZE + 1] = { 0 }, text_buffer[TEXT_MAX_LEN + 1] = { 0 };
	
	//first letter is checked if it's status
	if (string[0] == 'X')
	{
		index++;
		status = 'X';
	}
	
	//skipping separators and spaces
	while (string[index] != '\0' &&
		(isspace(string[index]) || isseparator(string[index]))) index++;

	//loading deadline
	if (isdigit(string[index]))
	{
		index += copy_until_sep(NUM_BUFFER_SIZE, num_buffer, string + index);
	}
	
	//loading text
	//string index now points to either end of string or at separator before text or text
	if (isseparator(string[index])) index++;
	//now at end (empty text) or text (non empty)
	strcpy_buffer(TEXT_MAX_LEN, (char*)text_buffer, string + index);
	
	return add_entry_splitted(list, status, NULL, (char*)num_buffer, (char*)text_buffer);
}

int delete_entry_string(llist *list, char *string)
{	/*parses input from string and deletes those entries specified by index
	in string, indexing is from 1, nonvalid index or letters generate errors
	returns nonzero only for invalid parameters, indicies are splitted by separators*/
	if (!list || !string) return -1;
	
	size_t i = 0, num = 0, deleted = 0, last = 0;
	
	while (string[i] != '\0')
	{
		num = (size_t)atoi(string + i);
		//printf("num to delete: %u\n", num);
		
		if (!num || num <= last || num < deleted + 1
			|| llist_delete_nth_entry(list, num - 1 - deleted))
		{
			fprintf(stderr, "Error: Wrong index '%u' specified to be deleted!\n", num);
		}
		else //so we can track how many was successfuly deleted
		{
			deleted++;
			last = num;
		}
		
		//skipping to the next number to load
		while (string[i] != '\0' && !isseparator(string[i])) i++;
		while (isseparator(string[i]) || isspace(string[i])) i++;
	}
	
	return 0;
}

int do_inter_cmd(llist *list, enum CmdType type, char *buffer)
{	//TODO err output?
	switch (type)
	{
		case print_c: print_llist(list);
		break;
		case add_c: return add_entry_string(list, buffer);
		case del_c: return delete_entry_string(list, buffer);
		default: return 1;
	}
	return 0;
}

int parse_cmd_type(char *cmd, enum CmdType *type_ptr)
{	//returns zero if not supported type, otherwise returns 1
	//and fills type_ptr with correct type

	//if (!cmd || !type_ptr) return 0;
	if (!strcmp("print", cmd))	*type_ptr = print_c;
	else if (!strcmp("add", cmd)) *type_ptr = add_c;
	else if (!strcmp("delete", cmd)) *type_ptr = del_c;
	else return 0;
	return 1;
}

int parse_inter_cmd(FILE *input, llist *list, char buffer[CLI_LINE_MAX_LEN + 1])
{	/*parses which command to be done from buffer and if needed loads
	more lines from the input, then executes correct cli function
	returns 1 if wrong input or other non-zero if error*/
	char *noncmd_start = NULL;
	size_t index = 0, cmd_end = 0;
	
	//skipping to the end of first word
	while (buffer[index] && !isspace(buffer[index])) index++;
	cmd_end = index;
	
	//skipping whitespaces
	while (buffer[index] && isspace(buffer[index])) index++;
	
	enum CmdType type;
	buffer[cmd_end] = '\0'; //splitting command string from the rest
	
	if (!parse_cmd_type((char*)buffer, &type))
	{
		//TODO err
		return 1;
	}
	
	//reading next line if more input needed
	if (!buffer[index] && type != print_c)
	{
		index = 0;
		//amount of loaded chars not needed
		readline(input, CLI_LINE_MAX_LEN, buffer);
	}
	
	if (do_inter_cmd(list, type, (char*)buffer + index))
	{
		//TODO err
		return 2;
	}
	
	return 0;
}

int interactive_mode(FILE *input, const char *todo_file_path)
{	/*reads lines from input until EOF and interprets them as cli commands
	todo_file is_path is the path to file where current todo list entries
	are possibly stored and where the result will be written*/
	
	//TODO interactive mode
	llist list = { NULL, NULL };
	if (load_entries(&list, todo_file_path))
	{
		//TODO err
		return 1;
	}
	
	char line_buffer[CLI_LINE_MAX_LEN + 1] = { 'x' };
	size_t line_len = 0;
	int parse_err = 0;
	
	while ((line_len = readline(input, CLI_LINE_MAX_LEN, line_buffer)))
	{	//also means that loaded line is not an empty string
		parse_err = parse_inter_cmd(input, &list, line_buffer);
		//printf("Parse err val: %d\n", parse_err);
	}

	FILE *out_file = fopen(todo_file_path, "w");
	if (!out_file)
	 {
		//TODO err
		//printf("Opening file at: '%s' failed!\n", write_path);
		llist_destroy_contents(&list);
		return 2;
	 }
	
	int write_err = write_entries(out_file, &list);
	
	/*TODO err
	if (write_err)
	{
		printf("Writing entries to file failed! Err: %d\n", write_err);
	}*/
	fclose(out_file);
	llist_destroy_contents(&list);
	return write_err;
}

//outputting
void print_todoentry(todo_entry_t entry, int style)
{
	//ignores style parameter for now
	if (entry.status) printf("[Y] ");
	else printf("[N] ");
	
	if (is_date_valid(entry.deadline))
	{
		write_date(stdout, entry.deadline);
		printf(" | ");
	}
	
	puts(entry.text_buffer);
	//printf("%s\n", entry.text_buffer);
}

void print_llist(llist *list)
{
	for (struct node *n = list->first; n != NULL; n = n->next)
	{
		print_todoentry(*(n->val), 0);
		//puts("------------");
	}
}
