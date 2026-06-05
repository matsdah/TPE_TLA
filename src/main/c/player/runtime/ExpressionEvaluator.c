#include "ExpressionEvaluator.h"
#include <stdio.h>
#include <string.h>

static int _getVariable(Story * story, const char * name) {
	for (Variable * v = story->variables; v != NULL; v = v->next) {
		if (strcmp(v->name, name) == 0) return v->value;
	}
	return 0;
}

int ExpressionEvaluator_evaluate(Story * story, Expression * expr) {
	if (expr == NULL) return 0;
	switch (expr->type) {
		case EXPR_INTEGER:
			return expr->integer;
		case EXPR_IDENTIFIER:
			return _getVariable(story, expr->identifier);
		case EXPR_BINARY:
		{
			int left = ExpressionEvaluator_evaluate(story, expr->binary.left);
			int right = ExpressionEvaluator_evaluate(story, expr->binary.right);
			switch (expr->binary.op) {
				case OP_ADD: return left + right;
				case OP_SUB: return left - right;
				case OP_MUL: return left * right;
				case OP_DIV: return right != 0 ? left / right : 0;
			}
		}
	}
	return 0;
}

bool ExpressionEvaluator_test(Story * story, Condition * cond) {
	if (cond == NULL) return false;
	int left = ExpressionEvaluator_evaluate(story, cond->left);
	int right = ExpressionEvaluator_evaluate(story, cond->right);
	switch (cond->op) {
		case CMP_LT: return left < right;
		case CMP_GT: return left > right;
		case CMP_LE: return left <= right;
		case CMP_GE: return left >= right;
		case CMP_EQ: return left == right;
		case CMP_NE: return left != right;
	}
	return false;
}
