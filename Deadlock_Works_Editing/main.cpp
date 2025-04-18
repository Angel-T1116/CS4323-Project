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
#include "detect_deadlock.h"

#define SHM_KEY 12345
#define MSGKEY 1234

struct TrainMessage {
    long type;
    int train_id;
    char command[10];
    char intersection[50];
};

SharedMemory* shm;
int* sim_time;
pthread_mutex_t* time_mutex;

std::vector<std::vector<int>> allocation;
std::vector<std::vector<int>> request;
std::vector<int> available;

void run_server(Logger& logger) {
    int msgid = msgget(MSGKEY, IPC_CREAT | 0666);
    TrainMessage msg;

    while (true) {
        if (msgrcv(msgid, &msg, sizeof(msg) - sizeof(long), 1, 0) > 0) {
            if (strcmp(msg.command, "shutdown") == 0) {
                logger.log_server("Shutdown command received. Exiting server.");
                break;
            }

            std::string inter = msg.intersection;
            logger.log_server("Received request from Train" + std::to_string(msg.train_id) + ": " + msg.command);

            int train_idx = msg.train_id - 1;
            int inter_idx = find_intersection_index(inter, shm);

            if (strcmp(msg.command, "acquire") == 0) {
                request[train_idx][inter_idx] = 1;

// Detect before acquiring (deadlock if both waiting)
	std::cout << "[DEBUG] Running deadlock detection check...\n";
bool deadlock = detect_deadlock(allocation, request, available);
logger.log_server("Deadlock check triggered"); // Confirm logger is hit


// Print matrix every time, not just when a deadlock is found
std::cout << "==== DEADLOCK CHECK ====\nAllocation:\n";
for (auto& row : allocation) {
    for (int x : row) std::cout << x << " ";
    std::cout << "\n";
}
std::cout << "Request:\n";
for (auto& row : request) {
    for (int x : row) std::cout << x << " ";
    std::cout << "\n";
}
std::cout << "Available:\n";
for (int x : available) std::cout << x << " ";
std::cout << "\n========================\n";

if (deadlock) {
    logger.log_server("Deadlock detected.");
    recover_from_deadlock(allocation, request, available, shm, logger);
    continue;
}

                handle_acquire_request(msg.train_id, inter, shm, request);
                request[train_idx][inter_idx] = 0;
                allocation[train_idx][inter_idx] = 1;
		available[inter_idx]--;
            } else if (strcmp(msg.command, "release") == 0) {
                handle_release_request(msg.train_id, inter, shm);
                allocation[train_idx][inter_idx] = 0;
                available[inter_idx]++;
		if (detect_deadlock(allocation, request, available)) {
        logger.log_server("Deadlock detected (post-acquire).");
        std::cout << "==== POST-DEADLOCK CHECK ====\n";
	recover_from_deadlock(allocation, request, available, shm, logger);
    	continue;
            }
	}
            msg.type = 2;
            strcpy(msg.command, "granted");
            msgsnd(msgid, &msg, sizeof(msg) - sizeof(long), 0);
            logger.log_server("Granted " + inter + " to Train" + std::to_string(msg.train_id));
        }
    }
}

void run_train(int train_id, const TrainRoute& route, Logger& logger) {
    sleep(1); // For deadlock
    int msgid = msgget(MSGKEY, 0666);
    TrainMessage msg;
    msg.train_id = train_id;
    msg.type = 1;
    
    // Acquire all intersections first
    for (size_t i = 0; i < route.route.size(); ++i) {
    const std::string& inter = route.route[i];

    strcpy(msg.command, "acquire");
    strncpy(msg.intersection, inter.c_str(), sizeof(msg.intersection));
    msgsnd(msgid, &msg, sizeof(msg) - sizeof(long), 0);
    logger.log_train("TRAIN" + std::to_string(train_id), "Sent ACQUIRE for " + inter);

    msgrcv(msgid, &msg, sizeof(msg) - sizeof(long), 2, 0);
    logger.log_train("TRAIN" + std::to_string(train_id), "Granted " + inter);

    if (i == 0) sleep(1);
}

    // Then release all intersections
    for (const std::string& inter : route.route) {
        strcpy(msg.command, "release");
        strncpy(msg.intersection, inter.c_str(), sizeof(msg.intersection));
        msgsnd(msgid, &msg, sizeof(msg) - sizeof(long), 0);
        logger.log_train("TRAIN" + std::to_string(train_id), "Sent RELEASE for " + inter);
    }

    logger.log_train("TRAIN" + std::to_string(train_id), "Completed route.");
    exit(0);
}


void init_shared_memory() {
    int shmid = shmget(SHM_KEY, sizeof(SharedMemory), IPC_CREAT | 0666);
    shm = (SharedMemory*)shmat(shmid, nullptr, 0);
    new (shm) SharedMemory();

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

void populate_intersections(const std::unordered_map<std::string, Intersection>& parsed) {
    int idx = 0;
    for (const auto& [name, inter] : parsed) {
        strncpy(shm->intersections[idx].name, name.c_str(), MAX_INTERSECTION_NAME_LENGTH);
        shm->intersections[idx].capacity = inter.capacity;
        shm->intersections[idx].lock_type = inter.isMutex ? 1 : inter.capacity;
        sem_init(&shm->intersections[idx].semaphore, 1, inter.capacity);
        ++idx;
	std::cout << "[DEBUG] Initialized " << name << " with capacity " << inter.capacity << std::endl;

    }
}

void init_matrices(int num_trains, int num_resources) {
    allocation.assign(num_trains, std::vector<int>(num_resources, 0));
    request.assign(num_trains, std::vector<int>(num_resources, 0));
    available.assign(num_resources, 1);
}

int main() {
    init_shared_memory();
    Logger logger("simulation.log", sim_time, time_mutex, true);
    
    auto intersections = parseIntersections("intersections.txt");
    auto trains = parseTrains("trains.txt");

    if (intersections.empty() || trains.empty()) {
        std::cerr << "Error: Failed to parse input files.\n";
        return 1;
    }

    logger.log_server("Initialized intersections");
    populate_intersections(intersections);
    init_matrices(trains.size(), intersections.size());

    pid_t server_pid = fork();
    if (server_pid == 0) {
        run_server(logger);
        exit(0);
    }

    for (int i = 0; i < trains.size(); ++i) {
        pid_t pid = fork();
        if (pid == 0) {
            run_train(i + 1, trains[i], logger);
        }
    }

    sleep(1);
TrainMessage shutdown_msg = {1, 0, "shutdown", ""};
int msgid = msgget(MSGKEY, 0666);
msgsnd(msgid, &shutdown_msg, sizeof(shutdown_msg) - sizeof(long), 0);

    sleep(1);
    logger.log_server("Simulation complete.");
    sleep(2);
    return 0;
}