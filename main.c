#include "todolist.h"


//main things
int main()
{
	FILE *f = stdin;
	const char *path = "./writefile";
	int err = interactive_mode(f, path);
	if (err)
	{
		fprintf(stderr, "Interactive err: %d\n", err);
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
