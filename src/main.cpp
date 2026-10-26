// main.cpp - entry point for the Online Quiz System.
//
// Modules (QZ-NF-004), each in its own src/<Module>.h/.cpp pair:
//   InputValidation - console input checks and re-prompting (QZ-F-012, QZ-F-042)
//   MenuModule      - main menu, category and difficulty selection (QZ-F-001..003)
//   FileIOModule    - question bank and results persistence (QZ-F-023..025, QZ-F-040/041)
//   QuizEngine      - question selection, shuffle, answer check, skip (QZ-F-004, QZ-F-010..015)
//   ScoringModule   - score, results summary, grade (QZ-F-020..022)
//   AdminModule     - password-protected question management (QZ-F-030..034, QZ-SR-001/003/006)
//
// This file only wires the modules together; keep game logic out of it.
#include <iostream>

int main() {
    std::cout << "=== Online Quiz System ===\n";
    // TODO(feature/integration): load the bank, show the main menu, dispatch to modules.
    return 0;
}
