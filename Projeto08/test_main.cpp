#include <unity.h>
#include <string>
#include <vector>

#include "measurement.h"

void test_measurements_to_json() {
    std::vector<Measurement> data = {
        {1000, 127.5, 2.0, 255.0},
        {2000, 128.0, 1.5, 192.0}
    };

    std::string json = measurementsToJson(data);

    std::string expected =
        "["
        "{\"timestamp\":1000,\"voltage\":127.50,\"current\":2.00,\"power\":255.00},"
        "{\"timestamp\":2000,\"voltage\":128.00,\"current\":1.50,\"power\":192.00}"
        "]";

    TEST_ASSERT_EQUAL_STRING(expected.c_str(), json.c_str());
}

int main() {
    UNITY_BEGIN();

    RUN_TEST(test_measurements_to_json);

    return UNITY_END();
}