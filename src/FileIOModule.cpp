#include "FileIOModule.h"
#include <cstdio>
#include <ctime>
#include <fstream>
#include <set>
#include <sstream>

namespace {

std::vector<std::string> split(const std::string& line, char d) {
    std::vector<std::string> f;
    std::string cur;
    std::istringstream ss(line);
    while (std::getline(ss, cur, d)) f.push_back(cur);
    if (!line.empty() && line.back() == d) f.push_back("");
    return f;
}

std::string trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n"), b = s.find_last_not_of(" \t\r\n");
    return a == std::string::npos ? "" : s.substr(a, b - a + 1);
}

bool toInt(const std::string& s, int& v) {
    try { size_t p; v = std::stoi(s, &p); return p == s.size(); }
    catch (...) { return false; }
}

std::string now() {
    std::time_t t = std::time(nullptr);
    char buf[32];
    std::strftime(buf, sizeof buf, "%Y-%m-%d %H:%M:%S", std::localtime(&t));
    return buf;
}

}  // namespace

namespace FileIOModule {

std::string sanitize(const std::string& s) {
    std::string r;
    for (char c : s) if (c != '|' && c != '\n' && c != '\r') r += c;
    return trim(r);
}

std::string validateQuestion(const Question& q) {
    if (q.id <= 0) return "ID must be a positive number";
    if (trim(q.category).empty()) return "Category is empty";
    if (q.difficulty != "Easy" && q.difficulty != "Medium" && q.difficulty != "Hard")
        return "Difficulty must be Easy, Medium or Hard";
    if (trim(q.text).empty()) return "Question text is empty";
    for (int i = 0; i < 4; i++)
        if (trim(q.options[i]).empty()) return std::string("Option ") + char('A' + i) + " is empty";
    if (q.correct < 'A' || q.correct > 'D') return "Correct answer must be exactly one of A-D";
    auto bad = [](const std::string& s) { return s.find('|') != std::string::npos; };
    if (bad(q.category) || bad(q.text) || bad(q.options[0]) || bad(q.options[1]) ||
        bad(q.options[2]) || bad(q.options[3]))
        return "Text must not contain the '|' character";
    return "";
}

bool loadQuestions(const std::string& path, std::vector<Question>& out,
                   std::vector<std::string>& warnings, std::string& error) {
    out.clear();
    warnings.clear();
    error.clear();
    std::ifstream in(path);
    if (!in) { error = "Cannot open question bank: " + path; return false; }

    std::set<int> seen;
    std::string line;
    int n = 0;
    while (std::getline(in, line)) {
        n++;
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;

        auto f = split(line, '|');
        if (f.size() != 9) { warnings.push_back("Line " + std::to_string(n) + ": expected 9 fields, got " + std::to_string(f.size())); continue; }

        Question q;
        if (!toInt(trim(f[0]), q.id)) { warnings.push_back("Line " + std::to_string(n) + ": bad ID"); continue; }
        q.category = trim(f[1]);
        q.difficulty = trim(f[2]);
        q.text = trim(f[3]);
        for (int i = 0; i < 4; i++) q.options[i] = trim(f[4 + i]);
        std::string c = trim(f[8]);
        q.correct = c.size() == 1 ? c[0] : '?';

        std::string why = validateQuestion(q);
        if (!why.empty()) { warnings.push_back("Line " + std::to_string(n) + ": " + why); continue; }
        if (!seen.insert(q.id).second) { warnings.push_back("Line " + std::to_string(n) + ": duplicate ID " + std::to_string(q.id)); continue; }
        out.push_back(q);
    }
    if (out.empty()) { error = "No valid questions found in " + path; return false; }
    return true;
}

bool saveQuestions(const std::string& path, const std::vector<Question>& qs, std::string& error) {
    error.clear();
    const std::string tmp = path + ".tmp";
    const std::string bak = path + ".bak";
    {
        std::ofstream o(tmp, std::ios::trunc);
        if (!o) { error = "Cannot write " + tmp; return false; }
        o << "# ID|Category|Difficulty|Question|OptA|OptB|OptC|OptD|CorrectLetter\n";
        for (const auto& q : qs)
            o << q.id << '|' << q.category << '|' << q.difficulty << '|' << q.text << '|'
              << q.options[0] << '|' << q.options[1] << '|' << q.options[2] << '|' << q.options[3]
              << '|' << q.correct << '\n';
        o.flush();
        if (!o) { o.close(); std::remove(tmp.c_str()); error = "Write failed for " + tmp; return false; }
    }

    // Fast path: POSIX rename() replaces the target atomically.
    if (std::rename(tmp.c_str(), path.c_str()) == 0) return true;

    // Windows: rename() fails if the target exists. Move the original aside first,
    // and restore it if the swap fails, so the original is never lost.
    std::remove(bak.c_str());
    bool hadOriginal = std::rename(path.c_str(), bak.c_str()) == 0;
    if (std::rename(tmp.c_str(), path.c_str()) != 0) {
        if (hadOriginal) std::rename(bak.c_str(), path.c_str());   // restore original
        std::remove(tmp.c_str());
        error = "Could not replace " + path + " (original left unchanged)";
        return false;
    }
    std::remove(bak.c_str());
    return true;
}

bool saveResult(const std::string& path, Result r, std::string& error) {
    error.clear();
    if (r.timestamp.empty()) r.timestamp = now();
    std::ofstream o(path, std::ios::app);
    if (!o) { error = "Cannot open results file: " + path; return false; }
    o << sanitize(r.player) << '|' << sanitize(r.category) << '|' << r.score << '|' << r.total << '|' << r.timestamp << '\n';
    o.flush();
    if (!o) { error = "Write failed for " + path; return false; }
    return true;
}

bool loadResults(const std::string& path, std::vector<Result>& out, std::vector<std::string>& warnings) {
    out.clear();
    warnings.clear();
    std::ifstream in(path);
    if (!in) return true;   // no history yet
    std::string line;
    int n = 0;
    while (std::getline(in, line)) {
        n++;
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;
        auto f = split(line, '|');
        Result r;
        if (f.size() != 5 || !toInt(f[2], r.score) || !toInt(f[3], r.total) || r.score < 0 || r.score > r.total) {
            warnings.push_back("Results line " + std::to_string(n) + ": malformed, skipped");
            continue;
        }
        r.player = f[0]; r.category = f[1]; r.timestamp = f[4];
        out.push_back(r);
    }
    return true;
}

}  // namespace FileIOModule
