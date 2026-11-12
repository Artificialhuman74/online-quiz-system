# Requirements Traceability Matrix (RTM)

**Project:** Online Quiz System  
**Document Version:** 1.0  
**Associated SRS:** `docs/Online_Quiz_System_SRS.pdf` (v1.0)  
**Associated Test Plan:** `docs/Online_Quiz_System_Test_Plan.pdf` (v1.1)  
**Date:** November 2026  

---

## 1. Overview & Verification Strategy

The Requirements Traceability Matrix (RTM) establishes forward and backward traceability across:
- **System Requirements**: Functional (`QZ-F-*`), Non-Functional (`QZ-NF-*`), and Security Requirements (`QZ-SR-*`).
- **Software Modules**: Architectural modules implementing the requirement.
- **Design Specifications & Epics**: Corresponding Jira Epics and User Stories (`OQSSMP-*`).
- **Test Cases & Test Implementations**: Verification test cases defined in the Software Test Plan and implemented in CTest suites (Unit, Component, System, and CI builds).

### Status Legend
- **Passed**: Fully implemented and passing automated tests in the test suite.
- **Partial**: Implemented or verified in part (e.g. basic unit handling exists, but full dedicated NFR test or manual validation is planned).
- **Pending**: Planned for subsequent sprint / phase.
- **Not Required**: Explicitly excluded from the accepted project scope.

---

## 2. Traceability Matrix

| Req ID | Category | Requirement Description | Module / Subsystem | Test Case ID | Test Level & Location | Jira Issue | Status |
|---|---|---|---|---|---|---|---|
| **QZ-F-001** | Functional | Display main menu with Start Quiz, Leaderboard/History, Admin Panel, and Exit options | `MenuModule` | `TC-Menu-01` | Component (`tests/component/test_menu.cpp`), System (`tests/system/menu_exit.*`) | `OQSSMP-11` | **Passed** |
| **QZ-F-002** | Functional | Allow player to select quiz category before starting | `MenuModule` | `TC-Menu-02` | Component (`tests/component/test_menu.cpp`), System (`tests/system/quiz_setup.*`) | `OQSSMP-11` | **Passed** |
| **QZ-F-003** | Functional | Allow player to select quiz difficulty (Easy, Medium, Hard, Any) | `MenuModule` | `TC-Menu-03` | Component (`tests/component/test_menu.cpp`), System (`tests/system/quiz_setup.*`) | `OQSSMP-11` | **Passed** |
| **QZ-F-004** | Functional | Randomly select configurable number of questions (default 10) without repetition within attempt | `QuizEngine` | `TC-Menu-04` | Unit (`tests/unit/test_quiz_engine.cpp`) | `OQSSMP-11` | **Passed** |
| **QZ-F-010** | Functional | Display one MCQ at a time with exactly 4 options labeled A–D | `QuizEngine` | `TC-Quiz-01` | Unit (`tests/unit/test_quiz_engine.cpp`) | `OQSSMP-16` | **Passed** |
| **QZ-F-011** | Functional | Shuffle the order of answer options for each question on every attempt | `QuizEngine` | `TC-Quiz-02` | Unit (`tests/unit/test_quiz_engine.cpp`) | `OQSSMP-16` | **Passed** |
| **QZ-F-012** | Functional | Accept answer as single character (A–D); re-prompt on invalid input without score penalty | `InputValidation`, `QuizEngine` | `TC-Quiz-03` | Unit (`tests/unit/test_input_validation.cpp`), System (`tests/system/menu_invalid_input.*`) | `OQSSMP-12`, `OQSSMP-26` | **Passed** |
| **QZ-F-013** | Functional | Enforce optional per-question time limit; mark unanswered as incorrect | `QuizEngine` | N/A | Not applicable | `OQSSMP-23` | **Not Required** |
| **QZ-F-014** | Functional | Allow player to skip question; mark as unattempted in summary | `InputValidation`, `QuizEngine` | `TC-Quiz-05` | Unit (`tests/unit/test_quiz_engine.cpp`, `tests/unit/test_input_validation.cpp`) | `OQSSMP-12` | **Passed** |
| **QZ-F-015** | Functional | Display immediate feedback (Correct/Incorrect) after each answer when enabled | `QuizEngine` | `TC-Quiz-06` | Unit (`tests/unit/test_quiz_engine.cpp`) | `OQSSMP-16` | **Passed** |
| **QZ-F-020** | Functional | Calculate total score as count of correct answers at end of quiz | `ScoringModule` | `TC-Score-01` | Unit (`tests/unit/test_scoring.cpp`) | `OQSSMP-18`, `OQSSMP-25` | **Passed** |
| **QZ-F-021** | Functional | Display results summary showing total questions, correct, incorrect, skipped, and percentage | `ScoringModule` | `TC-Score-02` | Unit (`tests/unit/test_scoring.cpp`) | `OQSSMP-18`, `OQSSMP-25` | **Passed** |
| **QZ-F-022** | Functional | Assign grade/remark (Excellent, Good, Needs Improvement) based on percentage score | `ScoringModule` | `TC-Score-03` | Unit (`tests/unit/test_scoring.cpp`) | `OQSSMP-18`, `OQSSMP-25` | **Passed** |
| **QZ-F-023** | Functional | Save completed attempt (player, category, score, total, timestamp) to persistent results file | `FileIOModule` | `TC-Score-04` | Unit (`tests/unit/test_fileio.cpp`) | `OQSSMP-20` | **Passed** |
| **QZ-F-024** | Functional | Display leaderboard of top scores on request | `FileIOModule`, `MenuModule` | `TC-Score-05` | System (`tests/system/results_display.*`) | `OQSSMP-20` | **Passed** |
| **QZ-F-025** | Functional | Allow player to view own past attempt history | `FileIOModule`, `MenuModule` | `TC-Score-06` | System (`tests/system/results_display.*`) | `OQSSMP-20` | **Passed** |
| **QZ-F-030** | Functional | Provide password-protected admin menu to manage question bank | `AdminModule` | `TC-Admin-01` | Unit (`tests/unit/test_admin.cpp`), System (`tests/system/admin_panel.*`) | `OQSSMP-31` | **Passed** |
| **QZ-F-031** | Functional | Admin can add new question with 4 options, correct answer, category, and difficulty | `AdminModule`, `FileIOModule` | `TC-Admin-02` | Unit (`tests/unit/test_admin.cpp`), System (`tests/system/admin_panel.*`) | `OQSSMP-32` | **Passed** |
| **QZ-F-032** | Functional | Admin can edit or delete an existing question by ID | `AdminModule`, `FileIOModule` | `TC-Admin-03` | Unit (`tests/unit/test_admin.cpp`), System (`tests/system/admin_panel.*`) | `OQSSMP-32` | **Passed** |
| **QZ-F-033** | Functional | Admin can view all questions, optionally filtered by category or difficulty | `AdminModule` | `TC-Admin-04` | System (`tests/system/admin_filter.*`, `tests/system/admin_panel.*`) | `OQSSMP-32` | **Passed** |
| **QZ-F-034** | Functional | Validate new/edited question has exactly 1 valid correct answer and non-empty options | `FileIOModule` | `TC-Admin-05` | Unit (`tests/unit/test_fileio.cpp`) | `OQSSMP-14` | **Passed** |
| **QZ-F-040** | Functional | Load question bank from local file at startup; clear error if missing/corrupt | `FileIOModule` | `TC-Data-01` | Unit (`tests/unit/test_fileio.cpp`), System (`tests/system/startup.*`) | `OQSSMP-14` | **Passed** |
| **QZ-F-041** | Functional | Persist question bank changes atomically immediately after edit | `FileIOModule` | `TC-Data-02` | Unit (`tests/unit/test_fileio.cpp`) | `OQSSMP-14` | **Passed** |
| **QZ-F-042** | Functional | Handle non-numeric and out-of-range input without crashing; re-prompt user | `InputValidation` | `TC-Data-03` | Unit (`tests/unit/test_input_validation.cpp`), System (`tests/system/menu_invalid_input.*`) | `OQSSMP-24`, `OQSSMP-26` | **Passed** |
| **QZ-NF-001** | Non-Functional | Load question bank and display first screen within 2 seconds | `FileIOModule`, `QuizEngine` | `TC-Perf-01` | Unit (`tests/unit/test_nfr.cpp`), System (`tests/system/run_system_test.cmake`) | `OQSSMP-23` | **Passed** |
| **QZ-NF-002** | Non-Functional | Atomic file operations prevent data loss/corruption on unexpected termination | `FileIOModule` | N/A | Not applicable | `OQSSMP-24` | **Not Required** |
| **QZ-NF-003** | Non-Functional | Clear, unambiguous prompts and error messages for seamless user experience | `InputValidation`, `MenuModule` | N/A | Not applicable | `OQSSMP-24` | **Not Required** |
| **QZ-NF-004** | Non-Functional | Modular code architecture (InputValidation, Menu, QuizEngine, Scoring, FileIO) | Source Structure | `TC-Maint-01` | Code Reviews, Architecture & Component Diagram | `OQSSMP-29` | **Passed** |
| **QZ-NF-005** | Non-Functional | Cross-platform build on Windows, Linux, and macOS using standard C++17 | Build System | `TC-Port-01` | CI Matrix (`.github/workflows/ci.yml`) | `OQSSMP-24` | **Passed** |
| **QZ-NF-006** | Non-Functional | Adding new categories or questions via data file requires zero code changes | `FileIOModule`, `MenuModule` | `TC-Maint-02` | Unit (`tests/unit/test_nfr.cpp`, `tests/unit/test_fileio.cpp`) | `OQSSMP-14` | **Passed** |
| **QZ-SR-001** | Security | Require valid admin password before granting question bank modification access | `AdminModule` | `TC-Sec-01` | Unit (`tests/unit/test_admin.cpp`) | `OQSSMP-31` | **Passed** |
| **QZ-SR-002** | Security | Do not reveal correct answer on screen before player submits response | `QuizEngine` | `TC-Sec-02` | Unit (`tests/unit/test_quiz_engine.cpp`) | `OQSSMP-22` | **Passed** |
| **QZ-SR-003** | Security | Store admin password in non-plaintext form (salted hash) | `AdminModule` | `TC-Sec-03` | Unit (`tests/unit/test_admin.cpp`) | `OQSSMP-31` | **Passed** |
| **QZ-SR-004** | Security | Validate and sanitize all file inputs; skip malformed records without crash | `FileIOModule` | `TC-Sec-04` | Unit (`tests/unit/test_fileio.cpp`) | `OQSSMP-21` | **Passed** |
| **QZ-SR-005** | Security | Question-bank write access restricted to admin workflow; no player edit path | Architecture / UI | `TC-Sec-05` | Architecture verification, Code Review | `OQSSMP-22` | **Passed** |
| **QZ-SR-006** | Security | Lock out admin menu after 3 consecutive failed password attempts | `AdminModule` | `TC-Sec-06` | Unit (`tests/unit/test_admin.cpp`) | `OQSSMP-31` | **Passed** |

---

## 3. Summary Metrics

- **Total Documented Requirements:** 36
  - Functional (`QZ-F-*`): 24
  - Non-Functional (`QZ-NF-*`): 6
  - Security (`QZ-SR-*`): 6
- **Test Implementation Status:**
  - **Passed (Verified with automated tests):** 33 requirements (91.7%)
  - **Partial:** 0 requirements (0%)
  - **Not Required:** 3 requirements (8.3%) — `QZ-F-013`, `QZ-NF-002`, `QZ-NF-003`
