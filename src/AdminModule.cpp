#include "AdminModule.h"
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <istream>
#include <ostream>
#include <random>
#include <sstream>

namespace {

// ---------- SHA-256 (FIPS 180-4) ----------
const uint32_t K[64] = {
 0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
 0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
 0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
 0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
 0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
 0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
 0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
 0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2};

inline uint32_t rotr(uint32_t x, int n) { return (x >> n) | (x << (32 - n)); }

std::string sha256Raw(const std::string& msg) {
    uint32_t h[8] = {0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19};
    std::string m = msg;
    uint64_t bits = static_cast<uint64_t>(msg.size()) * 8;
    m += char(0x80);
    while (m.size() % 64 != 56) m += char(0);
    for (int i = 7; i >= 0; i--) m += char((bits >> (i * 8)) & 0xff);

    for (size_t off = 0; off < m.size(); off += 64) {
        uint32_t w[64];
        for (int i = 0; i < 16; i++)
            w[i] = (uint32_t(uint8_t(m[off + 4*i])) << 24) | (uint32_t(uint8_t(m[off + 4*i + 1])) << 16) |
                   (uint32_t(uint8_t(m[off + 4*i + 2])) << 8) | uint32_t(uint8_t(m[off + 4*i + 3]));
        for (int i = 16; i < 64; i++) {
            uint32_t s0 = rotr(w[i-15], 7) ^ rotr(w[i-15], 18) ^ (w[i-15] >> 3);
            uint32_t s1 = rotr(w[i-2], 17) ^ rotr(w[i-2], 19) ^ (w[i-2] >> 10);
            w[i] = w[i-16] + s0 + w[i-7] + s1;
        }
        uint32_t a=h[0], b=h[1], c=h[2], d=h[3], e=h[4], f=h[5], g=h[6], hh=h[7];
        for (int i = 0; i < 64; i++) {
            uint32_t S1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
            uint32_t ch = (e & f) ^ (~e & g);
            uint32_t t1 = hh + S1 + ch + K[i] + w[i];
            uint32_t S0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
            uint32_t mj = (a & b) ^ (a & c) ^ (b & c);
            uint32_t t2 = S0 + mj;
            hh = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2;
        }
        h[0]+=a; h[1]+=b; h[2]+=c; h[3]+=d; h[4]+=e; h[5]+=f; h[6]+=g; h[7]+=hh;
    }
    std::string out;
    for (int i = 0; i < 8; i++) for (int j = 3; j >= 0; j--) out += char((h[i] >> (j * 8)) & 0xff);
    return out;
}

std::string toHex(const std::string& raw) {
    static const char* d = "0123456789abcdef";
    std::string s;
    for (unsigned char c : raw) { s += d[c >> 4]; s += d[c & 15]; }
    return s;
}

bool isHex(const std::string& s, size_t len) {
    if (s.size() != len) return false;
    for (char c : s) if (!std::isxdigit(static_cast<unsigned char>(c))) return false;
    return true;
}

bool sameString(const std::string& a, const std::string& b) {   // constant-time-ish compare
    if (a.size() != b.size()) return false;
    unsigned char diff = 0;
    for (size_t i = 0; i < a.size(); i++) diff |= static_cast<unsigned char>(a[i] ^ b[i]);
    return diff == 0;
}

// ---------- small helpers ----------
std::string trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n"), b = s.find_last_not_of(" \t\r\n");
    return a == std::string::npos ? "" : s.substr(a, b - a + 1);
}
std::string lower(std::string s) { for (char& c : s) c = char(std::tolower(static_cast<unsigned char>(c))); return s; }

void normalize(Question& q) {
    q.category = trim(q.category);
    q.text = trim(q.text);
    for (auto& o : q.options) o = trim(o);
    q.difficulty = lower(trim(q.difficulty));
    if (!q.difficulty.empty()) q.difficulty[0] = char(std::toupper(static_cast<unsigned char>(q.difficulty[0])));
    q.correct = char(std::toupper(static_cast<unsigned char>(q.correct)));
}

bool ask(std::istream& in, std::ostream& out, const std::string& label, std::string& value) {
    out << label;
    out.flush();
    if (!std::getline(in, value)) return false;     // EOF: caller bails out instead of looping forever
    return true;
}

bool parseId(const std::string& s, int& id) {
    try { size_t p; id = std::stoi(trim(s), &p); return p == trim(s).size() && id > 0; }
    catch (...) { return false; }
}

void printQuestion(std::ostream& out, const Question& q) {
    out << "[ID " << q.id << "] " << q.category << " / " << q.difficulty << "\n  Q: " << q.text << "\n";
    for (int i = 0; i < 4; i++) out << "  " << char('A' + i) << ") " << q.options[i] << "\n";
    out << "  Answer: " << q.correct << "\n";
}

}  // namespace

// ================= AdminModule =================

AdminModule::AdminModule(std::string questionPath, std::string configPath)
    : qPath_(std::move(questionPath)), cfgPath_(std::move(configPath)) { loadConfig(); }

std::string AdminModule::sha256Hex(const std::string& data) { return toHex(sha256Raw(data)); }

// Salted, iterated SHA-256. Fine for a course project; real systems should use bcrypt/argon2.
std::string AdminModule::hashPassword(const std::string& saltHex, const std::string& password) {
    std::string h = sha256Raw(saltHex + password);
    for (int i = 0; i < 20000; i++) h = sha256Raw(h + saltHex + password);
    return toHex(h);
}

void AdminModule::loadConfig() {
    configured_ = false; configError_.clear();
    std::ifstream in(cfgPath_);
    if (!in) return;                                   // first run: no password yet
    std::string line;
    std::getline(in, line);
    line = trim(line);
    std::vector<std::string> f;
    std::stringstream ss(line);
    for (std::string part; std::getline(ss, part, '$');) f.push_back(part);
    if (f.size() != 3 || f[0] != "v1" || !isHex(f[1], 32) || !isHex(f[2], 64)) {
        configError_ = "Admin config file is corrupted: " + cfgPath_ + " (delete it to set a new password)";
        return;
    }
    saltHex_ = f[1]; hashHex_ = f[2]; configured_ = true;
}

bool AdminModule::hasPassword() const { return configured_; }

bool AdminModule::setupPassword(const std::string& password, std::string& error) {
    error.clear();
    if (!configError_.empty()) { error = configError_; return false; }
    if (configured_) { error = "An admin password is already set"; return false; }
    if (password.size() < 6) { error = "Password must be at least 6 characters"; return false; }
    for (char c : password) if (c == '\n' || c == '\r') { error = "Password must not contain line breaks"; return false; }

    std::random_device rd;
    std::string raw;
    for (int i = 0; i < 16; i++) raw += char(rd() & 0xff);
    std::string salt = toHex(raw), hash = hashPassword(salt, password);

    std::ofstream o(cfgPath_, std::ios::trunc);
    if (!o) { error = "Cannot write " + cfgPath_; return false; }
    o << "v1$" << salt << '$' << hash << '\n';
    o.flush();
    if (!o) { error = "Write failed for " + cfgPath_; return false; }
    saltHex_ = salt; hashHex_ = hash; configured_ = true;
    return true;
}

AdminModule::LoginResult AdminModule::login(const std::string& password) {
    if (locked_) return LoginResult::LockedOut;
    if (!configError_.empty()) return LoginResult::ConfigError;
    if (!configured_) return LoginResult::NotConfigured;
    if (sameString(hashPassword(saltHex_, password), hashHex_)) {
        failed_ = 0; loggedIn_ = true;
        return LoginResult::Ok;
    }
    loggedIn_ = false;
    if (++failed_ >= kMaxAttempts) locked_ = true;     // stays locked until the program restarts
    return LoginResult::WrongPassword;
}

void AdminModule::logout() { loggedIn_ = false; }

int AdminModule::attemptsLeft() const { return locked_ ? 0 : kMaxAttempts - failed_; }

bool AdminModule::requireLogin(std::string& error) const {
    if (loggedIn_) return true;
    error = "Admin login required";
    return false;
}

bool AdminModule::load(std::string& error) {
    error.clear();
    if (!requireLogin(error)) return false;
    return FileIOModule::loadQuestions(qPath_, bank_, warnings_, error);
}

std::vector<Question> AdminModule::view(const std::string& category, const std::string& difficulty) const {
    std::vector<Question> r;
    if (!loggedIn_) return r;
    const std::string c = lower(trim(category)), d = lower(trim(difficulty));
    for (const auto& q : bank_)
        if ((c.empty() || lower(q.category) == c) && (d.empty() || lower(q.difficulty) == d)) r.push_back(q);
    return r;
}

// Saves the proposed bank; memory is only updated if the save succeeded (memory and disk never disagree).
bool AdminModule::checkAndCommit(std::vector<Question> next, std::string& error) {
    if (!FileIOModule::saveQuestions(qPath_, next, error)) return false;
    bank_ = std::move(next);
    return true;
}

bool AdminModule::addQuestion(Question q, int& newId, std::string& error) {
    error.clear();
    if (!requireLogin(error)) return false;
    int maxId = 0;
    for (const auto& x : bank_) maxId = std::max(maxId, x.id);
    q.id = maxId + 1;
    normalize(q);
    std::string why = FileIOModule::validateQuestion(q);
    if (!why.empty()) { error = why; return false; }
    auto next = bank_;
    next.push_back(q);
    if (!checkAndCommit(std::move(next), error)) return false;
    newId = q.id;
    return true;
}

bool AdminModule::editQuestion(int id, Question q, std::string& error) {
    error.clear();
    if (!requireLogin(error)) return false;
    auto it = std::find_if(bank_.begin(), bank_.end(), [id](const Question& x) { return x.id == id; });
    if (it == bank_.end()) { error = "No question with ID " + std::to_string(id); return false; }
    q.id = id;
    normalize(q);
    std::string why = FileIOModule::validateQuestion(q);
    if (!why.empty()) { error = why; return false; }
    auto next = bank_;
    next[static_cast<size_t>(it - bank_.begin())] = q;
    return checkAndCommit(std::move(next), error);
}

bool AdminModule::deleteQuestion(int id, std::string& error) {
    error.clear();
    if (!requireLogin(error)) return false;
    auto it = std::find_if(bank_.begin(), bank_.end(), [id](const Question& x) { return x.id == id; });
    if (it == bank_.end()) { error = "No question with ID " + std::to_string(id); return false; }
    if (bank_.size() == 1) { error = "Cannot delete the last remaining question"; return false; }
    auto next = bank_;
    next.erase(next.begin() + (it - bank_.begin()));
    return checkAndCommit(std::move(next), error);
}

// ---------------- console panel ----------------

void AdminModule::runMenu(std::istream& in, std::ostream& out) {
    out << "\n=== Admin Panel ===\n";
    if (!configError_.empty()) { out << configError_ << "\n"; return; }

    if (!hasPassword()) {
        out << "No admin password is set yet. Create one (at least 6 characters).\n";
        std::string p1, p2, err;
        if (!ask(in, out, "New password: ", p1) || !ask(in, out, "Confirm password: ", p2)) return;
        if (p1 != p2) { out << "Passwords do not match.\n"; return; }
        if (!setupPassword(p1, err)) { out << err << "\n"; return; }
        out << "Password saved.\n";
        login(p1);
    }

    while (!loggedIn_) {
        if (locked_) { out << "Admin panel is locked until the application is restarted.\n"; return; }
        std::string pw;
        if (!ask(in, out, "Admin password: ", pw)) return;
        if (login(pw) == LoginResult::WrongPassword) {
            if (locked_) out << "Too many failed attempts. Admin panel is locked until the application is restarted.\n";
            else out << "Wrong password. " << attemptsLeft() << " attempt(s) left.\n";
        }
    }

    std::string err;
    if (!load(err)) { out << "Error: " << err << "\n"; logout(); return; }
    if (!warnings_.empty()) out << warnings_.size() << " malformed line(s) in the question file were skipped.\n";

    while (true) {
        out << "\n1. View questions\n2. Add question\n3. Edit question\n4. Delete question\n5. Back to main menu\n";
        std::string c;
        if (!ask(in, out, "Choice: ", c)) break;
        c = trim(c);
        if (c == "1") menuView(in, out);
        else if (c == "2") menuAdd(in, out);
        else if (c == "3") menuEdit(in, out);
        else if (c == "4") menuDelete(in, out);
        else if (c == "5") break;
        else out << "Invalid choice. Please enter a number from 1 to 5.\n";
    }
    logout();
}

void AdminModule::menuView(std::istream& in, std::ostream& out) {
    std::string cat, diff;
    if (!ask(in, out, "Filter by category (blank = all): ", cat)) return;
    if (!ask(in, out, "Filter by difficulty (blank = all): ", diff)) return;
    auto r = view(cat, diff);
    if (r.empty()) { out << "No questions match.\n"; return; }
    for (const auto& q : r) printQuestion(out, q);
    out << r.size() << " question(s).\n";
}

void AdminModule::menuAdd(std::istream& in, std::ostream& out) {
    Question q; std::string a, ans, err;
    if (!ask(in, out, "Category: ", q.category) || !ask(in, out, "Difficulty (Easy/Medium/Hard): ", q.difficulty) ||
        !ask(in, out, "Question: ", q.text) || !ask(in, out, "Option A: ", q.options[0]) ||
        !ask(in, out, "Option B: ", q.options[1]) || !ask(in, out, "Option C: ", q.options[2]) ||
        !ask(in, out, "Option D: ", q.options[3]) || !ask(in, out, "Correct answer (A-D): ", ans)) return;
    ans = trim(ans);
    q.correct = ans.size() == 1 ? ans[0] : '?';
    int id = 0;
    if (addQuestion(q, id, err)) out << "Question added with ID " << id << ".\n";
    else out << "Not saved: " << err << "\n";
}

void AdminModule::menuEdit(std::istream& in, std::ostream& out) {
    std::string s, err; int id = 0;
    if (!ask(in, out, "Question ID to edit: ", s)) return;
    if (!parseId(s, id)) { out << "Please enter a valid question ID.\n"; return; }
    auto it = std::find_if(bank_.begin(), bank_.end(), [id](const Question& x) { return x.id == id; });
    if (it == bank_.end()) { out << "No question with ID " << id << ".\n"; return; }
    Question q = *it;
    printQuestion(out, q);
    out << "Press Enter to keep the current value.\n";
    auto field = [&](const std::string& label, std::string& dst) {
        std::string v;
        if (!ask(in, out, label, v)) return false;
        if (!trim(v).empty()) dst = v;
        return true;
    };
    std::string ans;
    if (!field("Category: ", q.category) || !field("Difficulty: ", q.difficulty) || !field("Question: ", q.text) ||
        !field("Option A: ", q.options[0]) || !field("Option B: ", q.options[1]) ||
        !field("Option C: ", q.options[2]) || !field("Option D: ", q.options[3])) return;
    if (!ask(in, out, "Correct answer (A-D): ", ans)) return;
    ans = trim(ans);
    if (!ans.empty()) q.correct = ans.size() == 1 ? ans[0] : '?';
    if (editQuestion(id, q, err)) out << "Question " << id << " updated.\n";
    else out << "Not saved: " << err << "\n";
}

void AdminModule::menuDelete(std::istream& in, std::ostream& out) {
    std::string s, yn, err; int id = 0;
    if (!ask(in, out, "Question ID to delete: ", s)) return;
    if (!parseId(s, id)) { out << "Please enter a valid question ID.\n"; return; }
    if (!ask(in, out, "Delete question " + std::to_string(id) + "? (y/n): ", yn)) return;
    if (lower(trim(yn)) != "y") { out << "Cancelled.\n"; return; }
    if (deleteQuestion(id, err)) out << "Question " << id << " deleted.\n";
    else out << "Not deleted: " << err << "\n";
}
