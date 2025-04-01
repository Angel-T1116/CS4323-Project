/*
Author: Samuel Shankle
Email: samuel.shankle@okstate.edu
Date: 04/01/2025
Description:
*/

#ifndef SYNC_H
#define SYNC_H

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

    IntersectionData();
    ~IntersectionData();
};

// Shared memory structure
struct SharedMemory {
    IntersectionData intersections[MAX_INTERSECTIONS];
    pthread_mutex_t shared_memory_mutex; // Auxiliary mutex

    SharedMemory();
    ~SharedMemory();
};

// Function to find the index of an intersection by name
int find_intersection_index(const std::string& name, SharedMemory* shm);

// Function to handle ACQUIRE request from a train
void handle_acquire_request(int train_id, const std::string& intersection_name, SharedMemory* shm);

// Function to handle RELEASE request from a train
void handle_release_request(int train_id, const std::string& intersection_name, SharedMemory* shm);

#endif 
