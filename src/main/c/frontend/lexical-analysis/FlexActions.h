#ifndef FLEX_ACTIONS_HEADER
#define FLEX_ACTIONS_HEADER

#include "../../support/configuration/Environment.h"
#include "../../support/language/String.h"
#include "../../support/logging/Logger.h"
#include "../../support/type/CompilationStatus.h"
#include "../../support/type/FlexContext.h"
#include "../../support/type/LexicalAnalyzer.h"
#include "../../support/type/ModuleDestructor.h"
#include "../../support/type/Token.h"
#include "../../support/type/TokenLabel.h"
#include "../Frontend.h"

ModuleDestructor initializeFlexActionsModule(LexicalAnalyzer * lexicalAnalyzer);

/* ── Single-token actions ─────────────────────────────────────────── */
CompilationStatus KeywordLexemeAction(TokenLabel label);
CompilationStatus OperatorLexemeAction(TokenLabel label);
CompilationStatus PunctuationLexemeAction(TokenLabel label);
CompilationStatus IntegerLexemeAction();
CompilationStatus StringLexemeAction();
CompilationStatus IdentifierLexemeAction();
CompilationStatus IgnoredLexemeAction();
CompilationStatus UnknownLexemeAction();
CompilationStatus EOFLexemeAction();

/* ── Multiline comment context ────────────────────────────────────── */
CompilationStatus EnterMultilineCommentLexemeAction(FlexContext context);
CompilationStatus LeaveMultilineCommentLexemeAction();

/* ── load "file"; context (transparent to Bison) ─────────────────── */
CompilationStatus EnterLoadFileLexemeAction(FlexContext context);
CompilationStatus LoadFileNameLexemeAction();
CompilationStatus LeaveLoadFileLexemeAction();

#endif
