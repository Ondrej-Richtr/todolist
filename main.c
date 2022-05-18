#include "todolist.h"


//main things
int main2()
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

int main()
{
	llist list = { NULL, NULL };

	add_entry_string(&list, "X|12.4.1982|nejaky text");
	add_entry_string(&list, "|12.10.1999|nejaky text cislo dva");
	add_entry_string(&list, "X|12.19.1999|a taky cislo tri");
	print_llist(&list);
	puts("after:");
	
	int del_err = delete_entry_string(&list, "0");
	printf("Returned err: %d\n", del_err);
	print_llist(&list);
	
	llist_destroy_contents(&list);
	return 0;
}
