#include "measurement.h"

#include <iomanip>
#include <sstream>

static void appendMeasurementJson(std::ostringstream& json, const Measurement& m) {
    json << "{";
    json << "\"timestamp\":" << m.timestamp << ",";
    json << "\"voltage\":" << std::fixed << std::setprecision(2) << m.voltage << ",";
    json << "\"current\":" << std::fixed << std::setprecision(3) << m.current << ",";
    json << "\"power\":" << std::fixed << std::setprecision(2) << m.power << ",";
    json << "\"energy\":" << std::fixed << std::setprecision(3) << m.energy << ",";
    json << "\"frequency\":" << std::fixed << std::setprecision(2) << m.frequency << ",";
    json << "\"powerFactor\":" << std::fixed << std::setprecision(2) << m.powerFactor << ",";
    json << "\"valid\":" << (m.valid ? "true" : "false");
    json << "}";
}

std::string measurementToJson(const Measurement& measurement) {
    std::ostringstream json;
    appendMeasurementJson(json, measurement);
    return json.str();
}

std::string measurementsToJson(const std::vector<Measurement>& measurements) {
    std::ostringstream json;
    json << "[";

    for (size_t i = 0; i < measurements.size(); i++) {
        appendMeasurementJson(json, measurements[i]);
        if (i < measurements.size() - 1) {
            json << ",";
        }
    }

    json << "]";
    return json.str();
}
