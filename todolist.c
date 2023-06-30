// This file is part of 'todolist' project which author is Ondřej Richtr
// Seek more information about copyright in the LICENSE file included

#include "todolist.h"

#define PRINT_DEFAULT 3

//declarations:
int is_date_valid(const date_t date);

void date_null(date_t *date);

int is_todoentry_valid(todo_entry *entry);

//definitions:

//time handling
date_t get_today_date(void)
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

//comparators for todo_entry structs
//expecting non-NULL pointers, used as func.pointers -> no point in making them inline
//	if entries are same comparator returns 0
//	if first entry is 'greater' it retruns negative number
//	if first entry is 'smaller' it returns positive number
int todo_compar_statdone(const todo_entry *e1, const todo_entry *e2)
{
	//this assumess that status done is always same positive number and undone always zero
	return (int)e1->status - (int)e2->status;
	//return e1->status > e2->status ? -1 : (e1->status < e2->status ? 1 : 0);
	//return e1->status ? (e2->status ? 0 : 1) : (e2->status ? -1 : 0);
}
int todo_compar_statundone(const todo_entry *e1, const todo_entry *e2)
{
	//this assumess that status done is always same positive number and undone always zero
	return (int)e2->status - (int)e1->status;
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
int todo_compar_deadline(const todo_entry *e1, const todo_entry *e2)
{
	return compar_dates(e1->deadline, e2->deadline);
}
int todo_compar_created(const todo_entry *e1, const todo_entry *e2)
{
	return compar_dates(e1->created_date, e2->created_date);
}
int todo_compar_text(const todo_entry *e1, const todo_entry *e2)
{
	//e2 and e1 must be switched to follow out comparison logic (maybe rework this later?)
	return strcmp((char*)e2->text_buffer, (char*)e1->text_buffer);
}

//cli functionality
int generate_entry_from_string(const char* string, todo_entry *entry)
{	//fills todo_entry according to given C-style string
	//the string should be in format: [STATUS|][DEADLINE|]TEXT; STATUS and DEADLINE are optional
	//'|' is separator, there needs to be atleast one between each of STATUS, DEADLINE, TEXT.
	//returns zero if success, -1 if bad parameters and positive ints if error when parsing
	if (!string || !entry) return -1;
	
	size_t index = 0;
	
	//first letter is checked if it's status
	entry->status = 0;
	if (string[0] == 'X' && string[1] == '|') //this wont do buffer overflow
	{
		entry->status = 1;
		index++;
	}
	
	//skipping separators and spaces
	while (string[index] &&
	 (isspace((int)string[index]) || isseparator((int)string[index]))) index++;

	//loading deadline
	if (!isdigit((int)string[index])) date_null(&entry->deadline);
	else 
	{
		if (load_date_string(&entry->deadline, string + index))
		{
			fprintf(stderr, "Err: Wrong formating of date entered! Entry aborted.\n");
			return 1;
		}
		
		//date was loaded, skipping until text 
		while (string[index] && !isseparator((int)string[index])) index++;
	}
	
	//loading text
	//string index now points to either end of string or at separator before text or text
	if (isseparator(string[index])) index++;
	
	//now at end (empty text) or text (non empty)
	size_t copied = strcpy_buffer(TEXT_MAX_LEN, (char*)entry->text_buffer, string + index);
	utf8_last_trim((char*)entry->text_buffer, copied);
	
	//loading the time this entry was created (current time)
	entry->created_date = get_today_date();
	
	return 0;
}

int llist_asc_index_map(llist *list, const char *string,
						int(*func)(llist*, size_t, size_t, size_t))
{	//reads indices (or ranges) from given string and performs 'func' on entries from llist under such indices
	//indices (or ranges) must be specified in ascending order, otherwise they will be ignored
	//'func' should return negative number when error and non-negative number of deleted entries
	//this function returns non-zero and prints errmsg when error
	if (!list || !string || !func)
	{
		fprintf(stderr, "Err: Program passed NULL pointer into asc_index_map function! Ignoring current command...\n");
		return -1;
	}
	
	size_t index = 0, deleted = 0, last = 0, load_end = 0, start = 0, end = 0;
	int func_ret = 0, num_ret = 0, no_err = 1; //no_err is flag (0=false, 1=true)
	const char *range_end = NULL;
	
	while (string[index] != '\0')
	{
		//repetitive skipping to the next number to load
		while (string[index] && (isseparator(string[index]) || isspace(string[index]))) index++;
		if (!string[index]) break; //nothing after whitespace or separator

		if (!parse_range_const(string + index, &start, &end, &range_end)) //it is a range
		{
			index = range_end - string;
		}
		else //not a range
		{
			num_ret = str_to_num(string + index, &load_end);
			index += load_end;
			if (num_ret < 0)
			{
				//this should skip to the end of potential text or negative number (or anything that's not separator/space)
				while (string[index] && !isseparator(string[index]) && !isspace(string[index])) index++;
				fprintf(stderr, "Err: Wrong index format, it can't be a negaitve number or a text!\n");
				no_err = 0; //err happened
				break;
			}
			start = end = (size_t)num_ret;
		}
		
		if (!start || end < start || start <= last || start < deleted + 1) //when indices are zero or not in ascending order
		{

			if (start && end < start) fprintf(stderr, "Err: Start of a range '%u-%u' can't be larger than the end!\n", start, end);
			else if (!start) fprintf(stderr, "Err: Wrong index specified, it can't be zero!\n");
			else fprintf(stderr, "Err: Wrong index/range '%u', indices must be written in ascending order!\n", start);
			no_err = 0; //err happened
			break;
		}
		
		//there we calculate actual index from visual index (by -deleted-1)
		if ((func_ret = func(list, start - 1 - deleted, end - 1 - deleted, deleted + 1)) < 0)
		{
			no_err = 0; //func should print it's own error msg when something goes wrong!
			break;
		}
		
		//so we can track how many was successfuly deleted
		deleted += func_ret;
		last = end;
		
	}

	if (!no_err)
	{
		//IDEA make those errmsgs respect interactive/noninteractive modes
		if (!last) fprintf(stderr, "Because of previous error(s) there were no changes made.\n");
		else fprintf(stderr, "Because of previous error(s) changes stopped at index '%lu'.\n", last);
		return 1;
	}

	if (!last) //we print err when no other err happened and there was no successful func call
	{
		fprintf(stderr, "Err: Bad formating in current command! No changes were made.\n");
		return 2; //maybe shouldn't return err code?
	}
	
	return 0;
}

void print_todoentry(FILE *out, const todo_entry *entry, const int style)
{	//prints todo entry into file 'out', if out or entry is NULL then it does nothing
	//for style explanation see 'print_todolist' function
	if (!out || !entry) return;
	
	if (style)
	{
		//done/undone
		if (entry->status) fputs("[X] ", out);
		else fputs("[ ] ", out);
		
		//deadline
		if (style >= 2 && (is_date_valid(entry->deadline) || style >= 4))
		{
			write_date(out, entry->deadline);
			fputs(" | ", out);
		}
		
		//created date
		if (style >= 5)
		{
			write_date(out, entry->created_date);
			fputs(" | ", out);
		}
	}
	
	fputs(entry->text_buffer, out);
}

void print_todolist(const llist *list, const int style)
{	//	style 0 - just the entry text
	//	style 1 - entry text, done/undone
	//	style 2 - entry text, done/undone, valid deadlines
	//	style 3 - entry text, done/undone, valid deadlines, index (from 1)
	//	style 4 - entry text, done/undone, all deadlines, index
	//	style 5 - entry text, done/undone, all deadlines, all created date, index
	size_t num = 1;
	for (struct node *n = list->first; n != NULL; n = n->next)
	{
		if (style > 2) printf("%3u\t", num++);
		print_todoentry(stdout, n->val, style);
		putchar('\n');
	}
}

int cmd_print(llist *list, char *data_buffer)
{	//prints llist in specified style parsed from data_buffer
	//if no style parsed it prints in PRINT_DEFAULT (macro) style
	//when error it prints errmsg and returns nonzero
	if (!list || !data_buffer)
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
{	//adds new entry at the end of llist, the new entry is parsed form data_buffer
	//parsing is in format: [STATUS|][DEADLINE|]TEXT, STATUS and DEADLINE is optional
	//also checks whether added entry is valid (non-empty text or valid deadline)
	//when error it prints errmsg and returns nonzero
	if (!list || !data_buffer)
	{
		fprintf(stderr, "Err: Program passed NULL pointer into adding command! Ignoring this command...\n");
		return -1;
	}
	
	todo_entry *entry = malloc(sizeof(todo_entry));
	if (!entry)
	{
		fprintf(stderr, "Err: Failed to allocate %u bytes of memory!\n", sizeof(todo_entry));
		return 1;
	}
	
	//maybe this could cause problems?
	while (*data_buffer && isspace((int)*data_buffer)) data_buffer++; //skipping initial whitespaces
	
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
		fprintf(stderr, "Err: Failed to add following entry into the list!\nThe entry: ");
		print_todoentry(stderr, entry, 2);
		fputc('\n', stderr);
		free(entry);
		return 4;
	}
	
	return 0;
}

int delete_range(llist *list, size_t indexS, size_t indexE, size_t orig_offset)
{	//deletes given range of entries from linked list
	//returns -1 if the entries were out of bounds and prints the original range
	//or otherwise couldnt get delete them
	//and returns the number of deleted entries (indexE - indexS + 1) if they were deleted succesfuly
	if (llist_delete_range(list, indexS, indexE))
	{
		if (indexS == indexE) fprintf(stderr, "Err: Index '%u' to be deleted is out of bounds!\n", indexS + orig_offset);
		else fprintf(stderr, "Err: Range '%u-%u' to be deleted is out of bounds!\n", indexS + orig_offset, indexE + orig_offset);
		return -1;
	}
	return indexE - indexS + 1; //should be positive
}

int mark_range(llist *list, size_t indexS, size_t indexE, size_t orig_offset, int is_done)
{
	//returns negative number if something went wrong, otherwise returns 0 (-> nothing deleted)
	//IDEA make this to be a map into section of linked list
	size_t length = llist_length(list);
	struct node *n = llist_nth_node(list, indexS);
	if (!n)
	{
		if (indexS == indexE) fprintf(stderr, "Err: Index '%u' to be marked is out of bounds!\n", indexS + orig_offset);
		else fprintf(stderr, "Err: Range '%u-%u' to be marked is out of bounds!\n", indexS + orig_offset, indexE + orig_offset);
		return -1;
	}
	
	if (indexE >= length)
	{
		if (indexS == indexE) fprintf(stderr, "Err: Index '%u' to be marked is out of bounds!\n", indexS + orig_offset);
		else fprintf(stderr, "Err: Range '%u-%u' to be marked is overflowing the bounds!\n", indexS + orig_offset, indexE + orig_offset);
		return -2;
	}
	
	for (size_t i = indexS; i <= indexE; i++)
	{
		if (!n) //usually shluldnt happen (we checked the length before)
		{
			fprintf(stderr, "Err: Unexpected NULL in mark command! Entries starting from '%u' are left unchanged.", i + orig_offset);
			return -3;
		}
		
		n->val->status = is_done ? 1 : 0; //ternary operator to be sure that status attribute is always 1 or 0		
		n = n->next;
	}
	
	return 0;
}

int mark_range_done(llist *list, size_t indexS, size_t indexE, size_t orig_offset)
{	//marks entry at given index as done
	return mark_range(list, indexS, indexE, orig_offset, 1);
}

int mark_range_undone(llist *list, size_t indexS, size_t indexE, size_t orig_offset)
{	//same as mark_range_done but marks it undone
	return mark_range(list, indexS, indexE, orig_offset, 0);
}

int cmd_mark(llist *list, const char *data_buffer)
{	//marks one/range of entries from llist as done or undone
	//parses data_buffer as such: SPECIFIER INDEX or SPECIFIER FROM-TO
	//data_buffer should be always smaller or same size as CLI_LINE_MAX_LEN + 1
	//when error it prints errmsg and returns nonzero
	if (!list || !data_buffer)
	{
		//TODO probably bad as the interactive while loops continues
		fprintf(stderr, "Err: Program passed NULL pointer into mark command! Ignoring this command...\n");
		return -1;
	}
	
	while (*data_buffer && isspace((int)*data_buffer)) data_buffer++; //skipping initial whitespaces
	
	enum SpecType spec = all_c;
	char spec_buffer[CLI_LINE_MAX_LEN + 1] = { 0 };
	size_t spec_size = copy_until_delimiter(CLI_LINE_MAX_LEN, spec_buffer, data_buffer, isspace);	

	if (!parse_specifier_type((char*)spec_buffer, &spec) || spec == all_c)
	{
		//done/undone is not specified so there is nothing to do?
		fprintf(stderr, "Err: You need to specify if mark as done or undone!\n");
		return 1;
	}
	
	//skipping to the start of non-specifier in data_buffer
	data_buffer += spec_size;
	while (*data_buffer && isspace((int)*data_buffer)) data_buffer++;
		
	if (!*data_buffer) //data is empty (no indices)
	{
		fprintf(stderr, "Err: You need to specify indices for marking as done/undone!\n");
		return 2;
	}
	
	switch (spec)
	{
	case done_c:
		//possible err printed in llist_asc_index_map already
		if (llist_asc_index_map(list, data_buffer, mark_range_done)) return 3;
		break;
	case undone_c:
		//possible err printed in llist_asc_index_map already
		if (llist_asc_index_map(list, data_buffer, mark_range_undone)) return 3;
		break;
	default: //this should not happen (for now):
		fprintf(stderr, "Err: Unsupported specifier '%s' in mark command!\n", (char*)spec_buffer);
		return 4;
	}
	
	return 0;
}

int cmd_clear(llist *list, char *data_buffer)
{	//deletes all/done/undone entries from llist based on what it parses from data_buffer
	//when error it prints errmsg and returns nonzero
	if (!list || !data_buffer)
	{
		//TODO probably bad as the interactive while loops continues
		fprintf(stderr, "Err: Program passed NULL pointer into clear command! Ignoring this command...\n");
		return -1;
	}
	
	while (*data_buffer && isspace((int)*data_buffer)) data_buffer++; //skipping initial whitespaces
	
	enum SpecType spec = done_c;
	char *end = word_skip(data_buffer);
	*end = '\0'; //cutting off excess data that we dont need
	
	if (parse_specifier_type(data_buffer, &spec))
	{
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

int cmd_change(llist *list, char *data_buffer, FILE *input, int is_verbose, int noninter)
{	//allows one specified entry in llist to be replaced (changed) by new entry,
	//it parses the index of said entry from data_buffer, prints instructions if is_verbose
	//if noninter false then it gets data for new entry from given input stream
	//else from the rest of data_buffer
	//assumess input != NULL for noninter == false
	//when empty string entered as new entry then it does nothing
	//when error it prints errmsg and returns nonzero
	if (!list || !data_buffer)
	{
		//TODO probably bad as the interactive while loops continues
		fprintf(stderr, "Err: Program passed NULL pointer into change command! Ignoring this command...\n");
		return -1;
	}
	
	//gets the number to change (only loads one)
	size_t offset = 0; //only for noniter mode
	int num_ret = str_to_num(data_buffer, &offset);
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

	todo_entry *old_entry = llist_nth_entry(list, num - 1);
	if (!old_entry)
	{
		fprintf(stderr, "Err: Index '%u' to be changed is out of bounds!\n", num);
		return 3;
	}
	
	if (is_verbose)
	{
		fprintf(stdout, "You are about to change the #%u entry: '", num);
		print_todoentry(stdout, old_entry, 3); //this should print in some meaningful style
		fputs("'\nWrite changed version at the next line or you can abort by writing an empty line\n", stdout);
	}
	
	char *entry_str = NULL, line_buffer[CLI_LINE_MAX_LEN + 1] = { 0 }; //buffer only for inter. mode
	
	if (noninter) //non-interactive mode - does not use line_buffer, uses data_buffer instead
	{
		entry_str = data_buffer + offset;
		while (*entry_str && isspace((int)*entry_str)) entry_str++; //skipping whitespaces after num
		if (!*entry_str)
		{
			fprintf(stderr, "Err: No text for new entry specified!\n");
			return 6;
		}
	}
	else //interactive mode - uses line_buffer from stdout and not data_buffer
	{
		size_t line_len = utf8_readline(input, CLI_LINE_MAX_LEN, line_buffer, NULL);
		if (!line_len)
		{
			if (is_verbose) puts("Entry was left unchanged."); //UNSURE stderr instead of stdout?
			return 0;
		}
		
		entry_str = (char*)line_buffer;
		utf8_last_trim(entry_str, line_len);
		
		if (!line_buffer[0]) //utf8_readline loaded something, but it was trimmed away by utf8_last_trim
		{
			fprintf(stderr, "Err: No valid UTF-8 data inputted!\n");
			return 7;
		}
	}
	
	todo_entry new_entry;
	int gen_err = generate_entry_from_string(entry_str, &new_entry);
	if (gen_err)
	{
		//error message should get printed by generate_entry_splitted
		if (is_verbose) fprintf(stderr, "Leaving old entry unchanged\n");
		return 4;
	}
	
	if (!is_todoentry_valid(&new_entry)) //we check if the final entry makes sense, if not discard it
	{
		fprintf(stderr, "Err: Entered entry is not valid! (Has no text and no date) Leaving old entry unchanged\n");
		return 5;
	}
	
	//change the old entry into new one
	*old_entry = new_entry;
	if (is_verbose) printf("The #%u entry was changed successfully\n", num); //UNSURE stderr insted of stdout?
	
	return 0;
}

int cmd_move(llist *list, char *data_buffer)
{	//moves one entry or range of entries in llist to specified index
	//it parses the data_buffer as such: OLDPOS NEWPOS or FROM-TO NEWPOS
 	//when error it prints errmsg and returns nonzero
	if (!list || !data_buffer)
	{
		//TODO probably bad as the interactive while loops continues
		fprintf(stderr, "Err: Program passed NULL pointer into move command! Ignoring this command...\n");
		return -1;
	}
	
	size_t from = 0, to = 0;
	if (parse_range(data_buffer, &from, &to, &data_buffer)) //this also skips initial whitespaces
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
			case 3: //no need to print this err, it should be printed in more detail in llist_move
				//fprintf(stderr, "Err: Move command couldn't move specified range '%u-%u'!\n", from, to);
				break;
			default:
				fprintf(stderr, "Err: Unexpected error in the move command!\n");
				break;
		}
		return 4;
	}
	
	return 0;
}

void print_basichelp(int isoption)
{	//just prints basic help for interactive mode or help mode (when isoption == true)
	if (isoption)
	{	//non-interactive mode
		puts("Usage:   todo [OPTION]");
		puts("         todo [OPTION] COMMAND");
		puts("         todo [OPTION] [-e COMMAND]");
		puts("\nExample: todo -f ./path/to/myfile");
		puts("         todo 'add My important note here'");
		puts("         todo -e 'sort done deadline' -e print");
		puts("\nOptions:");
		puts("  -v,  --version\tPrints version and author.");
		puts("  -h,  --help\t\tPrints this help.");
		puts("  --help=[COMMAND]\tPrints detailed help for all COMMAND commands.");
		puts("  -f <FILE>\t\tUse todo file at location FILE.");
		puts("\nNon-interactive mode commands are:");
		puts("  'print' - prints all todolist entries in current memory on stdout");
		puts("  'add' - adds one new entry into current todolist");
		puts("  'delete' - deletes specified entries from current todolist");
		puts("  'mark done/undone' - marks specified entries as done/undone");
		puts("  'clear all/done/undone' - clears all/done/undone todolist entries");
		puts("  'change' - changes one existing entry that you specify");
		puts("  'move' - moves one or range of entries to specified index or relatively");
		puts("  'swap' - swaps two specified entries in current todolist");
		puts("  'sort' - sorts the todolist by done/undone/deadline/text/age");
		return;
	}
	
	//interactive mode
	puts("------------INTERACTIVE HELP-------------");
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
	puts("-----------------------------------------");
}

void print_commandhelp(enum CmdType cmd, const char* cmd_str, int isoption)
{	//prints help text for specified command
	switch(cmd)
	{
	case help_c:
		if (isoption)
		{
			puts("Command 'help' is not allowed in non-interactive mode.");
			puts("Use one of '-h', '--help', '--help=COMMAND', '--help=COMMAND1 COMMAND2 ...' options.");
			break;
		}
		puts("Command 'help' prints list of all implemented commands and their short description.");
		puts("Using 'help COMMAND ...' where COMMAND are valid commands separated by space, you can get detailed help for given command(s).");
		puts("Example - 'help change' or 'help swap sort print'");
		break;
	case print_c:
		puts("Command 'print' prints the list of all todo-list entries in current memory.");
		printf("You can also specify the style of printing by numbers from 0 - the simplistic print, up to 5 - the most detailed print, the default is %d.\n", PRINT_DEFAULT);
		puts("The information you can get from print is: entry index, Y/N meaning done/undone, deadline date, date of creation of this entry, entry text.");
		puts("Example - 'print' or 'print 1'");
		break;
	case add_c:
		puts("Command 'add' adds new entry into the todo-list in current memory.");
		puts("The input formatting is: X|DEADLINE|TEXT, where X is optional to mark this entry as done, DEADLINE is in format: DAY. MONTH. YEAR and is also optional, TEXT is obligatory and should be nonempty.");
		puts("Example - 'add X|Writing documentation' or 'add 6. 9. 2023|My birthday, yay'");
		break;
	case del_c:
		puts("Command 'delete' deletes one or more entries from todo-list loaded in current memory.");
		puts("You specify which entries to delete by writing their index (indexing from 1) and splitting them with '|' or a space.");
		puts("You can also use range of indices with FROM-TO syntax.");
		puts("If you write more entries (or ranges) to be deleted then they must be in ascending order.");
		puts("Example - 'delete 2' or 'delete 1-2 3 8 | 11|16-20'");
		break;
	case mark_c:
		puts("Command 'mark' marks specified entries in current todo-list as done/undone.");
		puts("To mark them as done write 'mark done' and to mark them undone write 'mark undone', after that write list of indices of entries that you want to mark in ascending order.");
		puts("You can also use range of indices with FROM-TO syntax.");
		puts("Separate them either with '|' or a space.");
		puts("Example - 'mark done 3' or 'mark undone 1-2 3 8 | 11|16-20");
		break;
	case clear_c:
		puts("Command 'clear' deletes all entries in current todo-list that are either done or undone or all of them.");
		puts("You can specify which type to delete by writing 'clear done' or 'clear undone' or 'clear all'.");
		puts("Example - 'clear done'");
		break;
	case change_c:
		puts("Command 'change' allows you to change specified entry. You specify which entry to change by writing it's index.");
		if (isoption)
		{
			puts("The entry gets replace by new entry that will be constructed from text that you wrote after it's index.");
			puts("Example - 'change 7 12. 8. 2013|This is the replacement!'");
			break;
		}
		puts("The entry then gets replaced by new entry that you will write on new line in the same formatting as in 'add' command.");
		puts("Example - 'change 7' followed by '12. 8. 2013|This is the replacement!' on the next line");
		break;
	case move_c:
		puts("Command 'move' moves one or range of entries to different index or relatively up/down.");
		puts("Format for absolute move is WHICH WHERE for moving entry at index WHICH to index WHERE, or START-END WHERE for moving range of entries starting at START and ending at END to index WHERE.");
		puts("Format for relative move is similar WHICH up/down AMOUNT or START-END up/down AMOUNT, up/down specifies direction of move and AMOUNT specifies how much.");
		puts("WHERE can also be index one higher than maximal index - this moves specified entries to the end of the todo-list.");
		puts("Example - 'move 3 7' or 'move 7-10 3' or 'move 18-20 up 6' or 'move 7 down 1'");
		break;
	case swap_c:
		puts("Command 'swap' swaps two entries in todo-list.");
		puts("You need to specify two valid indices refering to entries that will be swapped.");
		puts("Example - 'swap 1 2' or 'swap 18  9'");
		break;
	case sort_c:
		puts("Command 'sort' sorts the current todo-list by given parameter(s).");
		puts("Supported parameter to sort by: done, undone, deadline, age, text. You can specify more of them if you want series of sorting - it starts with first parameter and continues to the end one by one.");
		puts("If error occurres in the sorting series it stops at that point, leaving the list only sorted by parameters before error.");
		puts("Example - 'sort text' or 'sort deadline undone' (this is almost the same as 'sort deadline' followed by 'sort undone')");
		break;
	default:
		printf("Help for command: '%s' is not implemented yet or something wrong happened!\n", cmd_str);
		break;
	}
}

int cmd_help(char *data_buffer, int isoption)
{	//help command, prints basic help for interactive mode and -h/--help option mode (when isoption)
	//or it prints help for specific command when specified (long help when isoption)
	//when error it prints errmsg and returns nonzero
	if (!data_buffer)
	{
		//TODO probably bad as the interactive while loops continues
		fprintf(stderr, "Err: Program passed NULL pointer into help command! Ignoring this command...\n");
		return -1;
	}
	
	//skipping the initial whitespaces
	while (*data_buffer && isspace((int)*data_buffer)) data_buffer++;
	
	if (!*data_buffer)
	{
		print_basichelp(isoption);
		return 0;
	}
	
	int first = 1;
	while (*data_buffer)
	{
		//skips to the end of possible cmd word
		char *data_tail = next_word_skip(data_buffer);
		
		enum CmdType cmd = help_c; //placeholder
		if (!parse_cmd_type(data_buffer, &cmd))
		{
			if (isoption) fprintf(stderr, "Err: Unknown command: '%s'! Use '-h' or '--help' option to get the list of all possible commands.\n", data_buffer);
			else fprintf(stderr, "Err: Unknown command: '%s'! Type 'help' to get the list of all possible commands.\n", data_buffer);
			return 1;
		}
		
		//splitting helps with newline for better readibility
		if (first) first = 0;
		else putchar('\n');

		print_commandhelp(cmd, data_buffer, isoption);
		data_buffer = data_tail; //while loops continues from the data_til
	}
	
	return 0;
}

int cmd_help_noninter_parse(size_t argc, const char** argv)
{	//prints help for more commands in helpmode_c,
	//accepted format is any number of '--help=CMD' or '--help=CMD1 CMD2 CMD3...'
	//other arguments are ignored unless they seem similar, returns nonzero when error
	if (!argv)
	{
		fprintf(stderr, "Err: Program passed NULL pointer into help mode!\n");
		return -1;
	}
	
	unsigned longhelps_count = 0, errored_count = 0;
	char cmd_buffer[CMD_NAME_MAX_LEN + 1] = { 0 };
	
	for (size_t i = 1; i < argc; i++)
	{
		if (!argv[i])
		{
			fprintf(stderr, "Err: Porgram got unexpected NULL argument when performing help mode!\n");
			return 1;
		}
		
		int parse_basichelp = parse_opt_basichelp(argv[i]);
		//not an error, argument is not close to '--help=CMD' syntax
		if (parse_basichelp >= 0) continue;

		//at this point we know that the string starts with "--help"
		//all of the following errs should be already recognized in parse_option_string
		int parse_cmdhelp = parse_opt_cmdhelp(argv[i]);
		if (!parse_cmdhelp) //missing equals in "--help=" syntax
		{
			errored_count++;
			fprintf(stderr, "Err: Syntax error when parsing help option! Usage is '-h', '--help', '--help=CMD' or '--help=CMD1 CMD2 CMD3...'.\n");
			continue; //maybe return
		}
		if (parse_cmdhelp < 0) //command in "--help=CMD" syntax is blank
		{
			errored_count++;
			fprintf(stderr, "Err: Syntax error when parsing command help - The specified command was empty!\n");
			continue; //maybe return
		}
		
		//handling of "--help=CMD" where CMD is nontrivial (nonempty with atleast one non-whitespace char)
		//whitespaces get copied too (but cmd_help will ignore them)
		size_t cmdhelpstr_len = sizeof("--help=")/sizeof(char) - 1; //-1 as we dont count term.char.
		strncpy((char*)cmd_buffer, argv[i] + cmdhelpstr_len, CMD_NAME_MAX_LEN); //the rest of possible cmd string is ignored
		cmd_buffer[CMD_NAME_MAX_LEN] = '\0'; //just to be sure
		
		if (!cmd_buffer[0])
		{
			//bad error, this should be already checked by parse_opt_cmdhelp!
			fprintf(stderr, "Err: Copy of command was unexpectedly empty when parsing help option '%s'!\n", argv[i]);
			return 2;
		}
		
		if (longhelps_count) putchar('\n'); //sepparating helps for each cmd if there's more than one
		
		//UNSURE maybe dont return
		int ret = cmd_help((char*)cmd_buffer, 1); //1 means that help was asked through options
		if (ret == 1) return 3; //unknown cmd, err was already printed
		else if (ret) return 4;	//some unexpected err that shouldn't typically happen, err printed
		
		longhelps_count++;
	}
	
	if (!longhelps_count)
	{
		fprintf(stderr, "Err: Unexpected error happened when resolving '--help=CMD' options!\n");
		return 5;
	}
	
	return 0;
}

int cmd_swap(llist *list, char *data_buffer)
{	//swaps positions of two entries in llist
	//indices of swapped entries needs to be parsed from data_buffer
	//when error it prints errmsg and returns nonzero
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
{	//sorts the llist according to specified criteria (str in data_buffer)
	//there can be multiple criteria and they are applied one after another
	//sorts based on following keywords: done, undone, deadline, age, text
	//when error it prints errmsg and returns nonzero
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
		
		//sorting - we dont print errmsg when err in llist_sort as it prints it's own (also might happen extremly rarily)
		if (!strcmp("done", data_buffer))
		{
			if (llist_sort(list, todo_compar_statdone)) return 3;
		}
		else if (!strcmp("undone", data_buffer))
		{
			if (llist_sort(list, todo_compar_statundone)) return 3;
		}
		else if (!strcmp("deadline", data_buffer))
		{
			if (llist_sort(list, todo_compar_deadline)) return 3;
		}
		else if (!strcmp("age", data_buffer)) //"age" might be bad term for this?
		{
			if (llist_sort(list, todo_compar_created)) return 3;
		}
		else if (!strcmp("text", data_buffer))
		{
			if (llist_sort(list, todo_compar_text)) return 3;
		}
		else
		{	
			fprintf(stderr, "Err: Wrong parameter '%s' in sort command! Type 'help sort' for details.\n", data_buffer);
			return 2;
		}
		
		data_buffer = next_word;
	}
	
	return 0;
}

int cmd_load() //TODO switching files
{
	
}

int do_inter_cmd(llist *list, enum CmdType type, char *buffer, FILE *input)
{	//calls correct functionality specified by command type argument
	//passes in buffer if the function requires it
	//error msgs are printed by functions themselves, except for wrong type
	//returns non-zero if the command couldn't be executed successfuly
	switch (type)
	{
		case help_c: return cmd_help(buffer, 0); //0 for an interactive mode (1 is --help mode)
		case print_c: return cmd_print(list, buffer);
		case add_c: return cmd_add(list, buffer);
		case del_c: return llist_asc_index_map(list, buffer, delete_range);
		case mark_c: return cmd_mark(list, buffer);
		case clear_c: return cmd_clear(list, buffer);
		case change_c: return cmd_change(list, buffer, input, 1, 0); //1 for verbose, 0 for interactive mode
		case move_c: return cmd_move(list, buffer);
		case swap_c: return cmd_swap(list, buffer);
		case sort_c: return cmd_sort(list, buffer);
		default: //this shouldn't normally happen
			fprintf(stderr, "Err: Unimplemented command type '%d'!\n", type);
			//fprintf(stderr, "Err: Wrong command type specified '%d' in command caller!\n", type);
		return 1;
	}
	return 0; //should not happen at this point
}

int do_noninter_cmd(llist *list, enum CmdType type, const char *data)
{	//calls correct command specified by CmdType enum,
	//error msgs are printed by functions themselves, except for wrong type (which typically should not happen here)
	//returns non-zero if the command couldn't be executed successfuly
	
	//maximum of characters that are taken into account from data string
	char buffer[CLI_LINE_MAX_LEN + 1] = { 0 }, *buffer_ptr = (char*)buffer;
	
	//making copy so commands can modify the string
	//strncpy(buffer_ptr, data, CLI_LINE_MAX_LEN);
	size_t copied = strcpy_buffer(CLI_LINE_MAX_LEN, buffer_ptr, data);
	utf8_last_trim(buffer_ptr, copied);
	
	switch (type)
	{
		case help_c:
		{
			fprintf(stderr, "Err: Help command is not implemented in non-interactive mode! Use options '-h', --help' or help in interactive mode.\n");
			return 2;
		}
		case print_c: return cmd_print(list, buffer_ptr);
		case add_c: return cmd_add(list, buffer_ptr);
		case del_c: return llist_asc_index_map(list, buffer_ptr, delete_range);
		case mark_c: return cmd_mark(list, buffer_ptr);
		case clear_c: return cmd_clear(list, buffer_ptr);
		case change_c: return cmd_change(list, buffer_ptr, NULL, 0, 1); //0 for not verbose, 1 for noninteractive mode
		case move_c: return cmd_move(list, buffer_ptr);
		case swap_c: return cmd_swap(list, buffer_ptr);
		case sort_c: return cmd_sort(list, buffer_ptr);
		default: //this should get checked already in parse_options
			fprintf(stderr, "Err: Unimplemented command type '%d'!\n", type);
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
{	//parses input from string in format "startnum-endnum"
	//ignores whitespaces at the start and other text after
	//returns non-zero if bounds not found
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
	
	return 0;
}
int parse_range_const(const char *string, size_t *start, size_t *end, const char **range_end) //same as parse_range but with const char*
{	//parses input from string in format "startnum-endnum"
	//ignores whitespaces at the start and other text after
	//returns non-zero if bounds not found
	while (*string && isspace(*string)) string++; //skipping initial whitespaces
	
	size_t offset = 0;
	int start_ret = str_to_num(string, &offset);
	if (start_ret < 0) return 1;
	
	const char *dash = string + offset;
	if (!*dash || *dash != '-' || !isdigit(dash[1])) return 2; //dash shouldnt be NULL
	
	int end_ret = str_to_num(dash + 1, &offset);
	if (end_ret < 0) return 3;
	
	if (start) *start = (size_t)start_ret;
	if (end) *end = (size_t)end_ret;
	if (range_end) *range_end = dash + 1 + offset;
	
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

int parse_cmd_type(const char *cmd, enum CmdType *type_ptr)
{	//returns zero if not suppworted type, otherwise returns 1
	//and fills type_ptr with correct type, type_ptr and cmd mustnt be NULL!

	//IDEA more effective way
	if (!strcmp("help", cmd)) *type_ptr = help_c;
	else if (!strcmp("print", cmd)) *type_ptr = print_c;
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

int is_valid_cmd(const char *str, enum CmdType *type)
{	//returns nonzero when str contains at the beginning valid command
	//ignores initial whitespaces, assumes that 'str' is a valid non-NULL string
	char str_copy[CMD_NAME_MAX_LEN + 1] = { 0 }, *copy_ptr = (char*)str_copy; //+1 for term. char.

	//skipping initial whitespaces of the input
	while (*str && isspace((int)*str)) str++;
	
	strncpy(copy_ptr, str, CMD_NAME_MAX_LEN); //let's hope the upper bound works
	
	//while (*copy_ptr && isspace((int)*copy_ptr)) copy_ptr++; //skipping initial whitespaces
	char *cmd_end = word_skip(copy_ptr); //shouldnt return NULL
	//IDEA dont split and just check for first n letters
	*cmd_end = '\0'; //slicing cmd name away from the possible rest
	
	enum  CmdType tmp; //throwaway variable (parse_cmd_type does not take NULL)
	return type ? parse_cmd_type(copy_ptr, type) : parse_cmd_type(copy_ptr, &tmp);
}

int inter_cmd(FILE *input, llist *list, char *str)
{	//parses which command to be done from the buffer,
	//then executes corresponding cli function,
	//returns 1 if wrong command or other non-zero if error

	//skipping initial whitespaces
	while (*str && isspace((int)*str)) str++;
	
	//splitting cmd and tail - they both shouldnt have initial whitespaces now
	char* tail_ptr = next_word_skip(str);
		
	enum CmdType type;
	if (!parse_cmd_type(str, &type))
	{
		fprintf(stderr, "Err: Unknown command: '%s'! Type 'help' to get the list of all possible commands.\n", str);
		return 1;
	}
	
	if (do_inter_cmd(list, type, tail_ptr, input)) return 2; //err msg printed inside of cmds
	return 0;
}

int interactive_mode(FILE *input, const char *todo_file_path)
{	//reads lines from input until EOF and interprets them as cli commands
	//todo_file is_path is the path to file where current todo list entries
	//are possibly stored and also where the result will be written
	
	llist list = { NULL, NULL };
	if (load_entries(&list, todo_file_path))
	{
		//this err msg assumess that more detailed err msg had been already printed by load_entries
		fprintf(stderr,"Abort: Exiting the interactive mode because of previous error(s)!\n");
		return 1;
	}
	
	char line_buffer[CLI_LINE_MAX_LEN + 1] = { 0 };
	size_t line_len = 0;
	int inter_err = 0; // no use for this now
	
	//IDEA prompt?
	while ((line_len = utf8_readline(input, CLI_LINE_MAX_LEN, line_buffer, NULL)))
	{	//also means that loaded line is not an empty string
		//err gets ignored as the program can continue and err msg is already printed
		inter_err = inter_cmd(input, &list, line_buffer);
	}
	
	int write_err = write_backedup_todofile(todo_file_path, &list); //detailed err printed inside
	if (write_err)
	{
		fprintf(stderr, "Abort: Because of previous error all changes were DISCARTED!\n");
	}
	
	llist_destroy_contents(&list);
	return write_err ? 2 : 0;
}

int noninter_cmd(llist *list, const char *str)
{
	while (*str && isspace((int)*str)) str++; //skipping initial whitespaces
	
	enum CmdType type;
	if (!is_valid_cmd(str, &type))
	{
		//this shouldnt normally happen (noninter mode usually gets commands checked in parse_options)
		fprintf(stderr, "Err: No valid command was parsed from '%s'!\n", str);
		return 1;
	}
	
	const char *data = word_skip_const(str); //skipping after the command part
	
	if (do_noninter_cmd(list, type, data)) return 2; //err msg printed inside of cmds
	return 0;
}

int noninteractive_mode(const size_t options_num, const char **options, const char *todo_file_path)
{	//Handles the non-interactive commands from options arr, they must be specified after -e option
	//with exception when using single command - then it must be at the end of argv array
	//assumess options has options_num strings (NULLs shouldn't appear there), returns nonzero when error
	
	llist list = { NULL, NULL };
	if (load_entries(&list, todo_file_path))
	{
		//this err msg assumess that more detailed err msg had been already printed by load_entries
		fprintf(stderr,"Loading of todo entries from file failed! Aborting - no command were done.\n");
		return 1;
	}
	
	size_t cmd_counter = 0;
	int loop_err = 0;
	for (size_t i = 1; i < options_num; i++)
	{
		if (!options[i])
		{
			loop_err = 1; //might rarely happen even after checks in parse_options
			break;
		}
		
		if (i + 1 >= options_num) //the last item in the options array
		{
			if (!strcmp("-e", options[i])) //if it's -e
			{
				loop_err = 2; //should not happen because of checks in parsing options
				break;
			}
		}
		else
		{
			if (strcmp("-e", options[i])) continue; //if it's NOT -e - we just skip
			i++; //safe to do as i + 1 < options_num
		}
		
		if (!options[i])
		{
			loop_err = 3; //might rarely happen even after checks in parse_options
			break;
		}
		
		if (noninter_cmd(&list, options[i])) //noninter_cmd return value gets ignored
		{
			loop_err = 4;
			break;
		}
	}
	
	if (loop_err)
	{
		switch (loop_err)
		{
		case 1:
		case 3:
			fprintf(stderr, "Err: Non-interactive mode encountered unexpected NULL as input command!\n");
			break;
		case 2:
			fprintf(stderr, "Err: Syntax error in Non-interactive mode while processing commands!\n");
			break;
		case 4:
			//no need to print anything as noninter_cmd should print it's own errors
			break;
		default:
			fprintf(stderr, "Err: Unexpected error happened in Non-interactive mode!\n");
			break;
		}
		
		fprintf(stderr, "Abort: Because of previous error(s) there were NO CHANGES MADE!\n");
		llist_destroy_contents(&list);
		return 3;
	}

	int write_err = write_backedup_todofile(todo_file_path, &list); //detailed err printed inside
	if (write_err)
	{
		fprintf(stderr, "Abort: Because of previous error(s) there were NO CHANGES MADE!\n");
	}
	
	llist_destroy_contents(&list);
	return write_err ? 2 : 0;
}
