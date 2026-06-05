# Project Context

This repository contains a base compiler example built with Flex and Bison.
It targets a small DSL for visual-novel style scripts (characters, assets,
scenes, dialogue, choices, and simple expressions/conditions).

## Stack

- Language: C
- Parser generator: Bison
- Lexer generator: Flex
- Build system: CMake
- Runtime/test tooling: Bash scripts
- Container/dev: Docker Compose

## Layout

- src/main/c/EntryPoint.c
  Initializes modules, runs the parse loop, logs status, and performs teardown.
- src/main/c/frontend/
  Frontend orchestration (lexing/parsing), token creation, input buffers.
- src/main/c/frontend/lexical-analysis/
  Flex patterns and actions (tokenization, ignored lexemes, file loading).
- src/main/c/frontend/syntactic-analysis/
  Bison grammar and semantic actions that build the AST.
- src/main/c/support/
  Logging, env configuration, string helpers, and shared types.
- src/test/c/
  Accept/reject language samples used by the test runner.

## Build and Run

- Build: src/main/bash/build.sh
  - Runs Bison then Flex via CMake custom commands and compiles the binary.
- Run: src/main/bash/run.sh <program>
  - Feeds a program file to .build/Flex-Bison-Compiler.
- Test: src/main/bash/test.sh
  - Executes accept/reject fixtures and validates JSON output creation.

## Lexical Layer (Flex)

Key patterns and features defined in src/main/c/frontend/lexical-analysis/FlexPatterns.l:

- Keywords: character, as, color, asset, scene, set, show, hide, background,
  sprite, play, stop, music, sound, choice, if, else, goto, end
- Operators: =, +=, -=, <, <=, >, >=, ==, !=, +, -, *, /
- Punctuation: { } ( ) ;
- Comments: // line comments and /* */ multiline comments
- Identifiers, integers, quoted strings, and color literals (#RRGGBB)
- "load" context: load "file"; temporarily switches input to another file
- Color literals: #RRGGBB (no quotes)

Flex actions live in src/main/c/frontend/lexical-analysis/FlexActions.c and
translate lexemes into tokens and semantic values, with optional logging of
ignored lexemes controlled by LOG_IGNORED_LEXEMES.

### Load Semantics (Flex)

- "load" triggers a lexer start condition (LOAD_FILE). Within this context:
  - The next quoted string is treated as a file path and opened as a new input buffer.
  - A trailing semicolon ends the load statement and returns to the default context.
- The loaded file is pushed via yypush_buffer_state, so its tokens appear inline.
- EOF handling: when a file ends, EOFLexemeAction calls popInputBuffer to resume
  the previous buffer; only the final EOF is pushed to the parser.
- Error case: if the path cannot be opened, a log error is emitted and parsing
  fails immediately.
- Context safety: if EOF occurs while still in a non-zero Flex context, it logs
  an error and returns FAILED.

## Syntactic Layer (Bison)

Grammar in src/main/c/frontend/syntactic-analysis/BisonGrammar.y covers:

- Declarations:
- character "Display" as id color #RRGGBB;
  - asset id = "path";
  - set id =|+=|-= expression;
  - scene "Name" { statementList }
- Statements:
  - id "dialogue";
  - show/hide background|sprite id;
  - play/stop music|sound id;
  - goto "Scene";
  - set id =|+=|-= expression;
  - choice { "option" { statementList } ... }
  - if (condition) { ... } [else { ... }]
  - end;
- Expressions: integers, identifiers, parentheses, + - * /
- Conditions: comparisons between expressions (<, <=, >, >=, ==, !=)

### Grammar Details

- program is a list of declarations (including empty list).
- declarationList and statementList are left-associative linked lists built by
  appending to an existing list node.
- The grammar includes empty statement lists and empty declaration lists.
- "else" ambiguity is resolved by %right ELSE, so else binds to the nearest if.
- Precedence (low to high): comparisons, then +/-, then */.
- Identifiers are used both for variable names and resource IDs.
- Some semantic constraints are not enforced at this stage (see tests in
  src/test/c/reject for examples like undeclared actors or bad goto targets).

Semantic actions in src/main/c/frontend/syntactic-analysis/BisonActions.c build
the AST and store it in the CompilerState.

## AST

AST definitions and constructors are in:

- src/main/c/frontend/syntactic-analysis/AbstractSyntaxTree.h
- src/main/c/frontend/syntactic-analysis/AbstractSyntaxTree.c

Node families include Expression, Condition, Statement, Declaration, and Program.
Destructors recursively free allocated memory.

### AST Structure Details

- Program holds a DeclarationList (head/tail, singly linked via Declaration->next).
- StatementList holds a Statement chain (head/tail, singly linked via Statement->next).
- ChoiceOption is a singly linked list (ChoiceOption->next) with a StatementList body.
- Expression is a tagged union: integer literal, identifier string, or binary node.
- Statements are a tagged union with specialized payloads (dialogue/show/hide/play/stop
  /goto/set/choice/if/end).
- Declarations are a tagged union for character, asset, set, and scene.
- Memory ownership: constructors copy strings into AST nodes, while Bison actions
  free the original token strings after building nodes; destructors recursively
  free all AST-owned memory.

## Runtime Behavior

- The compiler performs lexical/syntactic analysis, builds the AST, and emits JSON.
- JSON output is written to `.build/story.json` on successful parse (overwritten each run).
- Logging behavior and environment config live in src/main/c/support.

## JSON Output

On successful parse, the compiler writes `.build/story.json` (overwritten each
run). Operators are encoded symbolically.

Schema (high-level):

- meta: { version, source }
- actors: [{ id, name, color }]
- assets: [{ id, path }]
- declarations: [{ kind: "set", identifier, op, expression }]
- scenes: [{ name, statements[] }]
- expressions: { kind: "integer"|"identifier"|"binary", op?, left?, right?, value?, name? }
- conditions: { kind: "comparison", op, left, right }
- statements: tagged union (dialogue/show/hide/play/stop/choice/if/goto/set/end)

## CI/CD

GitHub Actions workflow in .github/workflows/pipeline.yaml installs dependencies
and runs build.sh and test.sh on pushes/PRs.

## Environment Variables

Configured in README and compose.yaml:

- ENVIRONMENT: Local | Development | Production
- LOG_IGNORED_LEXEMES: true/false
- LOGGING_LEVEL: ALL | DEBUGGING | INFORMATION | WARNING | ERROR | CRITICAL

## Notes

- PDF docs in docs/TLA - Primer Entregable.pdf could not be read by this model;
  user-provided excerpts are reflected below.

## Domain Summary (Primer Entregable)

The DSL targets narrative-heavy visual novels, abstracting away engine concerns
(rendering, audio management, event loops) to focus on story structure. The
language is described as statically and strongly typed, centered around first-
class entities: actors, scenes, resources (assets), decision flows, and state
variables.

## Language Constructs (Primer Entregable)

- Actor declaration: bind display name + internal identifier (+ metadata).
- Asset declaration: bind a multimedia file to an identifier.
- State variables: numeric variables with =, +=, -= assignments.
- Scene blocks: primary execution scope; all executable statements live inside.
- Choice blocks: player decisions with per-option scopes.
- If/else: conditional branching based on state variables.
- Goto: unconditional jump to another scene.
- Dialogue: actor identifier + string literal.
- Graphics: show/hide background or sprite resources.
- Audio: play/stop music or sound effects.
- Implicit input: handled by the engine during choice and dialogue progression.
- Program termination: end statement to finish execution.
- Cross-file loading: load statements to include actors/scenes/assets from
  other files.

## Test Cases (Primer Entregable)

Acceptance cases (initial):
- Basic declarations + linear scene execution.
- Branching with multiple options.
- Conditional mutation of a numeric state variable.
- Multiple goto between scenes.
- Audio play/stop usage.
- Graphics show/hide usage.
- Nested if/else evaluation.
- Arithmetic and accumulation across variables.
- Multi-actor dialogue exchanges.
- Cyclic choice (loop narrative).
- Cross-file resource access.

Rejection cases (initial):
- Undeclared actor/resource usage.
- goto to nonexistent scene.
- Type incompatibility in expressions/assignments.
- Executable statements outside scene scope.
- Reserved keyword used as identifier.

## Examples (Primer Entregable)

- Linear flow example: declare actors/resources globally, then a scene with
  sequential show/play/dialogue statements.
- Branching + state example: define a numeric variable, use choice to mutate
  state with +=/-=, then if/else to goto different scenes based on the state.
