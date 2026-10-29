// Self-test. From the repo root:
//   g++ -std=c++17 -Isrc -o t tests/unit/test_fileio.cpp src/FileIOModule.cpp && ./t   (t on Windows)
#include "FileIOModule.h"
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
using namespace FileIOModule;

static int failures = 0;
#define CHECK(cond)                                                              \
    do {                                                                         \
        if (!(cond)) { std::cerr << __FILE__ << ':' << __LINE__ << ": FAILED: " #cond "\n"; failures++; } \
    } while (0)

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
    CHECK(loadQuestions("data/questions.txt", qs, w, err) && qs.size() >= 9 && w.empty());
    CHECK(!loadQuestions("data/nope.txt", qs, w, err) && !err.empty());              // F-040 missing
    err = "stale"; w.push_back("stale");                                                // stale state cleared
    CHECK(loadQuestions("data/questions.txt", qs, w, err) && err.empty() && w.empty());

    { std::ofstream o("bad.txt");                                                       // SR-004 malformed
      o << "1|Math|Easy|2+2?|3|4|5|6|B\nGARBAGE\n2|Math|Easy|x?|a|b|c|d|Z\n1|Math|Easy|dup?|a|b|c|d|A\n"; }
    CHECK(loadQuestions("bad.txt", qs, w, err) && qs.size() == 1 && w.size() == 3);
    std::remove("bad.txt");

    // ---- validateQuestion: empty fields, '|', '\r', '\n' ----
    CHECK(validateQuestion(good(1)).empty());
    { Question q = good(1); q.options[2] = "";                    CHECK(!validateQuestion(q).empty()); }
    { Question q = good(1); q.correct = 'E';                      CHECK(!validateQuestion(q).empty()); }
    { Question q = good(1); q.difficulty = "Insane";              CHECK(!validateQuestion(q).empty()); }
    for (char bad : {'|', '\r', '\n'}) {
        Question q;
        q = good(1); q.text += bad;        CHECK(!validateQuestion(q).empty());
        q = good(1); q.category += bad;    CHECK(!validateQuestion(q).empty());
        q = good(1); q.options[3] += bad;  CHECK(!validateQuestion(q).empty());
    }

    // ---- saveQuestions: happy path + overwrite ----
    std::remove("t_q.txt");
    std::vector<Question> bank = {good(1), good(2)};
    CHECK(saveQuestions("t_q.txt", bank, err) && err.empty());
    bank[0].text = "Edited?";
    CHECK(saveQuestions("t_q.txt", bank, err));                                        // replaces existing file
    std::vector<Question> back;
    CHECK(loadQuestions("t_q.txt", back, w, err) && back.size() == 2 && back[0].text == "Edited?");
    CHECK(!exists("t_q.txt.tmp") && !exists("t_q.txt.bak"));

    // ---- saveQuestions: invalid banks are rejected and the file is untouched ----
    const std::string before = slurp("t_q.txt");
    { std::vector<Question> empty;                                  CHECK(!saveQuestions("t_q.txt", empty, err) && !err.empty()); }
    { std::vector<Question> v = {good(1), good(1)};                 CHECK(!saveQuestions("t_q.txt", v, err) && err.find("Duplicate") != std::string::npos); }
    { std::vector<Question> v = {good(1), good(2)}; v[1].options[0] = "";
                                                                    CHECK(!saveQuestions("t_q.txt", v, err) && !err.empty()); }
    { std::vector<Question> v = {good(1)}; v[0].text = "a\nb";      CHECK(!saveQuestions("t_q.txt", v, err) && !err.empty()); }
    { std::vector<Question> v = {good(0)};                          CHECK(!saveQuestions("t_q.txt", v, err)); }
    CHECK(slurp("t_q.txt") == before);                                                 // original intact
    CHECK(!exists("t_q.txt.tmp"));                                                     // no temp created

    // ---- saveQuestions: failed replacement keeps the original ----
    testing::replaceHook = failingReplace;
    bank[0].text = "Should not be saved";
    CHECK(!saveQuestions("t_q.txt", bank, err) && err.find("unchanged") != std::string::npos);
    testing::replaceHook = nullptr;
    CHECK(slurp("t_q.txt") == before);                                                 // original intact
    CHECK(!exists("t_q.txt.tmp") && !exists("t_q.txt.bak"));                           // cleaned up
    CHECK(loadQuestions("t_q.txt", back, w, err) && back[0].text == "Edited?");

    // unwritable temp location is reported
    CHECK(!saveQuestions("no_such_dir/q.txt", bank, err) && !err.empty());
    std::remove("t_q.txt");

    // ---- results: validation before append ----
    std::remove("t_r.txt");
    std::vector<Result> rs;
    CHECK(loadResults("t_r.txt", rs, w, err) && rs.empty() && err.empty());            // missing file = empty
    const char* ts = "2026-10-23 09:30:00";
    CHECK(saveResult("t_r.txt", {"Tanvi|x", "Math", 7, 10, ts}, err));                 // sanitized name
    CHECK(saveResult("t_r.txt", {"Abdul", "Science", 0, 5, ""}, err));                // auto timestamp
    const std::string rbefore = slurp("t_r.txt");
    CHECK(!saveResult("t_r.txt", {"A", "Math", -1, 10, ts}, err) && !err.empty());     // negative score
    CHECK(!saveResult("t_r.txt", {"A", "Math", 11, 10, ts}, err));                     // score > total
    CHECK(!saveResult("t_r.txt", {"A", "Math", 0, 0, ts}, err));                       // total 0
    CHECK(!saveResult("t_r.txt", {"A", "Math", 1, 2, "garbage"}, err));                // bad timestamp
    CHECK(!saveResult("t_r.txt", {"A", "Math", 1, 2, "2026-13-01 00:00:00"}, err));    // bad month
    CHECK(!saveResult("t_r.txt", {"A", "Math", 1, 2, "2026-10-23"}, err));             // missing time
    CHECK(!saveResult("t_r.txt", {"A", "Math", 1, 2, "2026-10-23 09:30:00\n"}, err));  // newline
    CHECK(!saveResult("t_r.txt", {"|", "Math", 1, 2, ts}, err));                       // empty after sanitize
    CHECK(!saveResult("t_r.txt", {"A", "", 1, 2, ts}, err));                           // empty category
    CHECK(slurp("t_r.txt") == rbefore);                                                // nothing appended
    CHECK(loadResults("t_r.txt", rs, w, err) && rs.size() == 2 && rs[0].player == "Tanvix" && w.empty());

    // ---- results: impossible calendar dates are rejected ----
    auto stamp = [](const char* s) { return validateResult({"A", "Math", 1, 2, s}).empty(); };
    CHECK(stamp("2026-12-31 23:59:59") && stamp("2024-02-29 00:00:00") && stamp("2000-02-29 12:00:00"));
    CHECK(!stamp("2026-02-31 10:00:00") && !stamp("2026-02-30 10:00:00") && !stamp("2025-02-29 10:00:00"));
    CHECK(!stamp("1900-02-29 10:00:00") && !stamp("2026-04-31 10:00:00") && !stamp("2026-06-31 10:00:00"));
    CHECK(!stamp("2026-09-31 10:00:00") && !stamp("2026-11-31 10:00:00") && !stamp("2026-10-00 10:00:00"));
    CHECK(!stamp("2026-00-10 10:00:00") && !stamp("2026-10-10 24:00:00") && !stamp("2026-10-10 10:60:00"));
    CHECK(!saveResult("t_r.txt", {"A", "Math", 1, 2, "2026-02-31 10:00:00"}, err));   // not written
    CHECK(slurp("t_r.txt") == rbefore);

    // ---- results: bad lines skipped; stale state cleared ----
    { std::ofstream o("t_r.txt", std::ios::app); o << "X|Math|9|5|2026-10-23 09:30:00\nY|Math|1|2|nope\njunk\n"; }
    w.push_back("stale"); err = "stale";
    CHECK(loadResults("t_r.txt", rs, w, err) && rs.size() == 2 && w.size() == 3 && err.empty());
    std::remove("t_r.txt");

    // ---- results: exists-but-unreadable is an error ----
    std::filesystem::create_directory("t_dir");
    CHECK(!loadResults("t_dir", rs, w, err) && !err.empty() && rs.empty());
    std::filesystem::remove("t_dir");

    if (failures) { std::cerr << failures << " FileIOModule check(s) failed\n"; return 1; }
    std::cout << "All FileIOModule tests passed\n";
    return 0;
}
