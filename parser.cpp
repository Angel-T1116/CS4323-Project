// Group : I
// Author: Frantisek Zubek
// Email: fero@okstate.edu
// Date: 04/01/2025
// Description: Implements parsing of intersection and train route data from input text files.

#include "parser.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

// parses the intersections text into a map of Intersection structs
std::unordered_map<std::string, Intersection> parseIntersections(const std::string& filename) {
    std::unordered_map<std::string, Intersection> intersections;
    std::ifstream file(filename);
    std::string line;

    if (!file.is_open()) {
        std::cerr << "Error: Could not open " << filename << std::endl;
        return intersections;
    }

    while (std::getline(file, line)) {
        size_t colonPos = line.find(':');
        if (colonPos == std::string::npos) {
            std::cerr << "Warning: Skipping invalid line: " << line << std::endl;
            continue;
        }

        std::string name = line.substr(0, colonPos);
        int capacity;
        try {
            capacity = std::stoi(line.substr(colonPos + 1));
        } catch (...) {
            std::cerr << "Could not parse capacity for intersection in line: " << line << std::endl;
            continue;
        }

        Intersection inter{name, capacity, capacity == 1};
        intersections[name] = inter;
    }

    file.close();
    return intersections;
}

// parsing the trans text file into a vctor of TrainRoute structs
std::vector<TrainRoute> parseTrains(const std::string& filename) {
    std::vector<TrainRoute> trainRoutes;
    std::ifstream file(filename);
    std::string line;

    if (!file.is_open()) {
        std::cerr << "Error: Could not open " << filename << std::endl;
        return trainRoutes;
    }

    while (std::getline(file, line)) {
        size_t colonPos = line.find(':');
        if (colonPos == std::string::npos) {
            std::cerr << "Warning: Skipping invalid line: " << line << std::endl;
            continue;
        }

        std::string trainName = line.substr(0, colonPos);
        std::string routeStr = line.substr(colonPos + 1);
        std::vector<std::string> route;
        std::istringstream ss(routeStr);
        std::string intersection;

        while (std::getline(ss, intersection, ',')) {
            intersection.erase(std::remove_if(intersection.begin(), intersection.end(), isspace), intersection.end());
            route.push_back(intersection);
        }

        TrainRoute tr{trainName, route};
        trainRoutes.push_back(tr);
    }

    file.close();
    return trainRoutes;
}