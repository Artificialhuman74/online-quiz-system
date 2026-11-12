#include "AdminModule.h"
#include "FileIOModule.h"
#include "InputValidation.h"
#include "MenuModule.h"
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <vector>

#ifdef _WIN32
#include <conio.h>
#include <io.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

namespace AdminModule {
namespace {
constexpr int kMaxFailures = 3;
int failures = 0;

const std::uint32_t K[64] = {
 0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
 0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
 0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
 0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
 0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
 0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
 0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
 0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2};
inline std::uint32_t rotr(std::uint32_t x, int n) { return (x >> n) | (x << (32 - n)); }

std::string sha256Raw(const std::string& msg) {
    std::uint32_t h[8] = {0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19};
    std::string m = msg;
    std::uint64_t bits = static_cast<std::uint64_t>(msg.size()) * 8;
    m += char(0x80);
    while (m.size() % 64 != 56) m += char(0);
    for (int i = 7; i >= 0; i--) m += char((bits >> (i * 8)) & 0xff);
    for (size_t off = 0; off < m.size(); off += 64) {
        std::uint32_t w[64];
        for (int i = 0; i < 16; i++)
            w[i] = (std::uint32_t(std::uint8_t(m[off+4*i])) << 24) | (std::uint32_t(std::uint8_t(m[off+4*i+1])) << 16) |
                   (std::uint32_t(std::uint8_t(m[off+4*i+2])) << 8) | std::uint32_t(std::uint8_t(m[off+4*i+3]));
        for (int i = 16; i < 64; i++) {
            std::uint32_t s0 = rotr(w[i-15],7) ^ rotr(w[i-15],18) ^ (w[i-15] >> 3);
            std::uint32_t s1 = rotr(w[i-2],17) ^ rotr(w[i-2],19) ^ (w[i-2] >> 10);
            w[i] = w[i-16] + s0 + w[i-7] + s1;
        }
        std::uint32_t a=h[0],b=h[1],c=h[2],d=h[3],e=h[4],f=h[5],g=h[6],hh=h[7];
        for (int i = 0; i < 64; i++) {
            std::uint32_t t1 = hh + (rotr(e,6)^rotr(e,11)^rotr(e,25)) + ((e&f)^(~e&g)) + K[i] + w[i];
            std::uint32_t t2 = (rotr(a,2)^rotr(a,13)^rotr(a,22)) + ((a&b)^(a&c)^(b&c));
            hh=g; g=f; f=e; e=d+t1; d=c; c=b; b=a; a=t1+t2;
        }
        h[0]+=a; h[1]+=b; h[2]+=c; h[3]+=d; h[4]+=e; h[5]+=f; h[6]+=g; h[7]+=hh;
    }
    std::string out;
    for (int i = 0; i < 8; i++) for (int j = 3; j >= 0; j--) out += char((h[i] >> (j*8)) & 0xff);
    return out;
}

std::string toHex(const std::string& raw) {
    static const char* d = "0123456789abcdef";
    std::string s;
    for (unsigned char c : raw) { s += d[c >> 4]; s += d[c & 15]; }
    return s;
}

// Salted repeated digest for the local classroom demo. This is not a production password KDF.
std::string digest(const std::string& salt, const std::string& password) {
    std::string h = sha256Raw(salt + password);
    for (int i = 0; i < 20000; ++i) h = sha256Raw(h + salt + password);
    return toHex(h);
}

std::string makeSalt() {
    std::random_device rd;
    std::ostringstream out;
    out << std::hex << rd() << rd() << rd() << rd();
    return out.str();
}

std::string readPassword(const std::string& prompt) {
    bool interactive = false;
#ifdef _WIN32
    interactive = _isatty(_fileno(stdin)) != 0;
#else
    interactive = isatty(STDIN_FILENO) != 0;
#endif
    if (!interactive) return InputValidation::readText(prompt, 50);

    for (;;) {
        std::cout << prompt << std::flush;
        std::string password;
#ifndef _WIN32
        termios original{};
        if (tcgetattr(STDIN_FILENO, &original) != 0)
            return InputValidation::readText("\n" + prompt, 50);
        termios masked = original;
        masked.c_lflag &= static_cast<tcflag_t>(~(ICANON | ECHO));
        masked.c_cc[VMIN] = 1;
        masked.c_cc[VTIME] = 0;
        if (tcsetattr(STDIN_FILENO, TCSANOW, &masked) != 0)
            return InputValidation::readText("\n" + prompt, 50);
#endif
        bool inputClosed = false;
        for (;;) {
            int value = -1;
#ifdef _WIN32
            value = _getch();
#else
            char c = 0;
            if (std::cin.get(c)) value = static_cast<unsigned char>(c);
#endif
            if (value < 0 || value == 4 || value == 26) { inputClosed = true; break; }
            if (value == '\r' || value == '\n') break;
            if (value == 8 || value == 127) {
                if (!password.empty()) {
                    password.pop_back();
                    std::cout << "\b \b" << std::flush;
                }
                continue;
            }
            if (value < 32) continue;
            if (password.size() < 50) {
                password.push_back(static_cast<char>(value));
                std::cout << '*' << std::flush;
            }
        }
#ifndef _WIN32
        tcsetattr(STDIN_FILENO, TCSANOW, &original);
#endif
        std::cout << '\n';
        if (inputClosed) throw InputValidation::InputClosed();
        const std::string why = InputValidation::checkText(password, 50);
        if (why.empty()) return password;
        std::cout << why << '\n';
    }
}

bool authenticate(const std::string& path) {
    if (failures >= kMaxFailures) {
        std::cout << "Admin panel locked until the program is restarted.\n";
        return false;
    }
    std::ifstream in(path);
    std::string salt, stored;
    if (!in) {
        MenuModule::printHeader("Create Admin Password");
        std::string password;
        for (;;) {
            password = readPassword("Create password (at least 6 characters): ");
            if (password.size() >= 6) break;
            std::cout << "Password must be at least 6 characters.\n";
        }
        salt = makeSalt();
        std::ofstream out(path, std::ios::trunc);
        if (!out || !(out << salt << '\n' << digest(salt, password) << '\n')) {
            std::cerr << "Could not save admin password to " << path << ".\n";
            return false;
        }
        std::cout << "Admin password created.\n";
        return true;
    }
    if (!std::getline(in, salt) || !std::getline(in, stored) || salt.empty() || stored.empty()) {
        std::cerr << "Admin password file is invalid. Delete " << path << " to set a new password.\n";
        return false;
    }
    const std::string password = readPassword("Admin password: ");
    if (digest(salt, password) == stored) { failures = 0; return true; }
    ++failures;
    std::cout << "Incorrect password. " << (kMaxFailures - failures) << " attempt(s) remain.\n";
    if (failures == kMaxFailures) std::cout << "Admin panel locked until the program is restarted.\n";
    return false;
}

void showQuestions(const std::vector<Question>& bank) {
    if (bank.empty()) { std::cout << "No questions in the bank.\n"; return; }
    for (const auto& q : bank) {
        std::cout << "\nID " << q.id << " | " << q.category << " | " << q.difficulty << "\n"
                  << q.text << "\n";
        for (int i = 0; i < 4; ++i)
            std::cout << "  " << char('A' + i) << ") " << q.options[i]
                      << (q.correct == 'A' + i ? " [correct]" : "") << '\n';
    }
}

Question readQuestion(int id) {
    Question q;
    q.id = id;
    q.category = InputValidation::readText("Category: ");
    const int difficulty = InputValidation::readMenuChoice("Difficulty (1 Easy, 2 Medium, 3 Hard): ", 1, 3);
    q.difficulty = difficulty == 1 ? "Easy" : difficulty == 2 ? "Medium" : "Hard";
    q.text = InputValidation::readText("Question: ");
    for (int i = 0; i < 4; ++i)
        q.options[i] = InputValidation::readText(std::string("Option ") + char('A' + i) + ": ");
    q.correct = InputValidation::readAnswer("Correct option (A-D): ", false);
    return q;
}

std::string readEditableField(const std::string& label, const std::string& current) {
    for (;;) {
        std::cout << label << " [" << current << "]: " << std::flush;
        std::string value;
        if (!std::getline(std::cin, value)) throw InputValidation::InputClosed();
        if (!value.empty()) {
            const std::string why = InputValidation::checkText(value);
            if (!why.empty()) { std::cout << why << '\n'; continue; }
            return InputValidation::trim(value);
        }
        return current;
    }
}

Question editQuestion(const Question& old) {
    Question q = old;
    q.category = readEditableField("Category", old.category);
    std::cout << "Difficulty: 1. Easy  2. Medium  3. Hard  (Enter keeps current) ["
              << old.difficulty << "]: " << std::flush;
    std::string level;
    if (!std::getline(std::cin, level)) throw InputValidation::InputClosed();
    if (!level.empty()) {
        int value = 0;
        while (!InputValidation::parseMenuChoice(level, 1, 3, value)) {
            std::cout << "Enter 1, 2 or 3: " << std::flush;
            if (!std::getline(std::cin, level)) throw InputValidation::InputClosed();
        }
        q.difficulty = value == 1 ? "Easy" : value == 2 ? "Medium" : "Hard";
    }
    q.text = readEditableField("Question", old.text);
    for (int i = 0; i < 4; ++i)
        q.options[i] = readEditableField(std::string("Option ") + char('A' + i), old.options[i]);
    std::cout << "Correct option (A-D, Enter keeps " << old.correct << "): " << std::flush;
    std::string correct;
    if (!std::getline(std::cin, correct)) throw InputValidation::InputClosed();
    if (!correct.empty()) {
        char answer;
        while (!InputValidation::parseAnswer(correct, answer, false)) {
            std::cout << "Enter A, B, C or D: " << std::flush;
            if (!std::getline(std::cin, correct)) throw InputValidation::InputClosed();
        }
        q.correct = answer;
    }
    return q;
}

int findIndex(const std::vector<Question>& bank, int id) {
    for (std::size_t i = 0; i < bank.size(); ++i)
        if (bank[i].id == id) return static_cast<int>(i);
    return -1;
}

int readQuestionId(const std::string& prompt) {
    for (;;) {
        const std::string raw = InputValidation::readText(prompt, 10);
        int id = 0;
        if (InputValidation::parseInt(raw, id) && id > 0) return id;
        std::cout << "Enter a positive numeric question ID.\n";
    }
}

void saveBank(const std::string& path, const std::vector<Question>& bank) {
    std::string error;
    if (FileIOModule::saveQuestions(path, bank, error)) std::cout << "Question bank saved.\n";
    else std::cerr << "Could not save question bank: " << error << '\n';
}
}  // namespace

void run(const std::string& questionsPath, const std::string& passwordPath) {
    if (!authenticate(passwordPath)) return;
    for (;;) {
        MenuModule::printHeader("Admin Panel");
        std::cout << "1. View questions\n2. Add question\n3. Edit question\n"
                     "4. Delete question\n5. Return to main menu\n";
        const int choice = InputValidation::readMenuChoice("Enter your choice (1-5): ", 1, 5);
        std::vector<Question> bank;
        std::vector<std::string> warnings;
        std::string error;
        const bool loaded = FileIOModule::loadQuestions(questionsPath, bank, warnings, error);
        if (!loaded) { std::cerr << error << '\n'; continue; }
        for (const auto& warning : warnings) std::cerr << "Warning: " << warning << '\n';
        if (choice == 1) {
            std::vector<std::string> categories;
            for (const auto& q : bank)
                if (std::find(categories.begin(), categories.end(), q.category) == categories.end())
                    categories.push_back(q.category);
            const std::string category = MenuModule::selectCategory(categories);
            const std::string difficulty = MenuModule::selectDifficulty();
            std::vector<Question> filtered;
            for (const auto& q : bank)
                if ((category.empty() || q.category == category) &&
                    (difficulty.empty() || q.difficulty == difficulty)) filtered.push_back(q);
            showQuestions(filtered);
        }
        else if (choice == 2) {
            int id = 1;
            for (const auto& q : bank) id = std::max(id, q.id + 1);
            bank.push_back(readQuestion(id));
            saveBank(questionsPath, bank);
        } else if (choice == 3) {
            showQuestions(bank);
            const int index = findIndex(bank, readQuestionId("Question ID to edit: "));
            if (index < 0) { std::cout << "Question ID not found.\n"; continue; }
            bank[index] = editQuestion(bank[index]);
            saveBank(questionsPath, bank);
        } else if (choice == 4) {
            showQuestions(bank);
            const int index = findIndex(bank, readQuestionId("Question ID to delete: "));
            if (index < 0) { std::cout << "Question ID not found.\n"; continue; }
            if (bank.size() == 1) { std::cout << "The last remaining question cannot be deleted.\n"; continue; }
            if (InputValidation::readYesNo("Delete this question? (y/n): ")) {
                bank.erase(bank.begin() + index);
                saveBank(questionsPath, bank);
            } else std::cout << "Deletion cancelled.\n";
        } else return;
    }
}
}  // namespace AdminModule
