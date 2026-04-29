#ifndef BISON_ACTIONS_HEADER
#define BISON_ACTIONS_HEADER

#include "../../support/logging/Logger.h"
#include "../../support/type/CompilerState.h"
#include "../../support/type/ModuleDestructor.h"
#include "../../support/type/TokenLabel.h"
#include "AbstractSyntaxTree.h"
#include "BisonParser.h"
#include <stdlib.h>

ModuleDestructor initializeBisonActionsModule(CompilerState * compilerState);

/* ── Expressions ─────────────────────────────────────────────────── */
Expression *      IntegerExpressionSemanticAction(int value);
Expression *      IdentifierExpressionSemanticAction(char * identifier);
Expression *      BinaryExpressionSemanticAction(Expression * left, ArithOp op, Expression * right);

/* ── Condition ───────────────────────────────────────────────────── */
Condition *       ConditionSemanticAction(Expression * left, ComparisonOp op, Expression * right);

/* ── Statements ──────────────────────────────────────────────────── */
Statement *       DialogueStatementSemanticAction(char * actorId, char * text);
Statement *       ShowStatementSemanticAction(ResourceDisplayType type, char * resourceId);
Statement *       HideStatementSemanticAction(ResourceDisplayType type, char * resourceId);
Statement *       PlayStatementSemanticAction(ResourceAudioType type, char * resourceId);
Statement *       StopStatementSemanticAction(ResourceAudioType type, char * resourceId);
Statement *       GotoStatementSemanticAction(char * sceneName);
Statement *       SetStatementSemanticAction(char * identifier, AssignOp op, Expression * expr);
Statement *       ChoiceStatementSemanticAction(ChoiceOption * options);
Statement *       IfStatementSemanticAction(Condition * cond, StatementList * thenBlock, StatementList * elseBlock);
Statement *       EndStatementSemanticAction();

/* ── StatementList ───────────────────────────────────────────────── */
StatementList *   EmptyStatementListSemanticAction();
StatementList *   AppendStatementSemanticAction(StatementList * list, Statement * stmt);

/* ── ChoiceOption ────────────────────────────────────────────────── */
ChoiceOption *    ChoiceOptionSemanticAction(char * text, StatementList * body);
ChoiceOption *    AppendChoiceOptionSemanticAction(ChoiceOption * list, ChoiceOption * option);

/* ── Declarations ────────────────────────────────────────────────── */
Declaration *     CharacterDeclarationSemanticAction(char * displayName, char * identifier, char * color);
Declaration *     AssetDeclarationSemanticAction(char * identifier, char * filePath);
Declaration *     SetDeclarationSemanticAction(char * identifier, AssignOp op, Expression * expr);
Declaration *     SceneDeclarationSemanticAction(char * name, StatementList * body);

/* ── DeclarationList ─────────────────────────────────────────────── */
DeclarationList * EmptyDeclarationListSemanticAction();
DeclarationList * AppendDeclarationSemanticAction(DeclarationList * list, Declaration * decl);

/* ── Program ─────────────────────────────────────────────────────── */
Program *         ProgramSemanticAction(DeclarationList * declarations);

#endif
