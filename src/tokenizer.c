#include <stdio.h>
#include <stdlib.h>

#include "tokenizer.h"
#include "util.h"

typedef enum {
  TS_EMPTY,
  TS_NUM,
  TS_EQUAL,
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

Token* tokenize(const char* buffer, size_t len) {
  // トークンは常に0文字目のTT_ROOTから
  // 始まってると考えることにして
  // 何かと楽をしましょう。
  Token* root = create_token(TT_ROOT, buffer, 0, 0);

  // トークンをどんどん追加しながら処理しよう
  Token* current = root;

  // ステートマシンとして全体の処理を行う
  TokenizerState state = TS_EMPTY;
  size_t pos = 0;
  size_t beg = 0;

  while( pos < len ) {
    const char c = buffer[pos];

    if( state == TS_EMPTY ) {
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
        beg = pos;
        pos += 1;
        state = TS_EMPTY;
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
        beg = pos;
        pos += 1;
        state = TS_NUM;
        continue;
      }

      // TS_EMPTYでイコールが着たら、多分Equal。
      if(c == '=') {
        beg = pos;
        pos += 1;
        state = TS_EQUAL;
        continue;
      }

      if( c == '+' ) {
        beg = pos;
        pos += 1;
        Token* t = create_token(TT_PLUS, buffer, beg, pos - beg);
        current->next = t;
        current = t;
        state = TS_EMPTY;
        continue;
      }

      if( c == '-' ) {
        beg = pos;
        pos += 1;
        Token* t = create_token(TT_MINUS, buffer, beg, pos - beg);
        current->next = t;
        current = t;
        state = TS_EMPTY;
        continue;
      }

      if( c == '*' ) {
        beg = pos;
        pos += 1;
        Token* t = create_token(TT_MUL, buffer, beg, pos - beg);
        current->next = t;
        current = t;
        state = TS_EMPTY;
        continue;
      }

      if( c == '/' ) {
        beg = pos;
        pos += 1;
        Token* t = create_token(TT_DIV, buffer, beg, pos - beg);
        current->next = t;
        current = t;
        state = TS_EMPTY;
        continue;
      }

      if( c == '(' ) {
        beg = pos;
        pos += 1;
        Token* t = create_token(TT_LEFT_BRACKET, buffer, beg, pos - beg);
        current->next = t;
        current = t;
        state = TS_EMPTY;
        continue;
      }

      if( c == ')' ) {
        beg = pos;
        pos += 1;
        Token* t = create_token(TT_RIGHT_BRACKET, buffer, beg, pos - beg);
        current->next = t;
        current = t;
        state = TS_EMPTY;
        continue;
      }

      // TS_EMPTYでここまでの以外が来るのはまずいですよ！
      fprintf(stderr, "Tokenize中に予想外の文字(%zu文字目の'%c')が来てしまいました。\n", pos, c);
      exit(1);
    }

    if( state == TS_NUM ) {
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
        pos += 1;
        continue;
      }

      // 数字でない何かが来たということは、
      // この時点で数値リテラルは終わったということ。
      // ということでトークンにしてしまう
      Token* t = create_token(TT_NUM, buffer, beg, pos - beg);
      current->next = t;
      current = t;

      // 文字の読み込みはしてないのでposは移動させなくていい
      beg = pos;

      // トークンを読み込んだので行き先状態はTS_EMPTY
      state = TS_EMPTY;

      continue;
    }

    if( state == TS_EQUAL ) {
      // ここでイコールがきたということは、
      // 同値判定のためのイコールだったということです
      if( c == '=' ) {
        Token* t = create_token(TT_EQUAL, buffer, beg, pos - beg);
        current->next = t;
        current = t;
      }

      // 取り扱えない文字なのでエラーを出して落とす
      fprintf(stderr, "Tokenize中に予想外の文字(%zu文字目の'%c')が着てしまいました。\n", pos, c);

      // メモリ解放は頑張らない(D言語方式)
      // free_token(current);
      return NULL;
    }

    // 取り扱えない文字なのでエラーを出して落とす
    fprintf(stderr, "Tokenize中に予想外の文字(%zu文字目の'%c')が着てしまいました。\n", pos, c);

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
