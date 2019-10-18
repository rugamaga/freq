#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "parser.h"
#include "util.h"

static AST* create_ast(SyntaxType type, Token* token, AST* child, ...);
static AST* parse_stmt(Parser* parser);
static AST* parse_assign(Parser* parser);

static Parser* create_parser(Token* root) {
  Parser* parser = (Parser*)malloc(sizeof(Parser));
  parser->ast = create_ast( ST_ROOT, NULL, NULL );
  parser->current = parser->root = root;
  return parser;
}

static Token* consume(Parser* parser, TokenType type) {
  if( !(parser->current) ) return NULL;
  if( parser->current->type != type ) return NULL;
  Token* consumed = parser->current;
  parser->current = parser->current->next;
  return consumed;
}

static AST* create_ast(SyntaxType type, Token* token, AST* child, ...) {
  AST* node = (AST*)malloc(sizeof(AST));
  node->type = type;
  node->token = token;
  // 数値ノードならtokenをstrtolで解釈する
  if( type == ST_NUM ) {
    const char* sp = token->buffer + token->pos;
    char* ep;
    node->val = strtol(sp, &ep, 10);
  }

  for( size_t i = 0; i < MAX_BLOCK_SIZE; ++i ) {
    node->children[ i ] = NULL;
  }

  va_list ap;
  size_t i = 0;
  va_start(ap, child);
  for (AST* arg = child; arg != NULL; arg = va_arg(ap, AST*)) {
    node->children[ i ] = arg;
    ++i;
  }
  va_end(ap);

  return node;
}

AST* get_lhs(AST* node) {
  return node->children[ 0 ];
}

AST* get_rhs(AST* node) {
  return node->children[ 1 ];
}

static AST* parse_lvar(Parser* parser) {
  Token* tok;
  if( (tok = consume( parser, TT_IDENT )) )
    return create_ast(ST_VAR, tok, NULL, NULL );
  return NULL;
}

static AST* parse_expr(Parser* parser);
static AST* parse_factor(Parser* parser) {
  Token* tok;
  if( consume( parser, TT_LEFT_PAREN ) ) {
    AST* node = parse_expr( parser );
    if( consume( parser, TT_RIGHT_PAREN ) ) {
      return node;
    }
    return NULL;
  }

  if( (tok = consume( parser, TT_NUM )) )
    return create_ast( ST_NUM, tok, NULL, NULL );

  if( (tok = consume( parser, TT_IDENT )) ) {
    if( consume( parser, TT_LEFT_PAREN ) ) {
      AST* node = create_ast( ST_CALL, tok, NULL );
      size_t i = 0;
      do {
        AST* arg = parse_stmt( parser );
        if( !arg ) break;
        node->children[ i++ ] = arg;
      } while( consume(parser, TT_COMMA) );
      consume( parser, TT_RIGHT_PAREN );
      return node;
    } else {
      return create_ast( ST_VAR, tok, NULL, NULL );
    }
  }

  return NULL;
}

static AST* parse_unary(Parser* parser) {
  Token* tok;
  if( (tok = consume( parser, TT_PLUS )) )
    return parse_unary( parser );

  if( (tok = consume( parser, TT_MINUS )) ) {
    // ここはシンタックスシュガーとして生成されるので
    // 後で解釈されるときのためにダミーのトークンを登録しておく
    Token* dummy = create_token(TT_NUM, "0", 0, 1);
    return create_ast( ST_SUB, tok, create_ast( ST_NUM, dummy, NULL, NULL ), parse_unary( parser ), NULL );
  }

  return parse_factor( parser );
}

static AST* parse_term(Parser* parser) {
  AST* node = parse_unary(parser);
  for( ; ; ) {
    Token* tok;
    if( (tok = consume( parser, TT_MUL )) )
      node = create_ast( ST_MUL, tok, node, parse_unary(parser), NULL );
    else if( (tok = consume( parser, TT_DIV )) )
      node = create_ast( ST_DIV, tok, node, parse_unary(parser), NULL );
    else
      return node;
  }
}

static AST* parse_expr(Parser* parser) {
  AST* node = parse_term(parser);
  for( ; ; ) {
    Token* tok;
    if( (tok = consume( parser, TT_PLUS )) )
      node = create_ast( ST_ADD, tok, node, parse_term(parser), NULL );
    else if( (tok = consume( parser, TT_MINUS )) )
      node = create_ast( ST_SUB, tok, node, parse_term(parser), NULL );
    else
      return node;
  }
}


static AST* parse_rational(Parser* parser) {
  AST* node = parse_expr(parser);
  for( ; ; ) {
    Token* tok;
    if( (tok = consume( parser, TT_LT )) )
      node = create_ast( ST_LT, tok, node, parse_expr(parser), NULL );
    else if( (tok = consume( parser, TT_LTEQ )) )
      node = create_ast( ST_LTEQ, tok, node, parse_expr(parser), NULL );
    else if( (tok = consume( parser, TT_GT )) )
      node = create_ast( ST_GT, tok, node, parse_expr(parser), NULL );
    else if( (tok = consume( parser, TT_GTEQ )) )
      node = create_ast( ST_GTEQ, tok, node, parse_expr(parser), NULL );
    else
      return node;
  }
}

static AST* parse_equality(Parser* parser) {
  AST* node = parse_rational(parser);
  for( ; ; ) {
    Token* tok;
    if( (tok = consume( parser, TT_EQUAL )) )
      node = create_ast( ST_EQUAL, tok, node, parse_rational(parser), NULL );
    else if( (tok = consume( parser, TT_NOT_EQUAL )) )
      node = create_ast( ST_NOT_EQUAL, tok, node, parse_rational(parser), NULL );
    else
      return node;
  }
}

static AST* parse_assign(Parser* parser) {
  AST* node = parse_equality(parser);
  Token* tok;
  if( (tok = consume(parser, TT_ASSIGN)) ) {
    return create_ast( ST_ASSIGN, tok, node, parse_assign(parser), NULL );
  } else {
    return node;
  }
}

static AST* parse_let(Parser* parser) {
  Token* tok;
  if( (tok = consume(parser, TT_LET)) ) {
    AST* lhs = parse_lvar(parser);
    AST* rhs = NULL;
    Token* assign;
    if( (assign = consume(parser, TT_ASSIGN)) )
      rhs = parse_assign(parser);
    return create_ast( ST_LET, tok, lhs, rhs, NULL );
  } else {
    return parse_assign(parser);
  }
}

static AST* parse_stmt(Parser* parser) {
  Token* tok;
  if( (tok = consume(parser, TT_LOOP)) ) {
    AST* stmt = parse_stmt(parser);
    return create_ast(ST_LOOP, tok, stmt, NULL);
  } else if( (tok = consume(parser, TT_IF)) ) {
    consume(parser, TT_LEFT_PAREN);
    AST* cond = parse_stmt(parser);
    consume(parser, TT_RIGHT_PAREN);
    AST* when_true = parse_stmt(parser);
    AST* when_false = NULL;
    if( consume(parser, TT_ELSE) ) {
      when_false = parse_stmt(parser);
    } else {
      Token* dummy = create_token(TT_NUM, "0", 0, 1);
      when_false = create_ast( ST_NUM, dummy, NULL );
    }
    return create_ast(ST_IF, tok, cond, when_true, when_false, NULL);
  } else if( (tok = consume(parser, TT_LEFT_BRACE)) ) {
    AST* node = create_ast(ST_BLOCK, tok, NULL);
    size_t i = 0;
    do {
      node->children[ i++ ] = parse_stmt(parser);
    } while( consume(parser, TT_SEMICOLON) );
    consume(parser, TT_RIGHT_BRACE);
    return node;
  } else if( (tok = consume(parser, TT_LET)) ) {
    AST* lhs = parse_lvar(parser);
    AST* rhs = NULL;
    Token* assign;
    if( (assign = consume(parser, TT_ASSIGN)) )
      rhs = parse_stmt(parser);
    return create_ast( ST_LET, tok, lhs, rhs, NULL );
  } else if( (tok = consume(parser, TT_RETURN) ) ){
    AST* node = parse_assign(parser);
    return create_ast( ST_RETURN, tok, node, NULL );
  } else {
    return parse_assign(parser);
  }
}

static AST* parse_args(Parser* parser) {
  Token* tok = consume(parser, TT_LEFT_PAREN);
  AST* node = create_ast(ST_ARGS, tok, NULL);
  size_t i = 0;
  do {
    if( !(tok = consume( parser, TT_IDENT )) ) break;
    node->children[i++] = create_ast(ST_VAR, tok, NULL );
  } while( consume(parser, TT_COMMA) );
  consume(parser, TT_RIGHT_PAREN);
  return node;
}

static AST* parse_func(Parser* parser) {
  if( consume(parser, TT_FUN) ) {
    Token* name = consume(parser, TT_IDENT);
    AST* args = parse_args(parser);
    AST* stmt = parse_stmt(parser);
    return create_ast(ST_FUNC, name, args, stmt);
  }
  return NULL;
}

Parser* parse(Token* token) {
  Parser* parser = create_parser(token);

  // ROOTから始まる
  if( !consume( parser, TT_ROOT ) )
    return NULL;

  // funcをすべて読み込む
  size_t i = 0;
  while( !consume(parser, TT_EOF) ) {
    parser->ast->children[i++] = parse_func(parser);
    consume(parser, TT_SEMICOLON);
  }

  // NULLで終端しておくことで後続で処理できるようにする
  parser->ast->children[i] = NULL;

  return parser;
}

void print_ast(AST* ast, size_t level) {
  if( ast == NULL ) return;
  indent(level); fprintf(stderr, "SyntaxType: %u (%ld)\n", ast->type, ast->val);
  for( AST** child = ast->children; *child; ++child )
    print_ast(*child, level + 1);
}

