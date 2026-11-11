// main.cpp - entry point for the Online Quiz System.
//
// Modules (QZ-NF-004), each in its own src/<Module>.h/.cpp pair:
//   InputValidation - console input checks and re-prompting (QZ-F-012, QZ-F-042)
//   MenuModule      - main menu, category and difficulty selection (QZ-F-001..003)
//   FileIOModule    - question bank and results persistence (QZ-F-023..025, QZ-F-040/041)
//   QuizEngine      - question selection, shuffle, answer check, skip (QZ-F-004, QZ-F-010..015)
//   ScoringModule   - score, results summary, grade (QZ-F-020..022)
//   AdminModule     - password-protected question management (QZ-F-030..034, QZ-SR-001/003/006)
//
// This file only wires the modules together; keep game logic out of it.
//
// This file wires the menu to the quiz, persistence, scoring and administration modules.
#include "InputValidation.h"
#include "FileIOModule.h"
#include "AdminModule.h"
#include "MenuModule.h"
#include "QuizEngine.h"
#include "ScoringModule.h"
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <random>
#include <string>
#include <vector>

int main() {
    const std::string questionsPath = "data/questions.txt";
    const std::string resultsPath = "data/results.txt";

    try {
        for (;;) {
            switch (MenuModule::showMainMenu()) {
            case MenuModule::MainChoice::StartQuiz: {
                std::vector<Question> bank;
                std::vector<std::string> warnings;
                std::string error;
                if (!FileIOModule::loadQuestions(questionsPath, bank, warnings, error)) {
                    std::cerr << error << '\n';
                    break;
                }
                for (const auto& warning : warnings) std::cerr << "Warning: " << warning << '\n';

                std::vector<std::string> categories;
                for (const auto& question : bank) {
                    if (std::find(categories.begin(), categories.end(), question.category) == categories.end())
                        categories.push_back(question.category);
                }
                MenuModule::QuizSetup s = MenuModule::setupQuiz(categories);
                std::cout << "\nPlayer: " << s.player
                          << " | Category: " << (s.category.empty() ? "All" : s.category)
                          << " | Difficulty: " << (s.difficulty.empty() ? "Any" : s.difficulty) << '\n';
                std::random_device rd;
                std::mt19937 rng(rd());
                const QuizEngine::QuizRun run = QuizEngine::runQuiz(
                    bank, s, QuizEngine::kDefaultQuestionCount, true, std::cin, std::cout, rng);
                if (run.total == 0) break;

                const ScoringModule::Summary summary =
                    ScoringModule::summarize(run.correct, run.incorrect, run.skipped);
                ScoringModule::printSummary(summary);

                Result result;
                result.player = s.player;
                result.category = s.category.empty() ? "All" : s.category;
                result.score = summary.correct;
                result.total = summary.total;
                if (FileIOModule::saveResult(resultsPath, result, error))
                    std::cout << "Result saved.\n";
                else
                    std::cerr << "Could not save result: " << error << '\n';
                break;
            }
            case MenuModule::MainChoice::Results:
                for (;;) {
                    const auto choice = MenuModule::showResultsMenu();
                    if (choice == MenuModule::ResultsChoice::Back) break;
                    std::vector<Result> results;
                    std::vector<std::string> warnings;
                    std::string error;
                    if (!FileIOModule::loadResults(resultsPath, results, warnings, error)) {
                        std::cerr << error << '\n';
                        continue;
                    }
                    for (const auto& warning : warnings) std::cerr << "Warning: " << warning << '\n';
                    if (choice == MenuModule::ResultsChoice::Leaderboard) {
                        std::sort(results.begin(), results.end(), [](const Result& a, const Result& b) {
                            const long long lhs = static_cast<long long>(a.score) * b.total;
                            const long long rhs = static_cast<long long>(b.score) * a.total;
                            return lhs != rhs ? lhs > rhs : a.timestamp > b.timestamp;
                        });
                        MenuModule::printHeader("Leaderboard");
                        if (results.empty()) std::cout << "No results yet.\n";
                        for (std::size_t i = 0; i < results.size() && i < 10; ++i) {
                            const auto& r = results[i];
                            std::cout << i + 1 << ". " << r.player << " | " << r.category << " | "
                                      << r.score << '/' << r.total << " (" << std::fixed
                                      << std::setprecision(1) << 100.0 * r.score / r.total << "%) | "
                                      << r.timestamp << '\n';
                        }
                    } else {
                        const std::string player = InputValidation::readText("Enter your name: ");
                        MenuModule::printHeader("Past Attempts: " + player);
                        bool found = false;
                        for (auto it = results.rbegin(); it != results.rend(); ++it) {
                            if (it->player != player) continue;
                            found = true;
                            std::cout << it->category << " | " << it->score << '/' << it->total
                                      << " (" << std::fixed << std::setprecision(1)
                                      << 100.0 * it->score / it->total << "%) | " << it->timestamp << '\n';
                        }
                        if (!found) std::cout << "No past attempts found for " << player << ".\n";
                    }
                }
                break;
            case MenuModule::MainChoice::Admin:
                AdminModule::run(questionsPath, "data/admin.cfg");
                break;
            case MenuModule::MainChoice::Exit:
                std::cout << "Goodbye!\n";
                return 0;
            }
        }
    } catch (const InputValidation::InputClosed&) {
        std::cout << "Input closed. Goodbye!\n";
    }
    return 0;
}
