#pragma once

typedef enum {
  // メタなtoken
  TT_ROOT, // ROOTトークン。tokenizerの実装を簡単にするのに最初に必ず入っている
  TT_EOF,  // EOFトークン。Parserの実装を簡単にするために最後にかならず入っている
  TT_SKIP, // RESETされ内容を記録されなかったトークン。実際には発生しないがtokenizerが内部で使う
  TT_CONT, // ACCEPTせず継続させるトークン。実際には発生しないがTokenizerが内部で使う

  // メタなtoken
  TT_NUM,
  TT_PLUS,
  TT_MINUS,
  TT_MUL,
  TT_DIV,
  TT_LET,
  TT_LEFT_BRACKET,
  TT_RIGHT_BRACKET,
  TT_SEMICOLON,
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
