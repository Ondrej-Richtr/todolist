#include "todolist.h"


//declaratons:
int is_date_valid(const date_t date);

void date_null(date_t *date);

int is_todoentry_valid(todo_entry_t *entry);

//definitions:

//cli funcionality
int add_entry_splitted(llist *list, char status, date_t orig_date, char *dead_date, char *text)
{
	todo_entry_t *entry = malloc(sizeof(todo_entry_t));
	if (!entry)
	{
		fprintf(stderr, "Err: Failed to allocate %u bytes of memory!\n", sizeof(todo_entry_t));
		return 1;
	}
	
	entry->status = 0;
	if (status == 'X') entry->status = 1;
	
	if (!dead_date) date_null(&entry->deadline);
	else if (load_date_string(&entry->deadline, dead_date))
	{
		fprintf(stderr, "Err: Wrong formating of date '%s' entered!\n", dead_date);
		free(entry);
		return 2;
	}
	
	entry->created_date = orig_date;
	/*if (!orig_date) date_null(&entry->created_date);
	else if (load_date_string(&entry->created_date, orig_date))
	{
		fprintf(stderr, "Err: Wrong formating of date '%s' entered!\n", orig_date);	
		free(entry);
		return 3;
	}*/
	
	//can't fail, at worst it loads nothing
	strcpy_buffer(TEXT_MAX_LEN, (char*)entry->text_buffer, text);
	
	//we check if the final entry makes sense, if not discard it
	if (!is_todoentry_valid(entry))
	{
		fprintf(stderr, "Err: Entry to be added is not valid! (Has no text and no date)\n");
		free(entry);
		return 5;
	}
	
	if (!llist_add_end(list, entry))
	{
		fprintf(stderr, "Err: Failed to add following entry into the list!\n");
		fprintf(stderr, "The entry: ");
		print_todoentry(*entry, 0);
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
	
	//loading the time this entry was created (current time)
	date_t orig_date;
	date_null(&orig_date); //setting the date to zeroes (invalid date)
	
	time_t sec = time(NULL); //getting current time and time structure to get current date
	struct tm *time_struct = localtime(&sec);
	if (time_struct) //if the localtime function fails we leave orig_date invalid (zeroes)
	{
		orig_date.day = (uint_least8_t)time_struct->tm_mday;
		orig_date.month = (uint_least8_t)time_struct->tm_mon + 1;
		orig_date.year = (uint_least16_t)time_struct->tm_year + 1900;
	}
	//write_date(stdout, orig_date);
	//putc('\n');
	
	return add_entry_splitted(list, status, orig_date, (char*)num_buffer, (char*)text_buffer);
}

int llist_asc_map(llist *list, char *string, int(*func)(llist*, size_t))
{
	if (!list || !string || !func) return -1;
	
	size_t i = 0, num = 0, deleted = 0, last = 0;
	
	while (string[i] != '\0')
	{
		num = (size_t)atoi(string + i);
		//printf("num to delete: %u\n", num);
		
		if (!num || num <= last || num < deleted + 1) //when index is zero or not in ascending order
		{
			if (num && num <= last) fprintf(stderr, "Err: Wrong index '%u', deleted indices must be in ascending order!\n", num);
			else fprintf(stderr, "Err: Wrong index '%u' specified to be deleted!\n", num);
		}
		//func shoould print it's own error msg when something goes wrong!
		else if (!func(list, num - 1 - deleted)) //so we can track how many was successfuly deleted
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
			if (num && num <= last) fprintf(stderr, "Err: Wrong index '%u', deleted indices must be in ascending order!\n", num);
			else fprintf(stderr, "Err: Wrong index '%u' specified to be deleted!\n", num);
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
{	/*calls correct functionality specified by command type argument
	passes in buffer if the function requires it
	error msgs are printed by functions themselves, except for wrong type
	returns non-zero if the command couldn't be executed successfuly*/
	switch (type)
	{
		case help_c: print_help();
		break;
		case print_c: print_llist(list, 1);
		break;
		case add_c: return add_entry_string(list, buffer);
		//TODO different delete function
		case del_c: return llist_asc_map(list, buffer, llist_delete_nth_entry);
		//case del_c: return delete_entry_string(list, buffer);
		default: fprintf(stderr, "Err: Wrong command type specified '%d' in command caller!\n", type);
		return 1;
	}
	return 0;
}

int parse_cmd_type(char *cmd, enum CmdType *type_ptr)
{	//returns zero if not supported type, otherwise returns 1
	//and fills type_ptr with correct type

	//if (!cmd || !type_ptr) return 0;
	if (!strcmp("help", cmd)) *type_ptr = help_c;
	else if (!strcmp("print", cmd))	*type_ptr = print_c;
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
		fprintf(stderr, "Err: Unknown command: '%s'! Type 'help' to get list of all known commands.\n", (char*)buffer);
		return 1;
	}
	
	//reading next line if more input needed (for add and delete)
	if (!buffer[index] && (type == add_c || type == del_c))
	{
		index = 0;
		//amount of loaded chars not needed
		readline(input, CLI_LINE_MAX_LEN, buffer);
	}
	
	if (do_inter_cmd(list, type, (char*)buffer + index)) return 2;
	
	return 0;
}

int interactive_mode(FILE *input, const char *todo_file_path)
{	/*reads lines from input until EOF and interprets them as cli commands
	todo_file is_path is the path to file where current todo list entries
	are possibly stored and where the result will be written*/
	
	llist list = { NULL, NULL };
	if (load_entries(&list, todo_file_path))
	{
		//this err msg assumess that more detailed err msg had been already printed by load_entries
		fprintf(stderr,"Loading of todo entries from file failed! Aborting...\n");
		return 1;
	}
	
	char line_buffer[CLI_LINE_MAX_LEN + 1] = { 'x' };
	size_t line_len = 0;
	int parse_err = 0;
	
	while ((line_len = readline(input, CLI_LINE_MAX_LEN, line_buffer)))
	{	//also means that loaded line is not an empty string
		//err gets ignored as the program can continue and err msg is already printed
		parse_err = parse_inter_cmd(input, &list, line_buffer);
	}

	FILE *out_file = fopen(todo_file_path, "w");
	if (!out_file)
	 {
		fprintf(stderr, "Err: Failed to open file for writing at path: '%s'!\n", todo_file_path);
		llist_destroy_contents(&list);
		return 2;
	 }
	
	//write_err gets ignored as if the error occurred the err msg was printed by write_entries
	//TODO better solution, because if error then the file gets emptied and all entries are lost
	int write_err = write_entries(out_file, &list);
	
	fclose(out_file);
	llist_destroy_contents(&list);
	return write_err;
}

//outputting
void print_help()
{
	//some basic help print
	//TODO print detailed help for each specific command
	puts("-------HELP-------");
	puts("Interactive mode commands are:");
	puts("\t'help' - prints this help on stdout");
	puts("\t'print' - prints all todolist entries in current memory on stdout");
	puts("\t'add' - adds one new entry into current todolist");
	puts("\t'delete' - deletes specified entries from current todolist");
	//puts("\t'add' - adds new entry into todolist, if followed by some nonempty text it tries to interpret this text as the new entry, else it expects another line of text as the new entry");
	//puts("\t'delete' - deletes entries from todolist, after 'delete' or on the next line enter ascending sequence of numbers separated by '|', those entries will get deleted");
	puts("------------------");	
}

void print_todoentry(todo_entry_t entry, int style)
{
	//TODO style
	if (style)
	{
		if (entry.status) printf("[Y] ");
		else printf("[N] ");
		
		if (is_date_valid(entry.deadline))
		{
			write_date(stdout, entry.deadline);
			printf(" | ");
		}
	}
	
	puts(entry.text_buffer);
}

void print_llist(llist *list, int style)
{
	size_t num = 1;
	for (struct node *n = list->first; n != NULL; n = n->next)
	{
		if (style) printf("%u\t", num++);
		print_todoentry(*(n->val), style);
	}
}
