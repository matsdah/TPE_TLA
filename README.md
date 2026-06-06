[![✗](https://img.shields.io/badge/Release-v2.0.0-ffb600.svg?style=for-the-badge)](https://github.com/matsdah/TPE_TLA/releases)

[![✗](https://github.com/matsdah/TPE_TLA/actions/workflows/pipeline.yaml/badge.svg?branch=development)](https://github.com/matsdah/TPE_TLA/actions/workflows/pipeline.yaml)

# Flex-Bison-Compiler & Visual Novel Player

> **Trabajo Práctico Especial — Teoría de Lenguajes y Autómatas**  
> Universidad Tecnológica Nacional (UTN) — Facultad Regional Buenos Aires

This repository contains a complete compiler and runtime system for a **visual-novel domain-specific language (DSL)**. The project is built with **Flex** (lexical analysis), **Bison** (syntactic analysis), **CMake** (build), and **Raylib** (interactive player). It demonstrates the full compiler pipeline: lexing → parsing → semantic analysis → code generation (JSON) → runtime execution.

---

## Table of Contents

- [Quick Start](#quick-start)
- [DSL Language Reference](#dsl-language-reference)
- [Project Architecture](#project-architecture)
- [Build & Run](#build--run)
- [Testing](#testing)
- [JSON Output Schema](#json-output-schema)
- [Docker Development Environment](#docker-development-environment)
- [CI / CD](#ci--cd)
- [Troubleshooting](#troubleshooting)

---

## Quick Start

Three commands to go from source to a playable visual novel:

```bash
# 1. Build the compiler and the Raylib player
src/main/bash/build.sh

# 2. Compile a story file to .build/story.json
src/main/bash/run.sh examples/demo.story

# 3. Play the compiled story
src/main/bash/play.sh
```

The `examples/demo.story` file exercises **every language construct** (actors, assets, scenes, dialogue, choices, conditions, arithmetic, goto, and multimedia commands).

---

## DSL Language Reference

### Declarations

All declarations are global and must appear before scene blocks.

#### Actor (Character)

```story
character "Display Name" as id color #RRGGBB;
```

Example:

```story
character "Aria" as aria color #FF5733;
```

#### Asset

```story
asset id = "path/to/file";
```

Asset paths are **resolved relative to `.build/story.json`** (i.e., the `.build/` directory). To reference files at the repository root, prefix with `../`:

```story
asset bg = "../src/test/assets/img/background.png";
asset bgm = "../src/test/assets/audio/music.wav";
```

#### Global State Variable

```story
set id = expression;
set id += expression;
set id -= expression;
```

Example:

```story
set score = 0;
set score += 10;
set score -= 5;
```

### Scenes

Scenes are the primary execution scope. The first scene declared is the entry point.

```story
scene "SceneName" {
    // statements...
}
```

### Statements

All executable statements live inside a `scene` block.

#### Dialogue

```story
actorId "Text to display.";
```

Example:

```story
aria "Welcome to the story.";
```

#### Show / Hide Graphics

```story
show background assetId;
show sprite assetId;
hide background assetId;
hide sprite assetId;
```

Example:

```story
show background bg_forest;
show sprite char_hero;
hide sprite char_hero;
```

#### Play / Stop Audio

```story
play music assetId;
play sound assetId;
stop music assetId;
stop sound assetId;
```

Example:

```story
play music bgm_theme;
play sound sfx_click;
stop sound sfx_click;
stop music bgm_theme;
```

#### Choice

```story
choice {
    "Option text" {
        // statements for this branch
    }
    "Another option" {
        // statements for this branch
    }
}
```

Example:

```story
choice {
    "Take the safe path" {
        aria "A wise decision.";
        goto "Safe_End";
    }
    "Enter the cave" {
        aria "Brave, but risky.";
        goto "Dark_End";
    }
}
```

#### If / Else

```story
if (expression operator expression) {
    // then block
} else {
    // else block (optional)
}
```

Supported comparison operators: `<`, `<=`, `>`, `>=`, `==`, `!=`.

Example:

```story
if (score > 5) {
    aria "High score!";
} else {
    aria "Keep trying.";
}
```

#### Set (Local / Scene-level)

Same syntax as global `set`, but inside a scene block:

```story
set lives -= 1;
set total = (points * 2) + bonus;
```

#### Goto

Unconditional jump to another scene:

```story
goto "TargetScene";
```

The target scene must exist (verified by the semantic analyzer).

#### End

Terminates execution:

```story
end;
```

### Expressions

Expressions are integer-only and support:

- Integer literals: `42`, `0`, `-3`
- Identifiers (variables): `score`, `bonus`
- Parentheses for grouping: `(a + b) * 2`
- Arithmetic operators (precedence: `* /` before `+ -`):
  - `+` addition
  - `-` subtraction
  - `*` multiplication
  - `/` division

Example:

```story
set result = (score + bonus) * 2 - 5;
```

### Comments

```story
// Single-line comment

/*
 * Multi-line comment
 */
```

### Cross-File Loading (Advanced)

```story
load "other_file.story";
```

The scanner pushes the loaded file onto the input stack. Its declarations and scenes are merged inline.

---

## Project Architecture

### Compiler Pipeline

| Phase | Tool / Module | Responsibility |
|-------|---------------|----------------|
| **Lexical Analysis** | Flex (`FlexPatterns.l`) | Tokenizes keywords, identifiers, strings, operators, colors, comments. Supports `load "file"` for cross-file inclusion. |
| **Syntactic Analysis** | Bison (`BisonGrammar.y`) | Parses tokens into an AST. Uses push-parser, pure interface, and `%destructor` rules for memory safety. |
| **Semantic Analysis** | `SemanticAnalyzer.c` | Validates: undeclared actors/assets/scenes, duplicate declarations, type mismatches, invalid goto targets. |
| **Code Generation** | `Generator.c` | Walks the AST and emits `.build/story.json`. |

### Player Runtime

| Module | File | Responsibility |
|--------|------|----------------|
| **Entry Point** | `player/PlayerEntryPoint.c` | Raylib window/audio init, main render loop, input handling, critical teardown order. |
| **Story Loader** | `player/runtime/StoryLoader.c` | Parses `story.json` into an in-memory runtime model (`Story`, `Scene`, `Statement`, etc.). |
| **Expression Evaluator** | `player/runtime/ExpressionEvaluator.c` | Evaluates integer expressions and conditions against the runtime variable table. |
| **Engine** | `player/runtime/Engine.c` | Drives the story via a context-stack state machine. Handles dialogue, choice, if/else, goto, set, and all media commands. |
| **JSON Parser** | `player/vendor/cJSON.c` | Vendored MIT-licensed JSON parser (no external network dependency). |

### Key Directories

```
src/main/c/
  EntryPoint.c                    # Compiler entry point
  frontend/
    lexical-analysis/             # Flex scanner & actions
    syntactic-analysis/           # Bison grammar & AST builders
    semantic-analysis/            # Semantic validator
  backend/
    code-generation/              # JSON generator
  player/
    PlayerEntryPoint.c            # Raylib player entry point
    runtime/
      StoryLoader.c               # JSON → runtime model
      ExpressionEvaluator.c       # Expression / condition evaluator
      Engine.c                    # Story state machine
    vendor/
      cJSON.c / cJSON.h           # Vendored JSON parser

src/test/
  c/accept/                       # Positive integration tests
  c/reject/                       # Negative integration tests
  assets/                         # Generated PNG/WAV test media

examples/
  demo.story                      # Full-featured showcase story
```

---

## Build & Run

### Requirements

- **Docker** (recommended for reproducible builds)
- Or native: `gcc`, `g++`, `bison`, `flex`, `cmake`, `make`, `git`
- **Linux / WSL / Docker** only (Raylib + GCC + AddressSanitizer)

### Build

```bash
# Full clean build (deletes generated sources, regenerates them, compiles)
src/main/bash/build.sh
```

- Always deletes `FlexScanner.*` and `BisonParser.*` before CMake.
- CMake enforces **Bison before Flex** because Flex is invoked with `--bison-bridge --bison-locations`.
- Raylib is fetched automatically via `FetchContent` (tag `5.5`) and linked to both binaries.

### Compile a Story

```bash
src/main/bash/run.sh <program-file>
```

This writes `.build/story.json` (overwritten each run).

### Play a Story

```bash
src/main/bash/play.sh [.build/story.json]
```

- The script auto-detects headless environments. If `DISPLAY` is missing and `xvfb-run` is available, it wraps the player automatically.
- Asset paths inside `story.json` are resolved relative to `.build/`.

---

## Testing

All tests are **integration tests** — no C unit-test framework is used.

```bash
# Run the full suite
src/main/bash/test.sh
```

What the suite verifies:

- **Accept tests**: Positive language samples in `src/test/c/accept/` must compile and exit `0`.
- **Reject tests**: Negative language samples in `src/test/c/reject/` must exit non-zero (semantic or syntactic errors).
- **JSON generation**: After compiling `01-linear-scene`, `.build/story.json` must be non-empty.
- **Player smoke-test**: Compiles `06-graphics` and runs the player under `xvfb-run` (when headless) to verify the binary does not crash.

### Running a Single Test Manually

```bash
cat src/test/c/accept/01-linear-scene | .build/Flex-Bison-Compiler src/test/c/accept/01-linear-scene
```

---

## JSON Output Schema

On successful compilation, `.build/story.json` follows this schema:

```json
{
  "meta": {
    "version": "1.0",
    "source": "path/to/input.story"
  },
  "actors": [
    { "id": "aria", "name": "Aria", "color": "#FF5733" }
  ],
  "assets": [
    { "id": "bg_forest", "path": "assets/img/forest.png" }
  ],
  "declarations": [
    { "kind": "set", "identifier": "score", "op": "=", "expression": { ... } }
  ],
  "scenes": [
    {
      "name": "Intro",
      "statements": [
        { "kind": "show", "target": "background", "resourceId": "bg_forest" },
        { "kind": "dialogue", "actorId": "aria", "text": "Hello!" },
        { "kind": "choice", "options": [
          { "text": "Yes", "statements": [ ... ] },
          { "text": "No",  "statements": [ ... ] }
        ]},
        { "kind": "if", "condition": { ... }, "then": [ ... ], "else": [ ... ] },
        { "kind": "goto", "scene": "NextScene" },
        { "kind": "end" }
      ]
    }
  ]
}
```

### Expression Encoding

| Kind | Fields |
|------|--------|
| integer | `{"kind":"integer","value":42}` |
| identifier | `{"kind":"identifier","name":"score"}` |
| binary | `{"kind":"binary","op":"+","left":{...},"right":{...}}` |

### Condition Encoding

| Kind | Fields |
|------|--------|
| comparison | `{"kind":"comparison","op":">","left":{...},"right":{...}}` |

---

## Docker Development Environment

```bash
# Start an ephemeral dev container
docker compose run --rm compiler

# Inside the container, run build/test/play as usual
src/main/bash/build.sh
src/main/bash/test.sh

# Shutdown afterwards (outside the container)
docker compose down
```

- Compose file: `compose.yaml`
- Dockerfile: `src/main/docker/compiler/Dockerfile` (Ubuntu 24.04 + build tools + X11/OpenGL dev headers for Raylib)
- Optional `.env` file is supported for `ENVIRONMENT`, `LOG_IGNORED_LEXEMES`, `LOGGING_LEVEL`.

---

## CI / CD

GitHub Actions workflow: `.github/workflows/pipeline.yaml`

Triggers on every `push` and on PR `closed` / `reopened`.

Steps:

1. Install dependencies: `bison cmake flex gcc g++ git make xvfb`
2. Install X11 / OpenGL dev packages for Raylib
3. Make bash scripts executable
4. Run `build.sh`
5. Run `test.sh`
6. Run dedicated **Player test** step (headless smoke-test)

---

## Troubleshooting

### "Story JSON not found"

Run `src/main/bash/run.sh <story-file>` before `play.sh`. The compiler generates `.build/story.json`.

### "Cannot open asset file" / black screen / no audio

Asset paths in `story.json` are resolved **relative to `.build/story.json`** (i.e., the `.build/` directory). If your assets are at the repository root, use `../` prefix:

```story
asset bg = "../src/test/assets/img/background.png";
```

### Headless / CI / Docker: display or audio errors

Raylib requires a display at runtime. In headless environments:

- `play.sh` **auto-detects** missing `DISPLAY` and wraps the player with `xvfb-run --auto-servernum`.
- If running the player binary directly, use: `xvfb-run --auto-servernum ./.build/Flex-Bison-Player .build/story.json`
- Audio may be unavailable in pure headless containers; the player guards against this and will not crash.

### AddressSanitizer leaks on exit

Player teardown order is critical. `Engine_destroy` **must** run before `CloseAudioDevice()` and `CloseWindow()`, or AddressSanitizer reports leaks. This is already handled correctly in `PlayerEntryPoint.c` — do not reorder calls.

### "Clock skew detected" warnings during build

Harmless Docker/WSL filesystem timing artifact. Safe to ignore.

### Adding a new `.c` file

You must append it to the `add_executable` list in `CMakeLists.txt`. New sources are **not** auto-discovered. The compiler and player have separate `add_executable` blocks.

---

## License

See `LICENSE.md`.
