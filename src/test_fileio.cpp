// Quick self-test: g++ -std=c++17 -o t src/test_fileio.cpp src/FileIOModule.cpp && ./t
#include "FileIOModule.h"
#include <cassert>
#include <cstdio>
#include <fstream>
#include <iostream>
using namespace FileIOModule;

int main() {
    std::vector<Question> qs; std::vector<std::string> w; std::string err;
    assert(loadQuestions("data/questions.txt", qs, w, err) && qs.size() == 9 && w.empty());

    assert(!loadQuestions("data/nope.txt", qs, w, err) && !err.empty());           // F-040 missing
    err = "stale"; w.push_back("stale");                                             // stale state cleared
    assert(loadQuestions("data/questions.txt", qs, w, err) && err.empty() && w.empty());

    { std::ofstream o("bad.txt");                                                    // SR-004 malformed
      o << "1|Math|Easy|2+2?|3|4|5|6|B\nGARBAGE\n2|Math|Easy|x?|a|b|c|d|Z\n1|Math|Easy|dup?|a|b|c|d|A\n"; }
    w.clear();
    assert(loadQuestions("bad.txt", qs, w, err) && qs.size() == 1 && w.size() == 3);
    std::remove("bad.txt");

    Question q = qs[0]; q.options[2] = "";                                           // F-034
    assert(!validateQuestion(q).empty());

    qs[0].text = "Edited?";                                                          // F-041 round trip
    assert(saveQuestions("tmp_q.txt", qs, err));
    std::vector<Question> back; w.clear();
    assert(loadQuestions("tmp_q.txt", back, w, err) && back[0].text == "Edited?");
    qs[0].text = "Edited again?";                                                    // overwrite existing file
    assert(saveQuestions("tmp_q.txt", qs, err) && err.empty());
    assert(loadQuestions("tmp_q.txt", back, w, err) && back[0].text == "Edited again?");
    assert(!std::ifstream("tmp_q.txt.tmp") && !std::ifstream("tmp_q.txt.bak"));      // no leftovers
    err.clear();
    assert(!saveQuestions("no_such_dir/q.txt", qs, err) && !err.empty());           // failure reported
    std::remove("tmp_q.txt");

    std::remove("tmp_r.txt");                                                        // results
    std::vector<Result> rs;
    assert(loadResults("tmp_r.txt", rs, w) && rs.empty());
    assert(saveResult("tmp_r.txt", {"Tanvi|x", "Math", 7, 10, ""}, err));
    assert(loadResults("tmp_r.txt", rs, w) && rs.size() == 1 && rs[0].player == "Tanvix");
    std::remove("tmp_r.txt");

    std::cout << "All FileIOModule tests passed\n";
}
