#include "executer.hpp"

#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cstdlib>
#include <iostream>

#include "builtins.hpp"

void Executer::execute(const std::vector<std::string>& tokens) {
  if (Builtins::handle(tokens)) {
    return;
  }

  std::vector<std::string> cmdTokens;
  std::string outputFile;
  bool hasRedirect = false;

  for (size_t i = 0; i < tokens.size(); ++i) {
    if (tokens[i] == ">") {
      hasRedirect = true;
      if (i + 1 < tokens.size()) {
        outputFile = tokens[i + 1];
      }
      ++i;
    } else {
      cmdTokens.push_back(tokens[i]);
    }
  }

  std::vector<const char*> argv;

  for (const std::string& token : cmdTokens) {
    argv.push_back(token.c_str());
  }
  argv.push_back(nullptr);

  pid_t pid = fork();

  if (pid < 0) {  // fork failed
    std::cerr << tokens[0] << ": failed to execute command" << std::endl;
  } else if (pid == 0) {  // child process
    if (hasRedirect) {
      int file_fd = open(outputFile.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);

      if (file_fd < 0) {
        std::cerr << "nsh: failed to open file: " << outputFile << std::endl;
        std::exit(EXIT_FAILURE);
      }

      dup2(file_fd, STDOUT_FILENO);
      close(file_fd);
    }

    int status = execvp(argv[0], const_cast<char* const*>(argv.data()));

    if (status != 0) {
      std::string msg = "failed to execute command";

      if (errno == ENOENT) {
        msg = "command not found";
      }

      std::cerr << tokens[0] << ": " << msg << std::endl;
    }
  } else {  // parent process (pid > 0)
    waitpid(pid, nullptr, 0);
  }
}
