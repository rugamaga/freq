#include <stdio.h>
#include <stdlib.h>

#include "tokenizer.h"
#include "util.h"

typedef enum {
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

static void reset(Tokenizer* tn) {
  tn->beg = tn->pos;
}

static void gain(Tokenizer* tn) {
  tn->pos += 1;
}

static void accept(Tokenizer* tn, TokenType type) {
  Token* t = create_token(type, tn->buffer, tn->beg, tn->pos - tn->beg);
  tn->current->next = t;
  tn->current = t;
}

static void into(Tokenizer* tn, TokenizerState state) {
  tn->state = state;
}

Token* tokenize(const char* buffer, size_t len) {
  Tokenizer* tn = create_tokenizer(buffer, len);

  while( tn->pos < tn->len ) {
    const char c = tn->buffer[tn->pos];

    if( tn->state == TS_EMPTY ) {
      // バッファの終了。
      // TS_EMPTYでバッファの終りが来るのが正常な終了。
      // なので読み取れたトークン情報のルートを返して終了
      if( c == '\0' ) {
        gain(tn);
        accept(tn, TT_EOF);
        reset(tn);
        // 終了状態は導入してないけどあってもいいかもね
        // into(tn, TS_END);
        break;
      }

      // この言語は改行やスペースでトークンを区切っていく
      // しかしTS_EMPTYモードでのスペースやトークンとは、
      // つまるところ「連続でスペースだったね…」ということ。
      // まだ何もトークンがないので読み飛ばす以外にすることはない
      if( c == ' ' || c == '\t' || c == '\r' || c == '\n' ) {
        gain(tn);
        // TODO: あえて受け取っておいて実際にはacceptしないみたいな作りにすると統一感出る？
        // accept(tn, TT_SKIP);
        reset(tn);
        into(tn, TS_EMPTY);
        continue;
      }

      // 'l'が来たならTT_LETっぽさがある
      if( c == 'l' ) {
        gain(tn);
        into(tn, TS_LET_0);
        continue;
      }

      if( 'a' <= c && c <= 'z' ) {
        gain(tn);
        into(tn, TS_IDENT);
        continue;
      }

      // TS_EMPTYで数値が始まったら、それは数値だ。
      if( '0' <= c && c <= '9' ) {
        // 1文字読んで数値解析モードに飛ぶ
        gain(tn);
        into(tn, TS_NUM);
        continue;
      }

      // TS_EMPTYでイコールが着たら、多分Equal。
      if( c == '=' ) {
        gain(tn);
        into(tn, TS_EQUAL);
        continue;
      }

      // TS_EMPTYでノットが着たら、多分Not。
      if( c == '!' ) {
        gain(tn);
        into(tn, TS_NOT);
        continue;
      }

      // TS_EMPTYでLTが着たら、多分LT
      if( c == '<' ) {
        gain(tn);
        into(tn, TS_LT);
        continue;
      }

      // TS_EMPTYでLTが着たら、多分LT
      if( c == '>' ) {
        gain(tn);
        into(tn, TS_GT);
        continue;
      }

      if( c == '+' ) {
        gain(tn);
        accept(tn, TT_PLUS);
        reset(tn);
        into(tn, TS_EMPTY);
        continue;
      }

      if( c == '-' ) {
        gain(tn);
        accept(tn, TT_MINUS);
        reset(tn);
        into(tn, TS_EMPTY);
        continue;
      }

      if( c == '*' ) {
        gain(tn);
        accept(tn, TT_MUL);
        reset(tn);
        into(tn, TS_EMPTY);
        continue;
      }

      if( c == '/' ) {
        gain(tn);
        accept(tn, TT_DIV);
        reset(tn);
        into(tn, TS_EMPTY);
        continue;
      }

      if( c == '(' ) {
        gain(tn);
        accept(tn, TT_LEFT_BRACKET);
        reset(tn);
        into(tn, TS_EMPTY);
        continue;
      }

      if( c == ')' ) {
        gain(tn);
        accept(tn, TT_RIGHT_BRACKET);
        reset(tn);
        into(tn, TS_EMPTY);
        continue;
      }

      if( c == ';' ) {
        gain(tn);
        accept(tn, TT_SEMICOLON);
        reset(tn);
        into(tn, TS_EMPTY);
        continue;
      }

      // TS_EMPTYでここまでの以外が来るのはまずいですよ！
      fprintf(stderr, "Tokenize中に予想外の文字(%zu文字目の'%c')が来てしまいました。\n", tn->pos, c);
      exit(1);
    }

    if( tn->state == TS_LET_0 ) {
      // 'e'が来るならlet節っぽさが増す
      if( c == 'e' ) {
        gain(tn);
        into(tn, TS_LET_1);
        continue;
      }

      if(
          ('a' <= c && c <= 'z') ||
          ('0' <= c && c <= '9')
      ) {
        gain(tn);
        into(tn, TS_IDENT);
        continue;
      }

      // それ以外の文字が来たので打ち切り
      accept(tn, TT_IDENT);
      reset(tn);
      into(tn, TS_EMPTY);
    }

    if( tn->state == TS_LET_1 ) {
      // 't'が来るならlet節っぽさが更に増す
      if( c == 't' ) {
        gain(tn);
        into(tn, TS_LET_2);
        continue;
      }

      // そうじゃないならIDENTだったんだよ。
      if(
          ('a' <= c && c <= 'z') ||
          ('0' <= c && c <= '9')
      ) {
        gain(tn);
        into(tn, TS_IDENT);
        continue;
      }

      // それら以外なら
      // ここまでIDENTを処理していて
      // そのIDENTが今終わったのだ
      accept(tn, TT_IDENT);
      reset(tn);
      into(tn, TS_EMPTY);
    }

    if( tn->state == TS_LET_2 ) {
      // ident的な文字が来てしまうなら
      // 実はletじゃなくてidentだったので
      // そのままidentとしてがんばってもらう
      if(
          ('a' <= c && c <= 'z') ||
          ('0' <= c && c <= '9')
      ) {
        reset(tn);
        gain(tn);
        into(tn, TS_IDENT);
        continue;
      }

      // それ以外の文字が来たのでついにletであることが判明！
      accept(tn, TT_LET);
      reset(tn);
      into(tn, TS_EMPTY);
      continue;
    }

    if( tn->state == TS_NUM ) {
      // 数字以外の文字が来たってことは、ここまでが数値リテラルだったってことでしょ？
      // TS_EMPTYで数値が始まったら、それは数値だ。
      if( '0' <= c && c <= '9' ) {
        // 引き続き数値として処理する
        gain(tn);
        into(tn, TS_NUM);
        continue;
      }

      // 数字でない何かが来たということは、
      // この時点で数値リテラルは終わったということ。
      // ということでトークンにしてしまう
      accept(tn, TT_NUM);

      // 文字の読み込みはしてないのでposは移動させなくていい
      reset(tn);

      // トークンを読み込んだので行き先状態はTS_EMPTY
      into(tn, TS_EMPTY);

      continue;
    }

    if( tn->state == TS_EQUAL ) {
      // ここでイコールがきたということは、
      // 同値判定のためのイコールだったということです
      if( c == '=' ) {
        gain(tn);
        accept(tn, TT_EQUAL);
        reset(tn);
        // トークンを読み込んだので行き先状態はTS_EMPTY
        into(tn, TS_EMPTY);
        continue;
      }

      // そうでにないなら、これは代入演算子なのでは？
      accept(tn, TT_ASSIGN);
      reset(tn);
      into(tn, TS_EMPTY);
      continue;
    }

    if( tn->state == TS_NOT ) {
      // ここでイコールがきたということは、
      // 不等号だったということです
      if( c == '=' ) {
        gain(tn);
        accept(tn, TT_NOT_EQUAL);
        reset(tn);
        // トークンを読み込んだので行き先状態はTS_EMPTY
        into(tn, TS_EMPTY);
        continue;
      }

      // 取り扱えない文字なのでエラーを出して落とす
      fprintf(stderr, "Tokenize中に予想外の文字(%zu文字目の'%c')が着てしまいました。\n", tn->pos, c);

      // メモリ解放は頑張らない(D言語方式)
      // free_token(current);
      return NULL;
    }

    if( tn->state == TS_LT ) {
      // ここでイコールがきたということは、
      // LTEQだったということです
      if( c == '=' ) {
        gain(tn);
        accept(tn, TT_LTEQ);
        reset(tn);
        // トークンを読み込んだので行き先状態はTS_EMPTY
        into(tn, TS_EMPTY);
        continue;
      }

      // それ以外の文字が来たということは
      // LTだったということです
      accept(tn, TT_LT);
      reset(tn);
      into(tn, TS_EMPTY);

      continue;
    }

    if( tn->state == TS_GT ) {
      // ここでイコールがきたということは、
      // GTEQだったということです
      if( c == '=' ) {
        gain(tn);
        accept(tn, TT_GTEQ);
        reset(tn);
        // トークンを読み込んだので行き先状態はTS_EMPTY
        into(tn, TS_EMPTY);
        continue;
      }

      // それ以外の文字が来たということは
      // GTだったということです
      accept(tn, TT_GT);
      reset(tn);
      into(tn, TS_EMPTY);

      continue;
    }

    if( tn->state == TS_IDENT ) {
      if(
        // 引き続き文字がきているようならIDENT
        ('a' <= c && c <= 'z') ||
        // 2文字目以降は数字も許す
        ('0' <= c && c <= '9')
      ) {
        gain(tn);
        into(tn, TS_IDENT);
        continue;
      }

      // それ以外の文字が来たので打ち切り
      accept(tn, TT_IDENT);
      reset(tn);
      into(tn, TS_EMPTY);
      continue;
    }

    // 取り扱えない文字なのでエラーを出して落とす
    fprintf(stderr, "Tokenize中に予想外の文字(%zu文字目の'%c')が着てしまいました。\n", tn->pos, c);

    // メモリ解放は頑張らない(D言語方式)
    // free_token(current);
    return NULL;
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
