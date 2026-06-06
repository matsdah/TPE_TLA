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

## Player Controls

| Input | Action |
|-------|--------|
| `SPACE` or left-click | Advance dialogue (when no choices are active) |
| `1` – `9` | Select choice option by number |
| Mouse hover + click on choice rectangle | Select choice option |
| `F5` | Save game to `.build/save.json` (single slot, silent overwrite) |
| `F9` | Load game from `.build/save.json` (refused if story source mismatches) |

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
- The suite includes a **player smoke-test** (compiles `06-graphics` and runs the player under `xvfb-run` when headless) to verify the player binary does not crash.

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
- Installs `bison cmake flex gcc g++ git make xvfb` plus `libglfw3-dev libx11-dev libxrandr-dev libxi-dev libxinerama-dev libxcursor-dev libgl1-mesa-dev` for Raylib compilation, makes bash scripts executable, then runs `build.sh`, `test.sh`, and a dedicated `"Player test."` step.

## Gotchas

- The scanner supports a `load "filename" ;` directive (see `FlexPatterns.l` start condition `LOAD_FILE`). This loads external story files; keep it in mind when testing cross-file features.
- `BisonGrammar.y` uses `%define api.push-pull push`, `%define api.pure full`, and `%destructor` rules for every allocated semantic value. Do not change those directives without understanding the memory-ownership model.
- Raylib requires a display at runtime. In a headless Docker/CI environment, use `xvfb-run` to launch the player, or only compile-test it.
- `Frontend.c` intentionally comments out `yy_delete_buffer` in `destroyInputBuffer` to avoid a double-free; leaving it commented leaks memory only on syntax errors inside secondary input buffers.
- **`play.sh` auto-detects headless environments:** When `DISPLAY` is missing and `xvfb-run` is available, the script wraps the player automatically. No manual `xvfb-run` needed.
- **Player teardown order is critical:** `Engine_destroy` must run **before** `CloseAudioDevice()` and `CloseWindow()` or AddressSanitizer catches leaks. Raylib's `UnloadTexture`/`UnloadMusicStream` need active GL/audio contexts to free resources.
- **Asset path resolution:** Asset paths in stories are resolved relative to `.build/story.json`'s directory (`.build/`). Stories that reference repo-root assets need a `../` prefix (e.g., `asset bg = "../src/test/assets/img/background.png"`).
- **Test assets:** `src/test/assets/` contains generated PNG/WAV files and `test_with_assets.story` for manual visual/audio verification.
- **Save/Load system:** Press `F5` to write `.build/save.json`, `F9` to load. The save includes variables, context stack, media state, and pending dialogue/choice. Saves are bound to the story source (from `meta.source` in `story.json`); loading a save from a different story is refused.
- **Pointer-based context stack:** The engine tracks execution via raw `StatementList *` / `Statement *` pointers. Save/load uses stable string paths (`scene:<name>/stmt:<i>/then`, `.../opt:<j>`) to resolve pointers after process restart. Any structural change to the story (added/removed statements) invalidates existing saves.
- **Media helpers in Engine.c:** `_applyShowBackground`, `_applyHideSprite`, `_applyPlayMusic`, etc. are reusable helpers used by both `_stepStatement` and `Engine_loadState` to apply media state without executing story statements.
- **Player init order is critical:** `InitWindow` must run **before** `Engine_create` because `LoadTexture` needs an active OpenGL context. `InitAudioDevice` runs after `InitWindow`.
- **Context stack limit:** `ENGINE_MAX_CONTEXTS` = 64; overflow silently sets `finished = true` and terminates the story. Deeply nested `choice` → `if` → `goto` chains can hit this.
- **Expressions are integer-only:** The DSL has no strings, booleans, or logical operators (`&&`, `||`, `!`). Conditions are numeric comparisons only.
- **Asset type mismatch is NOT validated:** The semantic analyzer does not check that an image asset is used with `show`/`hide` or an audio asset with `play`/`stop`.
- **`examples/demo.story`:** Exercises every language construct; use it for manual verification of compiler + player end-to-end.
- **Test asset generation:** `generate_test_assets.py` creates the PNG/WAV files in `src/test/assets/`.
