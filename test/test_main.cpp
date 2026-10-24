#include "minitest.h"

TEST(harness_runs) {
    ASSERT_EQ(1, 1);
}

int main() {
    int passed = 0;
    int failed = 0;
    const auto& tests = getTestRegistry();
    for (const auto& t : tests) {
        try {
            t.fn();
            passed++;
            std::cout << "[PASS] " << t.name << "\n";
        } catch (const std::exception& e) {
            failed++;
            std::cout << "[FAIL] " << t.name << ": " << e.what() << "\n";
        } catch (...) {
            failed++;
            std::cout << "[FAIL] " << t.name << ": unknown exception\n";
        }
    }
    std::cout << passed << " passed, " << failed << " failed\n";
    return failed == 0 ? 0 : 1;
}
