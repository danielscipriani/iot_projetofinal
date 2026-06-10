#include "measurement.h"

#include <sstream>
#include <iomanip>

std::string measurementsToJson(const std::vector<Measurement>& measurements) {
    std::ostringstream json;

    json << "[";

    for (size_t i = 0; i < measurements.size(); i++) {
        const Measurement& m = measurements[i];

        json << "{";
        json << "\"timestamp\":" << m.timestamp << ",";
        json << "\"voltage\":" << std::fixed << std::setprecision(2) << m.voltage << ",";
        json << "\"current\":" << std::fixed << std::setprecision(2) << m.current << ",";
        json << "\"power\":" << std::fixed << std::setprecision(2) << m.power;
        json << "}";

        if (i < measurements.size() - 1) {
            json << ",";
        }
    }

    json << "]";

    return json.str();
}