#include "log.h"
#include <iomanip>
#include <sstream>
#include <iostream>
#include <unistd.h>

Logger::Logger(const std::string& filename, int* sim_time_ptr, pthread_mutex_t* time_mutex_ptr, bool pid_tag)
    : sim_time(sim_time_ptr), time_mutex(time_mutex_ptr), include_pid(pid_tag)
{
    log_file.open(filename, std::ios::out | std::ios::app);
    if (!log_file.is_open()) {
        std::cerr << "Failed to open log file.\n";
    }
}

Logger::~Logger() {
    if (log_file.is_open()) {
        log_file.close();
    }
}

int Logger::increment_sim_time() {
    pthread_mutex_lock(time_mutex);
    (*sim_time)++;
    int current_time = *sim_time;
    pthread_mutex_unlock(time_mutex);
    return current_time;
}

std::string Logger::format_time(int seconds) {
    int hours = seconds / 3600;
    int minutes = (seconds % 3600) / 60;
    int secs = seconds % 60;

    std::ostringstream oss;
    oss << "[" << std::setw(2) << std::setfill('0') << hours << ":"
        << std::setw(2) << std::setfill('0') << minutes << ":"
        << std::setw(2) << std::setfill('0') << secs << "]";
    return oss.str();
}

void Logger::log(const std::string& source, const std::string& message) {
    int time_now = increment_sim_time();
    std::string timestamp = format_time(time_now);
    if (include_pid) {
        log_file << timestamp << " [PID " << getpid() << "] " << source << ": " << message << std::endl;
    } else {
        log_file << timestamp << " " << source << ": " << message << std::endl;
    }
    log_file.flush();
}

void Logger::log_server(const std::string& message) {
    log("SERVER", message);
}

void Logger::log_train(const std::string& train_name, const std::string& message) {
    log(train_name, message);
}
