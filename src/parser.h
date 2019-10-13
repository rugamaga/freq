#pragma once

#include "tokenizer.h"

typedef enum {
  ST_NUM,
  ST_ADD,
  ST_SUB,
  ST_MUL,
  ST_DIV,
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
