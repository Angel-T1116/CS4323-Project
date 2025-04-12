// Group : I
// Author: Angel Trujillo
// Date: 04/06/2025
// Description: Entry point for train simulation. Initializes shared memory, logging, parses input files, forks server and train processes, and coordinates simulation flow.

#include <iostream>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <unistd.h>
#include <vector>
#include <cstring>
#include <sys/wait.h>

#include "parser.h"
#include "sync.h"
#include "log.h"

// Shared Memory Setup
#define SHM_KEY 12345
#define MSGKEY 1234

// Message Structure
struct TrainMessage {
    long type;
    int train_id;
    char command[10];
    char intersection[50];
};

// Globals
SharedMemory* shm;
int* sim_time;
pthread_mutex_t* time_mutex;


// Server Process
void run_server(Logger& logger) {
    int msgid = msgget(MSGKEY, IPC_CREAT | 0666);
    TrainMessage msg;

    while (true) {
        if (msgrcv(msgid, &msg, sizeof(msg) - sizeof(long), 1, 0) > 0) {
            std::string intersection = msg.intersection;
            logger.log_server("Received request from Train" + std::to_string(msg.train_id) + ": " + msg.command);

            if (strcmp(msg.command, "acquire") == 0) {
                handle_acquire_request(msg.train_id, intersection, shm);
            } else if (strcmp(msg.command, "release") == 0) {
                handle_release_request(msg.train_id, intersection, shm);
            }

            msg.type = 2;
            strcpy(msg.command, "granted");
            msgsnd(msgid, &msg, sizeof(msg) - sizeof(long), 0);
            logger.log_server("Granted " + intersection + " to Train" + std::to_string(msg.train_id));
        }
    }
}

// Train Process
void run_train(int train_id, const TrainRoute& route, Logger& logger) {
    int msgid = msgget(MSGKEY, 0666);
    TrainMessage msg;
    msg.train_id = train_id;
    msg.type = 1;

    for (const std::string& inter : route.route) {

        strcpy(msg.command, "acquire");
        strncpy(msg.intersection, inter.c_str(), sizeof(msg.intersection));
        msgsnd(msgid, &msg, sizeof(msg) - sizeof(long), 0);
        logger.log_train("TRAIN" + std::to_string(train_id), "Sent ACQUIRE for " + inter);

        msgrcv(msgid, &msg, sizeof(msg) - sizeof(long), 2, 0);
        logger.log_train("TRAIN" + std::to_string(train_id), "Granted " + inter);

        sleep(1);

        strcpy(msg.command, "release");
        msgsnd(msgid, &msg, sizeof(msg) - sizeof(long), 0);
        logger.log_train("TRAIN" + std::to_string(train_id), "Sent RELEASE for " + inter);
    }

    logger.log_train("TRAIN" + std::to_string(train_id), "Completed route.");
    exit(0);
}

// Shared Memory Setup
void init_shared_memory() {
    int shmid = shmget(SHM_KEY, sizeof(SharedMemory), IPC_CREAT | 0666);
    shm = (SharedMemory*)shmat(shmid, nullptr, 0);
    new (shm) SharedMemory();  // placement new

    int time_shmid = shmget(0x1234, sizeof(int), IPC_CREAT | 0666);
    sim_time = (int*)shmat(time_shmid, nullptr, 0);
    *sim_time = 0;

    int mutex_shmid = shmget(0x5678, sizeof(pthread_mutex_t), IPC_CREAT | 0666);
    time_mutex = (pthread_mutex_t*)shmat(mutex_shmid, nullptr, 0);

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(time_mutex, &attr);
    pthread_mutexattr_destroy(&attr);
}

// Populate Shared Intersections
void populate_intersections(const std::unordered_map<std::string, Intersection>& parsed) {
    int idx = 0;
    for (const auto& [name, inter] : parsed) {
        strncpy(shm->intersections[idx].name, name.c_str(), MAX_INTERSECTION_NAME_LENGTH);
        shm->intersections[idx].capacity = inter.capacity;
        shm->intersections[idx].lock_type = inter.isMutex ? 1 : inter.capacity;
        sem_init(&shm->intersections[idx].semaphore, 1, inter.capacity);
        ++idx;
    }
}

// Main Function
int main() {
    init_shared_memory();
    Logger logger("simulation.log", sim_time, time_mutex, true);

    // Parse input files
    auto intersections = parseIntersections("intersections.txt");
    auto trains = parseTrains("trains.txt");

    if (intersections.empty() || trains.empty()) {
        std::cerr << "Error: Failed to parse input files.\n";
        return 1;
    }

    logger.log_server("Initialized intersections");
    populate_intersections(intersections);

    // Fork server
    pid_t server_pid = fork();
    if (server_pid == 0) {
        run_server(logger);
        exit(0);
    }

    // Fork each train process
    for (int i = 0; i < trains.size(); ++i) {
        pid_t pid = fork();
        if (pid == 0) {
            run_train(i + 1, trains[i], logger);
        }
    }

    // Wait for all child processes
    while (wait(nullptr) > 0);

    // Delay to ensure all logs are flushed before final message
    sleep(1);
    
    logger.log_server("Simulation complete.");
    return 0;
}
