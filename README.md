# todolist
Terminal based TO-DO list

strictly for my usage and anyone who dares to try it

Installation:
1) Clone this repository anywhere you want
2) Change the path of todo-list save file (if you want) by modifying the 'path' variable inside of main in main.c
3) Create this save file if it doesn't already exist - you should make it empty from the start
4) Compile all .c files with C compiler of your choice, C99 or later is recommended

Usage:

Open the executable as with any other program in your terminal.

Now you are in the interactive mode (the only mode for now), you can write commands - for list of all commands write 'help'.

The syntax of most commands is 'COMMAND [all/done/undone] [INDEX..]', where indices should be in the ascending order and divided either by space or '|'.

The 'add' command has special syntax for adding new todo entries - '[X]|[DEADLINE DATE]|TEXT', where X is optional for signaling that this entry is already done and deadline date is also optional (in format Day Month Year, parser is quite tolerant).

'print' now prints the entries in style Y(meaning done)/N(undone)| deadline date | text, for nonlogical dates is the date field omitted.
