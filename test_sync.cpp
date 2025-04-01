#include <iostream>
#include <cassert>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <cstring>
#include <vector>

// Include the header file containing the declarations from sync.cpp
// Since we're in a separate test file, we need this to know the
// structure of SharedMemory, IntersectionData, and the function prototypes.
#include "sync.h"

// Helper function to get the number of trains holding a lock
int get_holding_train_count(const std::string& intersection_name, SharedMemory* shm) {
    int index = find_intersection_index(intersection_name, shm);
    if (index == -1) {
        return -1; // Indicate error, should not happen in test
    }
    return shm->intersections[index].num_holding_trains;
}

// Helper function to check if a train is holding a lock
bool is_train_holding_lock(int train_id, const std::string& intersection_name, SharedMemory* shm) {
    int index = find_intersection_index(intersection_name, shm);
    if (index == -1) {
        return false; // Intersection not found
    }
    for (int i = 0; i < shm->intersections[index].num_holding_trains; ++i) {
        if (shm->intersections[index].holding_trains[i] == train_id) {
            return true;
        }
    }
    return false;
}

// Function to run tests
void run_tests() {
    // 1. Setup: Create shared memory
    int shmid = shmget(SHM_KEY, sizeof(SharedMemory), IPC_CREAT | 0666);
    assert(shmid >= 0);
    SharedMemory* shm = static_cast<SharedMemory*>(shmat(shmid, nullptr, 0));
    assert(shm != reinterpret_cast<SharedMemory*>(-1));

    // Initialize shared memory
    new (shm) SharedMemory();

    // Initialize some intersections for testing
    strncpy(shm->intersections[0].name, "IntersectionA", sizeof(shm->intersections[0].name) - 1);
    shm->intersections[0].capacity = 1;
    shm->intersections[0].lock_type = 1; // Mutex
    sem_init(&shm->intersections[0].semaphore, 1, shm->intersections[0].capacity);

    strncpy(shm->intersections[1].name, "IntersectionB", sizeof(shm->intersections[1].name) - 1);
    shm->intersections[1].capacity = 2;
    shm->intersections[1].lock_type = 2; // Semaphore
    sem_init(&shm->intersections[1].semaphore, 1, shm->intersections[1].capacity);

    // 2. Test Cases

    // --- Test Case 1: Mutex Acquisition and Release ---
    std::cout << "Running Test Case 1: Mutex Acquisition and Release...\n";
    int train1_id = 101;
    int train2_id = 102;

    handle_acquire_request(train1_id, "IntersectionA", shm);
    assert(get_holding_train_count("IntersectionA", shm) == 1);
    assert(is_train_holding_lock(train1_id, "IntersectionA", shm));

    handle_acquire_request(train2_id, "IntersectionA", shm);
    assert(get_holding_train_count("IntersectionA", shm) == 1); // Should still be 1, train2 is waiting
    assert(!is_train_holding_lock(train2_id, "IntersectionA", shm));

    handle_release_request(train1_id, "IntersectionA", shm);
    assert(get_holding_train_count("IntersectionA", shm) == 0);
    assert(!is_train_holding_lock(train1_id, "IntersectionA", shm));

    handle_acquire_request(train2_id, "IntersectionA", shm);
    assert(get_holding_train_count("IntersectionA", shm) == 1);
    assert(is_train_holding_lock(train2_id, "IntersectionA", shm));

    handle_release_request(train2_id, "IntersectionA", shm);
    assert(get_holding_train_count("IntersectionA", shm) == 0);

    std::cout << "Test Case 1 Passed!\n";

    // --- Test Case 2: Semaphore Acquisition and Release ---
    std::cout << "Running Test Case 2: Semaphore Acquisition and Release...\n";
    int train3_id = 201;
    int train4_id = 202;
    int train5_id = 203;

    handle_acquire_request(train3_id, "IntersectionB", shm);
    assert(get_holding_train_count("IntersectionB", shm) == 1);
    assert(is_train_holding_lock(train3_id, "IntersectionB", shm));

    handle_acquire_request(train4_id, "IntersectionB", shm);
    assert(get_holding_train_count("IntersectionB", shm) == 2);
    assert(is_train_holding_lock(train4_id, "IntersectionB", shm));

    handle_acquire_request(train5_id, "IntersectionB", shm);
    assert(get_holding_train_count("IntersectionB", shm) == 2); // Should be 2, semaphore is full.
    assert(!is_train_holding_lock(train5_id, "IntersectionB", shm));

    handle_release_request(train3_id, "IntersectionB", shm);
    assert(get_holding_train_count("IntersectionB", shm) == 1);
    assert(!is_train_holding_lock(train3_id, "IntersectionB", shm));

    handle_release_request(train4_id, "IntersectionB", shm);
    assert(get_holding_train_count("IntersectionB", shm) == 0);
    assert(!is_train_holding_lock(train4_id, "IntersectionB", shm));

    std::cout << "Test Case 2 Passed!\n";

    // --- Test Case 3:  Error Handling ---
    std::cout << "Running Test Case 3: Error Handling...\n";
    int invalid_train_id = 999;
    std::string invalid_intersection_name = "InvalidIntersection";

    // Try to release a lock that a train doesn't hold.
    handle_release_request(invalid_train_id, "IntersectionA", shm); // Should print an error message.
    assert(get_holding_train_count("IntersectionA", shm) == 0); //should still be 0

    // Try to acquire a lock for a non-existent intersection.
    handle_acquire_request(train1_id, invalid_intersection_name, shm); // Should print an error
    assert(find_intersection_index(invalid_intersection_name, shm) == -1);

     // Try to release a lock for a non-existent intersection.
    handle_release_request(train1_id, invalid_intersection_name, shm);  // Should print an error
    assert(find_intersection_index(invalid_intersection_name, shm) == -1);

    std::cout << "Test Case 3 Passed!\n";

    // 3. Cleanup: Detach and remove shared memory
    if (shmdt(shm) == -1) {
        perror("shmdt");
        // In a real test suite, you might want to assert(false) here, but
        // for this example, we'll just print an error.
    }
    if (shmctl(shmid, IPC_RMID, nullptr) == -1) {
        perror("shmctl");
        // assert(false); // Consider this in a robust test suite.
    }

    std::cout << "All tests passed!\n";
}

int main() {
    run_tests();
    return 0;
}

