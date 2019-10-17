#pragma once

#include "tokenizer.h"

typedef enum {
  ST_BLOCK,
  ST_NUM,
  ST_ADD,
  ST_SUB,
  ST_MUL,
  ST_DIV,
  ST_LET,
  ST_RET,
  ST_ASSIGN,
  ST_VAR,
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
  struct tAST** children;
} AST;

typedef struct {
  AST** code;
  Token* root;
  Token* current;
} Parser;

Parser* parse(Token* token);

void print_ast(AST* ast, size_t level);
