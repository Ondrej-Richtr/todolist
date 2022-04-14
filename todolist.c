#include "todolist.h"


//declaratons:
int is_date_valid(const date_t date);


//definitions:
void print_date(date_t date)
{
	printf("%d. %d. %d", date.day, date.month, date.year);
}

void print_todoentry(todo_entry_t entry, int style)
{
	//ignores style parameter for now
	if (entry.status) printf("[Y] ");
	else printf("[N] ");
	
	if (is_date_valid(entry.deadline))
	{
		print_date(entry.deadline);
		printf(" | ");
	}
	
	puts(entry.text_buffer);
	//printf("%s\n", entry.text_buffer);
}

void node_destroy(struct node *n)
{
	if (n == NULL) return;
	
	free(n->val);
	free(n);
}

void llist_add_node_end(llist *list, struct node *n)
{
	if (list->last == NULL) list->first = n;
	else list->last->next = n;
	
	list->last = n;
	n->next = NULL;
}

void llist_add_node_first(llist *list, struct node *n)
{
	n->next = list->first;
	
	if (list->last == NULL) list->last = n;
	list->first = n;
}

struct node* llist_pop_node_first(llist *list)
{
	if (list->first == NULL) return NULL;
	
	struct node *out = list->first;
	if (list->last == out) list->last = NULL;
	
	list->first = out->next;
	
	return out;
}

int llist_add_end(llist *list, todo_entry_t *val)
{
	if (list == NULL) return 0;
	
	struct node *n = malloc(sizeof(struct node));
	
	if (n == NULL) return 0;
	
	n->next = NULL;
	n->val = val;
	llist_add_node_end(list, n);
	
	return 1;
}

int llist_add_first(llist *list, todo_entry_t *val)
{
	if (list == NULL) return 0;
	
	struct node *n = malloc(sizeof(struct node));
	
	if (n == NULL) return 0;
	
	n->next = NULL;
	n->val = val;
	llist_add_node_first(list, n);
	
	return 1;
}

void llist_destroy_contents(llist *list)
{
	struct node *n = NULL;
	
	while ((n = llist_pop_node_first(list)) != NULL)
	{
		node_destroy(n);
	}
}

int isseparator(int c)
{
	return c == '|';
}

int load_num_8(FILE *f, uint_least8_t* num, int *in_char) //8 bit version
{	//expects first (given) character to be already number (digit)
	//only works with unsigned numbers
	if (!f || !num || !in_char) return -1;
	
	int c = *in_char;
	
	while (c != EOF && c >= '0' && c <= '9')
	{
		*num *= 10;
		*num += (uint_least8_t)(c - '0');
		c = fgetc(f);
	}
	
	*in_char = c;
	return 0;
}

int load_num_16(FILE *f, uint_least16_t* num, int *in_char) //16 bit version
{	//expects first (given) character to be already number (digit)
	//only works with unsigned numbers
	if (!f || !num || !in_char) return -1;
	
	int c = *in_char;
	
	while (c != EOF && c >= '0' && c <= '9')
	{
		*num *= 10;
		*num += (uint_least16_t)(c - '0');
		c = fgetc(f);
	}
	
	*in_char = c;
	return 0;
}

int load_num_tolerant_8(FILE *f, uint_least8_t* num, int *in_char) //8 bit version
{	//tolerates whitespaces at the beginning of the number
	if (!f || !num || !in_char) return -1;
	
	while (*in_char != EOF && isspace(*in_char)) *in_char = fgetc(f);
	
	return load_num_8(f, num, in_char);
}

int load_num_tolerant_16(FILE *f, uint_least16_t* num, int *in_char) //16 bit version
{	//tolerates whitespaces at the beginning of the number
	if (!f || !num || !in_char) return -1;
	
	while (*in_char != EOF && isspace(*in_char)) *in_char = fgetc(f);
	
	return load_num_16(f, num, in_char);
}

int load_date(FILE *f, date_t *d, int c)
{	//loads from stream 'f' date to given 'd', first char given as 'c'
	//returns nonzero if error occurred
	if (!f || !d) return 1;
	
	if (load_num_tolerant_8(f, &(d->day), &c)) return -1;
	c = fgetc(f);
	if (load_num_tolerant_8(f, &(d->month), &c)) return -1;
	c = fgetc(f);
	if (load_num_tolerant_16(f, &(d->year), &c)) return -1;
	return 0;
}

size_t load_buffer(FILE *f, char *buffer, int *in_char)
{	/*loads text from file into buffer, until it reaches max size or newline or EOF
	returns amount of characters loaded, 0 also when given invalid input
	if given non-NULL in_char then it takes that char as first character and returns last char there*/
	if (!f || !buffer) return 0;
	
	size_t count = 0;
	int c;
	if (in_char == NULL) c = fgetc(f);
	else c = *in_char;
	
	while (c != EOF && c != '\n' && count < TEXT_MAX_LEN)
	{
		buffer[count++] = (char)c;
		c = fgetc(f);
	}
	
	if (in_char != NULL)*in_char = c;
	return count;
}

int load_one_entry(FILE *f, todo_entry_t *entry)
{	//returns 0 if success, current line should not be comment, otherwise it returns 1
	if (!f || !entry) return 1;
	
	int c = fgetc(f);
	
	switch(c)
	{
		case ' ': entry->status = 0;	//not done
		break;
		case 'X': entry->status = 1;	//done
		break;
		default: return 1;				//otherwise
	}

	while (isseparator(c = fgetc(f))); //skipping separators
	if (load_date(f, &entry->deadline, c)) return 1;
	
	while (isseparator(c = fgetc(f))); //skipping separators
	if (load_date(f, &entry->created_date, c)) return 1;
	
	while (isseparator(c = fgetc(f))); //skipping separators
	size_t size = load_buffer(f, &entry->text_buffer, &c);
	
	//this should be always possible as the length of buffer is TEXT_MAX_SIZE + 1
	entry->text_buffer[size] = '\0';
	
	return 0;
}

int load_entries(llist *list, const char *path)
{	//loads entries from specified file into linked list (should be empty)
	//returns 0 if success, non-zero if failure which empties the linked list
	FILE *f = fopen(path, "r");
	if (f == NULL) return 1;
	
	struct node *n = NULL;
	todo_entry_t *entry = NULL;
	
	while (!feof(f))
	{
		n = malloc(sizeof(struct node));
		if (n == NULL)
		{
			//TODO destroy linked list
			return 2;
		}
		entry = malloc(sizeof(todo_entry_t));
		if (entry == NULL)
		{
			//TODO destroy linked list
			free(n);
			return 2;
		}
	}
	
	fclose(f);
	return 0;
}

//main things
void print_llist(llist *list)
{
	for (struct node *n = list->first; n != NULL; n = n->next)
	{
		print_todoentry(*(n->val), 0);
	}
}

int main()
{
	date_t d1 = {15, 7, 1999}, d2 = {13, 9, 2002};
	todo_entry_t e1 = { .status=0, .created_date={2, 3, 2022}, .deadline=d1 };
	todo_entry_t e2 = { .status=1, .created_date={1, 1, 2222}, .deadline=d2 };
	
	struct node *n1 = malloc(sizeof(struct node));
	n1->val = &e1;
	n1->next = NULL;
	struct node *n2 = malloc(sizeof(struct node));
	n2->val = &e2;
	n2->next = NULL;
	llist list = { .first=NULL, .last=NULL};
	
	llist_add_node_end(&list, n1);
	llist_add_node_first(&list, n2);
	
	print_llist(&list);
	
	free(n1);
	free(n2);
	return 0;
}
