#pragma once

#include "tokenizer.h"

typedef enum {
  ST_NUM,
  ST_ADD,
  ST_SUB,
  ST_MUL,
  ST_DIV,
  ST_EQUAL,
  ST_NOT_EQUAL,
  ST_LT,
  ST_LTEQ,
  ST_GT,
  ST_GTEQ,
} SyntaxType;

typedef struct tAST {
  SyntaxType type;
  Token* token;
  long val;
  struct tAST* lhs;
  struct tAST* rhs;
} AST;

AST* parse(Token* token);

void print_ast(AST* ast, size_t level);
