#include "AdminModule.h"
#include "FileIOModule.h"
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { std::cerr << __FILE__ << ':' << __LINE__ << ": FAILED: " #cond "\n"; failures++; } } while (0)

static std::string runAdmin(const std::string& questions, const std::string& password,
                           const std::string& input) {
    std::istringstream in(input);
    std::ostringstream out;
    auto* oldIn = std::cin.rdbuf(in.rdbuf());
    auto* oldOut = std::cout.rdbuf(out.rdbuf());
    try { AdminModule::run(questions, password); }
    catch (const std::exception& e) {
        std::cin.rdbuf(oldIn); std::cout.rdbuf(oldOut);
        std::cerr << "Admin interaction ended unexpectedly: " << e.what() << "\n" << out.str();
        throw;
    }
    std::cin.rdbuf(oldIn);
    std::cout.rdbuf(oldOut);
    return out.str();
}

int main() {
    const std::string questionsPath = "t_admin_questions.txt";
    const std::string passwordPath = "t_admin_password.cfg";
    std::remove(questionsPath.c_str());
    std::remove(passwordPath.c_str());

    Question seed;
    seed.id = 1; seed.category = "Math"; seed.difficulty = "Easy"; seed.text = "2+2?";
    seed.options[0] = "3"; seed.options[1] = "4"; seed.options[2] = "5"; seed.options[3] = "6";
    seed.correct = 'B';
    std::string error;
    CHECK(FileIOModule::saveQuestions(questionsPath, {seed}, error));

    // First use creates a password, and the authenticated admin can add a question.
    const std::string addOutput = runAdmin(questionsPath, passwordPath,
        "demo123\n2\nScience\n1\nWhat is H2O?\nHydrogen\nWater\nOxygen\nSalt\nB\n5\n");
    CHECK(addOutput.find("Admin password created.") != std::string::npos);
    CHECK(addOutput.find("Question bank saved.") != std::string::npos);
    std::vector<Question> bank;
    std::vector<std::string> warnings;
    CHECK(FileIOModule::loadQuestions(questionsPath, bank, warnings, error));
    CHECK(bank.size() == 2 && bank[1].id == 2 && bank[1].text == "What is H2O?");

    // Blank edit fields retain their values; an explicitly selected difficulty is applied.
    const std::string editOutput = runAdmin(questionsPath, passwordPath,
        "demo123\n3\n2\n\n2\n\n\n\n\n\nC\n5\n");
    CHECK(editOutput.find("Question bank saved.") != std::string::npos);
    CHECK(FileIOModule::loadQuestions(questionsPath, bank, warnings, error));
    CHECK(bank.size() == 2 && bank[1].category == "Science" && bank[1].difficulty == "Medium");
    CHECK(bank[1].text == "What is H2O?" && bank[1].correct == 'C');

    // Deletion persists, and the resulting question bank remains valid.
    const std::string deleteOutput = runAdmin(questionsPath, passwordPath,
        "demo123\n4\n2\ny\n5\n");
    CHECK(deleteOutput.find("Question bank saved.") != std::string::npos);
    CHECK(FileIOModule::loadQuestions(questionsPath, bank, warnings, error));
    CHECK(bank.size() == 1 && bank[0].id == 1);

    // Three consecutive incorrect passwords lock the panel until process restart.
    for (int i = 0; i < 3; ++i)
        CHECK(runAdmin(questionsPath, passwordPath, "wrong\n").find("Incorrect password") != std::string::npos);
    const std::string locked = runAdmin(questionsPath, passwordPath, "");
    CHECK(locked.find("locked until the program is restarted") != std::string::npos);
    CHECK(locked.find("Admin password:") == std::string::npos);

    std::remove(questionsPath.c_str());
    std::remove(passwordPath.c_str());
    if (failures) std::cerr << failures << " admin check(s) failed\n";
    return failures ? 1 : 0;
}
