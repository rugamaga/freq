#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "tokenizer.h"
#include "util.h"

Token* create_token(TokenType type, const char* buffer, size_t pos, size_t len) {
  Token* token = (Token*)malloc(sizeof(Token));
  token->type = type;
  token->buffer = buffer;
  token->pos = pos;
  token->len = len;
  token->next = NULL;
  return token;
}

typedef struct {
  const char* buffer;
  Token* root;
  Token* current;
  size_t pos;
  size_t len;
} Tokenizer;

static Tokenizer* create_tokenizer(const char* buffer, size_t len) {
  // ステートマシンとして全体の処理を行う
  Tokenizer* tn = (Tokenizer*)malloc(sizeof(Tokenizer));

  tn->buffer = buffer;

  // トークンは常に0文字目のTT_ROOTから
  // 始まってると考えることにして
  // 何かと楽をしましょう。
  tn->current = tn->root = create_token(TT_ROOT, buffer, 0, 0);

  tn->pos = 0;
  tn->len = len;

  return tn;
}

char read(Tokenizer* tn, size_t diff) {
  return *(tn->buffer + tn->pos + diff);
}

static void skip(Tokenizer* tn, size_t size) {
  tn->pos += size;
}

static void accept(Tokenizer* tn, TokenType type, size_t size) {
  Token* t = create_token(type, tn->buffer, tn->pos, size);
  tn->current->next = t;
  tn->current = t;
  skip(tn, size);
}

static void error(Tokenizer* tn, char c) {
  fprintf(stderr, "Tokenize中に予想外の文字(%zu文字目の'%c')が着てしまいました。\n", tn->pos, c);
  exit(EXIT_FAILURE);
}

typedef struct {
  size_t size;
  const char* word;
  TokenType type;
} Reserved;

bool skip_space(Tokenizer* tn) {
  char c = read(tn, 0);
  if( ' ' == c || '\t' == c || '\r' == c || '\n' == c ) {
    skip(tn, 1);
    return true;
  } else {
    return false;
  }
}

static const Reserved reserved[] = {
  { 3, "let", TT_LET },
  { 3, "ret", TT_RET },
  { 3, "fun", TT_FUN },
  { 2, "==", TT_EQUAL },
  { 2, "!=", TT_NOT_EQUAL },
  { 2, "<=", TT_LTEQ },
  { 2, ">=", TT_GTEQ },
  { 1, "<", TT_LT },
  { 1, ">", TT_GT },
  { 1, "=", TT_ASSIGN },
  { 1, "+", TT_PLUS },
  { 1, "-", TT_MINUS },
  { 1, "*", TT_MUL },
  { 1, "/", TT_DIV },
  { 1, "(", TT_LEFT_PAREN },
  { 1, ")", TT_RIGHT_PAREN },
  { 1, "[", TT_LEFT_BRACKET },
  { 1, "]", TT_RIGHT_BRACKET },
  { 1, "{", TT_LEFT_BRACE },
  { 1, "}", TT_RIGHT_BRACE },
  { 1, ";", TT_SEMICOLON },
  { 1, ",", TT_COMMA },
};

bool match_reserved(Tokenizer* tn) {
  for( size_t i = 0; i < (sizeof(reserved) / sizeof(Reserved)); ++i ) {
    const Reserved r = reserved[ i ];

    bool all_matched = true;
    for( size_t j = 0; j < r.size; ++j ) {
      all_matched &= (r.word[ j ] == read( tn, j ));
    }
    if( !all_matched ) continue;
    // 次の文字がアルファベットとかならこれはIDENTっぽいので駄目
    if( isalpha( read(tn, r.size) ) ) continue;
    // 先頭が文字であるなら、最後が数字もIDENTの可能性が残るので駄目。
    if( isalpha( read(tn, 0 ) ) && isdigit( read(tn, r.size) ) ) continue;
    // これだった
    accept( tn, r.type, r.size );
    return true;
  }
  return false;
}

bool match_num(Tokenizer* tn) {
  if( !isdigit( read( tn, 0 ) ) ) return false;

  size_t size;
  for( size = 1; isdigit( read( tn, size ) ); ++size ) ;
  accept( tn, TT_NUM, size );
  return true;
}

bool match_ident(Tokenizer* tn) {
  if( !isalpha( read( tn, 0 ) ) ) return false;

  size_t size;
  for( size = 1; isalnum( read( tn, size ) ); ++size ) ;
  accept( tn, TT_IDENT, size );
  return true;
}

Token* tokenize(const char* buffer, size_t len) {
  Tokenizer* tn = create_tokenizer(buffer, len);

  while( read( tn, 0 ) != '\0' ) {
    if( skip_space( tn ) ) continue;
    if( match_reserved( tn ) ) continue;
    if( match_num( tn ) ) continue;
    if( match_ident( tn ) ) continue;
    error( tn, read( tn, 0 ) );
  }

  accept( tn, TT_EOF, 1 );
  return tn->root;
}

// 指定されたtokenから先のtokenのメモリをすべて開放する
// void free_token(Token* token) {
//   Token* current = token;
//   while( current ) {
//     Token* next = current->next;
//     free(current);
//     current = next;
//   }
// }

void print_tokens(Token* token) {
  for( Token* t = token; t; t = t->next ) {
    fprintf(stderr, "%u: pos = %zu, chars = ", t->type, t->pos);
    for( size_t i = t->pos; i < t->pos + t->len; ++i ) {
      fprintf(stderr, "%c", t->buffer[i]);
    }
    fprintf(stderr, "\n");
  }
}
