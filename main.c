// This file is part of 'todolist' project which author is Ondřej Richtr
// This file is part of 'todolist' project which author is Ondřej Richtr
// Seek more information about copyright in the LICENSE file included

#include "todolist.h"

//CHANGE OR OVERRIDE THIS MACRO IF YOU WANT DIFFERENT DEFAULT TODOFILE PATH
#ifndef DEFAULT_PATH
#define DEFAULT_PATH "./.todofile"
#endif

//RELEASE
#define VER0 0
#define VER1 2
#define VER2 0


//options parsing stuff
enum Mode{ err_c, vermode_c, helpmode_c, basichelpmode_c, intermode_c, nonintermode_c };
enum Option{ undefopt_c, syntaxerrE_helpopt_c, syntaxerrC_helpopt_c, fileopt_c,
	veropt_c, cmdopt_c, helpopt_c, simplehelpopt_c };

int parse_opt_basichelp(const char *str)
{	//returns positive number if given string is '-h' (1) or '--help' (2)
	//negative number if given string starts with '--help' (-1), zero otherwise
	const char helpstr[] = "--help";
	size_t helpstr_len = sizeof(helpstr)/sizeof(char) - 1; //-1 as we dont count term.char.
	
	if (!strcmp("-h", str)) return 1; //str is '-h'
	if (strncmp((char*)helpstr, str, helpstr_len)) return 0; //str does not start with '--help'

	return str[helpstr_len] ? -1 : 2; //checks if it is more than just '--help'
}

int parse_opt_cmdhelp(const char *str)
{	//returns negative number (-1) if given string starts with '--help=' and
	//doesnt have any text after '=' char (e.g. only whitespaces follow after '=')
	//returns positive number (1) if there is non-whitespace text, zero otherwise
	const char cmdhelpstr[] = "--help=";
	size_t cmdhelpstr_len = sizeof(cmdhelpstr)/sizeof(char) - 1; //-1 as we dont count term.char.
	
	if (strncmp((char*)cmdhelpstr, str, cmdhelpstr_len)) return 0; //does not start with '--help='
	
	size_t index = cmdhelpstr_len;
	while (str[index] && isspace((int)str[index])) index++; //skipping whitespaces
	
	return str[index] ? 1 : -1;
}

enum Option parse_option_string(const char *str)
{
	if (!str) return undefopt_c;
	
	if (!strcmp("-e", str)) return cmdopt_c;
	if (!strcmp("-f", str)) return fileopt_c;
	if (!strcmp("-v", str) || !strcmp("--version", str)) return veropt_c; //maybe later -v as verbose mode?
	
	int parse_basichelp = parse_opt_basichelp(str);
	if (!parse_basichelp) return undefopt_c; //is not '-h' and does not start with "--help"
	if (parse_basichelp > 0) return simplehelpopt_c;

	//at this point we know that str starts with "--help"
	int parse_cmdhelp = parse_opt_cmdhelp(str);
	if (!parse_cmdhelp) return syntaxerrE_helpopt_c; //not starting with "--help=" (equals missing)
	if (parse_cmdhelp > 0) return helpopt_c;
	
	return syntaxerrC_helpopt_c; //empty command specifier
}

enum Mode parse_options(const int argc, char **argv, char **path_ptr)
{	//parses commandline options and decides which mode to launch program in
	//returns enum whether error occurred (also prints errors) or the mode enum:
	// -interactive		launches interactive loop (happens when no -e options specified)
	// -noninteractive 	launches non-interactive loop
	//					 (when atleast one -e option or last argument is valid cmd)
	// -version	 		prints current version, author and exits (when -v or --version)
	// -help 			prints commandline help and exits (when -h or --help and not version mode)
	
	*path_ptr = DEFAULT_PATH; //default path if not changed by -f option
	if (argc < 1 || argv[0] == NULL) return err_c; //basic checks
	
	enum Mode mode = intermode_c;
	
	for (size_t i = 1; i < (size_t)argc && argv[i]; i++)
	{
		enum Option opt = parse_option_string(argv[i]);
		
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
		case helpopt_c: 		//this makes help mode prioritized over others except the version mode
			if (mode != vermode_c) mode = helpmode_c;
			break;
		case simplehelpopt_c: 	//simple help has same priority as command help (the last one is used)
			if (mode != vermode_c) mode = basichelpmode_c;
			break;
		case cmdopt_c:
			{
				i++;
				if (i >= (size_t)argc || !argv[i])
				{
					fprintf(stderr, "Err: You need to write command after -e option!\n");
					return err_c;
				}
				
				enum CmdType cmd; 
				if (!is_valid_cmd(argv[i], &cmd))
				{
					fprintf(stderr, "Err: '%s' is not a valid command! Try 'help' for list of all commands.\n", argv[i]);
					return err_c;
				}
				else if (cmd == help_c)
				{
					fprintf(stderr, "Err: Help command is not implemented in non-interactive mode! Use options '-h', --help' or help in interactive mode.\n");
					return err_c;
				}
				
				if (mode == intermode_c) mode = nonintermode_c;
			}
			break;
		case syntaxerrE_helpopt_c:
			fprintf(stderr, "Err: Bad syntax for command help option! Usage is '--help=COMMAND' or '--help=COMMAND1 COMMAND2...'.\n");
			return err_c;
		case syntaxerrC_helpopt_c:
			fprintf(stderr, "Err: Bad syntax for command help option - empty command! Use '--help=COMMAND' or '--help=COMMAND1 COMMAND2...' syntax.\n");
			return err_c;
		case undefopt_c:
			{
				//this means single command noninteractive mode
				enum CmdType cmd;
				if (i + 1 == (size_t)argc && mode == intermode_c && is_valid_cmd(argv[i], &cmd))
				{
					if (cmd == help_c) 
					{
						fprintf(stderr, "Err: Help command is not implemented in non-interactive mode! Use options '-h', --help' or help in interactive mode.\n");
						return err_c;
					}
					
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
		printf("todo %d.%d.%d\nWritten by Ondrej Richtr\n", VER0, VER1, VER2); //IDEA add link to gitrepo/webpage
		return EXIT_SUCCESS;
	case helpmode_c:
		{
			//value of help_ret ignored
			int help_ret = cmd_help_noninter_parse((size_t)argc, (const char**)argv);
			return help_ret ? EXIT_FAILURE : EXIT_SUCCESS;
		}
	case basichelpmode_c:
		{
			print_basichelp(1); //1 for isoption == true
			return EXIT_SUCCESS;
		}
	case intermode_c:
		{
			//value of inter_ret ignored
			int inter_ret = interactive_mode(stdin, path);
			return inter_ret ? EXIT_FAILURE : EXIT_SUCCESS;
		}
	case nonintermode_c:
		{
			//const char** cast needed, but shouldn't cause problems
			//value of noninter_ret ignored
			int noninter_ret = noninteractive_mode((size_t)argc, (const char**)argv, path);
			return noninter_ret ? EXIT_FAILURE : EXIT_SUCCESS;
		}
	case err_c: //errmsgs printed in parse_options
		return EXIT_FAILURE;
	//default is to continue after switch
	}

	fprintf(stderr, "Err: Unimplemented todo mode! Exiting without changes.\n");
	return EXIT_FAILURE;
}
