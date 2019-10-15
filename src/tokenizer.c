#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "tokenizer.h"
#include "util.h"

typedef enum {
  TS_END,
  TS_EMPTY,
  TS_NUM,
  TS_EQUAL,
  TS_NOT,
  TS_LT,
  TS_GT,
  TS_IDENT,
  TS_LET_0,
  TS_LET_1,
  TS_LET_2,
  TS_RET_0,
  TS_RET_1,
  TS_RET_2,

  TS_SIZE
} TokenizerState;

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
  TokenizerState state;
  size_t beg;
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

  tn->state = TS_EMPTY;
  tn->beg = 0;
  tn->pos = 0;
  tn->len = len;

  return tn;
}

static void gain(Tokenizer* tn, size_t diff) {
  tn->pos += diff;
}

static void accept(Tokenizer* tn, TokenType type) {
  // TT_CONTのときは何もしない
  if( type == TT_CONT ) return;

  // TT_SKIPのときは特別に実際にはトークンは記録しない
  if( type != TT_SKIP ) {
    Token* t = create_token(type, tn->buffer, tn->beg, tn->pos - tn->beg);
    tn->current->next = t;
    tn->current = t;
  }

  tn->beg = tn->pos;
}

static void into(Tokenizer* tn, TokenizerState state) {
  tn->state = state;
}

static void error(Tokenizer* tn, char c) {
  fprintf(stderr, "Tokenize中に予想外の文字(%zu文字目の'%c')が着てしまいました。\n", tn->pos, c);
  exit(EXIT_FAILURE);
}

typedef enum {
  IS_NULL = 0,
  IS_L,
  IS_E,
  IS_T,
  IS_R,
  IS_EQUAL,
  IS_BANG,
  IS_LT,
  IS_GT,
  IS_PLUS,
  IS_MINUS,
  IS_STAR,
  IS_SLASH,
  IS_LEFT_BRACKET,
  IS_RIGHT_BRACKET,
  IS_SEMICOLON,
  IS_LOWER,
  IS_UPPER,
  IS_DIGIT,
  IS_SPACE,
  IS_DEFAULT,

  CONDS_SIZE
} Condition;

void check_conds(bool* conds, char c) {
  conds[IS_NULL]          = ('\0' == c);
  conds[IS_L]             = ('l' == c);
  conds[IS_E]             = ('e' == c);
  conds[IS_T]             = ('t' == c);
  conds[IS_R]             = ('r' == c);
  // conds[IS_E]             = ('e' == c);
  // conds[IS_T]             = ('t' == c);
  conds[IS_EQUAL]         = ('=' == c);
  conds[IS_BANG]          = ('!' == c);
  conds[IS_LT]            = ('<' == c);
  conds[IS_GT]            = ('>' == c);
  conds[IS_PLUS]          = ('+' == c);
  conds[IS_MINUS]         = ('-' == c);
  conds[IS_STAR]          = ('*' == c);
  conds[IS_SLASH]         = ('/' == c);
  conds[IS_LEFT_BRACKET]  = ('(' == c);
  conds[IS_RIGHT_BRACKET] = (')' == c);
  conds[IS_SEMICOLON]     = (';' == c );
  conds[IS_LOWER]         = ('a' <= c && c <= 'z');
  conds[IS_UPPER]         = ('A' <= c && c <= 'Z');
  conds[IS_DIGIT]         = ('0' <= c && c <= '9');
  conds[IS_SPACE]         = (' ' == c || '\t' == c || '\r' == c || '\n' == c);
  conds[IS_DEFAULT]       = true;
}

typedef struct {
  Condition cond;
  size_t gain;
  TokenType accept;
  TokenizerState into;
  bool error;
} Action;


static const Action ts_end[] = {
  { IS_DEFAULT, 0, TT_CONT, TS_END, true },
};

static const Action ts_empty[] = {
  { IS_NULL, 1, TT_EOF, TS_END, false },
  { IS_SPACE, 1, TT_SKIP, TS_EMPTY, false },
  { IS_L, 1, TT_CONT, TS_LET_0, false },
  { IS_R, 1, TT_CONT, TS_RET_0, false },
  { IS_LOWER, 1, TT_CONT, TS_IDENT, false },
  { IS_DIGIT, 1, TT_CONT, TS_NUM, false },
  { IS_EQUAL, 1, TT_CONT, TS_EQUAL, false },
  { IS_BANG, 1, TT_CONT, TS_NOT, false },
  { IS_LT, 1, TT_CONT, TS_LT, false },
  { IS_GT, 1, TT_CONT, TS_GT, false },
  { IS_PLUS, 1, TT_PLUS, TS_EMPTY, false },
  { IS_MINUS, 1, TT_MINUS, TS_EMPTY, false },
  { IS_STAR, 1, TT_MUL, TS_EMPTY, false },
  { IS_SLASH, 1, TT_DIV, TS_EMPTY, false },
  { IS_LEFT_BRACKET, 1, TT_LEFT_BRACKET, TS_EMPTY, false },
  { IS_RIGHT_BRACKET, 1, TT_RIGHT_BRACKET, TS_EMPTY, false },
  { IS_SEMICOLON, 1, TT_SEMICOLON, TS_EMPTY, false },
  { IS_DEFAULT, 0, TT_CONT, TS_EMPTY, true },
};
static const Action ts_num[] = {
  { IS_DIGIT, 1, TT_CONT, TS_NUM, false },
  { IS_DEFAULT, 0, TT_NUM, TS_EMPTY, false },
};
static const Action ts_equal[] = {
  { IS_EQUAL, 1, TT_EQUAL, TS_EMPTY, false },
  { IS_DEFAULT, 0, TT_ASSIGN, TS_EMPTY, false },
};
static const Action ts_not[] = {
  { IS_EQUAL, 1, TT_NOT_EQUAL, TS_EMPTY, false },
  // TODO: introduce op-NOT
  { IS_DEFAULT, 0, TT_CONT, TS_EMPTY, true },
};
static const Action ts_lt[] = {
  { IS_EQUAL, 1, TT_LTEQ, TS_EMPTY, false },
  { IS_DEFAULT, 0, TT_LT, TS_EMPTY, false },
};
static const Action ts_gt[] = {
  { IS_EQUAL, 1, TT_GTEQ, TS_EMPTY, false },
  { IS_DEFAULT, 0, TT_GT, TS_EMPTY, false },
};
static const Action ts_ident[] = {
  { IS_LOWER, 1, TT_CONT, TS_IDENT, false },
  { IS_DIGIT, 1, TT_CONT, TS_IDENT, false },
  { IS_DEFAULT, 0, TT_IDENT, TS_EMPTY, false },
};
static const Action ts_let_0[] = {
  { IS_E, 1, TT_CONT, TS_LET_1, false },
  { IS_LOWER, 1, TT_CONT, TS_IDENT, false },
  { IS_DIGIT, 1, TT_CONT, TS_IDENT, false },
  { IS_DEFAULT, 0, TT_IDENT, TS_EMPTY, false },
};
static const Action ts_let_1[] = {
  { IS_T, 1, TT_CONT, TS_LET_2, false },
  { IS_LOWER, 1, TT_CONT, TS_IDENT, false },
  { IS_DIGIT, 1, TT_CONT, TS_IDENT, false },
  { IS_DEFAULT, 0, TT_IDENT, TS_EMPTY, false },
};
static const Action ts_let_2[] = {
  { IS_LOWER, 1, TT_CONT, TS_IDENT, false },
  { IS_DIGIT, 1, TT_CONT, TS_IDENT, false },
  { IS_DEFAULT, 0, TT_LET, TS_EMPTY, false },
};
static const Action ts_ret_0[] = {
  { IS_E, 1, TT_CONT, TS_RET_1, false },
  { IS_LOWER, 1, TT_CONT, TS_IDENT, false },
  { IS_DIGIT, 1, TT_CONT, TS_IDENT, false },
  { IS_DEFAULT, 0, TT_IDENT, TS_EMPTY, false },
};
static const Action ts_ret_1[] = {
  { IS_T, 1, TT_CONT, TS_RET_2, false },
  { IS_LOWER, 1, TT_CONT, TS_IDENT, false },
  { IS_DIGIT, 1, TT_CONT, TS_IDENT, false },
  { IS_DEFAULT, 0, TT_IDENT, TS_EMPTY, false },
};
static const Action ts_ret_2[] = {
  { IS_LOWER, 1, TT_CONT, TS_IDENT, false },
  { IS_DIGIT, 1, TT_CONT, TS_IDENT, false },
  { IS_DEFAULT, 0, TT_RET, TS_EMPTY, false },
};

void setup_action_table(const Action** actions) {
  actions[TS_END] = ts_end;
  actions[TS_EMPTY] = ts_empty;
  actions[TS_NUM] = ts_num;
  actions[TS_EQUAL] = ts_equal;
  actions[TS_NOT] = ts_not;
  actions[TS_LT] = ts_lt;
  actions[TS_GT] = ts_gt;
  actions[TS_IDENT] = ts_ident;
  actions[TS_LET_0] = ts_let_0;
  actions[TS_LET_1] = ts_let_1;
  actions[TS_LET_2] = ts_let_2;
  actions[TS_RET_0] = ts_ret_0;
  actions[TS_RET_1] = ts_ret_1;
  actions[TS_RET_2] = ts_ret_2;
}

Token* tokenize(const char* buffer, size_t len) {
  const Action* actions[TS_SIZE];
  setup_action_table(actions);

  Tokenizer* tn = create_tokenizer(buffer, len);
  while(
      (tn->state != TS_END) &&
      (tn->pos < tn->len)
  ) {
    const char c = tn->buffer[tn->pos];

    bool conds[CONDS_SIZE];
    check_conds(conds, c);

    // 各アクションテーブルには必ずIS_DEFAULTが入っている
    // ...ので、このループは必ず終わる
    for( const Action* act = actions[ tn->state ]; ; ++act ) {
      if( !conds[ act->cond ] ) continue;
      if( act->error ) error( tn, c );
      gain( tn, act->gain );
      accept( tn, act->accept );
      into( tn, act->into );
      break;
    }
  }

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
