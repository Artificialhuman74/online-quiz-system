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
#include "InputValidation.h"
#include "MenuModule.h"
#include <iostream>
#include <string>
#include <vector>

int main() {
    // TODO(feature/integration): take categories from the loaded question bank (QZ-NF-006).
    const std::vector<std::string> categories = {"General Knowledge", "Science", "Math"};

    try {
        for (;;) {
            switch (MenuModule::showMainMenu()) {
            case MenuModule::MainChoice::StartQuiz: {
                MenuModule::QuizSetup s = MenuModule::setupQuiz(categories);
                std::cout << "\nPlayer: " << s.player
                          << " | Category: " << (s.category.empty() ? "All" : s.category)
                          << " | Difficulty: " << (s.difficulty.empty() ? "Any" : s.difficulty) << '\n';
                // TODO(feature/integration): run QuizEngine, show ScoringModule summary, save result.
                std::cout << "The quiz engine is not connected yet.\n";
                break;
            }
            case MenuModule::MainChoice::Results:
                // TODO(feature/integration): show the leaderboard or this player's history.
                if (MenuModule::showResultsMenu() != MenuModule::ResultsChoice::Back)
                    std::cout << "Results history is not connected yet.\n";
                break;
            case MenuModule::MainChoice::Admin:
                // TODO(feature/integration): hand over to AdminModule (password check first).
                std::cout << "The admin panel is not connected yet.\n";
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
