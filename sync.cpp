/*
Author: Samuel Shankle
Email: samuel.shankle@okstate.edu
Date: 04/01/2025
Description: 
*/

#include "sync.h"

IntersectionData::IntersectionData() : capacity(0), lock_type(0), num_holding_trains(0) {
    pthread_mutex_init(&mutex, nullptr);
    sem_init(&semaphore, 1, 0); // Initialize to 0, capacity set later
    memset(holding_trains, 0, sizeof(holding_trains));
    memset(name, 0, sizeof(name));
}

IntersectionData::~IntersectionData() {
    pthread_mutex_destroy(&mutex);
    sem_destroy(&semaphore);
}

SharedMemory::SharedMemory()  {
    pthread_mutex_init(&shared_memory_mutex, nullptr);
}

SharedMemory::~SharedMemory() {
    for (int i = 0; i < MAX_INTERSECTIONS; ++i) {
        intersections[i].~IntersectionData();
    }
    pthread_mutex_destroy(&shared_memory_mutex);
}

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
