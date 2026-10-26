// FileIOModule.h - question bank + results persistence (QZ-F-023/024/025/040/041, QZ-SR-004, QZ-NF-002)
#pragma once
#include <string>
#include <vector>

struct Question {
    int id = 0;
    std::string category;
    std::string difficulty;      // Easy | Medium | Hard
    std::string text;
    std::string options[4];      // A-D in file order (QuizEngine shuffles)
    char correct = 'A';          // 'A'..'D' = index into options in file order
};

struct Result {
    std::string player;
    std::string category;
    int score = 0;
    int total = 0;
    std::string timestamp;       // "YYYY-MM-DD HH:MM:SS"
};

namespace FileIOModule {

// Loads questions. Malformed lines are skipped and described in `warnings` (SR-004).
// `warnings` and `error` are cleared on entry.
// Returns false only if the file is missing/unreadable or has zero valid questions (F-040).
bool loadQuestions(const std::string& path, std::vector<Question>& out,
                   std::vector<std::string>& warnings, std::string& error);

// Validates the WHOLE bank first (non-empty, every question valid, no duplicate IDs).
// If anything is wrong, returns false and the existing file is not touched.
// Otherwise writes a temp file and replaces the original in one step (F-041, NF-002).
bool saveQuestions(const std::string& path, const std::vector<Question>& qs, std::string& error);

// Returns "" if valid, else a human-readable reason (used by AdminModule for F-034).
// Rejects empty fields, bad difficulty/answer letter, and '|', '\r', '\n' in any text field.
std::string validateQuestion(const Question& q);

// Returns "" if valid, else a reason. Checks names, score/total range, timestamp format.
std::string validateResult(const Result& r);

// Validates, then appends one attempt (F-023). Empty timestamp = current time.
// Player/category have '|' and newlines stripped first; nothing is written if invalid.
bool saveResult(const std::string& path, Result r, std::string& error);

// Loads past attempts (F-024/025). A file that does not exist = empty history (true).
// A file that exists but cannot be opened/read = false with `error` set.
// Malformed lines are skipped and listed in `warnings`.
bool loadResults(const std::string& path, std::vector<Result>& out,
                 std::vector<std::string>& warnings, std::string& error);

// Strips '|' and newlines from user-entered text.
std::string sanitize(const std::string& s);

// For unit tests only: if set, used instead of the real file replacement in saveQuestions().
namespace testing {
extern bool (*replaceHook)(const std::string& from, const std::string& to);
}

}  // namespace FileIOModule
