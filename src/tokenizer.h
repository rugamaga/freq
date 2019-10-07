#pragma once

typedef enum {
  TT_ROOT,
  TT_NUM,
  TT_PLUS,
  TT_MINUS,
} TokenType;

typedef struct tToken {
  TokenType type;
  const char* buffer;
  size_t pos;
  size_t len;
  struct tToken* next;
} Token;

Token* tokenize(const char* buffer, size_t len);
Token* create_token(TokenType type, const char* buffer, size_t pos, size_t len);

void print_tokens(Token* token);
