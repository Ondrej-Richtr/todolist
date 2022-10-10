#include "todolist.h"

//CHANGE OR OVERRIDE THIS MACRO IF YOU WANT DIFFERENT DEFAULT TODOFILE 
#ifndef DEFAULT_PATH
#define DEFAULT_PATH "./.todofile"
#endif
#define VER0 0
#define VER1 1
#define VER2 2


//options parsing stuff
enum Mode{ err_c, vermode_c, helpmode_c, intermode_c, nonintermode_c };
enum Option{ undefopt_c, fileopt_c, veropt_c, cmdopt_c, helpopt_c };
enum Option parse_option(const char *str)
{
	if (!str) return undefopt_c;
	
	if (!strcmp("-e", str)) return cmdopt_c;
	if (!strcmp("-f", str)) return fileopt_c;
	if (!strcmp("-v", str) || !strcmp("--version", str)) return veropt_c; //maybe later -v as verbose mode?
	if (!strcmp("-h", str) || !strcmp("--help", str)) return helpopt_c;
	return undefopt_c;
}

enum Mode parse_options(const int argc, char **argv, char **path_ptr)
{	//parses commandline options and decides which mode to launch program in
	//returns enum whether error occurred (also print error) or the mode enum:
	//interactive - launches interactive loop (happens when no -e options specified)
	//noninteractive - launches non-interactive loop (when atleast one -e option)
	//version - prints current version, author and exits (when -v or --version)
	//help - prints commandline help and exits (when -h or --help and not version mode)
	
	*path_ptr = DEFAULT_PATH; //default path if not changed by -f option
	if (argc < 1 || argv[0] == NULL) return err_c; //basic checks
	
	enum Mode mode = intermode_c;
	enum Option opt = undefopt_c;
	
	for (size_t i = 1; i < (size_t)argc && argv[i]; i++)
	{
		//printf("Parsing option: '%s'\n", argv[i]);
		opt = parse_option(argv[i]);
		
		switch (opt)
		{
		case fileopt_c:
			{
				i++;
				if (i >= (size_t)argc || !argv[i])
				{
					fprintf(stderr, "Err: You need to write path to todo file after -f option!\n");
					return err_c;
				}
				*path_ptr = argv[i];
			}
			break;
		case veropt_c:
			mode = vermode_c; //we dont return now - we want to check other options too
			break;
		case helpopt_c: //this makes help mode prioritized over others except the version mode
			if (mode != vermode_c) mode = helpmode_c;
			break;
		case cmdopt_c:
			{
				i++;
				if (i >= (size_t)argc || !argv[i])
				{
					fprintf(stderr, "Err: You need to write command after -e option!\n");
					return err_c;
				}
				
				if (!is_valid_cmd(argv[i], NULL))
				{
					fprintf(stderr, "Err: '%s' is not a valid command! Try 'help' for list of all commands.\n", argv[i]);
					return err_c;
				}
				if (mode == intermode_c) mode = nonintermode_c;
			}
			break;
		case undefopt_c:
			{
				//this means single command noninteractive mode
				if (i + 1 == (size_t)argc && mode == intermode_c && is_valid_cmd(argv[i], NULL)) //IDEA maybe not NULL?
				{
					//we can return as this is last string in argv anyways
					return nonintermode_c;
				}
				
				//IDEA maybe change the err msg based on value of 'mode'
				fprintf(stderr, "Err: Unrecognized option '%s'! Try '--help' option for command line usage.\n", argv[i]);
				return err_c;
			}
		default:
			fprintf(stderr, "Err: Unimplemented option '%s'!\n", argv[i]);
			return err_c;
		}	
	}

	return mode;
}

//main stuff
int main(int argc, char **argv)
{
	FILE *f = stdin;
	char *path = NULL;
	
	enum Mode mode = parse_options(argc, argv, &path);
	
	if (!path) //maybe this isn't problem for version or help mode?
	{
		fprintf(stderr, "Err: Path to todofile is NULL! Exiting without changes.\n");
		return EXIT_FAILURE;
	}
	
	switch (mode)
	{
	case vermode_c:
		printf("todo %d.%d.%d\nWritten by Ondrej Richtr\n", VER0, VER1, VER2);
		break;
	case helpmode_c:
		//TODO implement commandline help
		printf("Help not implemented yet!\n");
		break;
	case intermode_c:
		{
			int inter_err = interactive_mode(f, path);
			if (inter_err)
			{
				//RELEASE no need to print this in release version
				fprintf(stderr, "Interactive err: %d\n", inter_err);
				return EXIT_FAILURE;
			}
			return EXIT_SUCCESS;
		}
	case nonintermode_c:
		{
			//const char** cast needed, but shouldn't cause problems
			int noninter_err = noninteractive_mode((size_t)argc, (const char**)argv, path);
			if (noninter_err)
			{
				//RELEASE no need to print this in release version
				fprintf(stderr, "Non-interactive err: %d\n", noninter_err);
				return EXIT_FAILURE;
			}
			return EXIT_SUCCESS;
		}
	case err_c: //errmsgs printed in parse_options
		return EXIT_FAILURE;
	//default is to continue after switch
	}

	fprintf(stderr, "Err: Unimplemented todo mode! Exiting without changes.\n");
	return EXIT_FAILURE;
}
