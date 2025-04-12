// Group : I
// Author: Brandon Collings
// Email: brandon.l.collings@okstate.edu
// Date: 04/11/2025
// Description: Implements deadlock detection using a Resource Allocation Table and Banker's Algorithm.

#include <iostream>
#include <vector>
#include <string>
using namespace std;

bool detect_deadlock(const vector<vector<int>>& allocation, const vector<vector<int>>& request, const vector<int>& available)
{
    int n = allocation.size();
    int m = available.size();
    vector<bool> finish(n, false);
    vector<int> work = available;

    // Banker's Algorithm
    bool made_progress;
    do
    {
        made_progress = false;
        for (int i = 0; i < n; i++)
        {
            if (!finish[i])
            {
                bool can_proceed = true;
                for (int j = 0; j < m; j++)
                {
                    if (request[i][j] > work[j]) // Inversion of Banker's algorithm
                    {
                        can_proceed = false;
                        break;
                    }
                }
                if (can_proceed)
                {
                    for (int j = 0; j < m; j++)
                    {
                        work[j] += allocation[i][j];
                    }
                    finish[i] = true;
                    made_progress = true;
                }
            }
        }
    } while (made_progress);

    // Unfinished process detection
    for (int i = 0; i < n; i++)
    {
        if (!finish[i]) return true; // Deadlock detected
    }
    return false; // No deadlock
}

void test_deadlock(const string& desc, const vector<vector<int>>& allocation, const vector<vector<int>>& request, const vector<int>& available, bool expected_deadlock)
{
    cout << desc << "...";
    bool result = detect_deadlock(allocation, request, available);
    if (result == expected_deadlock)
    {
        cout << "PASS" << endl;
    }
    else
    {
        cout << "FAIL" << endl;
    }
}

int main()
{
    // Test 1: Deadlock scenario
    vector<vector<int>> allocation1 = {
        {1, 0}, // P0 holds 1 A
        {0, 1}  // P1 holds 1 B
    };
    vector<vector<int>> request1 = {
        {0, 1}, // P0 wants 1 B
        {1, 0}  // P1 wants 1 A
    };
    vector<int> available1 = {0, 0};

    test_deadlock("Test 1: Deadlock P0 and P1 (P0 and P1 holding each other's resource)", allocation1, request1, available1, true);

    // Test 2: No Deadlock, one process can finish
    vector<vector<int>> allocation2 = {
        {1, 0}, // P0 holds 1 A
        {0, 1}  // P1 holds 1 B
    };
    vector<vector<int>> request2 = {
        {0, 0}, // P0 wants nothing
        {1, 0}  // P1 wants 1 A
    };
    vector<int> available2 = {0, 0};

    test_deadlock("Test 2: No Deadlock (P0 can finish)", allocation2, request2, available2, false);

    // Test 3: No Deadlock, all resources available
    vector<vector<int>> allocation3 = {
        {0, 0},
        {0, 0}
    };
    vector<vector<int>> request3 = {
        {0, 0},
        {0, 0}
    };
    vector<int> available3 = {2, 2};

    test_deadlock("Test 3: No Deadlock (no allocations or requests)", allocation3, request3, available3, false);

    // Test 4: Deadlock, circular wait
    vector<vector<int>> allocation4 = {
        {1, 0}, // P0 holds 1 A
        {0, 1}  // P1 holds 1 B
    };
    vector<vector<int>> request4 = {
        {0, 1}, // P0 requests 1 B
        {1, 0}  // P1 requests 1 A
    };
    vector<int> available4 = {0, 0};

    test_deadlock("Test 4: Deadlock (circular wait)", allocation4, request4, available4, true);

    return 0;
}
