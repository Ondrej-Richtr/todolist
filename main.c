#include "todolist.h"


//main things
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
