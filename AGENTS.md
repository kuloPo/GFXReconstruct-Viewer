# AGENTS.md

## Code Style

- No excessive defensive programming: do not write boundary or null checks that can never trigger.
- No meaningless comments, such as ones that merely restate what the code does.
- Do not wrap lines in code; keep each statement on a single line.
- Only ASCII characters may be written into source files.

## Logging

- `LOGW` shows a popup dialog, so log messages must be user-facing (written for end users to read).
- `LOGE` also shows a popup and then aborts the program; use it only for internal program errors. Problems caused by the user should use `LOGW`.
