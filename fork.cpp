#include "log.h"
#include <iostream>
#include <unistd.h>
#include <sys/wait.h>

void fork_trains() {
      for (const auto& train : parseTrains) {
        pid_t pid = fork();
        if (pid == 0) {
	  
	  std::cout << "Train process started: " << trainName << "\n";
	  
	  // TODO: Add the train logic.
	  exit(0);

        } else if (pid > 0) {

	  std::cout << "Forked train: " << trainName << " (PID " << pid << ")\n";

        } else {

	  perror("fork");
	  exit(1);
        }
      }
      
      //Waits to finish child processes.
      while (wait(nullptr) > 0);
    }
