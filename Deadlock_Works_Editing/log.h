// Group : I
// Author: Angel Trujillo
// Date: 04/01/2025
// Description: Declares the Logger class for time-stamped logging of server and train events with mutex-based time tracking.

#ifndef LOG_H
#define LOG_H

#include <string>
#include <fstream>
#include <pthread.h>

class Logger {
public:
    Logger(const std::string& filename, int* sim_time_ptr, pthread_mutex_t* time_mutex_ptr, bool pid_tag = false);    ~Logger();

    void log_server(const std::string& message);
    void log_train(const std::string& train_name, const std::string& message);

private:
    std::ofstream log_file;
    int* sim_time;
    pthread_mutex_t* time_mutex;
    bool include_pid;

    int increment_sim_time();
    std::string format_time(int seconds);
    void log(const std::string& source, const std::string& message);
};

#endif
