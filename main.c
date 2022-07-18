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
	char *input = "2-33";
	size_t start = 0, end = 0;
	int err = parse_range(input, &start, &end);
	if (err)
	{
		printf("Error: %d\n", err);
	}
	else printf("start: %u end: %u\n", start, end);
	return 0;
}
