#include <iostream>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <unistd.h>
#include <pthread.h>
#include <cstring>
#include "log.h"

#define SHM_KEY_TIME 0x1234
#define SHM_KEY_MUTEX 0x5678

void fork_trains();

int main() {
    // Creates shared memory for sim_time
    int shmid_time = shmget(SHM_KEY_TIME, sizeof(int), IPC_CREAT | 0666);
    int* sim_time = (int*)shmat(shmid_time, nullptr, 0);
    *sim_time = 0;

    // Creates shared memory for mutex
    int shmid_mutex = shmget(SHM_KEY_MUTEX, sizeof(pthread_mutex_t), IPC_CREAT | 0666);
    pthread_mutex_t* time_mutex = (pthread_mutex_t*)shmat(shmid_mutex, nullptr, 0);

    // Inititialize mutex attributes for process sharing
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(time_mutex, &attr);
    pthread_mutexattr_destroy(&attr);

    // Fork two child processes
    for (int i = 0; i < 2; ++i) {
        pid_t pid = fork();

        if (pid == 0) {
            // Child process
            if (i == 1) sleep(2);

            std::string train_name = "TRAIN" + std::to_string(i + 1);
            std::cout << train_name << " process started. PID: " << getpid() << std::endl;

            Logger logger("simulation.log", sim_time, time_mutex, true);  // enable PID tag

            std::cout << train_name << ": wrote log 1" << std::endl;
            logger.log_train(train_name, "Sent ACQUIRE request for IntersectionX");
            sleep(1);

            std::cout << train_name << ": wrote log 2" << std::endl;
            logger.log_server("GRANTED IntersectionX to " + train_name);

            std::cout << train_name << ": wrote log 3" << std::endl;
            logger.log_train(train_name, "Acquired IntersectionX");
            sleep(1);

            std::cout << train_name << ": wrote log 4" << std::endl;
            logger.log_train(train_name, "Traversing IntersectionX");
            sleep(1);

            std::cout << train_name << ": wrote log 5" << std::endl;
            logger.log_train(train_name, "Released IntersectionX");

            std::cout << train_name << ": wrote log 6" << std::endl;
            logger.log_server("IntersectionX released by " + train_name);

            shmdt(sim_time);
            shmdt(time_mutex);
            std::cout << train_name << " process exiting.\n";
            exit(0);
        }
    }

    // Parent waits for both children to finish
    for (int i = 0; i < 2; ++i) {
        wait(nullptr);
    }

    // Detach from shared memory
    shmdt(sim_time);
    shmdt(time_mutex);

    sleep(2);

    // Removes shared memory segments
    shmctl(shmid_time, IPC_RMID, nullptr);
    shmctl(shmid_mutex, IPC_RMID, nullptr);

    std::cout << "Multi-process logger test complete. Check simulation.log\n";
    return 0;
}
