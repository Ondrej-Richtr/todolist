#include "todolist.h"

#define PRINT_DEFAULT 3

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

//comparators for todo_entry_t
//expecting non-NULL pointers, used as func.pointers -> no point in making them inline
/*	if entries are same comparator returns 0
	if first entry is 'greater' it retruns negative number
	if first entry is 'smaller' it returns positive number*/
int todo_compar_statdone(const todo_entry_t *e1, const todo_entry_t *e2)
{
	//this assumess that status done is always same positive number and undone always zero
	return e1->status - e2->status;
	//return e1->status > e2->status ? -1 : (e1->status < e2->status ? 1 : 0);
	//return e1->status ? (e2->status ? 0 : 1) : (e2->status ? -1 : 0);
}
int todo_compar_statundone(const todo_entry_t *e1, const todo_entry_t *e2)
{
	//this assumess that status done is always same positive number and undone always zero
	return e2->status - e1->status;
	//return e1->status > e2->status ? -1 : (e1->status < e2->status ? 1 : 0);
	//return e1->status ? (e2->status ? 0 : -1) : (e2->status ? 1 : 0);
}

int compar_dates(const date_t d1, const date_t d2) //date comparator
{
	//validity checks (invalid dates are treated as "bigger" than valid ones)
	if (!is_date_valid(d1)) return is_date_valid(d2) ? -1 : 0;
	if (!is_date_valid(d2)) return 1;
	
	//if both dates are valid
	if (d1.year != d2.year) return d1.year > d2.year ? -1 : 1;
	if (d1.month != d2.month) return d1.month > d2.month ? -1 : 1;
	if (d1.day != d2.day) return d1.day > d2.day ? -1 : 1;
	return 0;
}
int todo_compar_deadline(const todo_entry_t *e1, const todo_entry_t *e2)
{
	return compar_dates(e1->deadline, e2->deadline);
}
int todo_compar_created(const todo_entry_t *e1, const todo_entry_t *e2)
{
	return compar_dates(e1->created_date, e2->created_date);
}
int todo_compar_text(const todo_entry_t *e1, const todo_entry_t *e2)
{
	//e2 and e1 must be switched to follow out comparison logic (maybe rework this later?)
	return strcmp((char*)e2->text_buffer, (char*)e1->text_buffer);
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
	//func should return negative number when error and non-negative number of deleted entries
	//this function returns non-zero when err
	if (!list || !string || !func)
	{
		fprintf(stderr, "Err: Program passed NULL pointer into asc_index_map function! Ignoring current command...\n");
		return -1;
	}
	
	size_t i = 0, num = 0, deleted = 0, last = 0, load_end = 0;
	int func_ret = 0, num_ret = 0;
	
	while (string[i] != '\0')
	{
		num_ret = str_to_num(string + i, &load_end);
		if (num_ret < 0)
		{
			fprintf(stderr, "Err: Wrong index format, it can't be a negaitve number or text!\n");
		}
		else //ugly indentation, but whatever
		{
			num = (size_t)num_ret; //this should always cast correctly
			//printf("num to delete: %u\n", num);
			
			if (!num || num <= last || num < deleted + 1) //when index is zero or not in ascending order
			{
				if (num && num <= last) fprintf(stderr, "Err: Wrong index '%u', indices must be written in ascending order!\n", num);
				else fprintf(stderr, "Err: Wrong index '%u' specified, it can't be zero!\n", num);
			}
			//func shoould print it's own error msg when something goes wrong!
			else if ((func_ret = func(list, num - 1 - deleted, num)) >= 0) //so we can track how many was successfuly deleted
			{
				deleted += func_ret;
				last = num;
			}
		}
		//repetitive skipping to the next number to load
		i += load_end;
		while (string[i] != '\0' && !(isseparator(string[i]) || isspace(string[i]))) i++;
		while (isseparator(string[i]) || isspace(string[i])) i++;
	}
	
	return 0;
}

void print_todoentry(FILE *out, const todo_entry_t entry, const int style)
{	//prints todo entry into file 'out', if out is NULL then it does nothing
	if (!out) return;
	
	if (style)
	{
		//done/undone
		if (entry.status) fputs("[Y] ", out);
		else fputs("[N] ", out);
		
		//deadline
		if (style >= 2 && (is_date_valid(entry.deadline) || style >= 4))
		{
			write_date(out, entry.deadline);
			fputs(" | ", out);
		}
		
		//created date
		if (style >= 5)
		{
			write_date(out, entry.created_date);
			fputs(" | ", out);
		}
	}
	
	fputs(entry.text_buffer, out);
}

void print_todolist(const llist *list, const int style)
{	/*	style 0 - just the entry text
		style 1 - entry text, done/undone
		style 2 - entry text, done/undone, valid deadlines
		style 3 - entry text, done/undone, valid deadlines, index (from 1)
		style 4 - entry text, done/undone, all deadlines, index
		style 5 - entry text, done/undone, all deadlines, all created date, index*/
	size_t num = 1;
	for (struct node *n = list->first; n != NULL; n = n->next)
	{
		if (style > 2) printf("%u\t", num++);
		print_todoentry(stdout, *(n->val), style);
		putchar('\n');
	}
}

int cmd_print(llist *list, char *data_buffer)
{
	if (!list)
	{
		//TODO probably bad as the interactive while loops continues
		fprintf(stderr, "Err: Program passed NULL pointer into print command! Ignoring this command...\n");
		return -1;
	}
	
	int style = PRINT_DEFAULT;
	//skips to the end of data_buffer or start of a number
	while (*data_buffer && !isdigit((int)*data_buffer)) data_buffer++;
	if (*data_buffer) style = str_to_num(data_buffer, NULL); //only if there is a number
	
	if (style < 0)
	{
		fprintf(stderr, "Err: Unvalid style number in print command! Probably wrong number format.\n");
		return 1;
	}
	
	if (style > 5)
	{
		fprintf(stderr, "Err: Undefined style '%d' in print command!\n", style);
		return 2;
	}
	
	print_todolist(list, style);
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
	
	entry->status = is_done ? 1 : 0; //the ternary operator to be sure that status attribute is always 1 or 0
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

int cmd_change(llist *list, char *data_buffer, int is_verbose)
{
	if (!list || !data_buffer)
	{
		//TODO probably bad as the interactive while loops continues
		fprintf(stderr, "Err: Program passed NULL pointer into change command! Ignoring this command...\n");
		return -1;
	}
	
	//gets the number to change (only loads one)
	int num_ret = str_to_num(data_buffer, NULL);
	if (num_ret < 0)
	{
		fprintf(stderr, "Err: Wrong index format in change command!\n");
		return 1;
	}
	
	size_t num = (size_t)num_ret;
	if (!num)
	{
		fprintf(stderr, "Err: Wrong index '%u' specified, index can't be zero!\n", num);
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
		print_todoentry(stdout, *old_entry, 3); //this should print in some meaningful style
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
	
	if (is_verbose) printf("The #%u entry was changed successfully\n", num);
	
	return 0;
}

int cmd_move(llist *list, char *data_buffer)
{
	if (!list || !data_buffer)
	{
		//TODO probably bad as the interactive while loops continues
		fprintf(stderr, "Err: Program passed NULL pointer into move command! Ignoring this command...\n");
		return -1;
	}
	
	size_t from = 0, to = 0;
	if (parse_range(data_buffer, &from, &to, &data_buffer))
	{
		size_t num_offset = 0;
		int num_ret = str_to_num(data_buffer, &num_offset);
		
		if (num_ret < 0) from = 0; //this means we failed to read a number -> err is triggered at if (!from || !to) block
		else from = (size_t)num_ret;
		
		to = from;
		data_buffer = data_buffer + num_offset; //this sets data_buffer pointer after loaded from/to
	}
	
	if (!from || !to)
	{
		fprintf(stderr, "Err: Wrong moved range or index specified in the move command!\n");
		return 1;
	}

	//skipping whitespaces until next number or up/down
	while (*data_buffer && isspace((int)*data_buffer)) data_buffer++;
	
	size_t dir_offset = 0, num_offset = 0;
	int dir = parse_direction(data_buffer, &dir_offset);
	data_buffer += dir_offset; //if there is dir parsed it will offset data_buffer to point after
	
	int num_ret2 = str_to_num(data_buffer, &num_offset);
	if (num_ret2 < 0)
	{
		fprintf(stderr, "Err: You need to specify where/how much to move in move command!\n");
		return 6;
	}
	size_t where = (size_t)num_ret2;
	data_buffer += num_offset; //if there is where index it will offset data_buffer to point after

	//moving relatively up
	if (dir == 1)
	{
		if (!where) return 0; //nothing to do (we are moving 0 up)
		if (from <= where)
		{
			fprintf(stderr, "Err: Move command tries to move out of bounds!\n");
			return 5;
		}
		where = from - where;
	}
	//moving relatively down
	else if (dir == 2) where = to + where + 1;
	
	if (!where)
	{
		fprintf(stderr, "Err: Wrong destination index to be moved to specified in the move command!\n");
		return 2;
	}
	
	while (*data_buffer && isspace((int)*data_buffer)) data_buffer++;
	if (*data_buffer) //this means that there is still some nonempty text after where number
	{
		fprintf(stderr, "Err: There's unexpected text after index/range and index in the move command!\n");
		return 3;
	}

	int move_err = llist_move(list, from - 1, to - 1, where - 1); //-1 as llist index from 0
	if (move_err)
	{
		switch (move_err)
		{
			case -1:
				fprintf(stderr, "Err: Start '%u' of a range can't be bigger than the end '%u'!\n", from, to);
				break;
			case 1:
				fprintf(stderr, "Err: Can't move entries to index pointing on themselves!\n");
				break;
			case 2:
				if (!dir) fprintf(stderr, "Err: The index '%u' is out of bounds!\n", where);
				else fprintf(stderr, "Err: Move command tries to move out of bounds!\n");
				break;
			case 3: //TODO make this be printed instead in llist_move
				fprintf(stderr, "Err: Move command couldn't move specified range '%u-%u'!\n", from, to);
				break;
			default:
				fprintf(stderr, "Err: Unexpected error in the move command!\n");
				break;
		}
		return 4;
	}
	
	return 0;
}

void print_basichelp()
{
	//some basic help print
	puts("-------------HELP-------------");
	puts("Interactive mode commands are:");
	puts("\t'help' - prints this help on stdout");
	puts("\t'print' - prints all todolist entries in current memory on stdout");
	puts("\t'add' - adds one new entry into current todolist");
	puts("\t'delete' - deletes specified entries from current todolist");
	puts("\t'mark done/undone' - marks specified entries as done/undone");
	puts("\t'clear all/done/undone' - clears all/done/undone todolist entries");
	puts("\t'change' - changes one existing entry that you specify");
	puts("\t'move' - moves one or range of entries to specified index or relatively");
	puts("\t'swap' - swaps two specified entries in current todolist");
	puts("\t'sort' - sorts the todolist by done/undone/deadline/text/age");
	puts("------------------------------");
}

int cmd_help(char *data_buffer)
{
	if (!data_buffer)
	{
		//TODO probably bad as the interactive while loops continues
		fprintf(stderr, "Err: Program passed NULL pointer into help command! Ignoring this command...\n");
		return -1;
	}
	
	size_t index = 0;
	
	while (data_buffer[index] && isspace(data_buffer[index])) index++; //skipping the whitespaces
	
	if (!data_buffer[index])
	{
		print_basichelp();
		return 0;
	}
	
	char *cmd_start = data_buffer + index;
	
	while (data_buffer[index] && isalpha(data_buffer[index])) index++; //skipping to the end of word
	data_buffer[index] = '\0';
	
	enum CmdType cmd = help_c; //placeholder
	if (!parse_cmd_type(cmd_start, &cmd))
	{
		fprintf(stderr, "Err: Unknown command: '%s'! Type 'help' to get list of all known commands.\n", cmd_start);
		return 1;
	}
	
	switch(cmd) //TODO help print for each command
	{
	case help_c:
		puts("Command 'help' prints list of all implemented commands and their short description.");
		puts("Using 'help COMMAND' where COMMAND is valid command, you can get detailed help for given command.");
		puts("Example - 'help change'");
		break;
	case print_c:
		puts("Command 'print' prints the list of all todo-list entries in current memory.");
		puts("You can also specify the style of printing by numbers from 0 - the simplistic print, up to 5 - the most detailed print, the default is 3.");
		puts("The information you can get from print is: entry index, Y/N meaning done/undone, deadline date, date of creation of this entry, entry text.");
		puts("Example - 'print' or 'print 1'");
		break;
	case add_c:
		puts("Command 'add' adds new entry in the todo-list in current memory. You can either write this new entry on the same line as 'add' or on the next line.");
		puts("The input formatting is: X|DEADLINE|TEXT, where X is optional to mark this entry as done, DEADLINE is in format: DAY. MONTH. YEAR and is also optional, TEXT is obligatory and should be nonempty.");
		puts("Example - 'add X|Writing documentation' or 'add 6. 9. 2022|My birthday, yay'");
		break;
	case del_c:
		puts("Command 'delete' deletes one or more entries from todo-list loaded in current memory.");
		puts("You specify which entries to delete by writing their index (indexing from 1) and splitting them with any combination of characters '|' or a space.");
		puts("If you write more entries to be deleted then they must be in ascending order.");
		puts("Example - 'delete 2' or 'delete 1|2 3 8 | 11'");
		break;
	case mark_c:
		puts("Command 'mark' marks specified entries in current todo-list as done/undone.");
		puts("To mark them as done write 'mark done' and to mark them undone write 'mark undone', after that write list of indices of entries that you want to mark in ascending order. You can separate them either with '|' or a space.");
		puts("Example - 'mark done 3' or 'mark undone 1|2 3 8 | 11");
		break;
	case clear_c:
		puts("Command 'clear' deletes all entries in current todo-list that are either done or undone or all of them.");
		puts("You can specify which type to delete by writing 'clear done' or 'clear undone' or 'clear all'.");
		puts("Example - 'clear done'");
		break;
	case change_c:
		puts("Command 'change' allows you to change specified entry. You specify which entry to change by writing it's index.");
		puts("The entry then gets replaced by new entry that you will write on new line in the same formatting as in 'add' command.");
		puts("Example - 'change 7' followed by '12. 8. 2013|This is the replacement!'");
		break;
	case move_c:
		puts("Command 'move' moves one or range of entries to different index or relatively up/down.");
		puts("Format for absolute move is WHICH WHERE for moving entry at index WHICH to index WHERE, or START-END WHERE for moving range of entries starting at START and ending at END to index WHERE.");
		puts("Format for relative move is similar WHICH up/down AMOUNT or START-END up/down AMOUNT, up/down specifies direction of move and AMOUNT specifies how much.");
		puts("WHERE can also be index one higher than max. index - this moves specified entries at the end of the todo-list.");
		puts("Example - 'move 3 7' or 'move 7-10 3' or 'move 18-20 up 6' or 'move 7 down 1'");
		break;
	case swap_c:
		puts("Command 'swap' swaps two entries in todo-list.");
		puts("You need to specify two valid indices refering to those entries that will be swapped.");
		puts("Example - 'swap 1 2' or 'swap 18  9'");
		break;
	case sort_c:	//TODO better help for sort
		puts("Command 'sort' sorts the current todo-list by given parameter(s).");
		puts("Supported parameter to sort by: done, undone, deadline, age, text. You can specify more of them if you want series of sorting - it starts with first parameter and continues to the end one by one.");
		puts("If error occurres in the sorting series it stops at that point, leaving the list only sorted by parameters before error.");
		puts("Example - 'sort text' or 'sort deadline undone' (this is almost the same as 'sort deadline' followed by 'sort undone')");
		break;
	default:
		printf("Help for command: '%s' is not implemented yet or something wrong happened!\n", cmd_start);
		break;
	}
	
	return 0;
}

int cmd_swap(llist *list, char *data_buffer)
{
	if (!list || !data_buffer)
	{
		//TODO probably bad as the interactive while loops continues
		fprintf(stderr, "Err: Program passed NULL pointer into swap command! Ignoring this command...\n");
		return -1;
	}
	
	size_t num_offset = 0;
	int num_ret1 = str_to_num(data_buffer, &num_offset);
	data_buffer += num_offset; //offsetting data_buffer so it points after first num
	int num_ret2 = str_to_num(data_buffer, NULL);
	
	if (num_ret1 < 0 || num_ret2 < 0)
	{
		fprintf(stderr, "Err: Wrong index format in swap command!\n");
		return 3;
	}
	
	//casting should be okay here
	size_t idx1 = (size_t)num_ret1, idx2 = (size_t)num_ret2;

	if (!idx1 || !idx2)
	{
		fprintf(stderr, "Err: Not enough indices or wrong format specified in swap command!\n");
		return 1;
	}
	
	int swap_err = llist_swap(list, idx1 - 1, idx2 - 1);
	if (swap_err) //ignoring swap_err for now
	{
		fprintf(stderr, "Err: One or both indices '%u' '%u' are out of bounds!\n", idx1, idx2);
		return 2;
	}
	
	return 0;
}

int cmd_sort(llist *list, char *data_buffer)
{
	if (!list || !data_buffer)
	{
		//TODO probably bad as the interactive while loops continues
		fprintf(stderr, "Err: Program passed NULL pointer into sort command! Ignoring this command...\n");
		return -1;
	}
	
	//skipping initial whitespaces
	while (*data_buffer && isspace((int)*data_buffer)) data_buffer++;
	if (!*data_buffer) //no sorting happened
	{
		fprintf(stderr, "Err: You need to specify which parameter to sort by!\n");
		return 1;
	}

	char *next_word = NULL;
	while (*data_buffer) //while there's some text in data_buffer
	{
		next_word = next_word_skip(data_buffer); //next_word should not be NULL
		
		//sorting - we ignore return values as they are non-zero onůy for list == NULL (shouldn't happen here)
		if (!strcmp("done", data_buffer)) llist_sort(list, todo_compar_statdone);
		else if (!strcmp("undone", data_buffer)) llist_sort(list, todo_compar_statundone);
		else if (!strcmp("deadline", data_buffer)) llist_sort(list, todo_compar_deadline);
		else if (!strcmp("age", data_buffer)) llist_sort(list, todo_compar_created); //"age" might be bad term for this?
		else if (!strcmp("text", data_buffer)) llist_sort(list, todo_compar_text);
		else
		{	
			fprintf(stderr, "Err: Wrong parameter '%s' in sort command! Type 'help sort' for details.\n", data_buffer);
			return 2;
		}
		
		data_buffer = next_word;
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
		case help_c: return cmd_help(buffer);
		case print_c: return cmd_print(list, buffer);
		case add_c: return cmd_add(list, buffer);
		case del_c: return llist_asc_index_map(list, buffer, delete_entry);
		case mark_c: return cmd_mark(list, buffer);
		case clear_c: return cmd_clear(list, buffer);
		case change_c: return cmd_change(list, buffer, 1); //1 is for verbose mode (default in interactive)
		case move_c: return cmd_move(list, buffer);
		case swap_c: return cmd_swap(list, buffer);
		case sort_c: return cmd_sort(list, buffer);
		default: //this shouldn't normally happen
		fprintf(stderr, "Err: Wrong command type specified '%d' in command caller!\n", type);
		return 1;
	}
	return 0; //should not happen at this point
}

int parse_direction(const char *string, size_t *end_index)
{	//returns 1 if up, 2 if down, else returns 0
	//expects the potential 'up'/'down' to begin at index 0
	//if end_end is non-NULL it sets it at first char after possible 'up'/'down'
	if (!string[0] || !string[1] || (string[0] != 'u' && string[0] != 'd')) return 0;
	if (string[1] == 'p') //must be 'up' at the beginning
	{
		if (end_index) *end_index = 2; //setting the end_index
		return 1;
	}
	else if (string[1] != 'o') return 0;
	
	if (!string[2] || !string[3] || string[2] != 'w' || string[3] != 'n') return 0;
	
	//must be 'down' at the beginning
	if (end_index) *end_index = 4; 	//setting the end_index
	return 2;
}

int parse_range(char *string, size_t *start, size_t *end, char **range_end)
{	/*parses input from string in format "startnum-endnum"
	ignores whitespaces at the start and other text after
	returns non-zero if bounds not found*/
	while (*string && isspace((int)*string)) string++; //skipping initial whitespaces
	
	size_t offset = 0;
	int start_ret = str_to_num(string, &offset);
	if (start_ret < 0) return 1;
	
	char *dash = string + offset;
	if (!*dash || *dash != '-' || !isdigit((int)dash[1])) return 2; //dash shouldnt be NULL
	
	int end_ret = str_to_num(dash + 1, &offset);
	if (end_ret < 0) return 3;
	
	if (start) *start = (size_t)start_ret;
	if (end) *end = (size_t)end_ret;
	if (range_end) *range_end = dash + 1 + offset;
	
	/*
	//there must be a digit after whitespaces
	if (!string[index] || !isdigit((int)string[index])) return 1;
	
	//TODO string_num_end can be possibly replaced by usage of str_to_num
	char *num1 = string + index,
	 *dash = string_num_end(string + index, NULL),
	 *num2 = NULL;
	
	if (*dash != '-') return 2; //dash shouldnt be NULL
	char *num2_end = string_num_end(dash, &num2);
	if (!*num2) return 3; //the second number must be specified
	if (num2 - dash != 1) return 3; //epic pointer arithmetic hacking*/
	
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

	//IDEA more effective way
	if (!strcmp("help", cmd)) *type_ptr = help_c;
	else if (!strcmp("print", cmd))	*type_ptr = print_c;
	else if (!strcmp("add", cmd)) *type_ptr = add_c;
	else if (!strcmp("delete", cmd)) *type_ptr = del_c;
	else if (!strcmp("mark", cmd)) *type_ptr = mark_c;
	else if (!strcmp("clear", cmd)) *type_ptr = clear_c;
	else if (!strcmp("change", cmd)) *type_ptr = change_c;
	else if (!strcmp("move", cmd)) *type_ptr = move_c;
	else if (!strcmp("swap", cmd)) *type_ptr = swap_c;
	else if (!strcmp("sort", cmd)) *type_ptr = sort_c;
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
	//if error occurs then the file gets emptied, but usually there's not much of room for error
	int write_err = write_todofile(out_file, &list);
	
	fclose(out_file);
	llist_destroy_contents(&list);
	return write_err;
}
