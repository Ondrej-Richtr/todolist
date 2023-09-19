// This file is part of 'todolist' project which author is Ondřej Richtr
// Seek more information about copyright in the LICENSE file included

#include "todolist.h"

#define BAD_CHAR ('?')


int isseparator(int c)
{
	return c == '|';
}

unsigned utf8_1prefix(const unsigned char byte)
{	//returns the length of 1s prefix in byte
	unsigned char mask = 1 << 7;
	int length = 0;
	
	while (byte & mask)
	{
		length++;
		mask >>= 1;
	}
	
	return length;
}

int utf8_bytelen(const unsigned char byte)
{	//returns the byte length of the utf8 character beginning with 'byte',
	//for data carrying prefix returns -1 and for too long prefix -2
	unsigned prefix_len = utf8_1prefix(byte);
	
	if (prefix_len == 0) return 1;	//ascii
	if (prefix_len == 1) return -1;	//payload char
	if (prefix_len > 4) return -2;	//nonsensical utf8 header
	
	return prefix_len;				//non-ascii utf8 header
}

int utf8_load(FILE *f, char *output, size_t output_capacity)
{	//asumess that f is valid file stream and buffer_ptr is non-NULL
	int c = getc(f);
	if (c == EOF) return 0;
	
	int len = utf8_bytelen((unsigned char)c);
	if (len == 0) return -1; //should not happen
	if (len < 0) return -2; //bad UTF-8 format
	if ((size_t)len > output_capacity) return -3;
	
	output[0] = (char)c;
	for (int index = 1; index < len; index++)
	{
		//skipping first iteration as we already got that character
		c = getc(f);
		if (c == EOF || utf8_1prefix((unsigned char)c) != 1) return -4; //bad UTF-8 format
		
		output[index] = c;
	}
	
	return len;
}

void skip_until(FILE *f, int *in_char, char until)
{
	while (*in_char != EOF && (char)*in_char != until) *in_char = getc(f);
}

size_t copy_until_delimiter(size_t max_size, char buffer[max_size + 1], const char* source, int(*delim)(int))
{	//copies from source as much characters until delim. function returns true
	//or it reaches max size or end of source
	//puts null char at the end of loaded buffer, similar to srcpy_buffer
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
{	//skips to the START of the NEXT word or end of string
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

const char* word_skip_const(const char *string)
{	//skipts to the END of the current word
	if (!string) return NULL;
	
	while (*string && !isspace((int)*string)) string++;
	
	return string;
}

/*size_t readline(FILE *f, size_t max_size, char buffer[max_size + 1])
{	//reads single line from 'f' until newline char or EOF,
	//if the input is larger than buffer it ONLY stops storing it,
	//puts null character in buffer at the end of loaded string
	//returns number of characters written (excluding null char)
	//asumess that given file stream is non-NULL
	int c;
	size_t length = 0;
	
	while ((c = fgetc(f)) != EOF && c != '\n')
	{
		//length++ must happen only if it is under the boundary
		if (length < max_size) buffer[length++] = (char)c;
	}
	
	buffer[length] = '\0';
	return length;
}*/

size_t utf8_readline(FILE *f, size_t max_size, char buffer[max_size + 1], size_t *text_len)
{	//reads single line in UTF-8 format from 'f' until newline char or EOF,
	//if the input is larger than buffer it ONLY stops storing it,
	//if wrong UTF-8 format it replaces the bad data with BAD_CHAR,
	//puts null character in buffer at the end of loaded string
	//returns number of bytes written (excluding null char),
	//if text_len != NULL it stores amount of actual text characters in it
	//asumess that given file stream is non-NULL
	size_t byte_len = 0, _text_len = 0;
	int end_reached = 0;
	
	while (byte_len < max_size)
	{
		int loaded = utf8_load(f, (char*)buffer + byte_len, max_size - byte_len);
		
		if (loaded == 0)					//EOF encountered
		{
			end_reached = 1;
			break;
		}
		
		if (loaded == -3) break;			//not enough space in the buffer
		else if (loaded < 0)				//bad UTF-8 format
		{
			buffer[byte_len] = BAD_CHAR;
			byte_len++;
		}
		else if (buffer[byte_len] == '\n')	//newline encountered
		{
			end_reached = 1;
			break;
		}
		else byte_len += loaded;			//everything went allright
		
		_text_len++;
	}
	 
	while (!end_reached) //skip until end of line or EOF reached
	{
		int c = getc(f);
		end_reached = c == EOF || c == '\n';
	}
	
	buffer[byte_len] = '\0'; //IDEA maybe assert that byte_len <= max_size?
	if (text_len) *text_len = _text_len;
	return byte_len;
}

void utf8_last_trim(char *str, size_t length)
{
	if (!length) return;
	
	size_t index = length - 1;

	while (index > 0 && utf8_1prefix((unsigned char)str[index]) == 1)
	{
		index--;
	}
	
	int blen = utf8_bytelen((unsigned char)str[index]);
	if (blen <= 0 || (size_t)blen != length - index)
	{
		memset(str + index, '\0', length - index);
	}
}

void skip_comment_blank_lines(FILE *f, int *in_char)
{	//only works if in_char is '#' or '\n'
	//skips to the beginning of the next non-comment non-empty line or EOF
	while (*in_char != EOF && ((char)*in_char == '#' || (char)*in_char == '\n'))
	{
		skip_until(f, in_char, '\n');
		//now in_char contains newline char or EOF
		if (*in_char != EOF) *in_char = getc(f); //if is probably pointless
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
{	//expects first (given) character to be already number (digit)
	//only works with unsigned numbers, returns numbers of digits that it read
	if (!f || !num || !in_char) return 0;
	
	int c = *in_char;
	size_t count = 0;
	
	while (c != EOF && c >= '0' && c <= '9')
	{
		*num *= 10;
		*num += (uint_least8_t)(c - '0');
		c = getc(f);
		count++;
	}
	
	*in_char = c;
	return count;
}

size_t load_num_16(FILE *f, uint_least16_t* num, int *in_char) //16 bit version
{	//expects first (given) character to be already number (digit)
	//only works with unsigned numbers, returns numbers of digits that it read
	if (!f || !num || !in_char) return 0;
	
	int c = *in_char;
	size_t count = 0;
	
	while (c != EOF && c >= '0' && c <= '9')
	{
		*num *= 10;
		*num += (uint_least16_t)(c - '0');
		c = getc(f);
		count++;
	}
	
	*in_char = c;
	return count;
}

int load_num_tolerant_8(FILE *f, uint_least8_t* num, int *in_char) //8 bit version
{	//tolerates whitespaces at the beginning of the number
	//returns amount of digits that it read
	if (!f || !num || !in_char) return -1;
	
	while (*in_char != EOF && isspace(*in_char)) *in_char = getc(f);
	
	return load_num_8(f, num, in_char);
}

int load_num_tolerant_16(FILE *f, uint_least16_t* num, int *in_char) //16 bit version
{	//tolerates whitespaces at the beginning of the number
	//returns amount of digits that it read
	if (!f || !num || !in_char) return -1;
	
	while (*in_char != EOF && isspace(*in_char)) *in_char = getc(f);
	
	return load_num_16(f, num, in_char);
}

int load_date(FILE *f, date_t *d, int c)
{	//loads from stream 'f' date to given 'd', first char given as 'c'
	//returns nonzero if error occurred
	if (!f || !d) return 1;
	date_null(d); //nulling the date otherwise load works badly
	
	if (!load_num_tolerant_8(f, &(d->day), &c)) return -1;
	c = getc(f);
	if (!load_num_tolerant_8(f, &(d->month), &c)) return -1;
	c = getc(f);
	if (!load_num_tolerant_16(f, &(d->year), &c)) return -1;
	return 0;
}

int load_date_string(date_t *d, const char *str)
{	//loads date from given string, returns non-null if not all numbers were loaded
	//IDEA maybe add checks whether we load exactly 3 numbers (zeroes included)
	if (!d || !str) return -1; //IDEA remove this so we can be sure that 'd' gets filled
	
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

size_t strcpy_buffer(size_t buffer_size, char *buffer, const char *source)
{	//copies content of source string into buffer of given size + 1 (null char),
	//does nothing if given invalid pointers,
	//return the amount of chars copied from source to buffer
	if (!buffer || !source) return 0;
	
	size_t index = 0;
	for (; index < buffer_size && source[index] != '\0'; index++)
	{	//unoptimized solution but whatever
		buffer[index] = source[index];
	}

	buffer[index] = '\0';
	return index;
}

int load_one_entry(FILE *f, todo_entry *entry)
{	//loads entry from given file, ignores commented lines (starting with '#')
	//returns 0 if success, -1 if it reached the EOF, otherwise positive number
	if (!f || !entry) return 1;
	
	int c = getc(f);
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

	while (isseparator(c = getc(f))); //skipping separators
	if (load_date(f, &entry->deadline, c)) return 3;
	
	while (isseparator(c = getc(f))); //skipping separators
	if (load_date(f, &entry->created_date, c)) return 4;
	
	while (isseparator(c = getc(f))); //skipping separators

	if (ungetc(c, f) == EOF) return 5;
	
	//returned bytes loaded ignored
	utf8_readline(f, TEXT_MAX_LEN, (char*)entry->text_buffer, NULL);
	
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
		
	todo_entry entry = { 0 };
	int status = 0;
	size_t index = 1; //entry indexing starts from 1
	
	while (!status)
	{
		entry = (todo_entry){ 0 };
		
		if ((status = load_one_entry(f, &entry)) > 0)
		{	//positive return value means something went wrong
			fprintf(stderr, "Err: Loading of a entry '%u' failed!", index);
			switch (status)
			{
			case 2:
				fprintf(stderr, " -> Status wasn't in correct format.\n");
				break;
			case 3:
				fprintf(stderr, " -> Failure of loading deadline date.\n");
				break;
			case 4:
				fprintf(stderr, " -> Failure of loading created date.\n");
				break;
			case 5: //when ungetc fails
				fprintf(stderr, " -> Failure of character pushback.\n");
				break;
			//those shouldnt normally happen
			case 1:
			default:
				fprintf(stderr, " -> Unexpected error.\n");
				break;
			}
			
			llist_destroy_contents(list);
			fclose(f);
			return 3;
		}

		//-1 means EOF, llist_add success -> entry gets added to list
		if (status != -1 && !llist_add_end(list, &entry))
		{	//adding to list failed
			//same error message as in cmd_add:
			fprintf(stderr, "Err: Failed to add following entry into the list!\nThe entry: '");
			print_todoentry(stderr, &entry, 0);
			fputs("'\n", stderr);
			
			llist_destroy_contents(list);
			fclose(f);
			return 4;
		}
		
		index++;
	}
	
	fclose(f);
	return 0;
}
