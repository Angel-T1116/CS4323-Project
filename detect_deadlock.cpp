#include <iostream>
using namespace std;

const int n = 2; // number of processes
const int m = 2; // number of resources

bool detect_deadlock(int allocation[][m], int request[][m], int available[], int n, int m)
{
    bool finish[n] = {false};
    int work[m];

    for (int j=0; j<m; j++)
    {
        work[j] = available[j];
    }

    //Banker's Algorithm
    bool made_progess;
    do
    {
        made_progess = false;
        for (int i=0; i<n; i++)
        {
            if (!finish[i])
            {
                bool can_proceed = true;
                for (int j=0; j<m; j++)
                {
                    if (request[i][j] > work[j]) //inversion of banker's algorithm, so use request > available as a fail case rather than request <= available as a pass case.
                    {
                        can_proceed = false;
                        break;
                    }
                }
                if (can_proceed)
                {
                    for (int j=0; j<m; j++)
                    {
                        work[j] += allocation[i][j];
                    }
                    finish[i] = true;
                    made_progess = true;
                }
            }
        }
    } while (made_progess);

    //unfinished process detection
    for (int i=0; i<n; i++)
    {
        if (!finish[i]) return true;//deadlock
    }
    return false;//no deadlock
}

void test_deadlock(string desc, int allocation[][m], int request[][m], int available[], bool expected_deadlock)
{
    cout <<desc<<"...";
    bool result = detect_deadlock(allocation, request, available, n, m);
    if (result == expected_deadlock)
    {
        cout<<"PASS"<<endl;
    }
    else
    {
        cout<<"FAIL"<<endl;
    }
}

int main()
{
    // Test 1: Deadlock scenario
    int allocation1[n][m] = {
        {1, 0}, // P0 holds 1 A
        {0, 1}  // P1 holds 1 B
    };
    int request1[n][m] = {
        {0, 1}, // P0 wants 1 B
        {1, 0}  // P1 wants 1 A
    };
    int available1[m] = {0, 0};

    test_deadlock("Test 1: Deadlock P0 and P1", allocation1, request1, available1, true);

    // Test 2: No Deadlock, one process can finish
    int allocation2[n][m] = {
        {1, 0}, // P0 holds 1 A
        {0, 1}  // P1 holds 1 B
    };
    int request2[n][m] = {
        {0, 0}, // P0 wants nothing
        {1, 0}  // P1 wants 1 A
    };
    int available2[m] = {0, 0};

    test_deadlock("Test 2: No Deadlock (P0 can finish)", allocation2, request2, available2, false);

    // Test 3: No Deadlock, all resources available
    int allocation3[n][m] = {
        {0, 0},
        {0, 0}
    };
    int request3[n][m] = {
        {0, 0},
        {0, 0}
    };
    int available3[m] = {2, 2};

    test_deadlock("Test 3: No Deadlock (no allocations or requests)", allocation3, request3, available3, false);

    // Test 4: Deadlock, circular wait
    int allocation4[n][m] = {
        {1, 0}, // P0 holds 1 A
        {0, 1}  // P1 holds 1 B
    };
    int request4[n][m] = {
        {0, 1}, // P0 requests 1 B
        {1, 0}  // P1 requests 1 A
    };
    int available4[m] = {0, 0};

    test_deadlock("Test 4: Deadlock (circular wait)", allocation4, request4, available4, true);

    return 0;
}