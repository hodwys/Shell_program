# Shell_Program

This project implements a custom shell named `myshell`, providing various built-in commands and features commonly found in traditional Unix-like shells. Below is a brief guide to the functionality and usage of the shell.

#### 1. Routing Writes to `stderr`
Redirect standard error output to a file using `2>` followed by the filename.

#### 2. Appending to an Existing File
Append the output of a command to a file using `>>`. If the file does not exist, it will be created.

#### 3. Change Prompt Command
Change the shell prompt by using the `prompt` command followed by the new prompt string.

#### 4. Echo Command
Print arguments using the `echo' command followed by the arguments you want to print.

#### 5. Print Last Command Status
Print the exit status of the last executed command using `echo $?`.

#### 6. Change Working Directory
Change the current working directory using the `cd` command followed by the directory name.

#### 7. Repeat Last Command
Repeat the last command executed using `!!`.

#### 8. Exit Command
Exit the shell using the `Get out' command.

#### 9. Handle Control-C
If the user types Control-C, the shell will print "Typing Control-C!" and continue running.

#### 10. Pipe Commands
Chain several commands using a pipe (`|`). Each command in the pipeline requires dynamic allocation of argv.

#### 11. Variable Assignment
Add and use variables within the shell using the `$variable = value` syntax. Echo the variable using `echo $variable`.

#### 12. Read Command
Read input from the user using the ``read'' command. Prompt the user for input, read the input, and use the input in subsequent commands.

#### 13. Command History
The shell maintains a history of at least 20 commands. You can browse through the command history using the "up" and "down" arrow keys.

#### 14. Control Flow
You can write conditions in the following format: `if` <condition> `then` <action> `else` <alternative action> `fi`. Ensure the entire condition is written on a single line in the terminal.

### Building and Running the Shell

To build the `myshell` executable:
1. Ensure you have `clang++-14` installed.
2. Run the `make` command in the project root directory to compile the shell.

To run the shell:
Execute the `./myshell` command from the project root directory.

To clean up the build artifacts:
Run the `make clean' command to remove compiled files and the executable.


