#include "todolist.h"


/*
	WARNING FOR CODE READERS!
	THIS .c FILE WAS WRITTEN AS ONE OF THE FIRST AND HASN'T 
	BEEN REFACTORED YET! BEWARE OF BAD CODE QUALITY! thanks
*/

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

size_t copy_until_delimiter(size_t max_size, char buffer[max_size + 1], const char* source, int(*delim)(int))
{	/*copies from source as much characters until delim. function returns true
	or it reaches max size or end of source
	puts null char at the end of loaded buffer, similar to srcpy_buffer*/
	if (!source) return 0;
	
	size_t index = 0;
	
	while (source[index] != '\0' && !delim((int)source[index]) && index < max_size)
	{
		buffer[index] = source[index];
		index++;
	}
	
	buffer[index] = '\0';
	return index;
}

char* next_word_skip(char *string)
{	//skipts to the START of the NEXT word or end of string
	//and puts term. char at the END of the FIRST word
	if (!string) return NULL;
	
	while (*string && !isspace((int)*string)) string++;
	char *word_end = string;
	while (*string && isspace((int)*string)) string++;
	*word_end = '\0';
	
	return string;
}

char* word_skip(char *string)
{	//skipts to the END of the current word
	if (!string) return NULL;
	
	while (*string && !isspace((int)*string)) string++;
	
	return string;
}

size_t readline(FILE *f, size_t max_size, char buffer[max_size + 1])
{	/*reads single line from 'f' until newline char or EOF,
	if the input is larger than buffer it ONLY stops storing it (unlike load_buffer),
	puts null character in buffer at the end of loaded string
	return number of characters loaded (excluding null char)*/
	if (!f) return 0;
	
	int c;
	size_t index = 0;
	
	while ((c = fgetc(f)) != EOF && c != '\n')
	{
		if (index < max_size) buffer[index++] = (char)c;
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

int str_to_num(const char *string, size_t *end_index)
{	//skips initial whitespaces and then tries to read as much digits as possible
	//supports and returns only non-negative numbers for now
	//returns negative number when no digits encountered
	if (!string) return -1;
	
	size_t index = 0;
	while (string[index] && isspace((int)string[index])) index++; //skipping spaces
	
	//means we have nothing to read (encountered end of string or some other nonspace text)
	if (!isdigit((int)string[index]))
	{
		if (end_index) *end_index = index;
		return -2;
	}
	
	int num = 0;
	
	while (string[index] && isdigit((int)string[index])) //reading the digits
	{
		num = num * 10 + (int)string[index] - '0';
		index++;
	}
	
	if (end_index) *end_index = index;
	return num;
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

char* string_num_end(char *num_start, char **new_start) //TODO is this useless now?
{	//find first number in given string 'num_start' and returns where this number ends
	//if new_start is not NULL then stores start of this number there
	if (!num_start) return NULL;
	
	while (*num_start != '\0' && !isdigit((int)*num_start)) num_start++;
	
	if (new_start != NULL) *new_start = num_start; //setting where number starts
	
	while (*num_start != '\0' && isdigit((int)*num_start)) num_start++;
	return num_start;
}

int load_date(FILE *f, date_t *d, int c)
{	//loads from stream 'f' date to given 'd', first char given as 'c'
	//returns nonzero if error occurred
	if (!f || !d) return 1;
	date_null(d); //nulling the date otherwise load works badly
	
	if (!load_num_tolerant_8(f, &(d->day), &c)) return -1;
	c = fgetc(f);
	if (!load_num_tolerant_8(f, &(d->month), &c)) return -1;
	c = fgetc(f);
	if (!load_num_tolerant_16(f, &(d->year), &c)) return -1;
	return 0;
}

int load_date_string(date_t *d, char *str)
{	//loads date from given string, returns non-null if not all numbers were loaded
	//IDEA maybe add checks whether we load exactly 3 numbers (zeroes included)
	if (!d || !str) return -1;
	
	size_t index = 0;
	
	int day_ret = str_to_num(str, &index);
	str += index;
	//we dont need to set index to zero after, as str_to_num sets it always for non NULL string
	if (*str == '.') str++; //skipping the dot
	
	int month_ret = str_to_num(str, &index);
	str += index;
	if (*str == '.') str++; //skipping the dot
	
	int year_ret = str_to_num(str, NULL);
	
	if (day_ret < 0 || month_ret < 0 || year_ret < 0) return 1; //err is not needed
	
	d->day = (uint_least8_t)day_ret;
	d->month = (uint_least8_t)month_ret;
	d->year = (uint_least16_t)year_ret;
	
	return 0;
}

size_t load_buffer(FILE *f, char buffer[TEXT_MAX_LEN], int *in_char) //TODO probably useless - readline is better
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

void strcpy_buffer(size_t buffer_size, char *buffer, const char *source)
{	//copies content of source string into buffer of given size + 1 (null char)
	//does nothing if given invalid pointers
	if (!buffer || !source) return;
	
	size_t index = 0;
	for (; index < buffer_size && source[index] != '\0'; index++)
	{	//unoptimized solution but whatever
		buffer[index] = source[index];
	}
	//printf("ending index: %u\n", index);
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
		default: return 2;				//otherwise
	}

	while (isseparator(c = fgetc(f))); //skipping separators
	if (load_date(f, &entry->deadline, c)) return 3;
	
	while (isseparator(c = fgetc(f))); //skipping separators
	if (load_date(f, &entry->created_date, c)) return 4;
	
	while (isseparator(c = fgetc(f))); //skipping separators
	size_t size = load_buffer(f, (char*)&entry->text_buffer, &c);
	
	//this should be always possible as the length of buffer is TEXT_MAX_SIZE + 1
	entry->text_buffer[size] = '\0';
	
	/*printf("Test buffer: '%s'\n", (char*)entry->text_buffer);
	for (size_t i = 0; i < size; i++) printf("%d ", (int)entry->text_buffer[i]);
	putchar('\n');*/
	
	if (c != '\n') skip_until(f, &c, '\n'); //skips to end of line or EOF
	
	return 0;
}

int load_entries(llist *list, const char *path)
{	//loads entries from specified file into linked list (should be empty)
	//returns 0 if success, non-zero if failure which empties the linked list
	FILE *f = fopen(path, "r");
	if (f == NULL) //couldn't open file
	{
		fprintf(stderr, "Err: Couldn't read todo file at path: '%s'!\n", path);
		return 1;
	}
		
	todo_entry_t *entry = NULL;
	int status = 0;
	
	while (!status)
	{
		//entry = malloc(sizeof(todo_entry_t));
		//memset((void*)entry, 0, sizeof(todo_entry_t)); //nulling the entry
		
		entry = calloc(1, sizeof(todo_entry_t)); //using calloc so the entry is nulled already
		
		if (entry == NULL)
		{	//entry couldn't get allocated
			fprintf(stderr, "Err: Couldn't allocate memory of %u bytes!\n", sizeof(todo_entry_t));
			llist_destroy_contents(list);
			fclose(f);
			return 2;
		}
		
		if ((status = load_one_entry(f, entry)) > 0)
		{	//positive return value means something went wrong
			//TODO there could be more detailed error messages
			fprintf(stderr, "Err: Loading of one specific todo entry failed! Probably wrong format.\n");
			llist_destroy_contents(list);
			free(entry);
			fclose(f);
			return 3;
		}

		if (status == -1) free(entry);			//EOF -> entry gets deleted
		else if (!llist_add_end(list, entry))	//Success -> entry gets added to list
		{	//adding to list failed
			//same error message as in add_entry_splitted:
			fprintf(stderr, "Err: Failed to add following entry into the list!\nThe entry: '");
			print_todoentry(stderr, *entry, 0);
			fputs("'\n", stderr);
			llist_destroy_contents(list);
			free(entry);
			fclose(f);
			return 4;
		}
	}
	
	fclose(f);
	return 0;
}
