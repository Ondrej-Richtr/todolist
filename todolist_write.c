#include "todolist.h"


void write_buffer(FILE *f, char* buffer)
{	//writes buffer into file,
	//buffer should have size smaller or equal as TEXT_MAX_LEN
	for (size_t index = 0; index < TEXT_MAX_LEN && buffer[index]; index++)
		fputc(buffer[index], f);
}

int write_date(FILE *out, const date_t date)
{	//prints given date into file 'out' in style: DAY MONTH YEAR
	return fprintf(out, "%d. %d. %d", date.day, date.month, date.year);
	//return 0;
}

int write_one_entry(FILE *f, todo_entry_t *entry)
{
	if (!f || !entry) return 1;
	
	switch (entry->status) //TODO maybe print an error when not 0 or 1?
	{
		case 0: fputc(' ', f);
		break;
		default: fputc('X', f);	//usually 1
		break;
	}
	
	fputc('|', f);
	if (write_date(f, entry->deadline) < 0) return 2;
	fputc('|', f);
	if (write_date(f, entry->created_date) < 0) return 3;
	fputc('|', f);
	write_buffer(f, (char*)&entry->text_buffer);
	fputc('\n', f);
	
	return 0;
}

int write_entries(FILE *f, llist *list)
{	/*writes entries from linked list into given file
	returns non-zero if error and prints err msg*/
	//if (!f || !list) return -1; //useless probably
	int err_write = 0;
	
	for (struct node *n = list->first; n != NULL; n = n->next)
	{
		if (err_write = write_one_entry(f, n->val))
		{	//err_write value gets ignored
			//same error message as in add_entry_splitted:
			fprintf(stderr, "Err: Failed to write following entry into the todo file!\n");
			fprintf(stderr, "The entry: ");
			print_todoentry(stderr, *(n->val), 1);
			fputc('\n', stderr);
			return 1;
		}
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

void write_prompt() //IDEA maybe inline this?
{	//writes prompt into stdout
	fputs("> ", stdout);
}
