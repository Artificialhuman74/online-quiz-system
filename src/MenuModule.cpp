#include "MenuModule.h"
#include "InputValidation.h"
#include <algorithm>

namespace MenuModule {

void printHeader(const std::string& title, std::ostream& out) {
    out << "\n==== " << title << " ====\n";
}

MainChoice showMainMenu(std::istream& in, std::ostream& out) {
    printHeader("Online Quiz System", out);
    out << "1. Start Quiz\n"
           "2. Leaderboard / History\n"
           "3. Admin Panel\n"
           "4. Exit\n";
    return static_cast<MainChoice>(InputValidation::readMenuChoice("Enter your choice (1-4): ", 1, 4, in, out));
}

ResultsChoice showResultsMenu(std::istream& in, std::ostream& out) {
    printHeader("Leaderboard / History", out);
    out << "1. Leaderboard (top scores)\n"
           "2. My past attempts\n"
           "3. Back to main menu\n";
    return static_cast<ResultsChoice>(InputValidation::readMenuChoice("Enter your choice (1-3): ", 1, 3, in, out));
}

std::string selectCategory(const std::vector<std::string>& categories, std::istream& in, std::ostream& out) {
    std::vector<std::string> list;
    for (const auto& c : categories) {
        std::string t = InputValidation::trim(c);
        if (!t.empty() && std::find(list.begin(), list.end(), t) == list.end()) list.push_back(t);
    }
    if (list.empty()) {
        out << "No categories found; questions from the whole bank will be used.\n";
        return kAll;
    }

    printHeader("Choose a Category", out);
    for (size_t i = 0; i < list.size(); i++) out << i + 1 << ". " << list[i] << '\n';
    const int all = static_cast<int>(list.size()) + 1;
    out << all << ". All categories\n";

    int choice = InputValidation::readMenuChoice(
        "Enter your choice (1-" + std::to_string(all) + "): ", 1, all, in, out);
    return choice == all ? kAll : list[choice - 1];
}

std::string selectDifficulty(std::istream& in, std::ostream& out) {
    static const char* const levels[] = {"Easy", "Medium", "Hard"};
    printHeader("Choose a Difficulty", out);
    out << "1. Easy\n"
           "2. Medium\n"
           "3. Hard\n"
           "4. Any difficulty\n";
    int choice = InputValidation::readMenuChoice("Enter your choice (1-4): ", 1, 4, in, out);
    return choice == 4 ? kAll : levels[choice - 1];
}

QuizSetup setupQuiz(const std::vector<std::string>& categories, std::istream& in, std::ostream& out) {
    QuizSetup s;
    s.player = InputValidation::readText("\nEnter your name: ", InputValidation::kMaxTextLength, in, out);
    s.category = selectCategory(categories, in, out);
    s.difficulty = selectDifficulty(in, out);
    return s;
}

}  // namespace MenuModule
