// Group : I
// Author: Angel Trujillo
// Email: angel.trujillo@okstate.edu
// Date: 04/13/2025
// Description: Testing for detecting deadlocks I didnt get the chance to test this read the README
#include "detect_deadlock.h"
#include "sync.h"
#include "log.h"
#include <iostream>
#include <vector>
#include <string>
#include <numeric>
#include <cstring>

using namespace std;

// Use real Logger but log to /dev/null
int dummy_time = 0;
pthread_mutex_t dummy_mutex = PTHREAD_MUTEX_INITIALIZER;
Logger logger("/dev/null", &dummy_time, &dummy_mutex, false);

// Mock release function to override actual logic during testing
void mock_release_request(int train_id, const string& name, SharedMemory* shm) {
    cout << "[Release] Train " << train_id << " releases " << name << endl;
}

void run_test_case(const string& name,
                   vector<vector<int>> allocation,
                   vector<vector<int>> request,
                   vector<int> available) {
    SharedMemory shm;
    for (int i = 0; i < available.size(); ++i) {
        string inter_name = "I" + to_string(i);
        strncpy(shm.intersections[i].name, inter_name.c_str(), MAX_INTERSECTION_NAME_LENGTH);
        shm.intersections[i].lock_type = 1;
    }

    cout << "\n==== Test: " << name << " ====" << endl;
    bool deadlock = detect_deadlock(allocation, request, available);

    if (deadlock) {
        cout << "Deadlock detected. Recovering...\n";

        // Redirect handle_release_request to mock version
        auto real_release = handle_release_request;
        #define handle_release_request mock_release_request
        recover_from_deadlock(allocation, request, available, &shm, logger);
        #undef handle_release_request

        bool still_deadlock = detect_deadlock(allocation, request, available);
        cout << (still_deadlock ? "Deadlock persists." : "System recovered successfully.") << endl;
    } else {
        cout << "No deadlock detected." << endl;
    }
}

int main() {
    // Deadlock Case (Circular Wait)
    run_test_case("Circular Wait Deadlock", {
        {1, 0, 0}, {0, 1, 0}, {0, 0, 1}
    }, {
        {0, 1, 0}, {0, 0, 1}, {1, 0, 0}
    }, {0, 0, 0});

    // No deadlock: all requests can be satisfied
    run_test_case("No Deadlock - Safe State", {
        {0, 1}, {1, 0}
    }, {
        {0, 0}, {0, 0}
    }, {1, 1});

    // Empty system (no trains/resources)
    run_test_case("Empty System", {}, {}, {});

    // Starvation case (one train waits forever but no deadlock)
    run_test_case("Starvation - No Deadlock", {
        {1, 0}, {0, 1}, {0, 0}
    }, {
        {0, 0}, {0, 0}, {1, 1}
    }, {0, 0});

    // All processes request held resources (partial deadlock)
    run_test_case("Partial Deadlock", {
        {1, 0}, {0, 1}, {0, 0}
    }, {
        {0, 1}, {1, 0}, {1, 1}
    }, {0, 0});

    return 0;
}
