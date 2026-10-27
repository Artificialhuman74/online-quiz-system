// ScoringModule.cpp - see ScoringModule.h
#include "ScoringModule.h"
#include <iomanip>

namespace ScoringModule {

std::string gradeFor(double percentage) {
    if (percentage < 0.0) percentage = 0.0;
    if (percentage > 100.0) percentage = 100.0;
    if (percentage >= kExcellentMin) return "Excellent";
    if (percentage >= kGoodMin) return "Good";
    return "Needs Improvement";
}

Summary summarize(int correct, int incorrect, int skipped) {
    Summary s;
    s.correct = correct < 0 ? 0 : correct;
    s.incorrect = incorrect < 0 ? 0 : incorrect;
    s.skipped = skipped < 0 ? 0 : skipped;
    s.total = s.correct + s.incorrect + s.skipped;
    s.percentage = s.total == 0 ? 0.0 : 100.0 * s.correct / s.total;
    s.grade = gradeFor(s.percentage);
    return s;
}

void printSummary(const Summary& s, std::ostream& out) {
    out << "\n==== Results ====\n"
        << "Total questions: " << s.total << '\n'
        << "Correct:         " << s.correct << '\n'
        << "Incorrect:       " << s.incorrect << '\n'
        << "Skipped:         " << s.skipped << '\n'
        << "Score:           " << s.correct << " / " << s.total << '\n'
        << "Percentage:      " << std::fixed << std::setprecision(1) << s.percentage << "%\n"
        << "Grade:           " << s.grade << '\n';
    out.unsetf(std::ios::floatfield);
}

}  // namespace ScoringModule
