// Group : I
// Author: Brandon Collings
// Email: brandon.l.collings@okstate.edu
// Date: 04/11/2025
// Description: Implements deadlock detection using a Resource Allocation Table and Banker's Algorithm, and recovery via victim termination.

#include "detect_deadlock.h"
#include "sync.h"
#include "log.h"
#include <iostream>
#include <vector>
#include <string>
#include <numeric>
#include <algorithm>

using namespace std;

bool detect_deadlock(const vector<vector<int>>& allocation,
                     const vector<vector<int>>& request,
                     const vector<int>& available)
{
    std::cout << "[DEBUG] Running deadlock detection check...\n";

    int n = allocation.size();
    int m = available.size();
    vector<bool> finish(n, false);
    vector<int> work = available;

    bool made_progress;
    do {
        made_progress = false;
        for (int i = 0; i < n; i++) {
            if (!finish[i]) {
                bool can_proceed = true;
                for (int j = 0; j < m; j++) {
                    if (request[i][j] > work[j]) {
                        can_proceed = false;
                        break;
                    }
                }
                if (can_proceed) {
                    for (int j = 0; j < m; j++) {
                        work[j] += allocation[i][j];
                    }
                    finish[i] = true;
                    made_progress = true;
                }
            }
        }
    } while (made_progress);

    for (int i = 0; i < n; i++) {
        if (!finish[i]) return true;
    }
    return false;
}

// Angel's Deadlock Recovery
void recover_from_deadlock(vector<vector<int>>& allocation,
                           vector<vector<int>>& request,
                           vector<int>& available,
                           SharedMemory* shm,
                           Logger& logger)
{
    int victim_train = -1;
    int max_holding = -1;

    for (int train_id = 0; train_id < allocation.size(); ++train_id) {
        int holding = accumulate(allocation[train_id].begin(), allocation[train_id].end(), 0);
        if (holding > max_holding) {
            max_holding = holding;
            victim_train = train_id;
        }
    }

    if (victim_train == -1) {
        logger.log_server("Deadlock detected.");
        return;
    }

    logger.log_server("Recovering from deadlock: Terminating Train" + to_string(victim_train + 1));
    std::cout << "[DEBUG] Running deadlock recovery...\n";

    for (int i = 0; i < allocation[victim_train].size(); ++i) {
        if (allocation[victim_train][i] == 1) {
            if (i < MAX_INTERSECTIONS) {
                const char* inter_name = shm->intersections[i].name;
                logger.log_server("Force-releasing " + string(inter_name) + " from Train" + to_string(victim_train + 1));
                handle_release_request(victim_train + 1, inter_name, shm);
                allocation[victim_train][i] = 0;
                available[i]++;
            }
        }
    }

    fill(request[victim_train].begin(), request[victim_train].end(), 0);
    logger.log_server("Train" + to_string(victim_train + 1) + " released all locks.");
}
