#ifndef MEASUREMENT_H
#define MEASUREMENT_H

#include <string>
#include <vector>

struct Measurement {
    unsigned long timestamp;
    float voltage;
    float current;
    float power;
};

std::string measurementsToJson(const std::vector<Measurement>& measurements);

#endif