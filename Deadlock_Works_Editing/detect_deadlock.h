// Group : I
// Author: Brandon Collings
// Email: brandon.l.collings@okstate.edu
// Date: 04/11/2025
// Description: Implements deadlock detection using a Resource Allocation Table and Banker's Algorithm.

#ifndef DETECT_DEADLOCK_H
#define DETECT_DEADLOCK_H

#include <vector>
#include <string>

// Forward declarations for shared memory and logger
struct SharedMemory;
class Logger;

// Detects if a deadlock exists in the system.
// Returns true if deadlock detected, false otherwise.
bool detect_deadlock(const std::vector<std::vector<int>>& allocation,
                     const std::vector<std::vector<int>>& request,
                     const std::vector<int>& available);


// Angels Recovery Code
void recover_from_deadlock(std::vector<std::vector<int>>& allocation,
    std::vector<std::vector<int>>& request,
    std::vector<int>& available,
    SharedMemory* shm,
    Logger& logger);

#endif // DETECT_DEADLOCK_H