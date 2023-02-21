# todolist
Terminal based TO-DO list

#### *Important: This repository is currently in rework, you (probably) should rather use some older release. Also note that following text and helps are incomplete and maybe even incorrect.*

- Installation:
 1. Clone this repository anywhere you want
 2. Change the path of the default todo-list save file (if you want) by modifying the DEFAULT_PATH macro inside of main.c
 3. Create this save file if it doesn't already exist - you should make it empty from the start
 4. Compile all .c files with C compiler of your choice, C99 standard or later is recommended - `gcc -std=c99 *.c`
 5. Optionally you can set up symlink for this program into any of your directories in PATH

- Usage:
`todo [-f FILEPATH]`
Where todo is just path where you have your todo program and -f is option to load todo save file at FILEPATH rather than the default one.

  The todo program in current (first) release version works only in interactive mode, but in future release I want to also provide the non-interactive usage when you can do pretty much anything with just the shell options.

- Commands:
Now there are those commands: print, add, delete, mark, clear, change, move, swap, sort.
For the complete list of all commands and brief description simply type `help` in interactive mode. For detailed help on specific command type `help COMMAND` in interactive mode.
