#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <functional>

struct TestCase {
    const char* name;
    std::function<void()> fn;
};

inline std::vector<TestCase>& getTestRegistry() {
    static std::vector<TestCase> registry;
    return registry;
}

inline bool registerTest(const char* name, std::function<void()> fn) {
    getTestRegistry().push_back({name, fn});
    return true;
}

#define TEST(test_name) \
    static void test_name(); \
    static const bool _reg_##test_name = registerTest(#test_name, test_name); \
    static void test_name()

class TestFailureException : public std::exception {
public:
    std::string message;
    TestFailureException(std::string msg) : message(std::move(msg)) {}
    const char* what() const noexcept override { return message.c_str(); }
};

#define ASSERT_TRUE(cond) \
    do { \
        if (!(cond)) { \
            throw TestFailureException(std::string("Assertion failed: ") + #cond + " at " + __FILE__ + ":" + std::to_string(__LINE__)); \
        } \
    } while (0)

#define ASSERT_FALSE(cond) \
    do { \
        if (cond) { \
            throw TestFailureException(std::string("Assertion failed (expected false): ") + #cond + " at " + __FILE__ + ":" + std::to_string(__LINE__)); \
        } \
    } while (0)

#define ASSERT_EQ(expected, actual) \
    do { \
        auto _exp = (expected); \
        auto _act = (actual); \
        if (!(_exp == _act)) { \
            throw TestFailureException(std::string("Assertion failed: ") + #expected + " == " + #actual + " at " + __FILE__ + ":" + std::to_string(__LINE__)); \
        } \
    } while (0)

#define ASSERT_STREQ(expected, actual) \
    do { \
        const char* _exp = (expected); \
        const char* _act = (actual); \
        if (_exp == nullptr || _act == nullptr || std::strcmp(_exp, _act) != 0) { \
            throw TestFailureException(std::string("Assertion failed: STREQ(\"") + (_exp ? _exp : "null") + "\", \"" + (_act ? _act : "null") + "\") at " + __FILE__ + ":" + std::to_string(__LINE__)); \
        } \
    } while (0)
