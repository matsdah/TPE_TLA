%top{

#include "../../support/type/CompilationStatus.h"
#include "../../support/type/FlexContext.h"
#include "../../support/type/TokenLabel.h"
#include "../syntactic-analysis/AbstractSyntaxTree.h"
#include "../syntactic-analysis/BisonParser.h"

}

%{

/**
 * @see https://westes.github.io/flex/manual/Definitions-Section.html#Definitions-Section
 */

extern CompilationStatus KeywordLexemeAction(TokenLabel label);
extern CompilationStatus OperatorLexemeAction(TokenLabel label);
extern CompilationStatus PunctuationLexemeAction(TokenLabel label);
extern CompilationStatus IntegerLexemeAction();
extern CompilationStatus StringLexemeAction();
extern CompilationStatus IdentifierLexemeAction();
extern CompilationStatus IgnoredLexemeAction();
extern CompilationStatus UnknownLexemeAction();
extern CompilationStatus EOFLexemeAction();
extern CompilationStatus EnterMultilineCommentLexemeAction(FlexContext context);
extern CompilationStatus LeaveMultilineCommentLexemeAction();
extern CompilationStatus EnterLoadFileLexemeAction(FlexContext context);
extern CompilationStatus LoadFileNameLexemeAction();
extern CompilationStatus LeaveLoadFileLexemeAction();

%}

/**
 * Flex contexts (a.k.a. start conditions).
 *
 * @see https://westes.github.io/flex/manual/Start-Conditions.html
 */
%x MULTILINE_COMMENT
%x LOAD_FILE

/**
 * Reusable patterns.
 *
 * @see https://westes.github.io/flex/manual/Matching.html
 * @see https://westes.github.io/flex/manual/Patterns.html
 */
whitespace      [[:space:]]+
digit           [[:digit:]]
letter          [[:alpha:]_]
alphanumeric    [[:alnum:]_]

%%

%{

/**
 * @see https://westes.github.io/flex/manual/Rules-Section.html#Rules-Section
 */

%}

"/*"                        { return EnterMultilineCommentLexemeAction(MULTILINE_COMMENT); }
<MULTILINE_COMMENT>"*/"     { return LeaveMultilineCommentLexemeAction(); }
<MULTILINE_COMMENT>[^*]+    { return IgnoredLexemeAction(); }
<MULTILINE_COMMENT>.        { return IgnoredLexemeAction(); }

"//"[^\n]*                  { return IgnoredLexemeAction(); }

"load"                      { return EnterLoadFileLexemeAction(LOAD_FILE); }
<LOAD_FILE>\"[^\"]+\"       { return LoadFileNameLexemeAction(); }
<LOAD_FILE>";"              { return LeaveLoadFileLexemeAction(); }
<LOAD_FILE>{whitespace}     { return IgnoredLexemeAction(); }
<LOAD_FILE>.                { return UnknownLexemeAction(); }

"character"                 { return KeywordLexemeAction(CHARACTER); }
"as"                        { return KeywordLexemeAction(AS); }
"color"                     { return KeywordLexemeAction(COLOR); }
"asset"                     { return KeywordLexemeAction(ASSET); }
"scene"                     { return KeywordLexemeAction(SCENE); }
"set"                       { return KeywordLexemeAction(SET); }
"show"                      { return KeywordLexemeAction(SHOW); }
"hide"                      { return KeywordLexemeAction(HIDE); }
"background"                { return KeywordLexemeAction(BACKGROUND); }
"sprite"                    { return KeywordLexemeAction(SPRITE); }
"play"                      { return KeywordLexemeAction(PLAY); }
"stop"                      { return KeywordLexemeAction(STOP); }
"music"                     { return KeywordLexemeAction(MUSIC); }
"sound"                     { return KeywordLexemeAction(SOUND); }
"choice"                    { return KeywordLexemeAction(CHOICE); }
"if"                        { return KeywordLexemeAction(IF); }
"else"                      { return KeywordLexemeAction(ELSE); }
"goto"                      { return KeywordLexemeAction(GOTO); }
"end"                       { return KeywordLexemeAction(END); }

{letter}{alphanumeric}*     { return IdentifierLexemeAction(); }

\"[^\"]*\"                  { return StringLexemeAction(); }

{digit}+                    { return IntegerLexemeAction(); }

"+="                        { return OperatorLexemeAction(ASSIGN_ADD); }
"-="                        { return OperatorLexemeAction(ASSIGN_SUB); }
"<="                        { return OperatorLexemeAction(LESS_EQ); }
">="                        { return OperatorLexemeAction(GREATER_EQ); }
"=="                        { return OperatorLexemeAction(EQUAL); }
"!="                        { return OperatorLexemeAction(NOT_EQUAL); }

"="                         { return OperatorLexemeAction(ASSIGN); }
"<"                         { return OperatorLexemeAction(LESS); }
">"                         { return OperatorLexemeAction(GREATER); }
"+"                         { return OperatorLexemeAction(ADD); }
"-"                         { return OperatorLexemeAction(SUB); }
"*"                         { return OperatorLexemeAction(MUL); }
"/"                         { return OperatorLexemeAction(DIV); }

"{"                         { return PunctuationLexemeAction(OPEN_BRACE); }
"}"                         { return PunctuationLexemeAction(CLOSE_BRACE); }
"("                         { return PunctuationLexemeAction(OPEN_PARENTHESIS); }
")"                         { return PunctuationLexemeAction(CLOSE_PARENTHESIS); }
";"                         { return PunctuationLexemeAction(SEMICOLON); }

{whitespace}                { return IgnoredLexemeAction(); }
.                           { return UnknownLexemeAction(); }

<<EOF>>                     { return EOFLexemeAction(); }

%%

/**
 * @see https://westes.github.io/flex/manual/User-Code-Section.html#User-Code-Section
 */

#include "FlexActions.h"
#include "FlexExport.h"
