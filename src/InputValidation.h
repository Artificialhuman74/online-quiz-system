// InputValidation.h - console input checks and re-prompting (QZ-F-012, QZ-F-042, QZ-NF-003, OQSSMP-17/21/24)
//
// Two layers:
//   parse*  - pure functions on one line of text. No I/O, so they are easy to unit test.
//   read*   - prompt on `out`, read lines from `in`, and re-prompt with a clear message until
//             the input is valid. They never return an invalid value.
// All read* functions throw InputClosed if `in` hits end-of-file, so callers never spin forever
// on a closed console; main() catches it and exits cleanly.
#pragma once
#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <string>

namespace InputValidation {

// Thrown by read* functions when the input stream ends (Ctrl+D / Ctrl+Z, or piped input runs out).
struct InputClosed : std::runtime_error {
    InputClosed() : std::runtime_error("input stream closed") {}
};

// Returned by parseAnswer/readAnswer when the player chooses to skip (QZ-F-014).
constexpr char kSkip = 'S';

// Longest accepted free-text entry (player name, etc.).
constexpr std::size_t kMaxTextLength = 50;

// Removes leading and trailing whitespace.
std::string trim(const std::string& s);

// Whole-string integer parse after trimming. Rejects "", "3abc", "1.5", " + 2", and overflow.
bool parseInt(const std::string& s, int& out);

// parseInt plus range check min..max (inclusive).
bool parseMenuChoice(const std::string& s, int min, int max, int& out);

// One letter A-D, either case, surrounding spaces allowed. `out` is upper case.
// If allowSkip, "S"/"s" is also accepted and `out` is kSkip.
bool parseAnswer(const std::string& s, char& out, bool allowSkip);

// "y"/"yes"/"n"/"no", any case.
bool parseYesNo(const std::string& s, bool& out);

// Returns "" if `s` is acceptable free text, else the reason it is not. Rejects empty text,
// text longer than maxLen, '|' (the data-file field separator) and control characters.
std::string checkText(const std::string& s, std::size_t maxLen = kMaxTextLength);

// Prompts until the player types a whole number in min..max.
int readMenuChoice(const std::string& prompt, int min, int max,
                   std::istream& in = std::cin, std::ostream& out = std::cout);

// Prompts until the player types A-D (or S to skip, if allowSkip). Returns 'A'..'D' or kSkip.
// An invalid entry only re-prompts; it never counts as an answer (QZ-F-012).
char readAnswer(const std::string& prompt, bool allowSkip,
                std::istream& in = std::cin, std::ostream& out = std::cout);

// Prompts until the player types y/yes/n/no.
bool readYesNo(const std::string& prompt,
               std::istream& in = std::cin, std::ostream& out = std::cout);

// Prompts until checkText() accepts the line. Returns it trimmed.
std::string readText(const std::string& prompt, std::size_t maxLen = kMaxTextLength,
                     std::istream& in = std::cin, std::ostream& out = std::cout);

}  // namespace InputValidation
