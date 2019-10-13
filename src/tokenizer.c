#include <stdio.h>
#include <stdlib.h>

#include "tokenizer.h"
#include "util.h"

typedef enum {
  TS_EMPTY,
  TS_NUM,
  TS_EQUAL,
  TS_NOT,
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
  Token* current;
  TokenizerState state;
  size_t beg;
  size_t pos;
  size_t len;
} Tokenizer;

Token* tokenize(const char* buffer, size_t len) {
  // トークンは常に0文字目のTT_ROOTから
  // 始まってると考えることにして
  // 何かと楽をしましょう。
  Token* root = create_token(TT_ROOT, buffer, 0, 0);

  // ステートマシンとして全体の処理を行う
  Tokenizer* tn = (Tokenizer*)malloc(sizeof(Tokenizer));
  tn->buffer = buffer;
  tn->current = root;
  tn->state = TS_EMPTY;
  tn->beg = 0;
  tn->pos = 0;
  tn->len = len;

  while( tn->pos < tn->len ) {
    const char c = tn->buffer[tn->pos];

    if( tn->state == TS_EMPTY ) {
      // バッファの終了。
      // TS_EMPTYでバッファの終りが来るのが正常な終了。
      // なので読み取れたトークン情報のルートを返して終了
      if( c == '\0' ) {
        break;
      }

      // この言語は改行やスペースでトークンを区切っていく
      // しかしTS_EMPTYモードでのスペースやトークンとは、
      // つまるところ「連続でスペースだったね…」ということ。
      // まだ何もトークンがないので読み飛ばす以外にすることはない
      if( c == ' ' || c == '\t' || c == '\r' || c == '\n' ) {
        tn->beg = tn->pos;
        tn->pos += 1;
        tn->state = TS_EMPTY;
        continue;
      }

      // TS_EMPTYで数値が始まったら、それは数値だ。
      if(
          c == '0' ||
          c == '1' ||
          c == '2' ||
          c == '3' ||
          c == '4' ||
          c == '5' ||
          c == '6' ||
          c == '7' ||
          c == '8' ||
          c == '9'
      ) {
        // 1文字読んで数値解析モードに飛ぶ
        tn->beg = tn->pos;
        tn->pos += 1;
        tn->state = TS_NUM;
        continue;
      }

      // TS_EMPTYでイコールが着たら、多分Equal。
      if( c == '=' ) {
        tn->beg = tn->pos;
        tn->pos += 1;
        tn->state = TS_EQUAL;
        continue;
      }

      // TS_EMPTYでノットが着たら、多分Not。
      if( c == '!' ) {
        tn->beg = tn->pos;
        tn->pos += 1;
        tn->state = TS_NOT;
        continue;
      }

      if( c == '+' ) {
        tn->beg = tn->pos;
        tn->pos += 1;
        Token* t = create_token(TT_PLUS, tn->buffer, tn->beg, tn->pos - tn->beg);
        tn->current->next = t;
        tn->current = t;
        tn->state = TS_EMPTY;
        continue;
      }

      if( c == '-' ) {
        tn->beg = tn->pos;
        tn->pos += 1;
        Token* t = create_token(TT_MINUS, tn->buffer, tn->beg, tn->pos - tn->beg);
        tn->current->next = t;
        tn->current = t;
        tn->state = TS_EMPTY;
        continue;
      }

      if( c == '*' ) {
        tn->beg = tn->pos;
        tn->pos += 1;
        Token* t = create_token(TT_MUL, tn->buffer, tn->beg, tn->pos - tn->beg);
        tn->current->next = t;
        tn->current = t;
        tn->state = TS_EMPTY;
        continue;
      }

      if( c == '/' ) {
        tn->beg = tn->pos;
        tn->pos += 1;
        Token* t = create_token(TT_DIV, tn->buffer, tn->beg, tn->pos - tn->beg);
        tn->current->next = t;
        tn->current = t;
        tn->state = TS_EMPTY;
        continue;
      }

      if( c == '(' ) {
        tn->beg = tn->pos;
        tn->pos += 1;
        Token* t = create_token(TT_LEFT_BRACKET, tn->buffer, tn->beg, tn->pos - tn->beg);
        tn->current->next = t;
        tn->current = t;
        tn->state = TS_EMPTY;
        continue;
      }

      if( c == ')' ) {
        tn->beg = tn->pos;
        tn->pos += 1;
        Token* t = create_token(TT_RIGHT_BRACKET, tn->buffer, tn->beg, tn->pos - tn->beg);
        tn->current->next = t;
        tn->current = t;
        tn->state = TS_EMPTY;
        continue;
      }

      // TS_EMPTYでここまでの以外が来るのはまずいですよ！
      fprintf(stderr, "Tokenize中に予想外の文字(%zu文字目の'%c')が来てしまいました。\n", tn->pos, c);
      exit(1);
    }

    if( tn->state == TS_NUM ) {
      // 数字以外の文字が来たってことは、ここまでが数値リテラルだったってことでしょ？
      // TS_EMPTYで数値が始まったら、それは数値だ。
      if(
          c == '0' ||
          c == '1' ||
          c == '2' ||
          c == '3' ||
          c == '4' ||
          c == '5' ||
          c == '6' ||
          c == '7' ||
          c == '8' ||
          c == '9'
      ) {
        // 引き続き数値として処理する
        tn->pos += 1;
        continue;
      }

      // 数字でない何かが来たということは、
      // この時点で数値リテラルは終わったということ。
      // ということでトークンにしてしまう
      Token* t = create_token(TT_NUM, tn->buffer, tn->beg, tn->pos - tn->beg);
      tn->current->next = t;
      tn->current = t;

      // 文字の読み込みはしてないのでposは移動させなくていい
      tn->beg = tn->pos;

      // トークンを読み込んだので行き先状態はTS_EMPTY
      tn->state = TS_EMPTY;

      continue;
    }

    if( tn->state == TS_EQUAL ) {
      // ここでイコールがきたということは、
      // 同値判定のためのイコールだったということです
      if( c == '=' ) {
        Token* t = create_token(TT_EQUAL, tn->buffer, tn->beg, tn->pos - tn->beg);
        tn->current->next = t;
        tn->current = t;

        tn->beg = tn->pos;
        tn->pos += 1;
        // トークンを読み込んだので行き先状態はTS_EMPTY
        tn->state = TS_EMPTY;
        continue;
      }

      // 取り扱えない文字なのでエラーを出して落とす
      fprintf(stderr, "Tokenize中に予想外の文字(%zu文字目の'%c')が着てしまいました。\n", tn->pos, c);

      // メモリ解放は頑張らない(D言語方式)
      // free_token(current);
      return NULL;
    }

    if( tn->state == TS_NOT ) {
      // ここでイコールがきたということは、
      // 不等号だったということです
      if( c == '=' ) {
        Token* t = create_token(TT_NOT_EQUAL, tn->buffer, tn->beg, tn->pos - tn->beg);
        tn->current->next = t;
        tn->current = t;

        tn->beg = tn->pos;
        tn->pos += 1;
        // トークンを読み込んだので行き先状態はTS_EMPTY
        tn->state = TS_EMPTY;
        continue;
      }

      // 取り扱えない文字なのでエラーを出して落とす
      fprintf(stderr, "Tokenize中に予想外の文字(%zu文字目の'%c')が着てしまいました。\n", tn->pos, c);

      // メモリ解放は頑張らない(D言語方式)
      // free_token(current);
      return NULL;
    }

    // 取り扱えない文字なのでエラーを出して落とす
    fprintf(stderr, "Tokenize中に予想外の文字(%zu文字目の'%c')が着てしまいました。\n", tn->pos, c);

    // メモリ解放は頑張らない(D言語方式)
    // free_token(current);
    return NULL;
  }

  return root;
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
    fprintf(stderr, ")\n");
  }
}
