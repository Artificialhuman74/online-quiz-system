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
// Returns false only if the file is missing/unreadable or has zero valid questions (F-040).
bool loadQuestions(const std::string& path, std::vector<Question>& out,
                   std::vector<std::string>& warnings, std::string& error);

// Rewrites the whole bank atomically (temp file + rename). Call after every admin edit (F-041).
bool saveQuestions(const std::string& path, const std::vector<Question>& qs, std::string& error);

// Returns "" if valid, else a human-readable reason (used by AdminModule for F-034).
std::string validateQuestion(const Question& q);

// Appends one attempt to the results file (F-023). Timestamp is filled in if empty.
bool saveResult(const std::string& path, Result r, std::string& error);

// Loads all past attempts (F-024/025). Missing file = empty list, not an error.
bool loadResults(const std::string& path, std::vector<Result>& out,
                 std::vector<std::string>& warnings);

// Strips the '|' delimiter and newlines from user-entered text.
std::string sanitize(const std::string& s);

}  // namespace FileIOModule
