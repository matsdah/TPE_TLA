#ifndef STORY_LOADER_HEADER
#define STORY_LOADER_HEADER

/* ── Runtime model for the visual-novel player ─────────────────────── */

typedef struct Actor Actor;
typedef struct Asset Asset;
typedef struct Variable Variable;
typedef struct Expression Expression;
typedef struct Condition Condition;
typedef struct Statement Statement;
typedef struct StatementList StatementList;
typedef struct ChoiceOption ChoiceOption;
typedef struct Scene Scene;
typedef struct SceneList SceneList;
typedef struct Story Story;

/* ── Expression ─────────────────────────────────────────────────────── */

typedef enum {
	EXPR_INTEGER,
	EXPR_IDENTIFIER,
	EXPR_BINARY
} ExpressionType;

typedef enum {
	OP_ADD,
	OP_SUB,
	OP_MUL,
	OP_DIV
} BinaryOp;

struct Expression {
	ExpressionType type;
	union {
		int integer;
		char * identifier;
		struct {
			Expression * left;
			BinaryOp op;
			Expression * right;
		} binary;
	};
};

/* ── Condition ──────────────────────────────────────────────────────── */

typedef enum {
	CMP_LT,
	CMP_GT,
	CMP_LE,
	CMP_GE,
	CMP_EQ,
	CMP_NE
} ComparisonOp;

struct Condition {
	Expression * left;
	ComparisonOp op;
	Expression * right;
};

/* ── Statement ──────────────────────────────────────────────────────── */

typedef enum {
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
} StatementKind;

struct Statement {
	StatementKind kind;
	union {
		struct { char * actorId; char * text; } dialogue;
		struct { char * target; char * resourceId; } show;
		struct { char * target; char * resourceId; } hide;
		struct { char * target; char * resourceId; } play;
		struct { char * target; char * resourceId; } stop;
		struct { char * sceneName; } goto_;
		struct { char * identifier; char * op; Expression * expr; } set;
		struct { ChoiceOption * options; } choice;
		struct { Condition * condition; StatementList * thenBlock; StatementList * elseBlock; } if_;
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

/* ── Scene ──────────────────────────────────────────────────────────── */

struct Scene {
	char * name;
	StatementList * statements;
	Scene * next;
};

struct SceneList {
	Scene * head;
	Scene * tail;
};

/* ── Actor / Asset ──────────────────────────────────────────────────── */

struct Actor {
	char * id;
	char * name;
	char * color;
	Actor * next;
};

struct Asset {
	char * id;
	char * path;
	Asset * next;
};

/* ── Variable (runtime symbol table) ────────────────────────────────── */

struct Variable {
	char * name;
	int value;
	Variable * next;
};

/* ── Story ───────────────────────────────────────────────────────────── */

struct Story {
	Actor * actors;
	Asset * assets;
	Variable * variables;
	StatementList * declarations;
	SceneList * scenes;
};

/* ── Public API ────────────────────────────────────────────────────── */

Story * StoryLoader_load(const char * jsonPath);
void StoryLoader_destroy(Story * story);
void StoryLoader_resolveAssetPaths(Story * story, const char * jsonDirectory);

Scene * StoryLoader_findScene(Story * story, const char * name);

/* Helpers for deep copies of sub-structures (used by the engine) */
StatementList * StoryLoader_cloneStatementList(const StatementList * list);
void StoryLoader_freeStatementList(StatementList * list);

#endif
