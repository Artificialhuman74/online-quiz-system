// MenuModule.h - main menu, results sub-menu, category and difficulty selection
// (QZ-F-001, QZ-F-002, QZ-F-003, QZ-F-024/025 entry point, QZ-NF-003)
//
// The menu only collects choices and returns them; main() decides what to run.
// Input goes through InputValidation, so invalid entries re-prompt (QZ-F-042) and
// end of input throws InputValidation::InputClosed.
#pragma once
#include <iostream>
#include <string>
#include <vector>

namespace MenuModule {

enum class MainChoice { StartQuiz = 1, Results = 2, Admin = 3, Exit = 4 };

enum class ResultsChoice { Leaderboard = 1, MyHistory = 2, Back = 3 };

// Returned by selectCategory/selectDifficulty when the player picks "All"/"Any".
// An empty string means "do not filter on this field".
inline const std::string kAll;

// What the player chose before a quiz starts.
struct QuizSetup {
    std::string player;
    std::string category;    // one of the offered categories, or kAll
    std::string difficulty;  // "Easy" | "Medium" | "Hard", or kAll
};

// Prints a boxed title line, e.g. "==== Main Menu ====".
void printHeader(const std::string& title, std::ostream& out = std::cout);

// QZ-F-001: Start Quiz, Leaderboard/History, Admin Panel, Exit.
MainChoice showMainMenu(std::istream& in = std::cin, std::ostream& out = std::cout);

// Leaderboard (top scores), My History, Back.
ResultsChoice showResultsMenu(std::istream& in = std::cin, std::ostream& out = std::cout);

// QZ-F-002: lists the given categories (duplicates and blanks removed, original order kept)
// plus "All categories". Returns the chosen name or kAll. Returns kAll without prompting
// if there are no categories.
std::string selectCategory(const std::vector<std::string>& categories,
                           std::istream& in = std::cin, std::ostream& out = std::cout);

// QZ-F-003: Easy, Medium, Hard, or Any. Returns the name or kAll.
std::string selectDifficulty(std::istream& in = std::cin, std::ostream& out = std::cout);

// Asks for the player's name, then category and difficulty.
QuizSetup setupQuiz(const std::vector<std::string>& categories,
                    std::istream& in = std::cin, std::ostream& out = std::cout);

}  // namespace MenuModule
