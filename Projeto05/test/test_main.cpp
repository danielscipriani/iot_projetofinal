#include <unity.h>
#include <string>
#include <vector>

#include "measurement.h"

void test_measurement_to_json() {
    Measurement m{1000, 127.5f, 2.0f, 255.0f, 0.123f, 60.0f, 0.98f, true};

    std::string json = measurementToJson(m);

    std::string expected =
        "{\"timestamp\":1000,\"voltage\":127.50,\"current\":2.000,\"power\":255.00,\"energy\":0.123,\"frequency\":60.00,\"powerFactor\":0.98,\"valid\":true}";

    TEST_ASSERT_EQUAL_STRING(expected.c_str(), json.c_str());
}

void test_measurements_to_json() {
    std::vector<Measurement> data = {
        {1000, 127.5f, 2.0f, 255.0f, 0.123f, 60.0f, 0.98f, true},
        {2000, 128.0f, 1.5f, 192.0f, 0.456f, 60.0f, 0.95f, true}
    };

    std::string json = measurementsToJson(data);

    std::string expected =
        "["
        "{\"timestamp\":1000,\"voltage\":127.50,\"current\":2.000,\"power\":255.00,\"energy\":0.123,\"frequency\":60.00,\"powerFactor\":0.98,\"valid\":true},"
        "{\"timestamp\":2000,\"voltage\":128.00,\"current\":1.500,\"power\":192.00,\"energy\":0.456,\"frequency\":60.00,\"powerFactor\":0.95,\"valid\":true}"
        "]";

    TEST_ASSERT_EQUAL_STRING(expected.c_str(), json.c_str());
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_measurement_to_json);
    RUN_TEST(test_measurements_to_json);
    return UNITY_END();
}
