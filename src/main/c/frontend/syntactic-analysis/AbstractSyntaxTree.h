#ifndef ABSTRACT_SYNTAX_TREE_HEADER
#define ABSTRACT_SYNTAX_TREE_HEADER

#include "../../support/logging/Logger.h"
#include "../../support/type/ModuleDestructor.h"
#include <stdlib.h>

ModuleDestructor initializeAbstractSyntaxTreeModule();

/* ── Forward declarations ─────────────────────────────────────────── */

typedef enum AssignOp AssignOp;
typedef enum ComparisonOp ComparisonOp;
typedef enum ArithOp ArithOp;
typedef enum ResourceDisplayType ResourceDisplayType;
typedef enum ResourceAudioType ResourceAudioType;
typedef enum ExpressionType ExpressionType;
typedef enum StatementType StatementType;
typedef enum DeclarationType DeclarationType;

typedef struct Expression Expression;
typedef struct Condition Condition;
typedef struct Statement Statement;
typedef struct StatementList StatementList;
typedef struct ChoiceOption ChoiceOption;
typedef struct DialogueStatement DialogueStatement;
typedef struct ShowStatement ShowStatement;
typedef struct HideStatement HideStatement;
typedef struct PlayStatement PlayStatement;
typedef struct StopStatement StopStatement;
typedef struct GotoStatement GotoStatement;
typedef struct SetStatement SetStatement;
typedef struct ChoiceStatement ChoiceStatement;
typedef struct IfStatement IfStatement;
typedef struct CharacterDeclaration CharacterDeclaration;
typedef struct AssetDeclaration AssetDeclaration;
typedef struct SceneDeclaration SceneDeclaration;
typedef struct Declaration Declaration;
typedef struct DeclarationList DeclarationList;
typedef struct Program Program;

/* ── Enums ────────────────────────────────────────────────────────── */

enum AssignOp { 
	ASSIGN_OP, 			// =
	ASSIGN_ADD_OP, 		// +=
	ASSIGN_SUB_OP 		// -=
};

enum ComparisonOp { 
	CMP_LT, 			// <
	CMP_GT, 			// >
	CMP_LE, 			// <=
	CMP_GE, 			// >=
	CMP_EQ, 			// ==
	CMP_NE 				// !=			
};

enum ArithOp { 
	ARITH_ADD, 			// +
	ARITH_SUB, 			// -
	ARITH_MUL, 			// *
	ARITH_DIV 			// /
};

enum ResourceDisplayType { 
	DISPLAY_BACKGROUND,
	DISPLAY_SPRITE 
};

enum ResourceAudioType { 
	AUDIO_MUSIC, 
	AUDIO_SOUND 
};

enum ExpressionType { 
	EXPR_INTEGER, 
	EXPR_IDENTIFIER, 
	EXPR_BINARY 
};

enum StatementType {
	STMT_DIALOGUE,
	STMT_SHOW,
	STMT_HIDE,
	STMT_PLAY,
	STMT_STOP,
	STMT_GOTO,
	STMT_SET,
	STMT_CHOICE,
	STMT_IF,
	STMT_END
};

enum DeclarationType {
	DECL_CHARACTER,
	DECL_ASSET,
	DECL_SET,
	DECL_SCENE
};

/* ── Condition (COMPARADOR USADO EN IfStatement) ──────────────────────────────────── */

struct Condition {
	Expression * left;
	ComparisonOp op;
	Expression * right;
};

/* ── Expression (ARITMETICA/ID) ──────────────────────────────────── */

struct Expression {
	ExpressionType type;
	union {
		int integer;
		char * identifier;
		struct {
			Expression * left;
			ArithOp op;
			Expression * right;
		} binary;
	};
};

/* ── Statements (ACCIONES) ─────────────────────────────────────────── */

struct DialogueStatement {
	char * actorId; 
	char * text;
};

struct ShowStatement {
	char * resourceId;
	ResourceDisplayType displayType; 
};

struct HideStatement {
	char * resourceId;
	ResourceDisplayType displayType;
};

struct PlayStatement {
	char * resourceId;
	ResourceAudioType audioType;    
};

struct StopStatement {
	char * resourceId;
	ResourceAudioType audioType;
};

struct GotoStatement { 
	char * sceneName;
};

struct SetStatement { 
	char * identifier;
	AssignOp op; 
	Expression * expr; 
};

struct ChoiceStatement {
	ChoiceOption * options;
};

struct IfStatement {
	Condition * condition;
	StatementList * thenBlock;
	StatementList * elseBlock; 
};

struct Statement {
	StatementType type;
	union {
		DialogueStatement * dialogue;
		ShowStatement *     show;
		HideStatement *     hide;
		PlayStatement *     play;
		StopStatement *     stop;
		GotoStatement *     goto_;
		SetStatement *      set;
		ChoiceStatement *   choice;
		IfStatement *       if_;
	};
	Statement * next;
};

struct StatementList { 
	Statement * head; 
	Statement * tail; 
};

struct ChoiceOption {
	char * text;
	StatementList * body;
	ChoiceOption * next;
};

/* ── Declarations (CONJUNTO ORGANZIADOR DE STATMENTS) ──────────────────── */

struct CharacterDeclaration { 
	char * displayName; 
	char * identifier; 
	char * color; 
};

struct AssetDeclaration { 
	char * identifier;  
	char * filePath; 
};

struct SceneDeclaration { 
	char * name;        
	StatementList * body; 
};

struct Declaration {
	DeclarationType type;
	union {
		CharacterDeclaration * character;
		AssetDeclaration *     asset;
		SetStatement *         set;
		SceneDeclaration *     scene;
	};
	Declaration * next;
};

struct DeclarationList { 
	Declaration * head; 
	Declaration * tail; 
};

/* ── Program (NODO DE MAYOR NIVEL) ───────────────────────────────────────── */

struct Program { 
	DeclarationList * declarations;
};

/* ── Constructors ─────────────────────────────────────────────────── */

Expression * createIntegerExpression(int value);
Expression * createIdentifierExpression(char * identifier);
Expression * createBinaryExpression(Expression * left, ArithOp op, Expression * right);

Condition * createCondition(Expression * left, ComparisonOp op, Expression * right);

StatementList * createStatementList();
void appendStatement(StatementList * list, Statement * stmt);		// Agrega un statement al final de la lista

Statement * createDialogueStatement(char * actorId, char * text);
Statement * createShowStatement(ResourceDisplayType type, char * resourceId);
Statement * createHideStatement(ResourceDisplayType type, char * resourceId);
Statement * createPlayStatement(ResourceAudioType type, char * resourceId);
Statement * createStopStatement(ResourceAudioType type, char * resourceId);
Statement * createGotoStatement(char * sceneName);
Statement * createSetStatement(char * identifier, AssignOp op, Expression * expr);
Statement * createChoiceStatement(ChoiceOption * options);
Statement * createIfStatement(Condition * cond, StatementList * thenBlock, StatementList * elseBlock);
Statement * createEndStatement();

ChoiceOption * createChoiceOption(char * text, StatementList * body);
void appendChoiceOption(ChoiceOption ** head, ChoiceOption * option);	// Agrega una opción al final de la lista

DeclarationList * createDeclarationList();
void appendDeclaration(DeclarationList * list, Declaration * decl);	// Agrega una declaración al final de la lista

Declaration * createCharacterDeclaration(char * displayName, char * identifier, char * color);
Declaration * createAssetDeclaration(char * identifier, char * filePath);
Declaration * createSetDeclaration(char * identifier, AssignOp op, Expression * expr);
Declaration * createSceneDeclaration(char * name, StatementList * body);

Program * createProgram(DeclarationList * declarations);

/* ── Destructors ──────────────────────────────────────────────────── */

void destroyExpression(Expression * expr);
void destroyCondition(Condition * cond);
void destroyStatement(Statement * stmt);
void destroyStatementList(StatementList * list);
void destroyChoiceOption(ChoiceOption * option);
void destroyDeclaration(Declaration * decl);
void destroyDeclarationList(DeclarationList * list);
void destroyProgram(Program * program);

#endif
