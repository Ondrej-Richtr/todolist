#include "todolist.h"


//declarations:
int is_date_valid(const date_t date);

void date_null(date_t *date);

int is_todoentry_valid(todo_entry_t *entry);

//definitions:

//time handling
date_t get_current_date()
{	//tries to return current date given by system
	//if it fails then it returns (invalid) zero date
	date_t output;
	date_null(&output); //setting the date to zeroes (invalid date)
	
	time_t sec = time(NULL); //getting current time and time structure to get current date
	struct tm *time_struct = localtime(&sec);
	if (time_struct) //if the localtime function fails we leave orig_date invalid (zeroes)
	{
		output.day = (uint_least8_t)time_struct->tm_mday;
		output.month = (uint_least8_t)time_struct->tm_mon + 1;
		output.year = (uint_least16_t)time_struct->tm_year + 1900;
	}
	
	return output;
}

//cli funcionality
int generate_entry_splitted(todo_entry_t *entry, const char status, const date_t orig_date, char *dead_date)
{	/*fills entry attributes specified by other parameters, EXCLUDING the text!
	returns zero if success, -1 if NULL entry and non-zero if error*/
	
	if (!entry)
	{
		fprintf(stderr, "Err: NULL pointer passed into generating of new entry! Entry aborted\n");
		return -1;
	}
	
	if (!dead_date || !*dead_date) date_null(&entry->deadline);
	else if (load_date_string(&entry->deadline, dead_date))
	{
		fprintf(stderr, "Err: Wrong formating of date '%s' entered! Entry aborted\n", dead_date);
		return 1;
	}
	
	//no error should happen over there: (so we set thing only now)
	entry->status = 0;
	if (status == 'X') entry->status = 1;
	
	entry->created_date = orig_date;
	
	return 0;
}

int generate_entry_from_string(const char* string, todo_entry_t *entry)
{	/*fills todo_entry according to given C-styl string
	returns zero if success, -1 if bad parameters and positive ints if error when parsing*/
	if (!string || !entry) return -1;
	
	size_t index = 0;
	char status = ' ';
	//buffer for deadline number (could be static, but I chose it not to be for now)
	char num_buffer[NUM_BUFFER_SIZE + 1] = { 0 };
	
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
		index += copy_until_delimiter(NUM_BUFFER_SIZE, num_buffer, string + index, isseparator);
	}
	else num_buffer[0] = '\0'; //because the buffer is static and may contain something
	
	//loading text
	//string index now points to either end of string or at separator before text or text
	if (isseparator(string[index])) index++;
	//now at end (empty text) or text (non empty)
	strcpy_buffer(TEXT_MAX_LEN, (char*)entry->text_buffer, string + index);
	
	//loading the time this entry was created (current time)
	date_t orig_date = get_current_date();
	
	return generate_entry_splitted(entry, status, orig_date, (char*)num_buffer);
}

int llist_asc_index_map(llist *list, const char *string, int(*func)(llist*, size_t, size_t))
{	//func should be the mapped function on the list
	//it should return negative number when error and non-negative
	//number of deleted entries
	if (!list || !string || !func)
	{
		fprintf(stderr, "Err: Program passed NULL pointer into asc_index_map function! Ignoring current command...\n");
		return -1;
	}
	
	size_t i = 0, num = 0, deleted = 0, last = 0;
	int func_ret = 0, atoi_ret = 0;
	
	while (string[i] != '\0')
	{
		atoi_ret = atoi(string + i);
		if (atoi_ret < 0)
		{
			fprintf(stderr, "Err: Wrong index '%d' specified, it can't be negative!\n", atoi_ret);
		}
		else //ugly indentation, but whatever
		{
			num = (size_t)atoi_ret; //this should always cast correctly
			//printf("num to delete: %u\n", num);
			
			if (!num || num <= last || num < deleted + 1) //when index is zero or not in ascending order
			{
				if (num && num <= last) fprintf(stderr, "Err: Wrong index '%u', indices must be written in ascending order!\n", num);
				else fprintf(stderr, "Err: Wrong index '%u' specified, it can't be zero or letter!\n", num);
			}
			//func shoould print it's own error msg when something goes wrong!
			else if ((func_ret = func(list, num - 1 - deleted, num)) >= 0) //so we can track how many was successfuly deleted
			{
				deleted += func_ret;
				last = num;
			}
		}
		//repetitive skipping to the next number to load
		while (string[i] != '\0' && !(isseparator(string[i]) || isspace(string[i]))) i++;
		while (isseparator(string[i]) || isspace(string[i])) i++;
	}
	
	return 0;
}

int cmd_add(llist *list, char *data_buffer)
{	/*adds entry described by C-style string in data_buffer, returns zero if success
	-1 if bad parameters and failure codes (positive ints) from add_splitted*/
	if (!list || !data_buffer)
	{
		fprintf(stderr, "Err: Program passed NULL pointer into adding command! Ignoring this command...\n");
		return -1;
	}
	
	todo_entry_t *entry = malloc(sizeof(todo_entry_t));
	if (!entry)
	{
		fprintf(stderr, "Err: Failed to allocate %u bytes of memory!\n", sizeof(todo_entry_t));
		return 1;
	}
	
	if (generate_entry_from_string(data_buffer, entry)) //only deadline date can be loaded incorrectly
	{
		//error message should get printed by generate_entry_splitted
		free(entry);
		return 2;
	}
	
	if (!is_todoentry_valid(entry)) //we check if the final entry makes sense, if not discard it
	{
		fprintf(stderr, "Err: Entry to be added is not valid! (Has no text and no date)\n");
		free(entry);
		return 3;
	}
	
	if (!llist_add_end(list, entry))
	{
		fprintf(stderr, "Err: Failed to add following entry into the list!\n");
		fprintf(stderr, "The entry: ");
		print_todoentry(stderr, *entry, 0);
		fputc('\n', stderr);
		free(entry);
		return 4;
	}
	
	return 0;
}

int delete_entry(llist *list, size_t index, size_t orig_index)
{	//deletes entry at given index from linked list
	//returns -1 if the entry was out of bounds and prints the original index
	//or otherwise couldnt get deleted
	//and returns 1 if it was deleted succesfuly
	if (llist_delete_nth_entry(list, index))
	{
		fprintf(stderr, "Err: Index '%u' to be deleted is out of bounds!\n", orig_index);
		return -1;
	}
	return 1;
}

int mark_entry(llist *list, size_t index, size_t orig_index, int is_done)
{
	//returns -1 if something went wrong, otherwise returns 0
	todo_entry_t *entry = llist_nth_entry(list, index);
	if (!entry)
	{
		fprintf(stderr, "Err: Index '%u' to be marked is out of bounds!\n", orig_index);
		return -1;
	}
	
	entry->status = is_done;
	return 0;
}

int mark_entry_done(llist *list, size_t index, size_t orig_index)
{	//marks entry at given index as done
	return mark_entry(list, index, orig_index, 1);
}

int mark_entry_undone(llist *list, size_t index, size_t orig_index)
{	//same as mark_entry_done but marks it undone
	return mark_entry(list, index, orig_index, 0);
}
int cmd_mark(llist *list, const char *data_buffer)
{	//does the interactive mark command, returns non-null when error
	//data_buffer should be always smaller or same size as CLI_LINE_MAX_LEN + 1
	if (!list || !data_buffer)
	{
		//TODO probably bad as the interactive while loops continues
		fprintf(stderr, "Err: Program passed NULL pointer into mark command! Ignoring this command...\n");
		return -1;
	}
	
	//TODO all_c is not working currently (should it even be allowed here?)
	enum SpecType spec = all_c;
	char spec_buffer[CLI_LINE_MAX_LEN + 1] = { 0 };
	size_t spec_size = copy_until_delimiter(CLI_LINE_MAX_LEN, spec_buffer, data_buffer, isspace);	
	
	if (parse_specifier_type(spec_buffer, &spec) && spec != all_c) //specifier was specified and is not all_c
	{
		//skipping to the start of non-specifier in data_buffer
		while (data_buffer[spec_size] && isspace((int)data_buffer[spec_size])) spec_size++;
		
		if (data_buffer[spec_size] == '\0') //data is empty (no indices)
		{
			fprintf(stderr, "Err: You need to specify indices for marking as done/undone!\n");
			return 1;
		}
		
		switch (spec)
		{
			case done_c: return llist_asc_index_map(list, data_buffer + spec_size, mark_entry_done);
			case undone_c: return llist_asc_index_map(list, data_buffer + spec_size, mark_entry_undone);
		}
		//this should not happen (for now):
		fprintf(stderr, "Err: Unsupported specifier '%s' in mark command!\n", (char*)spec_buffer);
		return 1;
	}
	//done/undone is not specified so there is nothing to do?
	fprintf(stderr, "Err: You need to specify if mark as done or undone!\n");
	return 1;
}

int cmd_clear(llist *list, char *data_buffer)
{
	if (!list || !data_buffer)
	{
		//TODO probably bad as the interactive while loops continues
		fprintf(stderr, "Err: Program passed NULL pointer into clear command! Ignoring this command...\n");
		return -1;
	}
	
	enum SpecType spec = done_c;
	char *end = word_skip(data_buffer);
	*end = '\0'; //cutting off excess data that we dont need
	
	if (parse_specifier_type(data_buffer, &spec))
	{
		//printf("Spec: %d\n", (int)spec);
		switch (spec)
		{
			case all_c: llist_destroy_contents(list);
				break;
			case done_c: llist_clear(list, 1);
				break;
			case undone_c: llist_clear(list, 0);
				break;
			default:
				//this should not happen (for now):
				fprintf(stderr, "Err: Unsupported specifier '%s' in clear command!\n", data_buffer);
				return 1;
		}
		return 0;
	}
	
	//wrong specifier
	fprintf(stderr, "Err: You need to specify if clear all/done/undone entries!\n");
	return 1;
}

int cmd_change(llist *list, char *data_buffer, int is_verbose) //TODO
{
	if (!list || !data_buffer)
	{
		//TODO probably bad as the interactive while loops continues
		fprintf(stderr, "Err: Program passed NULL pointer into change command! Ignoring this command...\n");
		return -1;
	}
	
	//gets the number to change (only loads one)
	int atoi_ret = atoi(data_buffer);
	if (atoi_ret < 0)
	{
		fprintf(stderr, "Err: Wrong index '%d' specified, it can't be negative!\n", atoi_ret);
		return 1;
	}
	
	size_t num = (size_t)atoi_ret;
	if (!num)
	{
		fprintf(stderr, "Err: Wrong index '%u' specified, indices can't be zero or letter!\n", num);
		return 2;
	}

	todo_entry_t *old_entry = llist_nth_entry(list, num - 1);
	if (!old_entry)
	{
		fprintf(stderr, "Err: Index '%u' to be changed is out of bounds!\n", num);
		return 3;
	}
	
	if (is_verbose)
	{
		fprintf(stdout, "You are about to change the #%u entry: '", num);
		print_todoentry(stdout, *old_entry, 1);
		fputs("'\nWrite changed version at the next line or you can abort by writing an empty line\n", stdout);
	}
	
	char line_buffer[CLI_LINE_MAX_LEN + 1] = { 0 }; //could be static
	size_t line_len = readline(stdin, CLI_LINE_MAX_LEN, line_buffer);
	if (!line_len)
	{
		//TODO abort message if is_verbose?
		//puts("Aborted success");
		return 0;
	}
	
	todo_entry_t new_entry;
	int gen_err = generate_entry_from_string((char*)line_buffer, &new_entry);
	if (gen_err)
	{
		//error message should get printed by generate_entry_splitted
		fprintf(stderr, "Leaving old entry unchanged\n");
		//printf("Gen err: %d\n", gen_err);
		return 4;
	}
	
	if (!is_todoentry_valid(&new_entry)) //we check if the final entry makes sense, if not discard it
	{
		fprintf(stderr, "Err: Entered entry is not valid! (Has no text and no date) Leaving old entry unchanged\n");
		return 5;
	}
	
	//change the old entry into new one
	*old_entry = new_entry;
	
	//TODO maybe typo in success
	if (is_verbose) printf("The #%u entry was changed successfuly\n", num);
	
	return 0;
}

int cmd_move(llist *list, char *data_buffer)
{	//TODO errors
	if (!list || !data_buffer)
	{
		//TODO probably bad as the interactive while loops continues
		fprintf(stderr, "Err: Program passed NULL pointer into move command! Ignoring this command...\n");
		return -1;
	}
	
	size_t from = 0, to = 0;
	if (parse_range(data_buffer, &from, &to, &data_buffer))
	{
		from = atoi(data_buffer);
		to = from;
		
		data_buffer = string_num_end(data_buffer, NULL);
	}
	
	if (!from || !to)
	{
		fprintf(stderr, "Err: wrong moved range or index specified in the move command!\n");
		return 1;
	}
	
	size_t where = atoi(data_buffer);
	if (!where)
	{
		fprintf(stderr, "Err: wrong destination index to be moved to specified in the move command!\n");
		return 2;
	}
	data_buffer = string_num_end(data_buffer, NULL);
	
	while (*data_buffer && isspace((int)*data_buffer)) data_buffer++;
	if (*data_buffer) //this means that there is still some nonempty text after where number
	{
		fprintf(stderr, "Err: there's unexpected text after index/range and index in the move command!\n");
		return 3;
	}
	
	//printf("From: %u to: %u where: %u\n", from, to, where);
	
	int move_err = llist_move(list, from - 1, to - 1, where - 1); //-1 as llist index from 0
	if (move_err)
	{
		//TODO err, probably switch statement
		fprintf(stderr, "Err: failed to move specified entries in the move command! Err code: %d\n", move_err);
		return 4;
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
		case add_c: return cmd_add(list, buffer);
		case del_c: return llist_asc_index_map(list, buffer, delete_entry);
		case mark_c: return cmd_mark(list, buffer);
		case clear_c: return cmd_clear(list, buffer);
		case change_c: return cmd_change(list, buffer, 1); //1 is for verbose mode (default in interactive)
		case move_c: return cmd_move(list, buffer);
		default: //this shouldn't normally happen
		fprintf(stderr, "Err: Wrong command type specified '%d' in command caller!\n", type);
		return 1;
	}
	return 0;
}

int parse_range(char *string, size_t *start, size_t *end, char **range_end)
{	/*parses input from string in format "startnum-endnum"
	ignores whitespaces at the start and other text after
	returns non-zero if bounds not found*/
	size_t index = 0;
	//skipping initial whitespaces
	while (string[index] && isspace((int)string[index])) index++;
	
	//there must be a digit after whitespaces
	if (!string[index] || !isdigit((int)string[index])) return 1;
	
	char *num1 = string + index,
	 *dash = string_num_end(string + index, NULL),
	 *num2 = NULL;
	
	if (*dash != '-') return 2; //dash shouldnt be NULL
	char *num2_end = string_num_end(dash, &num2);
	if (!*num2) return 3; //the second number must be specified
	if (num2 - dash != 1) return 3; //epic pointer arithmetic hacking
	
	//TODO replace atoi with custom implementation
	if (start) *start = atoi(num1);
	if (end) *end = atoi(num2);
	if (range_end) *range_end = num2_end;
	
	return 0;
}

int parse_specifier_type(char *string, enum SpecType *spec_ptr)
{	//checks if string matches any specifier type, if yes then it returns 1
	//and tries to set the spec, otherwise just returns 0 (also when NULL buffer)
	if (!string) return 0;
	
	if (!strcmp("all", string))
	{
		if (spec_ptr) *spec_ptr = all_c;
	}
	else if (!strcmp("done", string))
	{
		if (spec_ptr) *spec_ptr = done_c;
	}
	else if (!strcmp("undone", string))
	{
		if (spec_ptr) *spec_ptr = undone_c;
	}
	else return 0;
	return 1;
}

int parse_cmd_type(char *cmd, enum CmdType *type_ptr)
{	//returns zero if not supported type, otherwise returns 1
	//and fills type_ptr with correct type

	//TODO more effective way
	if (!strcmp("help", cmd)) *type_ptr = help_c;
	else if (!strcmp("print", cmd))	*type_ptr = print_c;
	else if (!strcmp("add", cmd)) *type_ptr = add_c;
	else if (!strcmp("delete", cmd)) *type_ptr = del_c;
	else if (!strcmp("mark", cmd)) *type_ptr = mark_c;
	else if (!strcmp("clear", cmd)) *type_ptr = clear_c;
	else if (!strcmp("change", cmd)) *type_ptr = change_c;
	else if (!strcmp("move", cmd)) *type_ptr = move_c;
	else return 0;
	return 1;
}

int parse_inter_cmd(FILE *input, llist *list, char buffer[CLI_LINE_MAX_LEN + 1])
{	/*parses which command to be done from buffer and if needed loads
	more lines from the input, then executes correct cli function
	returns 1 if wrong input or other non-zero if error*/
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
	if (!buffer[index] && (type == add_c || type == del_c 
			|| type == mark_c || type == clear_c))
	{
		index = 0;
		//amount of loaded chars is not needed
		readline(input, CLI_LINE_MAX_LEN, buffer);
		while (buffer[index] && isspace(buffer[index])) index++;
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
	
	char line_buffer[CLI_LINE_MAX_LEN + 1] = { 0 };
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
	puts("-------------HELP-------------");
	puts("Interactive mode commands are:");
	puts("\t'help' - prints this help on stdout");
	puts("\t'print' - prints all todolist entries in current memory on stdout");
	puts("\t'add' - adds one new entry into current todolist");
	puts("\t'delete' - deletes specified entries from current todolist");
	puts("\t'mark done/undone' - marks specified entries as done/undone");
	puts("\t'clear all/done/undone' - clears all/done/undone todolist entries");
	puts("\t'change' - changes one existing entry that you specify");
	puts("------------------------------");	
}

void print_todoentry(FILE *out, const todo_entry_t entry, int style)
{	//prints todo entry into file 'out', if out is NULL then it does nothing
	if (!out) return;
	
	//TODO style
	if (style)
	{
		if (entry.status) fputs("[Y] ", out);
		else fputs("[N] ", out);
		
		if (is_date_valid(entry.deadline))
		{
			write_date(out, entry.deadline);
			fputs(" | ", out);
		}
	}
	
	fputs(entry.text_buffer, out);
}

void print_llist(llist *list, int style)
{
	size_t num = 1;
	for (struct node *n = list->first; n != NULL; n = n->next)
	{
		if (style) printf("%u\t", num++);
		print_todoentry(stdout, *(n->val), style);
		putchar('\n');
	}
}
