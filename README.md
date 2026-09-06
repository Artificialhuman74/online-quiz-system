# Online Quiz System

A console-based (command-line) multiple-choice quiz game written in C++. Players are presented with multiple-choice questions, answers are validated in real time, and a final score/results summary is generated at the end of each attempt.

## Team

4-member team project. Roles: Team Lead / Project Coordinator, Developer (Quiz Engine and Scoring), Developer (Question Bank and Admin Module), and QA and Documentation Lead. Names to be added by the team.

## Planned features

Main menu (Start Quiz, Leaderboard/History, Admin panel, Exit); category and difficulty selection before a quiz starts; multiple-choice questions with shuffled answer order per attempt; score calculation with a results summary and grade/remark; persistent question bank and results history in local files; password-protected admin workflow to add, edit, delete and view questions.

See the project's SRS (Software Requirements Specification) document for the full functional and non-functional requirements.

## Project structure

src holds the C++ source files, data holds the question bank and results data files, and docs holds the SRS and other design docs.

## Building

Requires a C++17 (or later) compiler. Example: g++ -std=c++17 -o quiz src/main.cpp

## Status

Repository and requirements setup in progress. Implementation has not started yet.
