#include <iostream>
#include <vector>
#include <sstream>
#include <iterator>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <csignal>
#include <termios.h>
#include <string>
#include <map>

class myShell {
private:
    static volatile sig_atomic_t childPid;
    static std::string prompt;
    static std::vector<std::string> history;
    std::map<std::string, std::string> variables;
    std::string lastCommand;
    int lastStatus;
    size_t historyIndex;
    std::string lastExecutedCommand;

public:
    myShell();
    void setPrompt(const std::string& newPrompt);
    static void sigintHandler(int /*signum*/);
    std::string getInput();
    int executeCommand(const std::string& input, bool isBackground, bool disablePrint = false);
    void executePipes(std::vector<std::string> commands, bool disablePrint = false);
    int ifCaseHandling(const std::string& ifCommand);
    int orCaseHandling(const std::vector<std::string>& orCommands);
    int status;
    void parse(const std::string& input);
    void run();
};

volatile sig_atomic_t myShell::childPid = 0;
std::string myShell::prompt = "hello:";
std::vector<std::string> myShell::history;

myShell::myShell() : lastCommand(""), lastStatus(0), historyIndex(0), lastExecutedCommand("") {
    signal(SIGINT, sigintHandler); // init signal handler for Ctrl+C
}

void myShell::setPrompt(const std::string& newPrompt) {
    prompt = newPrompt;
}

void myShell::sigintHandler(int /*signum*/) {
    if (childPid != 0) { // if there is a child process
        std::cout << "\r"; // clear the line, to avoid C^ print
        kill(childPid, SIGINT); // terminate child's process
        waitpid(childPid, nullptr, 0); // wait for child to finish
        childPid = 0;
    } else { // at parent process
        std::cout << "\rYou typed Control-C!\n";
        std::cout << prompt << ": ";
        std::cout.flush();
    }
}

std::string myShell::getInput() {
    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= static_cast<unsigned int>(~(ICANON | ECHO)); // Disable canonical mode and echo
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    std::string input;
    char ch;
    size_t cursorPosition = 0;

    while (true) {
        ch = getchar();
        if (ch == '\n') {
            std::cout << std::endl;
            break;
        } else if (ch == 27) { // Arrow keys start with an escape character
            getchar(); // Skip the [
            switch (getchar()) {
                case 'A': // Up arrow
                    if (historyIndex > 0) {
                        historyIndex--;
                        input = history[historyIndex];
                        cursorPosition = input.length();
                        std::cout << "\r" << prompt << " " << input << " \033[K"; // Clear line after cursor
                    }
                    break;
                case 'B': // Down arrow
                    if (historyIndex < history.size() - 1) {
                        historyIndex++;
                        input = history[historyIndex];
                        cursorPosition = input.length();
                        std::cout << "\r" << prompt << " " << input << " \033[K";
                    } else if (historyIndex == history.size() - 1) {
                        historyIndex++;
                        input.clear();
                        cursorPosition = 0;
                        std::cout << "\r" << prompt << " " << " \033[K";
                    }
                    break;
                case 'C': // Right arrow
                    if (cursorPosition < input.length()) {
                        cursorPosition++;
                        std::cout << "\033[C";
                    }
                    break;
                case 'D': // Left arrow
                    if (cursorPosition > 0) {
                        cursorPosition--;
                        std::cout << "\033[D";
                    }
                    break;
            }
        } else if (ch == 127) { // Backspace
            if (cursorPosition > 0 && !input.empty()) {
                input.erase(cursorPosition - 1, 1);
                cursorPosition--;
                std::cout << "\r" << prompt << " " << input << " \033[K";
                std::cout << "\r" << prompt << " " << input;
                for (size_t i = cursorPosition; i < input.length(); ++i) {
                    std::cout << "\033[D";
                }
            }
        } else {
            input.insert(input.begin() + static_cast<std::string::difference_type>(cursorPosition), ch);
            cursorPosition++;
            std::cout << "\r" << prompt << " " << input << " \033[K";
            std::cout << "\r" << prompt << " " << input;
            for (size_t i = cursorPosition; i < input.length(); ++i) {
                std::cout << "\033[D";
            }
        }
    }

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt); // Restore old terminal attributes
    return input;
}

int myShell::executeCommand(const std::string& input, bool isBackground, bool disablePrint) {
    if (input.empty()) {
        return 0;
    }

    // Replace variables with their values
    std::string command = input;
    size_t pos = 0;
    while ((pos = command.find('$', pos)) != std::string::npos) {
        size_t end = command.find_first_of(" \t\n", pos);
        std::string varName = command.substr(pos + 1, end - pos - 1);
        if (variables.find(varName) != variables.end()) {
            command.replace(pos, end - pos, variables[varName]);
            pos += variables[varName].length();
        } else {
            pos = end;
        }
    }

    bool redirect = false;
    bool append_redirect = false;
    std::string outfile;
    int fd;

    if (command.find('>') != std::string::npos || command.find(">>") != std::string::npos || command.find("2>") != std::string::npos) {
        std::vector<std::string> tokens;
        std::istringstream iss(command);
        std::copy(std::istream_iterator<std::string>(iss), std::istream_iterator<std::string>(), std::back_inserter(tokens));
        size_t i = tokens.size();
        if (i >= 2 && (tokens[i - 2] == ">" || tokens[i - 2] == ">>")) {
            redirect = true;
            if (tokens[i - 2] == ">>") {
                append_redirect = true;
            }
            outfile = tokens[i - 1];
            tokens[i - 2] = "";
            tokens[i - 1] = "";
        } else if (i >= 2 && tokens[i - 2] == "2>") {
            redirect = true;
            append_redirect = false;
            outfile = tokens[i - 1];
            tokens[i - 2] = "";
            tokens[i - 1] = "";
            fd = creat(outfile.c_str(), 0660);
            close(STDERR_FILENO);
            dup(fd);
            close(fd);
        } else {
            redirect = append_redirect = false;
        }
    }

    pid_t pid = fork(); // fork to create a child process
    if (pid == -1) {
        std::cerr << "Error forking process." << std::endl;
        exit(EXIT_FAILURE);
    } else if (pid == 0) { // child process
        if (redirect) {
            if (append_redirect) {
                fd = open(outfile.c_str(), O_WRONLY | O_CREAT | O_APPEND | O_RDONLY, 0644); // append mode
            } else {
                fd = open(outfile.c_str(), O_CREAT | O_TRUNC | O_WRONLY | O_RDONLY, 0644); // write mode
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }
        if (disablePrint) { // for "if" cases which exec used only for exit status
            int devNull = open("/dev/null", O_WRONLY);
            dup2(devNull, STDOUT_FILENO);
            close(devNull);
        }
        execlp("sh", "sh", "-c", command.c_str(), nullptr); // execute the command in the existing shell

        std::cerr << "Error executing command." << std::endl;
        exit(2); // exit with status 2 if exec fails
    } else { // parent process
        childPid = pid;
        int status;
        waitpid(pid, &status, 0); // wait for the child process to finish
        this->lastCommand = command; // store the last command
        if (WIFEXITED(status)) {
            this->lastStatus = WEXITSTATUS(status); // store the exit status of the last command
        } else {
            this->lastStatus = 2; // return 2 if the process did not terminate normally
        }
        //std::cout << "1Last Status: " << this->lastStatus << std::endl; // Add this line for debugging
        if(lastStatus == 2){
            return this->lastStatus;
        }
        childPid = 0; // reset childPid after child process has finished
        lastExecutedCommand = command; // update the last executed command

        if (this->lastStatus != 0 && disablePrint) {
            return this->lastStatus; // Return the exit status only if disablePrint is true
        }
    }
    return 0; // Return 0 for successful commands or when disablePrint is false
}

void myShell::executePipes(std::vector<std::string> commands, bool disablePrint) {
    // Create a pipe for each command except the last one
    std::vector<int> pipes((commands.size() - 1) * 2);
    for (size_t i = 0; i < commands.size() - 1; ++i) {
        if (pipe(pipes.data() + i * 2) < 0) {
            std::cerr << "Error creating pipe." << std::endl;
            exit(EXIT_FAILURE);
        }
    }
    // Execute each command
    for (size_t i = 0; i < commands.size(); ++i) {
        // std::cout << "Executing command: '" << commands[i] << "'" << std::endl;
        pid_t pid = fork();
        if (pid == -1) {
            std::cerr << "Error forking process." << std::endl;
            exit(EXIT_FAILURE);
        } else if (pid == 0) { // child process
            // If not the first command, redirect input from the previous pipe
            if (i != 0) {
                if ( dup2(pipes[(i - 1) << 1], STDIN_FILENO) < 0) {
                    std::cerr << "Error redirecting input." << std::endl;
                    exit(EXIT_FAILURE);
                }

            }
            // If not the last command, redirect output to the next pipe
            if (i != commands.size() - 1) {
                if (dup2(pipes[i * 2 + 1], STDOUT_FILENO) < 0) {
                    std::cerr << "Error redirecting output." << std::endl;
                    exit(EXIT_FAILURE);
                }
            }
            // Close all pipe ends
            for (size_t j = 0; j < pipes.size(); ++j) {
                close(pipes[j]);
            }
            // Execute the command
            this->executeCommand(commands[i], false, disablePrint);
            exit(EXIT_SUCCESS);
        }
    }
    // Close all pipe ends
    for (size_t i = 0; i < pipes.size(); ++i) {
        close(pipes[i]);
    }
    // Wait for all child processes to finish
    int status;
    for (size_t i = 0; i < commands.size(); ++i) {
        wait(&status);
    }
}

int myShell::orCaseHandling(const std::vector<std::string>& orCommands) {
    for (const auto& cmd : orCommands) {
        status = this->executeCommand(cmd, true, true);
        if (status == 0) {
            return 0;
        }
    }
    return 1;
}

int myShell::ifCaseHandling(const std::string& ifCommand) {
    size_t thenIndex = ifCommand.find("then");
    if (thenIndex == std::string::npos) {
        std::cerr << "Syntax error: 'then' not found." << std::endl;
        return -1;
    }

    std::string condition = ifCommand.substr(2, thenIndex - 2);
    if (condition.empty()) {
        std::cerr << "Syntax error: condition is empty." << std::endl;
        return -1;
    }

    size_t elseIndex = ifCommand.find("else");
    if (elseIndex == std::string::npos) {
        std::cerr << "Syntax error: 'else' not found." << std::endl;
        return -1;
    }

    std::string thenPart = ifCommand.substr(thenIndex + 4, elseIndex - thenIndex - 4);

    size_t fiIndex = ifCommand.find("fi");
    if (fiIndex == std::string::npos) {
        std::cerr << "Syntax error: 'fi' not found." << std::endl;
        return -1;
    }

    std::string elsePart = ifCommand.substr(elseIndex + 4, fiIndex - elseIndex - 4);

    if (thenPart.empty() || elsePart.empty()) {
        std::cerr << "Syntax error: then-part or else-part is empty." << std::endl;
        return -1;
    }

    // Execute the condition command
    status = this->executeCommand(condition, true, true);

    if (status == 0) {
        this->executeCommand(thenPart, false, false);
    } else {
        this->executeCommand(elsePart, false, false);
    }
    return status;
}

void myShell::parse(const std::string& input) {
    std::vector<std::string> tokens;
    std::istringstream iss(input);
    std::copy(std::istream_iterator<std::string>(iss),
              std::istream_iterator<std::string>(),
              std::back_inserter(tokens));

    // Add input to history
    history.push_back(input);
    historyIndex = history.size();

    if (tokens.empty()) {
        return;
    }

    if (tokens[0] == "if") {
        this->ifCaseHandling(input);
    } else if (tokens[0] == "or") {
        std::vector<std::string> orCommands(tokens.begin() + 1, tokens.end());
        this->orCaseHandling(orCommands);
    } else if (tokens[0] == "quit") {
        exit(EXIT_SUCCESS);
    } else if (tokens[0] == "echo" && tokens.size() > 1 && tokens[1] == "$?") {
        std::cerr << lastStatus << std::endl;
    } else if (tokens[0] == "echo" && tokens.size() > 1 && tokens[1][0] == '$') {
        std::string varName = tokens[1].substr(1);
        if (variables.find(varName) != variables.end()) {
            std::cout << variables[varName] << std::endl;
        } else {
            std::cerr << "Variable not found." << std::endl;
        }
    } else if (tokens.size() >= 2 && tokens[0] == "cd") {
        if (chdir(tokens[1].c_str()) == -1) {
            std::cerr << "Error changing directory." << std::endl;
        }
    } else if (tokens.size() == 2 && tokens[0] == "read") {
        std:: string readArg;
        //std::count << ">";
        std::getline(std::cin,readArg);
        variables[tokens[1]] = readArg;

    }else if (tokens.size() >= 3 && tokens[0] == "prompt" && tokens[1] == "=") {
        setPrompt(tokens[2]); // update the new prompt value
    } else if (tokens[0] == "!!") {
        if (!lastExecutedCommand.empty()) {
            std::string lastCommand = lastExecutedCommand;
            this->executeCommand(lastCommand, false); // Execute the last command
        } else {
            std::cerr << "No previous command in history." << std::endl;
        }
    } else if (tokens.size() == 3 && tokens[1] == "=") {
        variables[tokens[0].substr(1)] = tokens[2]; // Assign the value to the variable
    } else {
        // For other commands, execute them
        lastStatus = this->executeCommand(input, false);
        //std::cout << "2Last Status: " << this->lastStatus << std::endl; // Add this line for debugging
        lastExecutedCommand = input;
    }
}

void myShell::run() {
    while (true) {
        std::cout << prompt << " ";
        std::string input = this->getInput();
        this->parse(input);
    }
}

int main() {
    myShell shell;
    shell.run();
    return 0;
}