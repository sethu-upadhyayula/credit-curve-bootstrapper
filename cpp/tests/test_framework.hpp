#pragma once
// Minimal, dependency-free test framework: no Catch2/GoogleTest required.

#include <cmath>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

namespace testfw {

struct TestCase {
    std::string name;
    std::function<void()> fn;
};

inline std::vector<TestCase>& registry() {
    static std::vector<TestCase> tests;
    return tests;
}

struct Registrar {
    Registrar(const std::string& name, std::function<void()> fn) {
        registry().push_back({name, std::move(fn)});
    }
};

inline int currentFailures = 0;
inline std::string currentTestName;

inline void reportFailure(const std::string& msg) {
    std::cerr << "  [FAIL] " << currentTestName << ": " << msg << "\n";
    ++currentFailures;
}

inline int runAll() {
    int totalFailuresBefore, failedTests = 0;
    for (auto& t : registry()) {
        currentTestName = t.name;
        totalFailuresBefore = currentFailures;
        t.fn();
        if (currentFailures == totalFailuresBefore) {
            std::cout << "  [ OK ] " << t.name << "\n";
        } else {
            ++failedTests;
        }
    }
    std::cout << "\n" << registry().size() << " tests run, " << failedTests << " failed.\n";
    return failedTests == 0 ? 0 : 1;
}

}  // namespace testfw

#define TEST(name)                                                            \
    void name();                                                              \
    static testfw::Registrar registrar_##name(#name, name);                   \
    void name()

#define CHECK(cond)                                                           \
    do {                                                                      \
        if (!(cond)) testfw::reportFailure(std::string("CHECK failed: ") + #cond); \
    } while (0)

#define CHECK_CLOSE(a, b, tol)                                                \
    do {                                                                      \
        const double av_ = (a), bv_ = (b), tolv_ = (tol);                     \
        if (std::abs(av_ - bv_) > tolv_) {                                    \
            testfw::reportFailure(std::string("CHECK_CLOSE failed: ") + #a + " (" +   \
                                   std::to_string(av_) + ") vs " + #b + " (" +  \
                                   std::to_string(bv_) + "), tol=" + std::to_string(tolv_)); \
        }                                                                      \
    } while (0)
