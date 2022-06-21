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
