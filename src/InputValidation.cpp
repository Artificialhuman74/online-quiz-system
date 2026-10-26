#include "InputValidation.h"
#include <cctype>
#include <climits>
#include <cstdlib>

namespace {

std::string upper(std::string s) {
    for (char& c : s) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return s;
}

// Shows the prompt and reads one line. Throws InputClosed at end of input.
std::string promptLine(const std::string& prompt, std::istream& in, std::ostream& out) {
    out << prompt << std::flush;
    std::string line;
    if (!std::getline(in, line)) {
        out << '\n';
        throw InputValidation::InputClosed();
    }
    if (!line.empty() && line.back() == '\r') line.pop_back();  // Windows line endings in piped input
    return line;
}

}  // namespace

namespace InputValidation {

std::string trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n"), b = s.find_last_not_of(" \t\r\n");
    return a == std::string::npos ? "" : s.substr(a, b - a + 1);
}

bool parseInt(const std::string& s, int& out) {
    std::string t = trim(s);
    if (t.empty()) return false;
    size_t i = (t[0] == '-' || t[0] == '+') ? 1 : 0;
    if (i == t.size()) return false;
    for (size_t j = i; j < t.size(); j++)
        if (!std::isdigit(static_cast<unsigned char>(t[j]))) return false;
    if (t.size() - i > 10) return false;  // longer than any int; avoids strtoll overflow
    long long v = std::strtoll(t.c_str(), nullptr, 10);
    if (v < INT_MIN || v > INT_MAX) return false;
    out = static_cast<int>(v);
    return true;
}

bool parseMenuChoice(const std::string& s, int min, int max, int& out) {
    int v;
    if (!parseInt(s, v) || v < min || v > max) return false;
    out = v;
    return true;
}

bool parseAnswer(const std::string& s, char& out, bool allowSkip) {
    std::string t = upper(trim(s));
    if (t.size() != 1) return false;
    if (t[0] >= 'A' && t[0] <= 'D') { out = t[0]; return true; }
    if (allowSkip && t[0] == kSkip) { out = kSkip; return true; }
    return false;
}

bool parseYesNo(const std::string& s, bool& out) {
    std::string t = upper(trim(s));
    if (t == "Y" || t == "YES") { out = true; return true; }
    if (t == "N" || t == "NO") { out = false; return true; }
    return false;
}

std::string checkText(const std::string& s, std::size_t maxLen) {
    std::string t = trim(s);
    if (t.empty()) return "This cannot be empty.";
    if (t.size() > maxLen) return "Please use at most " + std::to_string(maxLen) + " characters.";
    for (char c : t) {
        if (c == '|') return "The '|' character is not allowed.";
        if (std::iscntrl(static_cast<unsigned char>(c))) return "Control characters are not allowed.";
    }
    return "";
}

int readMenuChoice(const std::string& prompt, int min, int max, std::istream& in, std::ostream& out) {
    for (;;) {
        int v;
        if (parseMenuChoice(promptLine(prompt, in, out), min, max, v)) return v;
        out << "Invalid choice. Please enter a number from " << min << " to " << max << ".\n";
    }
}

char readAnswer(const std::string& prompt, bool allowSkip, std::istream& in, std::ostream& out) {
    for (;;) {
        char c;
        if (parseAnswer(promptLine(prompt, in, out), c, allowSkip)) return c;
        out << (allowSkip ? "Invalid answer. Please enter A, B, C or D (or S to skip).\n"
                          : "Invalid answer. Please enter A, B, C or D.\n");
    }
}

bool readYesNo(const std::string& prompt, std::istream& in, std::ostream& out) {
    for (;;) {
        bool v;
        if (parseYesNo(promptLine(prompt, in, out), v)) return v;
        out << "Please enter y or n.\n";
    }
}

std::string readText(const std::string& prompt, std::size_t maxLen, std::istream& in, std::ostream& out) {
    for (;;) {
        std::string line = promptLine(prompt, in, out);
        std::string why = checkText(line, maxLen);
        if (why.empty()) return trim(line);
        out << why << '\n';
    }
}

}  // namespace InputValidation
