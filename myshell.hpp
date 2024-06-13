#ifndef MYSHELL_HPP
#define MYSHELL_HPP

#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <unistd.h>
#include <sys/wait.h>
#include <csignal>
#include <cstdlib>
#include <sys/ioctl.h>
#include <iomanip>
#include <map>
#include <algorithm> 
#include <iterator>
#include <fcntl.h>
#include <sys/stat.h> 
#include <termios.h>



class myShell {
private:
    static volatile sig_atomic_t childPid;
    static std::string prompt;
    static std::vector<std::string> history;
    std::string lastCommand;
    int lastExitStatus;
    size_t historyIndex;
    std::string lastExecutedCommand;

    std::unordered_map<std::string, std::string> vars; // Declare vars to store user-defined variables



    int lastStatus;

public:
    myShell();
    void setPrompt(const std::string& newPrompt);
    static void sigintHandler(int signum);
    std::string getInput();
    int executeCommand(const std::string& input, bool isBackground, bool disablePrint = false);
    void executePipes(std::vector<std::string> commands, bool disablePrint = false);
    int ifCaseHandling(const std::string& ifCommand);
    int orCaseHandling(const std::vector<std::string>& orCommands);
    void parse(const std::string& input);
    void run();
};