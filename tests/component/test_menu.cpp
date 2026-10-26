// Component test for MenuModule + InputValidation. From the repo root:
//   g++ -std=c++17 -Isrc -o t tests/component/test_menu.cpp src/MenuModule.cpp src/InputValidation.cpp && ./t
#include "InputValidation.h"
#include "MenuModule.h"
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
using namespace MenuModule;

static int failures = 0;
#define CHECK(cond)                                                              \
    do {                                                                         \
        if (!(cond)) { std::cerr << __FILE__ << ':' << __LINE__ << ": FAILED: " #cond "\n"; failures++; } \
    } while (0)

static bool has(const std::string& hay, const std::string& needle) { return hay.find(needle) != std::string::npos; }

int main() {
    const std::vector<std::string> cats = {"General Knowledge", "Science", "Math"};

    // ---- QZ-F-001: all four main-menu options shown, each maps to the right choice ----
    {
        std::istringstream in("1\n");
        std::ostringstream out;
        CHECK(showMainMenu(in, out) == MainChoice::StartQuiz);
        for (const char* opt : {"Start Quiz", "Leaderboard", "Admin", "Exit"}) CHECK(has(out.str(), opt));
    }
    {
        std::istringstream in("2\n");  std::ostringstream out;  CHECK(showMainMenu(in, out) == MainChoice::Results);
    }
    {
        std::istringstream in("3\n");  std::ostringstream out;  CHECK(showMainMenu(in, out) == MainChoice::Admin);
    }
    {
        std::istringstream in("4\n");  std::ostringstream out;  CHECK(showMainMenu(in, out) == MainChoice::Exit);
    }

    // ---- QZ-F-042: invalid main-menu input re-prompts ----
    {
        std::istringstream in("0\n5\nhello\n\n4\n");
        std::ostringstream out;
        CHECK(showMainMenu(in, out) == MainChoice::Exit);
        CHECK(has(out.str(), "Invalid choice"));
    }

    // ---- results sub-menu ----
    {
        std::istringstream in("1\n");  std::ostringstream out;  CHECK(showResultsMenu(in, out) == ResultsChoice::Leaderboard);
    }
    {
        std::istringstream in("9\n3\n");  std::ostringstream out;  CHECK(showResultsMenu(in, out) == ResultsChoice::Back);
    }

    // ---- QZ-F-002: category selection ----
    {
        std::istringstream in("2\n");
        std::ostringstream out;
        CHECK(selectCategory(cats, in, out) == "Science");
        CHECK(has(out.str(), "1. General Knowledge") && has(out.str(), "3. Math") && has(out.str(), "4. All categories"));
    }
    {
        std::istringstream in("4\n");  std::ostringstream out;  CHECK(selectCategory(cats, in, out) == kAll);
    }
    {
        std::istringstream in("5\n1\n");  // 5 is out of range for 3 categories + All
        std::ostringstream out;
        CHECK(selectCategory(cats, in, out) == "General Knowledge");
        CHECK(has(out.str(), "Invalid choice"));
    }
    {
        // duplicates and blanks are dropped, order kept
        std::istringstream in("2\n");
        std::ostringstream out;
        CHECK(selectCategory({"Math", " ", "Science", "Math", " Science "}, in, out) == "Science");
        CHECK(has(out.str(), "3. All categories"));
    }
    {
        // no categories: nothing to ask, nothing read
        std::istringstream in("");
        std::ostringstream out;
        CHECK(selectCategory({}, in, out) == kAll);
    }

    // ---- QZ-F-003: difficulty selection ----
    {
        std::istringstream in("1\n");  std::ostringstream out;  CHECK(selectDifficulty(in, out) == "Easy");
    }
    {
        std::istringstream in("2\n");  std::ostringstream out;  CHECK(selectDifficulty(in, out) == "Medium");
    }
    {
        std::istringstream in("x\n3\n");  std::ostringstream out;  CHECK(selectDifficulty(in, out) == "Hard");
    }
    {
        std::istringstream in("4\n");  std::ostringstream out;  CHECK(selectDifficulty(in, out) == kAll);
    }

    // ---- full setup flow ----
    {
        std::istringstream in("\n  Ashvin \n3\n2\n");  // empty name re-prompts; name is trimmed
        std::ostringstream out;
        QuizSetup s = setupQuiz(cats, in, out);
        CHECK(s.player == "Ashvin");
        CHECK(s.category == "Math");
        CHECK(s.difficulty == "Medium");
    }

    // ---- end of input propagates as InputClosed ----
    {
        std::istringstream in("7\n");
        std::ostringstream out;
        bool threw = false;
        try { showMainMenu(in, out); } catch (const InputValidation::InputClosed&) { threw = true; }
        CHECK(threw);
    }

    if (failures) { std::cerr << failures << " MenuModule check(s) failed\n"; return 1; }
    std::cout << "All MenuModule tests passed\n";
    return 0;
}
