// Self-test. From the repo root:
//   g++ -std=c++17 -o t src/test_fileio.cpp src/FileIOModule.cpp && ./t      (t on Windows)
#include "FileIOModule.h"
#include <cassert>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
using namespace FileIOModule;

static std::string slurp(const std::string& p) {
    std::ifstream in(p, std::ios::binary);
    std::stringstream ss; ss << in.rdbuf();
    return ss.str();
}
static bool exists(const std::string& p) { return std::filesystem::exists(p); }
static bool failingReplace(const std::string&, const std::string&) { return false; }

static Question good(int id) {
    Question q; q.id = id; q.category = "Math"; q.difficulty = "Easy"; q.text = "2+2?";
    q.options[0] = "3"; q.options[1] = "4"; q.options[2] = "5"; q.options[3] = "6"; q.correct = 'B';
    return q;
}

int main() {
    std::vector<Question> qs; std::vector<std::string> w; std::string err;

    // ---- loading ----
    assert(loadQuestions("data/questions.txt", qs, w, err) && qs.size() == 9 && w.empty());
    assert(!loadQuestions("data/nope.txt", qs, w, err) && !err.empty());              // F-040 missing
    err = "stale"; w.push_back("stale");                                                // stale state cleared
    assert(loadQuestions("data/questions.txt", qs, w, err) && err.empty() && w.empty());

    { std::ofstream o("bad.txt");                                                       // SR-004 malformed
      o << "1|Math|Easy|2+2?|3|4|5|6|B\nGARBAGE\n2|Math|Easy|x?|a|b|c|d|Z\n1|Math|Easy|dup?|a|b|c|d|A\n"; }
    assert(loadQuestions("bad.txt", qs, w, err) && qs.size() == 1 && w.size() == 3);
    std::remove("bad.txt");

    // ---- validateQuestion: empty fields, '|', '\r', '\n' ----
    assert(validateQuestion(good(1)).empty());
    { Question q = good(1); q.options[2] = "";                    assert(!validateQuestion(q).empty()); }
    { Question q = good(1); q.correct = 'E';                      assert(!validateQuestion(q).empty()); }
    { Question q = good(1); q.difficulty = "Insane";              assert(!validateQuestion(q).empty()); }
    for (char bad : {'|', '\r', '\n'}) {
        Question q;
        q = good(1); q.text += bad;        assert(!validateQuestion(q).empty());
        q = good(1); q.category += bad;    assert(!validateQuestion(q).empty());
        q = good(1); q.options[3] += bad;  assert(!validateQuestion(q).empty());
    }

    // ---- saveQuestions: happy path + overwrite ----
    std::remove("t_q.txt");
    std::vector<Question> bank = {good(1), good(2)};
    assert(saveQuestions("t_q.txt", bank, err) && err.empty());
    bank[0].text = "Edited?";
    assert(saveQuestions("t_q.txt", bank, err));                                        // replaces existing file
    std::vector<Question> back;
    assert(loadQuestions("t_q.txt", back, w, err) && back.size() == 2 && back[0].text == "Edited?");
    assert(!exists("t_q.txt.tmp") && !exists("t_q.txt.bak"));

    // ---- saveQuestions: invalid banks are rejected and the file is untouched ----
    const std::string before = slurp("t_q.txt");
    { std::vector<Question> empty;                                  assert(!saveQuestions("t_q.txt", empty, err) && !err.empty()); }
    { std::vector<Question> v = {good(1), good(1)};                 assert(!saveQuestions("t_q.txt", v, err) && err.find("Duplicate") != std::string::npos); }
    { std::vector<Question> v = {good(1), good(2)}; v[1].options[0] = "";
                                                                    assert(!saveQuestions("t_q.txt", v, err) && !err.empty()); }
    { std::vector<Question> v = {good(1)}; v[0].text = "a\nb";      assert(!saveQuestions("t_q.txt", v, err) && !err.empty()); }
    { std::vector<Question> v = {good(0)};                          assert(!saveQuestions("t_q.txt", v, err)); }
    assert(slurp("t_q.txt") == before);                                                 // original intact
    assert(!exists("t_q.txt.tmp"));                                                     // no temp created

    // ---- saveQuestions: failed replacement keeps the original ----
    testing::replaceHook = failingReplace;
    bank[0].text = "Should not be saved";
    assert(!saveQuestions("t_q.txt", bank, err) && err.find("unchanged") != std::string::npos);
    testing::replaceHook = nullptr;
    assert(slurp("t_q.txt") == before);                                                 // original intact
    assert(!exists("t_q.txt.tmp") && !exists("t_q.txt.bak"));                           // cleaned up
    assert(loadQuestions("t_q.txt", back, w, err) && back[0].text == "Edited?");

    // unwritable temp location is reported
    assert(!saveQuestions("no_such_dir/q.txt", bank, err) && !err.empty());
    std::remove("t_q.txt");

    // ---- results: validation before append ----
    std::remove("t_r.txt");
    std::vector<Result> rs;
    assert(loadResults("t_r.txt", rs, w, err) && rs.empty() && err.empty());            // missing file = empty
    const char* ts = "2026-10-23 09:30:00";
    assert(saveResult("t_r.txt", {"Tanvi|x", "Math", 7, 10, ts}, err));                 // sanitized name
    assert(saveResult("t_r.txt", {"Abdul", "Science", 0, 5, ""}, err));                // auto timestamp
    const std::string rbefore = slurp("t_r.txt");
    assert(!saveResult("t_r.txt", {"A", "Math", -1, 10, ts}, err) && !err.empty());     // negative score
    assert(!saveResult("t_r.txt", {"A", "Math", 11, 10, ts}, err));                     // score > total
    assert(!saveResult("t_r.txt", {"A", "Math", 0, 0, ts}, err));                       // total 0
    assert(!saveResult("t_r.txt", {"A", "Math", 1, 2, "garbage"}, err));                // bad timestamp
    assert(!saveResult("t_r.txt", {"A", "Math", 1, 2, "2026-13-01 00:00:00"}, err));    // bad month
    assert(!saveResult("t_r.txt", {"A", "Math", 1, 2, "2026-10-23"}, err));             // missing time
    assert(!saveResult("t_r.txt", {"A", "Math", 1, 2, "2026-10-23 09:30:00\n"}, err));  // newline
    assert(!saveResult("t_r.txt", {"|", "Math", 1, 2, ts}, err));                       // empty after sanitize
    assert(!saveResult("t_r.txt", {"A", "", 1, 2, ts}, err));                           // empty category
    assert(slurp("t_r.txt") == rbefore);                                                // nothing appended
    assert(loadResults("t_r.txt", rs, w, err) && rs.size() == 2 && rs[0].player == "Tanvix" && w.empty());

    // ---- results: bad lines skipped; stale state cleared ----
    { std::ofstream o("t_r.txt", std::ios::app); o << "X|Math|9|5|2026-10-23 09:30:00\nY|Math|1|2|nope\njunk\n"; }
    w.push_back("stale"); err = "stale";
    assert(loadResults("t_r.txt", rs, w, err) && rs.size() == 2 && w.size() == 3 && err.empty());
    std::remove("t_r.txt");

    // ---- results: exists-but-unreadable is an error ----
    std::filesystem::create_directory("t_dir");
    assert(!loadResults("t_dir", rs, w, err) && !err.empty() && rs.empty());
    std::filesystem::remove("t_dir");

    std::cout << "All FileIOModule tests passed\n";
}
