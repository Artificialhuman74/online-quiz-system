// Self-test for QuizEngine. Run through CTest (see tests/README.md).
#include "InputValidation.h"
#include "QuizEngine.h"
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <vector>
using namespace QuizEngine;

static int failures = 0;
#define CHECK(cond)                                                              \
    do {                                                                         \
        if (!(cond)) { std::cerr << __FILE__ << ':' << __LINE__ << ": FAILED: " #cond "\n"; failures++; } \
    } while (0)

static Question makeQ(int id, const std::string& cat, const std::string& diff, char correct = 'B') {
    Question q;
    q.id = id; q.category = cat; q.difficulty = diff;
    q.text = "Question " + std::to_string(id);
    q.options[0] = "opt-a"; q.options[1] = "opt-b"; q.options[2] = "opt-c"; q.options[3] = "opt-d";
    q.correct = correct;
    return q;
}

static std::vector<Question> makeBank() {
    std::vector<Question> bank;
    int id = 1;
    for (const char* c : {"Math", "Science"})
        for (const char* d : {"Easy", "Hard"})
            for (int i = 0; i < 6; i++) bank.push_back(makeQ(id++, c, d));
    return bank;  // 24 questions
}

int main() {
    std::mt19937 rng(12345);
    const std::vector<Question> bank = makeBank();

    // ---- filtering ----
    CHECK(filterQuestions(bank, "", "").size() == 24);
    CHECK(filterQuestions(bank, "Math", "").size() == 12);
    CHECK(filterQuestions(bank, "", "Hard").size() == 12);
    CHECK(filterQuestions(bank, "Science", "Easy").size() == 6);
    CHECK(filterQuestions(bank, "History", "").empty());
    for (const Question& q : filterQuestions(bank, "Math", "Hard"))
        CHECK(q.category == "Math" && q.difficulty == "Hard");

    // ---- selection: count, no duplicates, matches filter (QZ-F-004) ----
    for (int round = 0; round < 20; round++) {
        std::vector<Question> sel = selectQuestions(bank, "Math", "", 10, rng);
        CHECK(sel.size() == 10);
        std::set<int> ids;
        for (const Question& q : sel) { ids.insert(q.id); CHECK(q.category == "Math"); }
        CHECK(ids.size() == 10);
    }
    CHECK(selectQuestions(bank, "Science", "Easy", 10, rng).size() == 6);  // fewer than requested
    CHECK(selectQuestions(bank, "Nope", "", 10, rng).empty());
    CHECK(selectQuestions(bank, "", "", 0, rng).empty());

    // selection order varies between runs
    std::set<int> firstIds;
    for (int i = 0; i < 30; i++) firstIds.insert(selectQuestions(bank, "", "", 10, rng)[0].id);
    CHECK(firstIds.size() > 1);

    // ---- option shuffling keeps the right answer (QZ-F-011) ----
    Question orig;
    orig.id = 1; orig.category = "C"; orig.difficulty = "Easy"; orig.text = "t";
    orig.options[0] = "w"; orig.options[1] = "x"; orig.options[2] = "y"; orig.options[3] = "z";
    orig.correct = 'C';  // "y"
    std::set<std::string> orders;
    for (int i = 0; i < 200; i++) {
        Question s = shuffleOptions(orig, rng);
        std::multiset<std::string> a(s.options, s.options + 4), b(orig.options, orig.options + 4);
        CHECK(a == b);
        CHECK(s.correct >= 'A' && s.correct <= 'D');
        CHECK(s.options[s.correct - 'A'] == "y");
        orders.insert(s.options[0] + s.options[1] + s.options[2] + s.options[3]);
    }
    CHECK(orders.size() > 10);  // 24 possible orders; many should appear
    CHECK(orig.options[0] == "w" && orig.correct == 'C');  // original untouched

    // ---- answer checking ----
    Question q = makeQ(1, "Math", "Easy", 'B');
    CHECK(checkAnswer(q, 'B') == Outcome::Correct);
    CHECK(checkAnswer(q, 'A') == Outcome::Incorrect);
    CHECK(checkAnswer(q, 'D') == Outcome::Incorrect);
    CHECK(checkAnswer(q, InputValidation::kSkip) == Outcome::Skipped);

    // ---- full game loop with scripted input ----
    // 3 questions available (Math/Easy limited by count); answers: skip, invalid, invalid, then A..
    {
        std::vector<Question> small = {makeQ(1, "Math", "Easy"), makeQ(2, "Math", "Easy"),
                                       makeQ(3, "Math", "Easy")};
        MenuModule::QuizSetup setup{"Tester", "Math", "Easy"};
        std::istringstream in("s\nzzz\n7\n\nA\n a \n");  // skip, [3 invalid], A, A
        std::ostringstream out;
        std::mt19937 r(1);
        QuizRun run = runQuiz(small, setup, 10, true, in, out, r);
        CHECK(run.total == 3);
        CHECK(run.skipped == 1);
        CHECK(run.correct + run.incorrect == 2);
        CHECK(run.correct + run.incorrect + run.skipped == run.total);
        CHECK(out.str().find("Skipped.") != std::string::npos);
        CHECK(out.str().find("Question 3 of 3") != std::string::npos);
    }

    // all-skip run, no feedback text
    {
        std::vector<Question> small = {makeQ(1, "Math", "Easy"), makeQ(2, "Math", "Easy")};
        MenuModule::QuizSetup setup{"T", "", ""};
        std::istringstream in("S\ns\n");
        std::ostringstream out;
        std::mt19937 r(2);
        QuizRun run = runQuiz(small, setup, 10, false, in, out, r);
        CHECK(run.total == 2 && run.skipped == 2 && run.correct == 0 && run.incorrect == 0);
        CHECK(out.str().find("Skipped.") == std::string::npos);
    }

    // no matching questions: nothing asked, empty run
    {
        MenuModule::QuizSetup setup{"T", "History", ""};
        std::istringstream in("");
        std::ostringstream out;
        std::mt19937 r(3);
        QuizRun run = runQuiz(bank, setup, 10, true, in, out, r);
        CHECK(run.total == 0);
    }

    // input ends mid-quiz -> InputClosed
    {
        MenuModule::QuizSetup setup{"T", "", ""};
        std::istringstream in("A\n");
        std::ostringstream out;
        std::mt19937 r(4);
        bool threw = false;
        try { runQuiz(bank, setup, 5, true, in, out, r); }
        catch (const InputValidation::InputClosed&) { threw = true; }
        CHECK(threw);
    }

    if (failures == 0) std::cout << "All QuizEngine tests passed.\n";
    return failures == 0 ? 0 : 1;
}
