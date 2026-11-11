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
    CHECK(gradeFor(80.0)  == "Excellent");
    CHECK(gradeFor(79.9)  == "Good");
    CHECK(gradeFor(50.0)  == "Good");
    CHECK(gradeFor(49.9)  == "Needs Improvement");
    CHECK(gradeFor(0.0)   == "Needs Improvement");
    CHECK(gradeFor(-5.0)  == "Needs Improvement");  // clamped below 0
    CHECK(gradeFor(250.0) == "Excellent");           // clamped above 100

    // ---- gradeFor: finer boundary values just inside each band (gap 1) ----
    // Just below the Excellent threshold (79.999... should still be "Good")
    CHECK(gradeFor(79.99)  == "Good");
    CHECK(gradeFor(79.999) == "Good");
    // Just below the Good threshold (49.999... should still be "Needs Improvement")
    CHECK(gradeFor(49.99)  == "Needs Improvement");
    CHECK(gradeFor(49.999) == "Needs Improvement");
    // Exact threshold values remain in the upper band
    CHECK(gradeFor(kExcellentMin) == "Excellent");
    CHECK(gradeFor(kGoodMin)      == "Good");

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

    // ---- summarize: single-question edge cases (gap 2) ----
    s = summarize(1, 0, 0);   // one question, correct → 100%
    CHECK(s.total == 1 && s.correct == 1 && s.incorrect == 0 && s.skipped == 0);
    CHECK(s.percentage > 99.99 && s.grade == "Excellent");

    s = summarize(0, 1, 0);   // one question, wrong → 0%
    CHECK(s.total == 1 && s.correct == 0 && s.incorrect == 1);
    CHECK(s.percentage == 0.0 && s.grade == "Needs Improvement");

    s = summarize(0, 0, 1);   // one question, skipped → 0%
    CHECK(s.total == 1 && s.skipped == 1);
    CHECK(s.percentage == 0.0 && s.grade == "Needs Improvement");

    // ---- summarize: mixed negative and positive inputs (partial clamp) ----
    s = summarize(5, -2, 3);  // negative incorrect clamped to 0 → 5+0+3=8 total, 5/8 = 62.5%
    CHECK(s.total == 8 && s.correct == 5 && s.incorrect == 0 && s.skipped == 3);
    CHECK(s.percentage > 62.4 && s.percentage < 62.6);
    CHECK(s.grade == "Good");

    // ---- printed summary ----
    {
        std::ostringstream out;
        printSummary(summarize(7, 2, 1), out);
        const std::string t = out.str();
        CHECK(t.find("Total questions: 10") != std::string::npos);
        CHECK(t.find("Correct:         7")  != std::string::npos);
        CHECK(t.find("Incorrect:       2")  != std::string::npos);
        CHECK(t.find("Skipped:         1")  != std::string::npos);
        CHECK(t.find("70.0%")               != std::string::npos);
        CHECK(t.find("Grade:           Good") != std::string::npos);

        // ---- printSummary: Score line format (gap 3) ----
        CHECK(t.find("Score:           7 / 10") != std::string::npos);
    }

    // ---- printSummary: zero-total case (gap 4) ----
    {
        std::ostringstream out;
        printSummary(summarize(0, 0, 0), out);
        const std::string t = out.str();
        CHECK(t.find("Total questions: 0")  != std::string::npos);
        CHECK(t.find("Score:           0 / 0") != std::string::npos);
        CHECK(t.find("0.0%")                != std::string::npos);
        // Grade for 0% is "Needs Improvement"
        CHECK(t.find("Grade:           Needs Improvement") != std::string::npos);
    }

    // ---- printSummary: 100% case (perfect score) ----
    {
        std::ostringstream out;
        printSummary(summarize(5, 0, 0), out);
        const std::string t = out.str();
        CHECK(t.find("Score:           5 / 5")   != std::string::npos);
        CHECK(t.find("100.0%")                    != std::string::npos);
        CHECK(t.find("Grade:           Excellent") != std::string::npos);
    }

    // ---- printSummary: all-skipped case ----
    {
        std::ostringstream out;
        printSummary(summarize(0, 0, 4), out);
        const std::string t = out.str();
        CHECK(t.find("Total questions: 4")  != std::string::npos);
        CHECK(t.find("Skipped:         4")  != std::string::npos);
        CHECK(t.find("Score:           0 / 4") != std::string::npos);
        CHECK(t.find("0.0%")                != std::string::npos);
    }

    // ---- printSummary: stream formatting state is restored (gap 5) ----
    {
        std::ostringstream formatted;
        formatted << std::scientific << std::setprecision(4);

        const auto originalFlags     = formatted.flags();
        const auto originalPrecision = formatted.precision();

        printSummary(summarize(7, 2, 1), formatted);

        CHECK(formatted.flags()     == originalFlags);
        CHECK(formatted.precision() == originalPrecision);

        // Also confirm the stream still formats in scientific notation after the call.
        formatted << 1.5;
        CHECK(formatted.str().find("1.5000e") != std::string::npos);
    }

    if (failures == 0) std::cout << "All ScoringModule tests passed.\n";
    return failures == 0 ? 0 : 1;
}
