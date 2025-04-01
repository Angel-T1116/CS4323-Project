// Group : I
// Author: Frantisek Zubek
// Email: fero@okstate.edu
// Date: 04/01/2025
// Description: Parse the intersections and trains files

#ifndef PARSER_H
#define PARSER_H

#include <string>
#include <vector>
#include <unordered_map>

//intersection w/ capacity and type
struct Intersection {
    std::string name;
    int capacity;
    bool isMutex; //true on 1
};

//train and route
struct TrainRoute {
    std::string trainName;
    std::vector<std::string> route;
};

//parse intersection.txt and return a map of intersection name to intersectn struct
std::unordered_map<std::string, Intersection> parseIntersections(const std::string& filename);

//parsing the trans.txt and returns a vector of TrainRoute structs
std::vector<TrainRoute> parseTrains(const std::string&filename);

#endif