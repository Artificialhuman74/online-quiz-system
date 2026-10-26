# Tests

CI runs every test below on Linux, Windows and macOS for each pull request, and once more on
Linux with AddressSanitizer/UndefinedBehaviorSanitizer. CMake finds new test files
automatically, so you never need to edit `CMakeLists.txt` to add one.

| Level | Where | What it checks | CTest label |
|---|---|---|---|
| Unit | `tests/unit/test_<module>.cpp` | one module on its own, e.g. `InputValidation` | `unit` |
| Component | `tests/component/test_<module>.cpp` | modules working together, e.g. `MenuModule` + `InputValidation` | `component` |
| System | `tests/system/<name>.in` + `<name>.expect` | the real `quiz` program, end to end | `system` |

## Unit and component tests

Each `test_*.cpp` is its own program with a `main()`. It returns 0 when every check passes and
non-zero otherwise. Prefer a `CHECK` macro over `assert`, because `assert` is compiled out in
Release builds. See `tests/unit/test_input_validation.cpp` for the pattern.

## System tests

- `<name>.in` is fed to `quiz` as keyboard input, one line per Enter press.
- `<name>.expect` lists text that must appear in the output, in order: one snippet per line.
  Blank lines and lines starting with `#` are ignored.
- The program must exit with code 0. When the input runs out, it exits cleanly.

## Running locally

```
cmake -S . -B build
cmake --build build
ctest --test-dir build -C Debug --output-on-failure            # everything
ctest --test-dir build -C Debug -L unit --output-on-failure    # one level only
```
