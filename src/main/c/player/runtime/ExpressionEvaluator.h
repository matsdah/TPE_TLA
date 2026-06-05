#ifndef EXPRESSION_EVALUATOR_HEADER
#define EXPRESSION_EVALUATOR_HEADER

#include "StoryLoader.h"
#include <stdbool.h>

int ExpressionEvaluator_evaluate(Story * story, Expression * expr);
bool ExpressionEvaluator_test(Story * story, Condition * cond);

#endif
