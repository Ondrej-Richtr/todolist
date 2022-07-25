#include "todolist.h"


enum Option{ undef_c, file_c };
enum Option parse_option(const char *str)
{
	if (!str) return undef_c;
	
	if (!strcmp("-f", str)) return file_c;
	return undef_c;
}

int parse_options(int argc, char **argv, char **path_ptr)
{	//TODO all of the other choices, -f only works now
	//the todo file path default:
	*path_ptr = "./writefile";
	if (argc < 2) return 0;
	if (argc == 2)
	{
		fprintf(stderr, "Err: Too few arguments when launching the program!\n");
		return 1;
	}
	if (argc > 3)
	{
		fprintf(stderr, "Err: Too many arguments when launching the program!\n");
		return 2;
	}
	
	enum Option opt = parse_option(argv[1]);
	switch (opt)
	{
	case file_c:
		*path_ptr = argv[2];
		break;
	default:
		fprintf(stderr, "Err: Undefined option '%s' specified!\n", argv[1]);
		return 3;
	}
	
	return 0;
}

//main things
int main(int argc, char **argv)
{
	FILE *f = stdin;
	char *path = NULL;
	
	int opt_err = parse_options(argc, argv, &path);
	if (opt_err) return EXIT_FAILURE; //errmsg printed in parse_options
	
	int inter_err = interactive_mode(f, path);
	if (inter_err)
	{
		fprintf(stderr, "Interactive err: %d\n", inter_err);
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

int main2()
{
	//for testing the disconnect function
	todo_entry_t *e1 = malloc(sizeof(todo_entry_t)),
	*e2 = malloc(sizeof(todo_entry_t)),
	*e3 = malloc(sizeof(todo_entry_t)),
	*e4 = malloc(sizeof(todo_entry_t)),
	*e5 = malloc(sizeof(todo_entry_t));
	llist list = { NULL, NULL };
	strcpy((char*)e1->text_buffer, "prvni");
	strcpy((char*)e2->text_buffer, "druhy");
	strcpy((char*)e3->text_buffer, "treti");
	strcpy((char*)e4->text_buffer, "ctvrty");
	strcpy((char*)e5->text_buffer, "paty");
	llist_add_end(&list, e1);
	llist_add_end(&list, e2);
	llist_add_end(&list, e3);
	llist_add_end(&list, e4);
	llist_add_end(&list, e5);

	print_llist(&list, 1);
	
	int err = llist_move(&list, 4, 4, 1);
	if (err)
	{
		printf("Error: %d\n", err);
		return 1;
	}
	
	puts("------------");
	print_llist(&list, 1);

	/*int check = list.firsts>val == e1 && list.last->val == e4 && into.first->val == e5 && into.last->val == e5;
	//int check = list.first == NULL && list.last == NULL && into.first->val == e1 && into.last->val == e5;
	if (check) puts("Test passed");
	else puts("Test failed");*/

	llist_destroy_contents(&list);
	return 0;
}
