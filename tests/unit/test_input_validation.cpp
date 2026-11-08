// Self-test for InputValidation. From the repo root:
//   g++ -std=c++17 -Isrc -o t tests/unit/test_input_validation.cpp src/InputValidation.cpp && ./t
#include "InputValidation.h"
#include <iostream>
#include <sstream>
#include <string>
using namespace InputValidation;

static int failures = 0;
#define CHECK(cond)                                                              \
    do {                                                                         \
        if (!(cond)) { std::cerr << __FILE__ << ':' << __LINE__ << ": FAILED: " #cond "\n"; failures++; } \
    } while (0)

// Counts how many times `needle` appears in `hay` (used to count re-prompts).
static int count(const std::string& hay, const std::string& needle) {
    int n = 0;
    for (size_t p = hay.find(needle); p != std::string::npos; p = hay.find(needle, p + 1)) n++;
    return n;
}

int main() {
    int v = -99;

    // ---- parseInt ----
    CHECK(parseInt("42", v) && v == 42);
    CHECK(parseInt("  7 \t", v) && v == 7);
    CHECK(parseInt("-3", v) && v == -3);
    CHECK(parseInt("+5", v) && v == 5);
    CHECK(parseInt("2147483647", v) && v == 2147483647);
    CHECK(parseInt("-2147483648", v) && v == -2147483647 - 1);
    v = 1;
    for (const char* bad : {"", "   ", "abc", "3abc", "1.5", "1 2", "-", "+", "2147483648",
                            "-2147483649", "99999999999999999999", "0x10", "+ 2"})
        CHECK(!parseInt(bad, v));
    CHECK(v == 1);  // untouched on failure

    // ---- parseInt: 10-digit boundary (gap 2) ----
    // exactly 10 digits: 1000000000 fits in int (< 2147483647)
    CHECK(parseInt("1000000000", v) && v == 1000000000);
    // 11 digits: rejected by the length guard before strtoll even runs
    CHECK(!parseInt("10000000000", v));

    // ---- parseMenuChoice (QZ-F-042: non-numeric or out-of-range) ----
    CHECK(parseMenuChoice("1", 1, 4, v) && v == 1);
    CHECK(parseMenuChoice("4", 1, 4, v) && v == 4);
    CHECK(!parseMenuChoice("0", 1, 4, v));
    CHECK(!parseMenuChoice("5", 1, 4, v));
    CHECK(!parseMenuChoice("two", 1, 4, v));

    // ---- parseAnswer (QZ-F-012) ----
    char c = '?';
    CHECK(parseAnswer("A", c, false) && c == 'A');
    CHECK(parseAnswer("a", c, false) && c == 'A');
    CHECK(parseAnswer("B", c, false) && c == 'B');
    CHECK(parseAnswer("c", c, false) && c == 'C');
    CHECK(parseAnswer(" d ", c, false) && c == 'D');
    CHECK(!parseAnswer("E", c, false));
    CHECK(!parseAnswer("AB", c, false));  // two chars (gap 3)
    CHECK(!parseAnswer("AA", c, false));  // two upper-case chars (gap 3)
    CHECK(!parseAnswer("", c, false));
    CHECK(!parseAnswer("1", c, false));
    CHECK(!parseAnswer("S", c, false));            // skip not allowed here
    CHECK(!parseAnswer("s", c, false));
    CHECK(parseAnswer("s", c, true) && c == kSkip);  // QZ-F-014
    CHECK(parseAnswer(" S ", c, true) && c == kSkip);
    c = '?';
    CHECK(!parseAnswer("E", c, true) && c == '?');   // failed parse leaves output untouched

    // ---- parseYesNo ----
    bool b = false;
    CHECK(parseYesNo("y", b) && b);
    CHECK(parseYesNo(" YES ", b) && b);
    CHECK(parseYesNo("n", b) && !b);
    CHECK(parseYesNo("No", b) && !b);
    CHECK(!parseYesNo("maybe", b));
    CHECK(!parseYesNo("", b));
    b = true;
    CHECK(!parseYesNo("yesterday", b) && b);        // whole token only; unchanged on failure

    // ---- trim (gap 1): direct tests of the public function ----
    CHECK(trim("  hello  ") == "hello");
    CHECK(trim("\t\r\n") == "");           // all whitespace → empty
    CHECK(trim("") == "");
    CHECK(trim("no spaces") == "no spaces");
    CHECK(trim("  a") == "a");            // leading only
    CHECK(trim("a  ") == "a");            // trailing only

    // ---- checkText ----
    CHECK(checkText("Chiranth").empty());
    CHECK(!checkText("").empty());
    CHECK(!checkText("   ").empty());
    CHECK(!checkText("a|b").empty());
    CHECK(!checkText(std::string("a\tb")).empty());
    CHECK(!checkText(std::string("a\nb")).empty());
    CHECK(!checkText(std::string("a\0b", 3)).empty());
    CHECK(!checkText(std::string(kMaxTextLength + 1, 'x')).empty());
    CHECK(checkText(std::string(kMaxTextLength, 'x')).empty());
    CHECK(checkText("  trimmed  ").empty());
    CHECK(!checkText("x", 0).empty());
    CHECK(checkText("", 0) == "This cannot be empty.");

    // ---- checkText: exact error message text (gap 4) ----
    CHECK(checkText(std::string(kMaxTextLength + 1, 'x'))
          == "Please use at most " + std::to_string(kMaxTextLength) + " characters.");
    CHECK(checkText("a|b") == "The '|' character is not allowed.");
    CHECK(checkText(std::string("a\tb")) == "Control characters are not allowed.");

    // ---- checkText: custom maxLen edge cases (gap 5) ----
    CHECK(checkText("x", 1).empty());     // single char, maxLen 1 → passes
    CHECK(!checkText("xy", 1).empty());   // two chars, maxLen 1 → fails

    // ---- read* re-prompt on invalid input, then return the valid value ----
    {
        std::istringstream in("abc\n9\n\n2\n");
        std::ostringstream out;
        CHECK(readMenuChoice("> ", 1, 4, in, out) == 2);
        CHECK(count(out.str(), "Invalid choice") == 3);
        CHECK(count(out.str(), "> ") == 4);
    }
    {
        std::istringstream in("x\nS\nE\nb\r\n");  // invalid choices don't count; CRLF tolerated
        std::ostringstream out;
        CHECK(readAnswer("Answer: ", false, in, out) == 'B');
        CHECK(count(out.str(), "Invalid answer") == 3);
        // ---- readAnswer error message text (gap 6) ----
        // When allowSkip=false the message must NOT mention 'S to skip'.
        CHECK(out.str().find("Please enter A, B, C or D.") != std::string::npos);
        CHECK(out.str().find("S to skip") == std::string::npos);
    }
    {
        // ---- readAnswer with allowSkip=true error message (gap 6) ----
        std::istringstream in("x\nA\n");
        std::ostringstream out;
        CHECK(readAnswer("Answer: ", true, in, out) == 'A');
        CHECK(out.str().find("S to skip") != std::string::npos);
    }
    {
        std::istringstream in("s\n");
        std::ostringstream out;
        CHECK(readAnswer("Answer: ", true, in, out) == kSkip);
    }
    {
        std::istringstream in("ok?\nY\n");
        std::ostringstream out;
        CHECK(readYesNo("Sure? ", in, out));
    }
    {
        // ---- readYesNo re-prompt message text (gap 7) ----
        std::istringstream in("maybe\nn\n");
        std::ostringstream out;
        CHECK(!readYesNo("Sure? ", in, out));
        CHECK(out.str().find("Please enter y or n.") != std::string::npos);
    }
    {
        std::istringstream in("\nbad|name\n  Tanvi  \n");
        std::ostringstream out;
        // ---- readText return value is trimmed (gap 8) ----
        const std::string result = readText("Name: ", kMaxTextLength, in, out);
        CHECK(result == "Tanvi");                  // spaces stripped
        CHECK(result.front() != ' ' && result.back() != ' ');
        CHECK(count(out.str(), "Name: ") == 3);
    }

    // ---- end of input throws instead of looping forever ----
    {
        std::istringstream in("nope\n");
        std::ostringstream out;
        bool threw = false;
        try { readMenuChoice("> ", 1, 4, in, out); } catch (const InputClosed&) { threw = true; }
        CHECK(threw);
    }
    {
        std::istringstream in("");
        std::ostringstream out;
        bool threw = false;
        try { readText("Name: ", kMaxTextLength, in, out); } catch (const InputClosed&) { threw = true; }
        CHECK(threw);
    }
    {
        std::istringstream in("");
        std::ostringstream out;
        bool threw = false;
        try { readAnswer("Answer: ", true, in, out); } catch (const InputClosed&) { threw = true; }
        CHECK(threw);
    }
    {
        std::istringstream in("");
        std::ostringstream out;
        bool threw = false;
        try { readYesNo("Sure? ", in, out); } catch (const InputClosed&) { threw = true; }
        CHECK(threw);
    }

    if (failures) { std::cerr << failures << " InputValidation check(s) failed\n"; return 1; }
    std::cout << "All InputValidation tests passed\n";
    return 0;
}
