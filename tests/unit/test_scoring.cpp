// Self-test for ScoringModule. Run through CTest (see tests/README.md).
#include "ScoringModule.h"
#include <iostream>
#include <sstream>
#include <iomanip>
using namespace ScoringModule;

static int failures = 0;
#define CHECK(cond)                                                              \
    do {                                                                         \
        if (!(cond)) { std::cerr << __FILE__ << ':' << __LINE__ << ": FAILED: " #cond "\n"; failures++; } \
    } while (0)

int main() {
    // ---- grade bands (QZ-F-022), including the exact boundaries ----
    CHECK(gradeFor(100.0) == "Excellent");
    CHECK(gradeFor(80.0) == "Excellent");
    CHECK(gradeFor(79.9) == "Good");
    CHECK(gradeFor(50.0) == "Good");
    CHECK(gradeFor(49.9) == "Needs Improvement");
    CHECK(gradeFor(0.0) == "Needs Improvement");
    CHECK(gradeFor(-5.0) == "Needs Improvement");  // clamped
    CHECK(gradeFor(250.0) == "Excellent");         // clamped

    // ---- summary (QZ-F-020 / QZ-F-021) ----
    Summary s = summarize(7, 2, 1);
    CHECK(s.total == 10 && s.correct == 7 && s.incorrect == 2 && s.skipped == 1);
    CHECK(s.percentage > 69.99 && s.percentage < 70.01);
    CHECK(s.grade == "Good");
    CHECK(s.correct + s.incorrect + s.skipped == s.total);  // reconciles

    s = summarize(10, 0, 0);
    CHECK(s.percentage > 99.99 && s.grade == "Excellent");

    s = summarize(0, 0, 10);  // all skipped scores 0
    CHECK(s.percentage == 0.0 && s.grade == "Needs Improvement" && s.skipped == 10);

    s = summarize(8, 2, 0);   // exactly 80%
    CHECK(s.grade == "Excellent");

    s = summarize(5, 5, 0);   // exactly 50%
    CHECK(s.grade == "Good");

    s = summarize(1, 2, 0);   // 33.33...%
    CHECK(s.percentage > 33.3 && s.percentage < 33.4 && s.grade == "Needs Improvement");

    s = summarize(0, 0, 0);   // no questions: no divide by zero
    CHECK(s.total == 0 && s.percentage == 0.0);

    s = summarize(-3, -1, -2);  // negatives treated as 0
    CHECK(s.total == 0);

    // ---- printed summary ----
    std::ostringstream out;
    printSummary(summarize(7, 2, 1), out);
    const std::string t = out.str();
    CHECK(t.find("Total questions: 10") != std::string::npos);
    CHECK(t.find("Correct:         7") != std::string::npos);
    CHECK(t.find("Incorrect:       2") != std::string::npos);
    CHECK(t.find("Skipped:         1") != std::string::npos);
    CHECK(t.find("70.0%") != std::string::npos);
    CHECK(t.find("Grade:           Good") != std::string::npos);

    // Printing a summary must not change the caller's stream formatting.
    std::ostringstream formatted;
    formatted << std::scientific << std::setprecision(4);

    const auto originalFlags = formatted.flags();
    const auto originalPrecision = formatted.precision();

    printSummary(summarize(7, 2, 1), formatted);

    CHECK(formatted.flags() == originalFlags);
    CHECK(formatted.precision() == originalPrecision);

    if (failures == 0) std::cout << "All ScoringModule tests passed.\n";
    return failures == 0 ? 0 : 1;
}
