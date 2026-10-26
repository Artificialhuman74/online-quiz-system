# Online Quiz System

A console-based (command-line) multiple-choice quiz game written in C++. Players are presented with multiple-choice questions, answers are validated in real time, and a final score/results summary is generated at the end of each attempt.

## Team

**Team Lead / Project Coordinator** - Chiranth R. (PES1UG24AM354)

**Developer, Quiz Engine and Scoring** - Abdul Mateen Shaikh (PES1UG24AM332)

**Developer, Question Bank and Admin Module** - Tanvi B. Shetty (PES1UG24AM302)

**QA and Documentation Lead** - Ashvin Ronald Lobo (PES1UG24AM345).

## Planned features

Main menu (Start Quiz, Leaderboard/History, Admin panel, Exit); category and difficulty selection before a quiz starts; multiple-choice questions with shuffled answer order per attempt; score calculation with a results summary and grade/remark; persistent question bank and results history in local files; password-protected admin workflow to add, edit, delete and view questions.

See the project's SRS (Software Requirements Specification) document in `docs/` for the full functional and non-functional requirements.

## Project structure

```
src/        C++ sources, one .h/.cpp pair per module, plus main.cpp
tests/      unit/, component/ and system/ tests (see tests/README.md)
data/       question bank (questions.txt); results.txt is created at runtime
docs/       SRS, test plan, UML diagrams, design docs
```

Modules (QZ-NF-004): `InputValidation`, `MenuModule`, `FileIOModule`, `QuizEngine`, `ScoringModule`, `AdminModule`. `main.cpp` only wires them together.

## Building

Requires a C++17 compiler (g++ 8+, clang 7+, or MSVC 2019+). Run all commands from the repository root, because the program reads `data/questions.txt` relative to the current directory.

**With CMake (recommended, works on Windows, Linux and macOS):**

```
cmake -S . -B build
cmake --build build
./build/quiz
ctest --test-dir build -C Debug --output-on-failure
```

On Windows (Visual Studio generator), run `build\Debug\quiz.exe` instead of `./build/quiz`.

**With g++ only (Linux, macOS, or Git Bash on Windows; not `cmd`):**

```
g++ -std=c++17 -Isrc -o quiz src/main.cpp $(ls src/*.cpp | grep -v -e main.cpp -e test_)
./quiz
```

In Windows `cmd` or PowerShell, use the CMake commands above, or list the files yourself: `g++ -std=c++17 -Isrc -o quiz.exe src/main.cpp src/<Module>.cpp ...`

To build and run one test, for example: `g++ -std=c++17 -Isrc -o t tests/unit/test_<module>.cpp src/<Module>.cpp && ./t`

CMake picks up any new `src/*.cpp` and any test under `tests/unit`, `tests/component` or `tests/system` automatically, so you do not need to edit `CMakeLists.txt` when you add a module or a test. See `tests/README.md`.

## Workflow

`main` is the only permanent branch. All work happens on short-lived branches that are merged by pull request:

- `feature/<name>` for code, `test/<name>` for tests, `docs/<name>` for documents.
- Branch from the latest `main`. Keep a branch to 1–2 days of work.
- Open a PR into `main` and request a review from at least one teammate. Merge only after an approval and a green CI run (GitHub Actions builds and tests on all three OSes).
- Delete the branch after it is merged.

## Status

Repository structure, build system and CI are set up. Module implementation is in progress.
