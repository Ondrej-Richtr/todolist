# todolist
Terminal based unix-like TO-DO list and note keeping program

Works on Linux, other Unix-like operating systems, Windows and basically anywhere.
UTF-8 support sucks in Windows command line (because Windows command line sucks).

## Installation:
1. Clone this repository anywhere you want
  ```console
  git clone https://github.com/Ondrej-Richtr/todolist
  ```
2. _(Optional)_ Change the path of the default todo-list save file by modifying the DEFAULT_PATH macro inside of main.c
3. Create this save file at the desired location if it doesn't already exist - you should make it empty from the start
  ```console
  touch /path/to/new/default/todofile
  ```
4. Compile all .c files with C compiler of your choice, C99 standard or later is recommended
  ```console
  gcc -std=c99 *.c -o todo
  ```
5. _(Optional)_ you can set up symlink for this program into any of your directories in PATH
  ```console
  ln -s /path/to/todo /bin/todo
  ```

## Usage:
The todo program currently supports both Interactive and Non-interactive modes. To launch in Interactive mode simply provide no commands.
  ```console
  todo
  ```
  or with non-default todofile location
  ```console
  todo -f ./path/to/other/todofile
  ```

For Non-interactive mode provide command line commands, if you want to use more commands you will need `-e` option.
  ```console
  todo 'add My important note here'
  todo -e 'sort done deadline' -e print
  ```
For more information use `todo -h`.

## Commands:
Now there are commands: `print`, `add`, `delete`, `mark`, `clear`, `change`, `move`, `swap`, `sort` and (only for interactive mode) `help`.

For the complete list of all commands and brief description use `-h` or `--help` option or use `help` command in interactive mode.
You can also get detailed help for each command by using `--help=COMMAND` option or by using `help COMMAND` command in interactive mode.
