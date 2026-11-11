// AdminModule.h - admin login + question-bank management
// QZ-F-030..034, QZ-F-041, QZ-SR-001, QZ-SR-003, QZ-SR-006
#pragma once
#include <iosfwd>
#include <string>
#include <vector>
#include "FileIOModule.h"

class AdminModule {
public:
    enum class LoginResult { Ok, WrongPassword, LockedOut, NotConfigured, ConfigError };
    static constexpr int kMaxAttempts = 3;   // SR-006

    // questionPath: e.g. "data/questions.txt"   configPath: e.g. "data/admin.cfg" (hashed password)
    AdminModule(std::string questionPath, std::string configPath);

    // ---- authentication (SR-001, SR-003, SR-006) ----
    bool hasPassword() const;                                            // false on first run
    bool setupPassword(const std::string& password, std::string& error); // first run only, min 6 chars
    LoginResult login(const std::string& password);
    void logout();
    bool isLoggedIn() const { return loggedIn_; }
    bool isLocked() const { return locked_; }                            // locked for the rest of the session
    int attemptsLeft() const;

    // ---- question bank (all require login; every change is saved to disk immediately) ----
    bool load(std::string& error);                                       // loads bank from file into memory
    const std::vector<std::string>& warnings() const { return warnings_; }
    // Empty category/difficulty = any. Case-insensitive. Returns {} if not logged in.
    std::vector<Question> view(const std::string& category, const std::string& difficulty) const;
    bool addQuestion(Question q, int& newId, std::string& error);        // ID assigned automatically
    bool editQuestion(int id, Question q, std::string& error);           // keeps the same ID
    bool deleteQuestion(int id, std::string& error);                     // refuses to delete the last question

    // Console admin panel. Called by MenuModule when the player picks "Admin".
    void runMenu(std::istream& in, std::ostream& out);

    // Exposed for unit tests.
    static std::string sha256Hex(const std::string& data);
    static std::string hashPassword(const std::string& saltHex, const std::string& password);

private:
    bool requireLogin(std::string& error) const;
    bool checkAndCommit(std::vector<Question> next, std::string& error);
    void loadConfig();
    void menuView(std::istream& in, std::ostream& out);
    void menuAdd(std::istream& in, std::ostream& out);
    void menuEdit(std::istream& in, std::ostream& out);
    void menuDelete(std::istream& in, std::ostream& out);

    std::string qPath_, cfgPath_;
    std::vector<Question> bank_;
    std::vector<std::string> warnings_;
    std::string saltHex_, hashHex_, configError_;
    bool configured_ = false, loggedIn_ = false, locked_ = false;
    int failed_ = 0;
};
