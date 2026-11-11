// Self-test. From the repo root:
//   g++ -std=c++17 -o ta src/test_admin.cpp src/AdminModule.cpp src/FileIOModule.cpp && ./ta   (ta on Windows)
#include "AdminModule.h"
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
using LR = AdminModule::LoginResult;

static int failures = 0;
#define CHECK(cond)                                                              \
    do {                                                                         \
        if (!(cond)) { std::cerr << __FILE__ << ':' << __LINE__ << ": FAILED: " #cond "\n"; failures++; } \
    } while (0)

static std::string slurp(const std::string& p) {
    std::ifstream in(p, std::ios::binary); std::stringstream ss; ss << in.rdbuf(); return ss.str();
}
static Question mk(int id, const char* cat, const char* diff) {
    Question q; q.id = id; q.category = cat; q.difficulty = diff; q.text = "Q" + std::to_string(id) + "?";
    q.options[0] = "a"; q.options[1] = "b"; q.options[2] = "c"; q.options[3] = "d"; q.correct = 'C';
    return q;
}
static const char* QP = "t_admin_q.txt";
static const char* CP = "t_admin.cfg";

static void reset() {
    std::remove(CP);
    std::string e;
    FileIOModule::saveQuestions(QP, {mk(1, "Math", "Easy"), mk(2, "Science", "Hard"), mk(5, "Math", "Hard")}, e);
}

int main() {
    // ---- SHA-256 known-answer test ----
    CHECK(AdminModule::sha256Hex("abc") == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    CHECK(AdminModule::sha256Hex("") == "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");

    reset();
    std::string err;
    {   // ---- first run / password setup (SR-003) ----
        AdminModule a(QP, CP);
        CHECK(!a.hasPassword() && a.login("x") == LR::NotConfigured);
        CHECK(!a.setupPassword("abc", err) && !err.empty());                 // too short
        CHECK(a.setupPassword("secret1", err) && a.hasPassword());
        CHECK(!a.setupPassword("another1", err));                            // can't overwrite
        std::string cfg = slurp(CP);
        CHECK(cfg.rfind("v1$", 0) == 0 && cfg.find("secret1") == std::string::npos);   // hashed, not plaintext
        AdminModule b(QP, CP);
        CHECK(b.hasPassword());                                              // persists across restarts
    }
    {   // ---- SR-001: nothing works before login; file untouched ----
        AdminModule a(QP, CP);
        const std::string before = slurp(QP);
        int id = 0;
        CHECK(!a.load(err) && err.find("login") != std::string::npos);
        CHECK(!a.addQuestion(mk(0, "Math", "Easy"), id, err));
        CHECK(!a.editQuestion(1, mk(1, "Math", "Easy"), err));
        CHECK(!a.deleteQuestion(1, err));
        CHECK(a.view("", "").empty());
        CHECK(slurp(QP) == before);
    }
    {   // ---- SR-006: 3 wrong = locked until restart ----
        AdminModule a(QP, CP);
        CHECK(a.login("no1") == LR::WrongPassword && a.attemptsLeft() == 2);
        CHECK(a.login("no2") == LR::WrongPassword && a.attemptsLeft() == 1);
        CHECK(a.login("no3") == LR::WrongPassword && a.isLocked() && a.attemptsLeft() == 0);
        CHECK(a.login("secret1") == LR::LockedOut && !a.isLoggedIn());       // 4th attempt blocked, even if correct
        AdminModule b(QP, CP);                                                // "restart"
        CHECK(b.login("secret1") == LR::Ok && b.isLoggedIn());
    }
    {   // ---- consecutive means consecutive ----
        AdminModule a(QP, CP);
        CHECK(a.login("x") == LR::WrongPassword && a.login("y") == LR::WrongPassword);
        CHECK(a.login("secret1") == LR::Ok);
        a.logout();
        CHECK(a.login("x") == LR::WrongPassword && a.login("y") == LR::WrongPassword && !a.isLocked());
        CHECK(a.login("secret1") == LR::Ok);
    }
    {   // ---- CRUD (F-031/032/033/034/041) ----
        AdminModule a(QP, CP);
        CHECK(a.login("secret1") == LR::Ok && a.load(err));
        CHECK(a.view("", "").size() == 3);
        CHECK(a.view("math", "").size() == 2 && a.view("", "HARD").size() == 2 && a.view("Math", "Hard").size() == 1);
        CHECK(a.view("History", "").empty());

        int id = 0;                                                           // add
        Question n = mk(0, "  History ", "medium"); n.correct = 'b';
        CHECK(a.addQuestion(n, id, err) && id == 6);                         // max ID 5 + 1
        std::vector<Question> disk; std::vector<std::string> w;
        CHECK(FileIOModule::loadQuestions(QP, disk, w, err) && disk.size() == 4);   // saved immediately
        CHECK(disk.back().id == 6 && disk.back().category == "History" && disk.back().difficulty == "Medium" && disk.back().correct == 'B');

        const std::string before = slurp(QP);                                 // invalid adds change nothing
        Question bad = mk(0, "Math", "Easy"); bad.options[1] = "";
        CHECK(!a.addQuestion(bad, id, err) && !err.empty());
        bad = mk(0, "Math", "Easy"); bad.correct = 'Z';
        CHECK(!a.addQuestion(bad, id, err));
        bad = mk(0, "Math", "Easy"); bad.text = "a|b";
        CHECK(!a.addQuestion(bad, id, err));
        bad = mk(0, "Math", "Easy"); bad.difficulty = "Insane";
        CHECK(!a.addQuestion(bad, id, err));
        CHECK(slurp(QP) == before && a.view("", "").size() == 4);

        Question e = mk(0, "Math", "Easy"); e.text = "Edited?";                // edit
        CHECK(a.editQuestion(2, e, err));
        CHECK(FileIOModule::loadQuestions(QP, disk, w, err));
        bool found = false; for (auto& q : disk) if (q.id == 2) { found = true; CHECK(q.text == "Edited?" && q.category == "Math"); }
        CHECK(found);
        CHECK(!a.editQuestion(99, e, err) && err.find("99") != std::string::npos);
        e.options[0] = "";
        CHECK(!a.editQuestion(2, e, err));

        CHECK(a.deleteQuestion(6, err));                                     // delete
        CHECK(FileIOModule::loadQuestions(QP, disk, w, err) && disk.size() == 3);
        CHECK(!a.deleteQuestion(6, err) && !a.deleteQuestion(99, err));
        CHECK(a.deleteQuestion(1, err) && a.deleteQuestion(2, err));
        const std::string last = slurp(QP);
        CHECK(!a.deleteQuestion(5, err) && err.find("last") != std::string::npos);   // keeps at least 1
        CHECK(slurp(QP) == last);
    }

    // ---- console panel ----
    reset();
    {   // first run: create password, view all, back
        AdminModule a(QP, CP);
        std::istringstream in("secret1\nsecret1\n1\n\n\n5\n"); std::ostringstream out;
        a.runMenu(in, out);
        CHECK(out.str().find("Password saved") != std::string::npos && out.str().find("3 question(s)") != std::string::npos);
        CHECK(!a.isLoggedIn());                                              // logged out on exit
    }
    {   // lockout via console
        AdminModule a(QP, CP);
        std::istringstream in("a\nb\nc\nsecret1\n"); std::ostringstream out;
        a.runMenu(in, out);
        CHECK(out.str().find("locked") != std::string::npos && !a.isLoggedIn());
        std::istringstream in2("secret1\n"); std::ostringstream out2;
        a.runMenu(in2, out2);                                                 // still locked this session
        CHECK(out2.str().find("locked") != std::string::npos);
    }
    {   // invalid choices don't crash; add via menu; EOF ends cleanly
        AdminModule a(QP, CP);
        std::istringstream in("secret1\nabc\n9\n\n2\nGeography\nEasy\nCapital of Italy?\nParis\nRome\nMadrid\nBerlin\nB\n4\nxyz\n4\n7\nn\n");
        std::ostringstream out;
        a.runMenu(in, out);                                                   // ends at EOF
        CHECK(out.str().find("Invalid choice") != std::string::npos && out.str().find("added with ID 6") != std::string::npos);
        CHECK(out.str().find("valid question ID") != std::string::npos && out.str().find("Cancelled") != std::string::npos);
    }

    std::remove(QP); std::remove(CP);
    if (failures) { std::cerr << failures << " AdminModule check(s) failed\n"; return 1; }
    std::cout << "All AdminModule tests passed\n";
    return 0;
}
