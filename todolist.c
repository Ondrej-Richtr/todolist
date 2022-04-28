#include "todolist.h"


//declaratons:
int is_date_valid(const date_t date);


//definitions:

//date_t
int write_date(FILE *f, const date_t date)
{
	return fprintf(f, "%d. %d. %d", date.day, date.month, date.year);
}

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

//reading functions
int isseparator(int c)
{
	return c == '|';
}

int isempty(char* str)
{
	return !str || !str[0];
}

void skip_until(FILE *f, int *in_char, char until)
{
	while (*in_char != EOF && (char)*in_char != until) *in_char = fgetc(f);
}

size_t copy_until_sep(size_t max_size, char buffer[max_size + 1], char* source)
{	/*copies from source as much characters until it reaches max size or end of source
	puts null char at the end of loaded buffer
	similar to srcpy_buffer*/
	if (!source) return 0;
	
	size_t index = 0;
	
	while (source[index] != '\0' && !isseparator(source[index]) && index < max_size)
	{
		buffer[index] = source[index];
		index++;
	}
	
	buffer[index] = '\0';
	return index;
}

size_t readline(FILE *f, size_t max_size, char buffer[max_size + 1])
{	/*reads single line from 'f' until newline char or EOF
	puts null character at correct end of buffer
	return number of characters loaded (excluding null char)*/
	if (!f) return 0;
	
	int c;
	size_t index = 0;
	
	while ((c = fgetc(f)) != EOF && c != '\n' && index < max_size)
	{
		buffer[index++] = (char)c;
	}
	
	buffer[index] = '\0';
	return index;
}

void skip_comment_blank_lines(FILE *f, int *in_char)
{	/*only works if in_char is '#' or '\n'
	skips to the beginning of the next non-comment non-empty line or EOF*/
	while (*in_char != EOF && ((char)*in_char == '#' || (char)*in_char == '\n'))
	{
		skip_until(f, in_char, '\n');
		//now in_char contains newline char or EOF
		if (*in_char != EOF) *in_char = fgetc(f); //if is probably pointless
	}
}

size_t load_num_8(FILE *f, uint_least8_t* num, int *in_char) //8 bit version
{	/*expects first (given) character to be already number (digit)
	only works with unsigned numbers, returns numbers of digits that it read*/
	if (!f || !num || !in_char) return 0;
	
	int c = *in_char;
	size_t count = 0;
	
	while (c != EOF && c >= '0' && c <= '9')
	{
		*num *= 10;
		*num += (uint_least8_t)(c - '0');
		c = fgetc(f);
		count++;
	}
	
	*in_char = c;
	return count;
}

size_t load_num_16(FILE *f, uint_least16_t* num, int *in_char) //16 bit version
{	/*expects first (given) character to be already number (digit)
	only works with unsigned numbers, returns numbers of digits that it read*/
	if (!f || !num || !in_char) return 0;
	
	int c = *in_char;
	size_t count = 0;
	
	while (c != EOF && c >= '0' && c <= '9')
	{
		*num *= 10;
		*num += (uint_least16_t)(c - '0');
		c = fgetc(f);
		count++;
	}
	
	*in_char = c;
	return count;
}

int load_num_tolerant_8(FILE *f, uint_least8_t* num, int *in_char) //8 bit version
{	//tolerates whitespaces at the beginning of the number
	//returns amount of digits that it read
	if (!f || !num || !in_char) return -1;
	
	while (*in_char != EOF && isspace(*in_char)) *in_char = fgetc(f);
	
	return load_num_8(f, num, in_char);
}

int load_num_tolerant_16(FILE *f, uint_least16_t* num, int *in_char) //16 bit version
{	//tolerates whitespaces at the beginning of the number
	//returns amount of digits that it read
	if (!f || !num || !in_char) return -1;
	
	while (*in_char != EOF && isspace(*in_char)) *in_char = fgetc(f);
	
	return load_num_16(f, num, in_char);
}

char* string_num_end(char *num_start, char **new_start)
{	//find first number in given string 'num_start' and returns where this number end
	//if new_start is not NULL then stores start of this number there
	if (!num_start) return NULL;
	
	while (*num_start != '\0' && !isdigit(*num_start)) num_start++;
	
	if (new_start != NULL) *new_start = num_start; //setting where number starts
	
	while (*num_start != '\0' && isdigit(*num_start)) num_start++;
	return num_start;
}

int load_date(FILE *f, date_t *d, int c)
{	//loads from stream 'f' date to given 'd', first char given as 'c'
	//returns nonzero if error occurred
	if (!f || !d) return 1;
	
	if (!load_num_tolerant_8(f, &(d->day), &c)) return -1;
	c = fgetc(f);
	if (!load_num_tolerant_8(f, &(d->month), &c)) return -1;
	c = fgetc(f);
	if (!load_num_tolerant_16(f, &(d->year), &c)) return -1;
	return 0;
}

int load_date_string(date_t *d, char *str_start)
{	//loads date from given string, returns non-null if not all numbers were loaded
	if (!d || !str_start) return -1;
	
	char *str_end = NULL;
	int num = 0;
	
	str_end = string_num_end(str_start, &str_start);
	if (str_end == NULL) return 1;	//failed at loading 1
	//printf("end char: '%c'\n", *str_end);
	d->day = (uint8_t)atoi(str_start);
	str_start = str_end;
	
	str_end = string_num_end(str_start, &str_start);
	if (str_end == NULL) return 2;	//failed at loading 2
	//printf("end char: '%c'\n", *str_end);
	d->month = (uint8_t)atoi(str_start);
	str_start = str_end;
	
	str_end = string_num_end(str_start, &str_start);
	if (str_end == NULL) return 3;	//failed at loading 3
	//printf("end char: '%c'\n", *str_end);
	d->year = (uint16_t)atoi(str_start);	
	
	return 0;
}

size_t load_buffer(FILE *f, char buffer[TEXT_MAX_LEN], int *in_char)
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

void strcpy_buffer(size_t buffer_size, char *buffer, char *source)
{	//copies content of source string into buffer of given size + 1 (null char)
	//does nothing if given invalid pointers
	if (!buffer || !source) return;
	
	size_t index = 0;
	for (; index < buffer_size && source[index] != '\0'; index++)
	{	//unoptimized solution but whatever
		buffer[index] = source[index];
	}
	printf("ending index: %u\n", index);
	buffer[index] = '\0';
}

int load_one_entry(FILE *f, todo_entry_t *entry)
{	/*loads entry from given file, ignores commented lines (starting with '#')
	returns 0 if success, -1 if it reached the EOF, otherwise it positive number*/
	if (!f || !entry) return 1;
	
	int c = fgetc(f);
	skip_comment_blank_lines(f, &c);	//skips commented lines and empty lines
	
	switch(c)
	{
		case EOF: return -1;
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
	size_t size = load_buffer(f, (char*)&entry->text_buffer, &c);
	
	//this should be always possible as the length of buffer is TEXT_MAX_SIZE + 1
	entry->text_buffer[size] = '\0';
	
	if (c != '\n') skip_until(f, &c, '\n'); //skips to end of line or EOF
	
	return 0;
}

int load_entries(llist *list, const char *path)
{	//loads entries from specified file into linked list (should be empty)
	//returns 0 if success, non-zero if failure which empties the linked list
	FILE *f = fopen(path, "r");
	if (f == NULL) return 1;	//couldn't open file
	
	todo_entry_t *entry = NULL;
	int status = 0;
	
	while (!status)
	{
		entry = malloc(sizeof(todo_entry_t));
		if (entry == NULL)
		{	//entry couldn't get allocated
			llist_destroy_contents(list);
			fclose(f);
			return 2;
		}
		
		if ((status = load_one_entry(f, entry)) > 0)
		{	//positive return value means something went wrong
			llist_destroy_contents(list);
			free(entry);
			fclose(f);
			return 3;
		}

		//printf("status: %d\n", status);

		if (status == -1) free(entry);			//EOF -> entry gets deleted
		else if (!llist_add_end(list, entry))	//Success -> entry gets added to list
		{	//adding to list failed
			llist_destroy_contents(list);
			free(entry);
			fclose(f);
			return 4;
		}
	}
	
	fclose(f);
	return 0;
}

//writing functions
void write_buffer(FILE *f, char* buffer)
{
	for (size_t index = 0; index < TEXT_MAX_LEN && buffer[index]; index++)
		fputc(buffer[index], f);
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

//cli funcionality
int add_entry_splitted(llist *list, char status, char *orig_date, char *dead_date, char *text)
{
	todo_entry_t *entry = malloc(sizeof(todo_entry_t));
	if (!entry) return 1;
	
	entry->status = 0;
	if (status == 'X') entry->status = 1;
	
	if (dead_date && load_date_string(&entry->deadline, dead_date))
	{
		free(entry);
		return 2;
	}
	if (orig_date && load_date_string(&entry->created_date, orig_date))
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
{	//adds entry described by C-style string
	//returns zero if success
	if (!list || !string) return -1;
	
	size_t index = 0;
	char status = ' ';
	//buffer for deadline number and for text
	char num_buffer[NUM_BUFFER_SIZE + 1], text_buffer[TEXT_MAX_LEN + 1];
	
	//first letter is checked if it's status
	if (string[0] == 'X')
	{
		index++;
		status = 'X';
	}
	
	//skipping separators
	while (string[index] != '\0' && isseparator(string[index])) index++;

	//loading deadline
	if (isdigit(string[index]))
	{
		index += 1 + copy_until_sep(NUM_BUFFER_SIZE, num_buffer, string + index);
	}
	
	//loading text
	strcpy_buffer(TEXT_MAX_LEN, (char*)text_buffer, string + index);
	
	return add_entry_splitted(list, status, NULL, (char*)num_buffer, (char*)text_buffer);
}

//main things
void print_llist(llist *list)
{
	for (struct node *n = list->first; n != NULL; n = n->next)
	{
		print_todoentry(*(n->val), 0);
		puts("------------");
	}
}

int main()
{
	char string[129];
	llist list = { NULL, NULL };
	
	size_t string_len = readline(stdin, 128, string);
	string[string_len] = '\0';
	printf("input: '%s'\n", string);
	
	if (add_entry_string(&list, string))
	{
		puts("error");
		llist_destroy_contents(&list);
		return 1;
	}
	if (list.first == NULL) puts("Nothing");
	else
	{
		print_llist(&list);
	}

	llist_destroy_contents(&list);
	return 0;
}

int main2()
{
	const char *path_r = "./longerfile", *path_w = "./newfile", *path_w2 = "./newfile2";
	llist list = { NULL, NULL };
	
	int out = load_entries(&list, path_r);
	if (out)
	{
		printf("Error during reading: %d\n", out);
		return 1;
	}
	
	print_llist(&list);
	
	FILE *file = fopen(path_w, "w");
	out = write_entries(file, &list);
	if (out)
	{
		printf("Error during writing: %d\n", out);
		return 1;
	}
	
	llist_destroy_contents(&list);
	fclose(file);
	return 0;
}
