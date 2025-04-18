// Group : I
// Author: Samuel Shankle
// Email: samuel.shankle@okstate.edu
// Date: 04/01/2025
// Description: Implements acquire and release logic for intersections using synchronization primitives stored in shared memory.

#include "sync.h"

IntersectionData::IntersectionData() : capacity(0), lock_type(0), num_holding_trains(0) {
    pthread_mutex_init(&mutex, nullptr);
    //sem_init(&semaphore, 1, 0); // Initialize to 0, capacity set later
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


void handle_acquire_request(int train_id, const std::string& intersection_name, SharedMemory* shm, std::vector<std::vector<int>>& request) {
    pthread_mutex_lock(&shm->shared_memory_mutex);

    IntersectionData* intersection = nullptr;
    for (int i = 0; i < MAX_INTERSECTIONS; ++i) {
        if (strcmp(shm->intersections[i].name, intersection_name.c_str()) == 0) {
            intersection = &shm->intersections[i];
            break;
        }
    }

    if (!intersection) {
        std::cerr << "Error: Intersection " << intersection_name << " not found.\n";
        pthread_mutex_unlock(&shm->shared_memory_mutex);
        return;
    }

    pthread_mutex_unlock(&shm->shared_memory_mutex);  // ?? UNLOCK here before blocking call

    if (intersection->lock_type == 1) {
        std::cout << "Train " << train_id << " trying to acquire mutex for " << intersection_name << std::endl;
request[train_id - 1][find_intersection_index(intersection_name, shm)] = 1;
        pthread_mutex_lock(&intersection->mutex);  // ? blocks if already held
        pthread_mutex_lock(&shm->shared_memory_mutex);
        intersection->holding_trains[0] = train_id;
        intersection->num_holding_trains = 1;
        std::cout << "Train " << train_id << " acquired mutex for " << intersection_name << std::endl;
request[train_id - 1][find_intersection_index(intersection_name, shm)] = 0;
        pthread_mutex_unlock(&shm->shared_memory_mutex);
    } else {
        std::cout << "Train " << train_id << " trying to acquire semaphore for " << intersection_name << std::endl;
request[train_id - 1][find_intersection_index(intersection_name, shm)] = 1;
        sem_wait(&intersection->semaphore);  // ? blocks if none available
        pthread_mutex_lock(&shm->shared_memory_mutex);
        intersection->holding_trains[intersection->num_holding_trains++] = train_id;
        std::cout << "Train " << train_id << " acquired semaphore for " << intersection_name << std::endl;
request[train_id - 1][find_intersection_index(intersection_name, shm)] = 0;
        pthread_mutex_unlock(&shm->shared_memory_mutex);
    }
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
