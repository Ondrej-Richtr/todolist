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
