# Agent Instructions — TPE_TLA

Compact reference for AI sessions working in this repo. Skip anything that is obvious from filenames.

## Project

C compiler for a visual-novel / story DSL, built with **Flex + Bison + CMake + GCC**. Now includes a **Raylib-based player** (`Flex-Bison-Player`) that reads `.build/story.json` and renders the story interactively.

- **Compiler entry point:** `src/main/c/EntryPoint.c`
- **Player entry point:** `src/main/c/player/PlayerEntryPoint.c`
- **Compiler phases:** Lexical analysis → Syntactic analysis → **Semantic analysis** → Code generation (JSON). The semantic analyzer validates undeclared actors/assets/scenes, duplicate declarations, and type mismatches.
- **Compiler modules:**
  - `src/main/c/frontend/semantic-analysis/SemanticAnalyzer.c` — validates the AST before code generation (actors, assets, scenes, variables, expressions).
- **Player runtime modules:**
  - `src/main/c/player/runtime/StoryLoader.c` — parses `.build/story.json` into an in-memory runtime model (`Story`, `Scene`, `Statement`, etc.).
  - `src/main/c/player/runtime/ExpressionEvaluator.c` — evaluates integer expressions and conditions against the runtime variable table.
  - `src/main/c/player/runtime/Engine.c` — drives the story via a context-stack state machine; handles `dialogue`, `choice`, `if/else`, `goto`, `set`, and all media commands.
  - `src/main/c/player/vendor/cJSON.c` — vendored MIT-licensed JSON parser (no external dependency).
- **Generated code:** `FlexScanner.c/h` (from `FlexPatterns.l`) and `BisonParser.c/h` (from `BisonGrammar.y`) live under `src/main/c/frontend/` but are `.gitignore`d. Do not edit them by hand.

## Build

Use the wrapper script; do not invoke `cmake` / `make` directly unless you know the generated files are already present.

```bash
# Full clean build (deletes generated sources, regenerates them, compiles)
src/main/bash/build.sh
```

- The script **always deletes** `FlexScanner.*` and `BisonParser.*` before running CMake, so regeneration is mandatory.
- CMake itself enforces **Bison before Flex** because Flex is invoked with `--bison-bridge --bison-locations`.
- GCC-only flags in `CMakeLists.txt`: `-fsanitize=address -O3 -static-libgcc -std=gnu99`. The `else` branch is a no-op.
- **Raylib** is fetched automatically via CMake `FetchContent` (tag `5.5`) and linked to both binaries. It inherits the global compile flags.

**Adding a new `.c` file:** You must append it to the `add_executable` list in `CMakeLists.txt`. New sources are not auto-discovered. The compiler and player have separate `add_executable` blocks.

## Run

```bash
# Compile a program (reads from stdin, writes .build/story.json when given a source-path argument)
src/main/bash/run.sh <program-file>

# Play the compiled story with the Raylib visual-novel player
src/main/bash/play.sh [.build/story.json]
```

## Test

All tests are **integration tests** — no C unit-test framework is used.

```bash
# Run the full suite (accept files must exit 0, reject files must exit non-zero; also checks story.json generation)
src/main/bash/test.sh
```

- **There is no single-test script.** To run one test manually:
  ```bash
  cat src/test/c/accept/01-linear-scene | .build/Flex-Bison-Compiler src/test/c/accept/01-linear-scene
  ```
- The suite also verifies that `.build/story.json` is non-empty after compiling `src/test/c/accept/01-linear-scene`.

## Docker Dev Environment

```bash
# Start an ephemeral dev container
docker compose run --rm compiler

# Shutdown afterwards (outside the container)
docker compose down
```

- Compose file: `compose.yaml`
- Dockerfile: `src/main/docker/compiler/Dockerfile` (Ubuntu 24.04 + bison cmake flex gcc g++ git make + X11/OpenGL dev headers for Raylib)
- Optional `.env` file is supported (not required) for `ENVIRONMENT`, `LOG_IGNORED_LEXEMES`, `LOGGING_LEVEL`.

## CI / CD

- Workflow: `.github/workflows/pipeline.yaml`
- Triggers on every `push` and on PR `closed` / `reopened`.
- Installs `bison cmake flex gcc g++ git make` plus `libglfw3-dev libx11-dev libxrandr-dev libxi-dev libxinerama-dev libxcursor-dev libgl1-mesa-dev` for Raylib compilation, makes bash scripts executable, then runs `build.sh` followed by `test.sh`.

## Gotchas

- The scanner supports a `load "filename" ;` directive (see `FlexPatterns.l` start condition `LOAD_FILE`). This loads external story files; keep it in mind when testing cross-file features.
- `BisonGrammar.y` uses `%define api.push-pull push`, `%define api.pure full`, and `%destructor` rules for every allocated semantic value. Do not change those directives without understanding the memory-ownership model.
- Raylib requires a display at runtime. In a headless Docker/CI environment, use `xvfb-run` to launch the player, or only compile-test it.
- `Frontend.c` intentionally comments out `yy_delete_buffer` in `destroyInputBuffer` to avoid a double-free; leaving it commented leaks memory only on syntax errors inside secondary input buffers.
