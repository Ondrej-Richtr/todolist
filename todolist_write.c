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
	
	switch (entry->status)
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
