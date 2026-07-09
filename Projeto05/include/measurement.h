#ifndef MEASUREMENT_H
#define MEASUREMENT_H

#include <cstdint>
#include <string>
#include <vector>

struct Measurement {
    uint32_t timestamp = 0;
    float voltage = 0.0f;
    float current = 0.0f;
    float power = 0.0f;
    float energy = 0.0f;
    float frequency = 0.0f;
    float powerFactor = 0.0f;
    bool valid = false;
};

std::string measurementToJson(const Measurement& measurement);
std::string measurementsToJson(const std::vector<Measurement>& measurements);

#endif
