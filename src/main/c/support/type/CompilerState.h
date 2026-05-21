#ifndef COMPILER_STATE_HEADER
#define COMPILER_STATE_HEADER

#include <stddef.h>

/**
 * The global state of the compiler. Should transport every data structure
 * needed across the different phases of a compilation.
 */
typedef struct {
	/**
	 * The root node of the AST.
	 */
	void * abstractSyntaxtTree;

	/**
	 * The source path used to generate the AST.
	 */
	const char * sourcePath;

	// TODO: Add a symbol table.
	// TODO: Add a stack to handle nested scopes.
} CompilerState;

#endif
