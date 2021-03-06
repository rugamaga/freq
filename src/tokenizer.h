#pragma once

typedef enum {
  // メタなtoken
  TT_ROOT, // ROOTトークン。tokenizerの実装を簡単にするのに最初に必ず入っている
  TT_EOF,  // EOFトークン。Parserの実装を簡単にするために最後にかならず入っている

  // メタでないtoken
  TT_IF,
  TT_ELSE,
  TT_LOOP,
  TT_NUM,
  TT_PLUS,
  TT_MINUS,
  TT_MUL,
  TT_DIV,
  TT_LET,
  TT_RETURN,
  TT_FUN,
  TT_LEFT_PAREN,
  TT_RIGHT_PAREN,
  TT_LEFT_BRACKET,
  TT_RIGHT_BRACKET,
  TT_LEFT_BRACE,
  TT_RIGHT_BRACE,
  TT_SEMICOLON,
  TT_COMMA,
  TT_ASSIGN,
  TT_EQUAL,
  TT_NOT_EQUAL,
  TT_LT,
  TT_LTEQ,
  TT_GT,
  TT_GTEQ,
  TT_IDENT,
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
