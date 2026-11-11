// QuizEngine.h - question selection, shuffle, answer check, skip, game loop
// (QZ-F-004, QZ-F-010 .. QZ-F-012, QZ-F-014, QZ-F-015)
#pragma once
#include "FileIOModule.h"
#include "MenuModule.h"
#include <cstddef>
#include <iostream>
#include <random>
#include <string>
#include <vector>

namespace QuizEngine {

constexpr std::size_t kDefaultQuestionCount = 10;  // QZ-F-004

enum class Outcome { Correct, Incorrect, Skipped };

// Raw counts for one finished quiz. correct + incorrect + skipped == total.
struct QuizRun {
    int total = 0;
    int correct = 0;
    int incorrect = 0;
    int skipped = 0;
};

// Keeps questions matching category/difficulty (empty string = do not filter on that field).
std::vector<Question> filterQuestions(const std::vector<Question>& bank,
                                      const std::string& category,
                                      const std::string& difficulty);

// Filters, shuffles, and returns up to `count` DIFFERENT questions (QZ-F-004).
// If fewer match than `count`, all matching questions are returned.
std::vector<Question> selectQuestions(const std::vector<Question>& bank,
                                      const std::string& category,
                                      const std::string& difficulty,
                                      std::size_t count, std::mt19937& rng);

// Returns a copy with the four options in random order. `correct` is updated so that it
// still points at the same option text (QZ-F-011).
Question shuffleOptions(const Question& q, std::mt19937& rng);

// answer is 'A'..'D' or InputValidation::kSkip.
Outcome checkAnswer(const Question& q, char answer);

// Plays one whole quiz. Answers are read with InputValidation::readAnswer(prompt, true),
// so invalid entries only re-prompt and never affect the score (QZ-F-012).
// Throws InputValidation::InputClosed if the input ends mid-quiz.
// With showFeedback, prints Correct/Incorrect after each answer (QZ-F-015).
QuizRun runQuiz(const std::vector<Question>& bank, const MenuModule::QuizSetup& setup,
                std::size_t count, bool showFeedback,
                std::istream& in, std::ostream& out, std::mt19937& rng);

// Same, with a randomly seeded generator and the default question count, using std::cin/cout.
QuizRun runQuiz(const std::vector<Question>& bank, const MenuModule::QuizSetup& setup);

}  // namespace QuizEngine
