#include "todolist.h"

//CHANGE OR OVERRIDE THIS MACRO IF YOU WANT DIFFERENT DEFAULT TODOFILE 
#ifndef DEFAULT_PATH
#define DEFAULT_PATH "./.todofile"
#endif

enum Option{ undef_c, file_c };
enum Option parse_option(const char *str)
{
	if (!str) return undef_c;
	
	if (!strcmp("-f", str)) return file_c;
	return undef_c;
}

int parse_options(int argc, char **argv, char **path_ptr)
{	//TODO noninteractive mode (next realease I hope)
	*path_ptr = DEFAULT_PATH; //default path if not changed by -f option
	if (argc < 2 || argv[0] == NULL || argv[1] == NULL) return 0; //NULL check so we dont change env. values
	if (argc == 2) //TODO too few arguments? bad err msg
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

//main stuff
int main(int argc, char **argv)
{
	FILE *f = stdin;
	char *path = NULL;
	
	int opt_err = parse_options(argc, argv, &path);
	if (opt_err) return EXIT_FAILURE; //errmsg printed in parse_options
	
	int inter_err = interactive_mode(f, path);
	if (inter_err)
	{
		//RELEASE no need to print this in release version
		fprintf(stderr, "Interactive err: %d\n", inter_err);
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
