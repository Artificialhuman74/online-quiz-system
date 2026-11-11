// test_nfr.cpp - Automated verification for Non-Functional Requirements:
//   QZ-NF-001 (TC-Perf-01): Load question bank & select/render question within 2.0 seconds
//   QZ-NF-002 (TC-Rel-01): Atomic file operations prevent data loss/corruption on failure/unexpected termination
//   QZ-NF-003 (TC-UX-01): Clear, unambiguous prompts and error messages across all user interactions
//   QZ-NF-006 (TC-Maint-02): Question bank file format supports adding new categories without code changes
#include "FileIOModule.h"
#include "InputValidation.h"
#include "MenuModule.h"
#include "QuizEngine.h"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <vector>

static int failures = 0;
#define CHECK(cond)                                                              \
    do {                                                                         \
        if (!(cond)) {                                                           \
            std::cerr << __FILE__ << ':' << __LINE__ << ": FAILED: " #cond "\n"; \
            failures++;                                                          \
        }                                                                        \
    } while (0)

static std::string slurp(const std::string& p) {
    std::ifstream in(p, std::ios::binary);
    std::stringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

static bool failingReplace(const std::string&, const std::string&) { return false; }

static Question makeQ(int id, const std::string& cat, const std::string& diff) {
    Question q;
    q.id = id;
    q.category = cat;
    q.difficulty = diff;
    q.text = "Sample Question " + std::to_string(id) + "?";
    q.options[0] = "Alpha";
    q.options[1] = "Beta";
    q.options[2] = "Gamma";
    q.options[3] = "Delta";
    q.correct = 'B';
    return q;
}

int main() {
    std::cout << "Running Non-Functional Requirements (NFR) Test Suite...\n";

    // =========================================================================
    // QZ-NF-001 / TC-Perf-01: Performance Benchmark
    // The system shall load question bank and display first question within 2.0s
    // =========================================================================
    {
        std::cout << "  [QZ-NF-001 / TC-Perf-01] Testing load & first-question latency...\n";
        auto start = std::chrono::high_resolution_clock::now();

        std::vector<Question> qs;
        std::vector<std::string> warnings;
        std::string err;
        bool loaded = FileIOModule::loadQuestions("data/questions.txt", qs, warnings, err);
        CHECK(loaded && !qs.empty());

        // Simulate question selection and setup
        std::mt19937 rng(42);
        std::vector<Question> picked = QuizEngine::selectQuestions(qs, "", "", 10, rng);
        CHECK(!picked.empty());

        // Simulate rendering of the first question and its options
        std::ostringstream out;
        const Question first = QuizEngine::shuffleOptions(picked[0], rng);
        out << "Question 1 of " << picked.size() << "\n" << first.text << "\n";
        for (int k = 0; k < 4; k++) {
            out << "  " << static_cast<char>('A' + k) << ") " << first.options[k] << "\n";
        }
        std::string rendered = out.str();
        CHECK(!rendered.empty());
        CHECK(rendered.find("Question 1 of") != std::string::npos);

        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end - start;
        std::cout << "    Elapsed time: " << elapsed.count() << " seconds (budget: 2.0s)\n";
        CHECK(elapsed.count() < 2.0); // Within the 2-second SRS requirement
    }

    // =========================================================================
    // QZ-NF-002 / TC-Rel-01: Reliability & Crash/Data Loss Resistance
    // The system shall not lose previously saved quiz results or question data
    // if application terminates or file replacement fails mid-session.
    // =========================================================================
    {
        std::cout << "  [QZ-NF-002 / TC-Rel-01] Testing atomic replace & failure durability...\n";
        const std::string qFile = "t_nfr_q.txt";
        std::remove(qFile.c_str());
        std::string err;

        // 1. Establish initial consistent question bank
        std::vector<Question> initial = {makeQ(1, "Science", "Easy"), makeQ(2, "Math", "Hard")};
        CHECK(FileIOModule::saveQuestions(qFile, initial, err));
        const std::string originalData = slurp(qFile);
        CHECK(!originalData.empty());

        // 2. Simulate replacement failure (e.g. crash / power cut / disk lock / unprivileged rename)
        FileIOModule::testing::replaceHook = failingReplace;
        std::vector<Question> corrupted = {makeQ(1, "Science", "Easy")};
        corrupted[0].text = "This should never corrupt original file";
        bool saveOk = FileIOModule::saveQuestions(qFile, corrupted, err);
        CHECK(!saveOk);
        CHECK(err.find("unchanged") != std::string::npos);

        // Reset replacement hook
        FileIOModule::testing::replaceHook = nullptr;

        // 3. Confirm original data on disk remains byte-identical and intact
        CHECK(slurp(qFile) == originalData);

        // 4. Verify no lingering temp files left behind
        CHECK(!std::filesystem::exists(qFile + ".tmp"));
        CHECK(!std::filesystem::exists(qFile + ".bak"));

        // 5. Test results file append durability (interrupted or malformed entry does not corrupt prior history)
        const std::string rFile = "t_nfr_r.txt";
        std::remove(rFile.c_str());
        CHECK(FileIOModule::saveResult(rFile, {"Alice", "Math", 8, 10, "2026-11-10 10:00:00"}, err));
        const std::string validHistory = slurp(rFile);

        // Attempting to append invalid/corrupt result (e.g. impossible date, out-of-bounds score)
        CHECK(!FileIOModule::saveResult(rFile, {"Bob", "Math", 15, 10, "2026-11-10 10:00:00"}, err)); // score > total
        CHECK(slurp(rFile) == validHistory); // prior history untouched

        std::remove(qFile.c_str());
        std::remove(rFile.c_str());
    }

    // =========================================================================
    // QZ-NF-003 / TC-UX-01: Usability & Self-Explanatory Prompts
    // The system shall present clear, unambiguous prompts and error messages
    // so a user can navigate and complete actions without external instruction.
    // =========================================================================
    {
        std::cout << "  [QZ-NF-003 / TC-UX-01] Testing usability, clarity of prompts & re-prompts...\n";

        // Main menu guidance
        std::ostringstream menuOut;
        MenuModule::printHeader("Online Quiz System", menuOut);
        std::string menuHeader = menuOut.str();
        CHECK(menuHeader.find("Online Quiz System") != std::string::npos);

        // Test clear error messages for invalid choices
        {
            std::istringstream in("bad_choice\n2\n");
            std::ostringstream out;
            int choice = InputValidation::readMenuChoice("Select (1-4): ", 1, 4, in, out);
            CHECK(choice == 2);
            // Must contain explicit bounds in error message
            CHECK(out.str().find("Invalid choice. Please enter a number from 1 to 4.") != std::string::npos);
        }

        // Test clear error messages for invalid quiz answers
        {
            std::istringstream in("Z\nB\n");
            std::ostringstream out;
            char ans = InputValidation::readAnswer("Answer (A-D, or S to skip): ", true, in, out);
            CHECK(ans == 'B');
            CHECK(out.str().find("Invalid answer. Please enter A, B, C or D (or S to skip).") != std::string::npos);
        }

        // Test clear text validation instructions
        CHECK(InputValidation::checkText("") == "This cannot be empty.");
        CHECK(InputValidation::checkText("a|b") == "The '|' character is not allowed.");
        CHECK(InputValidation::checkText(std::string(InputValidation::kMaxTextLength + 1, 'x')) ==
              "Please use at most " + std::to_string(InputValidation::kMaxTextLength) + " characters.");
    }

    // =========================================================================
    // QZ-NF-006 / TC-Maint-02: Extensibility via Data Files
    // The question bank file format shall support adding new categories or
    // questions dynamically via data files without requiring code changes.
    // =========================================================================
    {
        std::cout << "  [QZ-NF-006 / TC-Maint-02] Testing dynamic category & question extensibility...\n";
        const std::string customDataFile = "t_extensible_bank.txt";
        std::remove(customDataFile.c_str());

        // Write a question file introducing completely new categories not hardcoded in the codebase
        {
            std::ofstream out(customDataFile);
            out << "# ID|Category|Difficulty|Question|OptA|OptB|OptC|OptD|CorrectLetter\n";
            out << "101|Astronomy|Easy|What is the closest star to Earth?|Proxima Centauri|Sun|Sirius|Alpha Centauri|B\n";
            out << "102|World History|Medium|In which year was the UN founded?|1942|1945|1948|1950|B\n";
            out << "103|Computer Science|Hard|What is the time complexity of binary search?|O(n)|O(n log n)|O(log n)|O(1)|C\n";
        }

        std::vector<Question> customQs;
        std::vector<std::string> warnings;
        std::string err;
        CHECK(FileIOModule::loadQuestions(customDataFile, customQs, warnings, err));
        CHECK(customQs.size() == 3);

        // Dynamically extract categories from the loaded data bank
        std::vector<std::string> discoveredCategories;
        for (const auto& q : customQs) {
            std::string cat = InputValidation::trim(q.category);
            if (!cat.empty() &&
                std::find(discoveredCategories.begin(), discoveredCategories.end(), cat) == discoveredCategories.end()) {
                discoveredCategories.push_back(cat);
            }
        }

        // Confirm new categories were discovered dynamically
        CHECK(discoveredCategories.size() == 3);
        CHECK(std::find(discoveredCategories.begin(), discoveredCategories.end(), "Astronomy") != discoveredCategories.end());
        CHECK(std::find(discoveredCategories.begin(), discoveredCategories.end(), "World History") != discoveredCategories.end());
        CHECK(std::find(discoveredCategories.begin(), discoveredCategories.end(), "Computer Science") != discoveredCategories.end());

        // Verify MenuModule::selectCategory handles dynamically discovered categories without code modification
        {
            std::istringstream in("1\n"); // Select Astronomy
            std::ostringstream out;
            std::string selected = MenuModule::selectCategory(discoveredCategories, in, out);
            CHECK(selected == "Astronomy");
            CHECK(out.str().find("1. Astronomy") != std::string::npos);
            CHECK(out.str().find("2. World History") != std::string::npos);
            CHECK(out.str().find("3. Computer Science") != std::string::npos);
            CHECK(out.str().find("4. All categories") != std::string::npos);
        }

        // Clean up
        std::remove(customDataFile.c_str());
    }

    if (failures) {
        std::cerr << failures << " Non-Functional check(s) failed\n";
        return 1;
    }
    std::cout << "All Non-Functional Requirements tests passed successfully!\n";
    return 0;
}
