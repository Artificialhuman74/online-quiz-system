#include "FileIOModule.h"
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <set>
#include <sstream>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace fs = std::filesystem;

namespace FileIOModule {
namespace testing {
bool (*replaceHook)(const std::string&, const std::string&) = nullptr;
}
}  // namespace FileIOModule

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

bool hasBadChar(const std::string& s) {
    return s.find_first_of("|\r\n") != std::string::npos;
}

bool validTimestamp(const std::string& t) {
    if (t.size() != 19) return false;
    for (int i = 0; i < 19; i++) {
        char c = t[i];
        if (i == 4 || i == 7) { if (c != '-') return false; }
        else if (i == 10)     { if (c != ' ') return false; }
        else if (i == 13 || i == 16) { if (c != ':') return false; }
        else if (c < '0' || c > '9') return false;
    }
    int mo = std::stoi(t.substr(5, 2)), d = std::stoi(t.substr(8, 2)), h = std::stoi(t.substr(11, 2)),
        mi = std::stoi(t.substr(14, 2)), s = std::stoi(t.substr(17, 2));
    return mo >= 1 && mo <= 12 && d >= 1 && d <= 31 && h < 24 && mi < 60 && s <= 60;
}

// Replaces `to` with `from` in one step. On failure `to` is left untouched.
//  - POSIX: rename() replaces the target atomically.
//  - Windows: MoveFileEx with REPLACE_EXISTING swaps it directly; there is no moment where
//    the original is missing, so no backup/restore is needed.
bool replaceFile(const std::string& from, const std::string& to) {
    if (FileIOModule::testing::replaceHook) return FileIOModule::testing::replaceHook(from, to);
#ifdef _WIN32
    return MoveFileExA(from.c_str(), to.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
    return std::rename(from.c_str(), to.c_str()) == 0;
#endif
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
    if (hasBadChar(q.category) || hasBadChar(q.difficulty) || hasBadChar(q.text) ||
        hasBadChar(q.options[0]) || hasBadChar(q.options[1]) || hasBadChar(q.options[2]) ||
        hasBadChar(q.options[3]))
        return "Text must not contain '|' or line breaks";
    return "";
}

std::string validateResult(const Result& r) {
    if (trim(r.player).empty()) return "Player name is empty";
    if (trim(r.category).empty()) return "Category is empty";
    if (hasBadChar(r.player) || hasBadChar(r.category)) return "Text must not contain '|' or line breaks";
    if (r.total < 1) return "Total must be at least 1";
    if (r.score < 0) return "Score cannot be negative";
    if (r.score > r.total) return "Score cannot exceed total";
    if (!validTimestamp(r.timestamp)) return "Timestamp must look like YYYY-MM-DD HH:MM:SS";
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

    // 1. Validate the whole bank BEFORE touching any file.
    if (qs.empty()) { error = "Refusing to save an empty question bank"; return false; }
    std::set<int> ids;
    for (size_t i = 0; i < qs.size(); i++) {
        std::string why = validateQuestion(qs[i]);
        if (!why.empty()) { error = "Question #" + std::to_string(i + 1) + " (ID " + std::to_string(qs[i].id) + "): " + why; return false; }
        if (!ids.insert(qs[i].id).second) { error = "Duplicate question ID " + std::to_string(qs[i].id); return false; }
    }

    // 2. Write everything to a temp file.
    const std::string tmp = path + ".tmp";
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

    // 3. Swap it in. If this fails the original is untouched; just clean up the temp file.
    if (!replaceFile(tmp, path)) {
        std::remove(tmp.c_str());
        error = "Could not replace " + path + " (original left unchanged)";
        return false;
    }
    return true;
}

bool saveResult(const std::string& path, Result r, std::string& error) {
    error.clear();
    r.player = sanitize(r.player);
    r.category = sanitize(r.category);
    if (r.timestamp.empty()) r.timestamp = now();

    std::string why = validateResult(r);
    if (!why.empty()) { error = "Invalid result: " + why; return false; }

    std::ofstream o(path, std::ios::app);
    if (!o) { error = "Cannot open results file: " + path; return false; }
    o << (r.player + '|' + r.category + '|' + std::to_string(r.score) + '|' + std::to_string(r.total) + '|' + r.timestamp + '\n');
    o.flush();
    if (!o) { error = "Write failed for " + path; return false; }
    return true;
}

bool loadResults(const std::string& path, std::vector<Result>& out,
                 std::vector<std::string>& warnings, std::string& error) {
    out.clear();
    warnings.clear();
    error.clear();

    std::error_code ec;
    bool exists = fs::exists(path, ec);
    if (ec) { error = "Cannot access results file " + path + ": " + ec.message(); return false; }
    if (!exists) return true;                       // no history yet
    if (!fs::is_regular_file(path, ec) || ec) { error = "Results path is not a regular file: " + path; return false; }

    std::ifstream in(path);
    if (!in) { error = "Cannot open results file: " + path; return false; }

    std::string line;
    int n = 0;
    while (std::getline(in, line)) {
        n++;
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;
        auto f = split(line, '|');
        Result r;
        if (f.size() != 5 || !toInt(f[2], r.score) || !toInt(f[3], r.total)) {
            warnings.push_back("Results line " + std::to_string(n) + ": malformed, skipped");
            continue;
        }
        r.player = f[0]; r.category = f[1]; r.timestamp = f[4];
        std::string why = validateResult(r);
        if (!why.empty()) { warnings.push_back("Results line " + std::to_string(n) + ": " + why + ", skipped"); continue; }
        out.push_back(r);
    }
    if (in.bad()) { out.clear(); error = "Read error in results file: " + path; return false; }
    return true;
}

}  // namespace FileIOModule