#include "parser.h"
#include <iostream>

int main() {
    auto intersections = parseIntersections("intersections.txt");
    auto trains = parseTrains("trains.txt");

    std::cout << "\n--- Intersections ---\n";
    for (const auto& [name, inter] : intersections) {
        std::cout << name << ": Capacity=" << inter.capacity
                  << ", Type=" << (inter.isMutex ? "Mutex" : "Semaphore") << "\n";
    }

    std::cout << "\n--- Train Routes ---\n";
    for (const auto& train : trains) {
        std::cout << train.trainName << ": ";
        for (const auto& stop : train.route) {
            std::cout << stop << " ";
        }
        std::cout << "\n";
    }

    return 0;
}
