// ScoringModule.h - score, results summary, grade (QZ-F-020, QZ-F-021, QZ-F-022)
#pragma once
#include <iostream>
#include <string>

namespace ScoringModule {

// Grade bands (QZ-F-022). Percentage is out of the total number of questions.
constexpr double kExcellentMin = 80.0;  // >= 80%  -> "Excellent"
constexpr double kGoodMin = 50.0;       // >= 50%  -> "Good", otherwise "Needs Improvement"

struct Summary {
    int total = 0;
    int correct = 0;     // this is the score (QZ-F-020)
    int incorrect = 0;
    int skipped = 0;
    double percentage = 0.0;  // 0..100, 0 when total == 0
    std::string grade;
};

// Grade/remark for a percentage (values outside 0..100 are clamped).
std::string gradeFor(double percentage);

// Builds the summary. Negative counts are treated as 0. total = correct + incorrect + skipped,
// so the numbers always reconcile (QZ-F-021).
Summary summarize(int correct, int incorrect, int skipped);

// Prints the results summary.
void printSummary(const Summary& s, std::ostream& out = std::cout);

}  // namespace ScoringModule
