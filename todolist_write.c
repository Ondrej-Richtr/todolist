#include "todolist.h"


//USELESS
/*void write_buffer(FILE *f, const char* buffer)
{	//writes buffer into file,
	//buffer should have size smaller or equal as TEXT_MAX_LEN
	for (size_t index = 0; index < TEXT_MAX_LEN && buffer[index]; index++)
		fputc(buffer[index], f);
}*/

int write_date(FILE *out, const date_t date)
{	//prints given date into file 'out' in style: DAY MONTH YEAR
	return fprintf(out, "%d. %d. %d", date.day, date.month, date.year);
	//return 0;
}

int write_one_entry(FILE *f, todo_entry *entry)
{	//writes given entry into given file, returns nonzero when err
	//file ptr must be non-NULL!
	if (!entry) return 1;
	
	switch (entry->status)
	{
	case 0: fputc(' ', f);
		break;
	default:
		fprintf(stderr, "Warning: Status of a entry has unexpected value '%d'! Interpeting entry as done.\nThe entry: ", entry->status);
		print_todoentry(stderr, entry, 2);
		fputc('\n', stderr);
	case 1:
		fputc('X', f);	//usually 1
		break;
	}
	
	fputc('|', f);
	if (write_date(f, entry->deadline) < 0) return 2;
	fputc('|', f);
	if (write_date(f, entry->created_date) < 0) return 3;
	fputc('|', f);
	fputs((char*)entry->text_buffer, f);
	fputc('\n', f);
	
	return 0;
}

int write_entries(FILE *f, llist *list)
{	//writes entries from linked list into given file
	//returns non-zero if error and prints err msg
	//file and list points must be non-NULL!
	
	for (struct node *n = list->first; n != NULL; n = n->next)
	{
		int write_err = write_one_entry(f, n->val);
		if (!write_err) continue;
		
		//write_err value gets ignored
		//same error message as in cmd_add:
		fprintf(stderr, "Err: Failed to write following entry into the todo file!\nThe entry: ");
		print_todoentry(stderr, n->val, 2);
		fputc('\n', stderr);
		return 1;
	}
	
	return 0;
}

void write_standard_comments(FILE *f)
{	//writes the standard comment lines at the beginning of each todofile
	//which means after each run of this todo program only those comment lines will be in todofile
	fputs("# Lines marked with '#' at the beginning are comments.\n", f);
	fputs("# This file stores todo entries for todo program, you typically\
 shouldn't modify this file directly (but it's up to you).\n", f);
	fputs("# Note that after each run of todo program this file gets wiped\
 out and rewritten again, which means any other comments other then those\
 three will get deleted.\n", f);
}

int write_todofile(FILE *f, llist *list)
{	//writes standard comment lines and entries from list into given file
	//returns non-zero if err and prints errmsg
	if (!f || !list)
	{
		fprintf(stderr, "Err: Function writing todofile recieved NULL!\n");
		return -1;
	}
	
	write_standard_comments(f);
	if (write_entries(f, list)) return 1; //errmsgs printed inside of write_entries
	return 0;
}
