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

    // ---- loadQuestions: file with only blank lines / comments = no valid questions (gap 7) ----
    { std::ofstream o("blank.txt"); o << "\n# just a comment\n\n   \n"; }
    CHECK(!loadQuestions("blank.txt", qs, w, err) && !err.empty());
    std::remove("blank.txt");

    // ---- loadQuestions: every line has wrong field count → all warned, file fails (gap 8) ----
    { std::ofstream o("wrongfields.txt");
      o << "1|Math|Easy|2+2?\n"           // 4 fields (too few)
        << "2|Math|Easy|x?|a|b|c\n"; }    // 7 fields (too few)
    CHECK(!loadQuestions("wrongfields.txt", qs, w, err) && w.size() == 2 && !err.empty());
    std::remove("wrongfields.txt");

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

    // ---- validateQuestion: extra field-level gaps (gaps 2–4) ----
    { Question q = good(1); q.id = 0;      CHECK(!validateQuestion(q).empty()); }   // id == 0  (gap 2)
    { Question q = good(1); q.id = -5;     CHECK(!validateQuestion(q).empty()); }   // id negative
    { Question q = good(1); q.text = "";   CHECK(!validateQuestion(q).empty()); }   // empty text (gap 3)
    { Question q = good(1); q.category = ""; CHECK(!validateQuestion(q).empty()); } // empty category (gap 4)

    // ---- validateQuestion: all three valid difficulties ----
    { Question q = good(1); q.difficulty = "Medium"; CHECK(validateQuestion(q).empty()); }
    { Question q = good(1); q.difficulty = "Hard";   CHECK(validateQuestion(q).empty()); }

    // ---- validateResult: score == total is valid (gap 5) ----
    CHECK(validateResult({"Ali", "Math", 5, 5, "2026-10-23 09:30:00"}).empty());   // score == total
    CHECK(validateResult({"Ali", "Math", 0, 1, "2026-10-23 09:30:00"}).empty());   // score == 0 is valid

    // ---- validateResult: pipe / newline in player or category (gap 6) ----
    CHECK(!validateResult({"Ali\rX", "Math", 1, 2, "2026-10-23 09:30:00"}).empty());  // \r in player
    CHECK(!validateResult({"Ali\nX", "Math", 1, 2, "2026-10-23 09:30:00"}).empty());  // \n in player
    CHECK(!validateResult({"Ali",  "Ma|th", 1, 2, "2026-10-23 09:30:00"}).empty());   // | in category

    // ---- sanitize: direct tests (gap 1) ----
    CHECK(sanitize("hello")       == "hello");          // unchanged
    CHECK(sanitize("a|b")         == "ab");             // | stripped
    CHECK(sanitize("a\nb")        == "ab");             // \n stripped
    CHECK(sanitize("a\rb")        == "ab");             // \r stripped
    CHECK(sanitize("|\n\r")       == "");               // all bad → empty after trim
    CHECK(sanitize("  hello  ")   == "hello");          // leading/trailing whitespace trimmed
    CHECK(sanitize("a|b\nc|d\r") == "abcd");            // multiple bad chars

    // ---- saveQuestions: happy path + overwrite ----
    std::remove("t_q.txt");
    std::vector<Question> bank = {good(1), good(2)};
    CHECK(saveQuestions("t_q.txt", bank, err) && err.empty());
    bank[0].text = "Edited?";
    CHECK(saveQuestions("t_q.txt", bank, err));                                        // replaces existing file
    std::vector<Question> back;
    CHECK(loadQuestions("t_q.txt", back, w, err) && back.size() == 2 && back[0].text == "Edited?");
    CHECK(!exists("t_q.txt.tmp") && !exists("t_q.txt.bak"));

    // ---- saveQuestions: round-trip fidelity with Medium difficulty (gap 9) ----
    // Uses a separate temp file so it does not disturb the state of t_q.txt.
    {
        std::vector<Question> med = {good(10)};
        med[0].difficulty = "Medium";
        CHECK(saveQuestions("t_med.txt", med, err));
        std::vector<Question> reloaded;
        CHECK(loadQuestions("t_med.txt", reloaded, w, err) && reloaded.size() == 1
              && reloaded[0].difficulty == "Medium");
        std::remove("t_med.txt");
    }

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

    // ---- saveResult: score == total is valid (gap 5) ----
    CHECK(saveResult("t_r.txt", {"Ali", "Science", 5, 5, ts}, err));
    CHECK(slurp("t_r.txt") != rbefore);  // a new line was actually appended

    // ---- saveResult to an unwritable location (gap 10) ----
    CHECK(!saveResult("no_such_dir/r.txt", {"A", "Math", 1, 2, ts}, err) && !err.empty());

    std::remove("t_r.txt");

    // ---- results: impossible calendar dates are rejected ----
    auto stamp = [](const char* s) { return validateResult({"A", "Math", 1, 2, s}).empty(); };
    CHECK(stamp("2026-12-31 23:59:59") && stamp("2024-02-29 00:00:00") && stamp("2000-02-29 12:00:00"));
    CHECK(!stamp("2026-02-31 10:00:00") && !stamp("2026-02-30 10:00:00") && !stamp("2025-02-29 10:00:00"));
    CHECK(!stamp("1900-02-29 10:00:00") && !stamp("2026-04-31 10:00:00") && !stamp("2026-06-31 10:00:00"));
    CHECK(!stamp("2026-09-31 10:00:00") && !stamp("2026-11-31 10:00:00") && !stamp("2026-10-00 10:00:00"));
    CHECK(!stamp("2026-00-10 10:00:00") && !stamp("2026-10-10 24:00:00") && !stamp("2026-10-10 10:60:00"));
    // confirm nothing was written for the invalid date above
    { std::string e2; CHECK(!saveResult("t_r.txt", {"A", "Math", 1, 2, "2026-02-31 10:00:00"}, e2)); }
    CHECK(!exists("t_r.txt"));  // file was never created

    // ---- results: bad lines skipped; stale state cleared ----
    // Seed t_r.txt with 2 valid records first, then append 3 bad lines.
    CHECK(saveResult("t_r.txt", {"Priya", "Math", 3, 5, ts}, err));
    CHECK(saveResult("t_r.txt", {"Rahul", "Science", 2, 5, ts}, err));
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
