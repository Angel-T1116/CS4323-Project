/*
Author: Samuel Shankle
Email: samuel.shankle@okstate.edu
Date: 04/01/2025
Description: This program is a template for the train intersection problem. 
             It demonstrates how to use shared memory, semaphores, and mutexes to coordinate access to shared resources.
*/

#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <pthread.h>
#include <semaphore.h>
#include <cstring>
#include <unistd.h>
#include <map>
#include <algorithm>

// Define constants
#define SHM_KEY 12345
#define MAX_INTERSECTIONS 50
#define MAX_TRAINS_AT_INTERSECTION 10
#define MAX_TRAIN_NAME_LENGTH 50
#define MAX_INTERSECTION_NAME_LENGTH 50

// Structure to hold intersection data in shared memory
struct IntersectionData {
    char name[MAX_INTERSECTION_NAME_LENGTH];
    int capacity;
    pthread_mutex_t mutex;
    sem_t semaphore;
    int lock_type; // 1 for mutex, >1 for semaphore
    int holding_trains[MAX_TRAINS_AT_INTERSECTION]; // Array to store holding train IDs
    int num_holding_trains;

    IntersectionData() : capacity(0), lock_type(0), num_holding_trains(0) {
        pthread_mutex_init(&mutex, nullptr);
        sem_init(&semaphore, 1, 0); // Initialize to 0, capacity set later
        memset(holding_trains, 0, sizeof(holding_trains));
        memset(name, 0, sizeof(name));
    }

    ~IntersectionData() {
        pthread_mutex_destroy(&mutex);
        sem_destroy(&semaphore);
    }
};

// Shared memory structure
struct SharedMemory {
    IntersectionData intersections[MAX_INTERSECTIONS];
    pthread_mutex_t shared_memory_mutex; // Auxiliary mutex

    SharedMemory() {
        pthread_mutex_init(&shared_memory_mutex, nullptr);
    }

    ~SharedMemory() {
        for (int i = 0; i < MAX_INTERSECTIONS; ++i) {
            intersections[i].~IntersectionData();
        }
        pthread_mutex_destroy(&shared_memory_mutex);
    }
};

// Function to find the index of an intersection by name
int find_intersection_index(const std::string& name, SharedMemory* shm) {
    for (int i = 0; i < MAX_INTERSECTIONS; ++i) {
        if (strcmp(shm->intersections[i].name, name.c_str()) == 0) {
            return i;
        }
    }
    return -1;
}

// Function to handle ACQUIRE request from a train
void handle_acquire_request(int train_id, const std::string& intersection_name, SharedMemory* shm) {
    pthread_mutex_lock(&shm->shared_memory_mutex); // Lock shared memory for atomic access

    int intersection_index = find_intersection_index(intersection_name, shm);
    if (intersection_index == -1) {
        std::cerr << "Error: Intersection " << intersection_name << " not found." << std::endl;
        pthread_mutex_unlock(&shm->shared_memory_mutex);
        return;
    }

    IntersectionData* intersection = &shm->intersections[intersection_index];

    if (intersection->lock_type == 1) { // Mutex
        // Check if the mutex is locked
        bool is_locked = false;
        for (int i = 0; i < intersection->num_holding_trains; ++i) {
            if (intersection->holding_trains[i] == train_id) {
                std::cerr << "Error: Train " << train_id << " already holds mutex for " << intersection_name << std::endl;
                is_locked = true;
                break;
            }
        }
        if (!is_locked && intersection->num_holding_trains == 0) {
            pthread_mutex_lock(&intersection->mutex);
            intersection->holding_trains[0] = train_id;
            intersection->num_holding_trains = 1;
            std::cout << "Train " << train_id << " acquired mutex for " << intersection_name << std::endl;
            // Send GRANT message to train (implementation depends on IPC)
        } else {
            std::cout << "Train " << train_id << " waiting for mutex on " << intersection_name << std::endl;
            // Send WAIT message to train (implementation depends on IPC)
        }
    } else { // Semaphore
        // Check if there are available slots
        bool already_holding = false;
        for (int i = 0; i < intersection->num_holding_trains; ++i) {
            if (intersection->holding_trains[i] == train_id) {
                std::cerr << "Error: Train " << train_id << " already holding semaphore for " << intersection_name << std::endl;
                already_holding = true;
                break;
            }
        }
        if (!already_holding && intersection->num_holding_trains < intersection->capacity) {
            sem_wait(&intersection->semaphore); // Decrement semaphore
            intersection->holding_trains[intersection->num_holding_trains++] = train_id;
            std::cout << "Train " << train_id << " acquired semaphore for " << intersection_name << std::endl;
            // Send GRANT message to train (implementation depends on IPC)
        } else {
            std::cout << "Train " << train_id << " waiting for semaphore on " << intersection_name << std::endl;
            // Send WAIT message to train (implementation depends on IPC)
        }
    }

    pthread_mutex_unlock(&shm->shared_memory_mutex); // Unlock shared memory
}

// Function to handle RELEASE request from a train
void handle_release_request(int train_id, const std::string& intersection_name, SharedMemory* shm) {
    pthread_mutex_lock(&shm->shared_memory_mutex); // Lock shared memory for atomic access

    int intersection_index = find_intersection_index(intersection_name, shm);
    if (intersection_index == -1) {
        std::cerr << "Error: Intersection " << intersection_name << " not found." << std::endl;
        pthread_mutex_unlock(&shm->shared_memory_mutex);
        return;
    }

    IntersectionData* intersection = &shm->intersections[intersection_index];
    bool found_train = false;

    if (intersection->lock_type == 1) { // Mutex
        if (intersection->num_holding_trains == 1 && intersection->holding_trains[0] == train_id) {
            pthread_mutex_unlock(&intersection->mutex);
            intersection->num_holding_trains = 0;
            memset(intersection->holding_trains, 0, sizeof(intersection->holding_trains));
            found_train = true;
            std::cout << "Train " << train_id << " released mutex for " << intersection_name << std::endl;
        } else {
            std::cerr << "Error: Train " << train_id << " does not hold mutex for " << intersection_name << std::endl;
        }
    } else { // Semaphore
        for (int i = 0; i < intersection->num_holding_trains; ++i) {
            if (intersection->holding_trains[i] == train_id) {
                // Remove train_id from holding_trains
                for (int j = i; j < intersection->num_holding_trains - 1; ++j) {
                    intersection->holding_trains[j] = intersection->holding_trains[j + 1];
                }
                intersection->num_holding_trains--;
                sem_post(&intersection->semaphore); // Increment semaphore
                found_train = true;
                std::cout << "Train " << train_id << " released semaphore for " << intersection_name << std::endl;
                break;
            }
        }
        if (!found_train) {
            std::cerr << "Error: Train " << train_id << " does not hold semaphore for " << intersection_name << std::endl;
        }
    }

    pthread_mutex_unlock(&shm->shared_memory_mutex); // Unlock shared memory
}

int main() {
    // Create shared memory segment
    int shmid = shmget(SHM_KEY, sizeof(SharedMemory), IPC_CREAT | 0666);
    if (shmid < 0) {
        perror("shmget");
        return 1;
    }

    // Attach shared memory
    SharedMemory* shm = static_cast<SharedMemory*>(shmat(shmid, nullptr, 0));
    if (shm == reinterpret_cast<SharedMemory*>(-1)) {
        perror("shmat");
        return 1;
    }

    // Initialize shared memory (only necessary for the parent process initially)
    bool is_parent = true; // Replace with actual parent check

    if (is_parent) {
        new (shm) SharedMemory(); // Placement new to initialize in shared memory
        // Initialize intersection data based on your intersections.txt
        // Example initialization:
        strncpy(shm->intersections[0].name, "IntersectionA", sizeof(shm->intersections[0].name) - 1);
        shm->intersections[0].capacity = 1;
        shm->intersections[0].lock_type = 1;
        sem_init(&shm->intersections[0].semaphore, 1, shm->intersections[0].capacity);

        strncpy(shm->intersections[1].name, "IntersectionB", sizeof(shm->intersections[1].name) - 1);
        shm->intersections[1].capacity = 2;
        shm->intersections[1].lock_type = 2;
        sem_init(&shm->intersections[1].semaphore, 1, shm->intersections[1].capacity);
    }

    // Example usage (simulating train actions):
    int train1_id = 1;
    int train2_id = 2;

    // Train 1 tries to acquire IntersectionA
    handle_acquire_request(train1_id, "IntersectionA", shm);
    sleep(1);

    // Train 2 tries to acquire IntersectionA
    handle_acquire_request(train2_id, "IntersectionA", shm);
    sleep(1);

    // Train 1 releases IntersectionA
    handle_release_request(train1_id, "IntersectionA", shm);
    sleep(1);

    // Train 2 tries to acquire IntersectionA again (should succeed)
    handle_acquire_request(train2_id, "IntersectionA", shm);
    sleep(1);

    // Train 1 tries to acquire IntersectionB
    handle_acquire_request(train1_id, "IntersectionB", shm);
    sleep(1);

    // Train 2 tries to acquire IntersectionB
    handle_acquire_request(train2_id, "IntersectionB", shm);
    sleep(1);

    // Train 3 tries to acquire IntersectionB
    handle_acquire_request(3, "IntersectionB", shm); // Should wait

    // Train 1 releases IntersectionB
    handle_release_request(train1_id, "IntersectionB", shm);
    sleep(1);

    // Train 2 releases IntersectionB
    handle_release_request(train2_id, "IntersectionB", shm);
    sleep(1);

    // Detach shared memory
    if (shmdt(shm) == -1) {
        perror("shmdt");
        return 1;
    }

    // Optionally remove shared memory (usually done by the parent after all processes finish)
    if (is_parent) {
        if (shmctl(shmid, IPC_RMID, nullptr) == -1) {
            perror("shmctl");
            return 1;
        }
    }

    return 0;
}