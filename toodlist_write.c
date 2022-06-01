#include "todolist.h"


void write_buffer(FILE *f, char* buffer)
{	//writes buffer into file,
	//buffer should have size smaller or equal as TEXT_MAX_LEN
	for (size_t index = 0; index < TEXT_MAX_LEN && buffer[index]; index++)
		fputc(buffer[index], f);
}

int write_date(FILE *f, const date_t date)
{
	return fprintf(f, "%d. %d. %d", date.day, date.month, date.year);
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
{
	int out = 0;
	if (!f || !list) return 1;
	
	for (struct node *n = list->first; n != NULL; n = n->next)
	{
		if (out = write_one_entry(f, n->val))
		{
			//printf("err: %d\n ", out);
			return 2;
		}
	}
	
	return 0;
}
