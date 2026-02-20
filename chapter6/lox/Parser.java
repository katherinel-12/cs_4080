package lox;

import java.util.List;

import tool.Expr_v3;

import static lox.TokenType.*;

class Parser {
  private static class ParseError extends RuntimeException {}

  private final List<Token> tokens;
  private int current = 0;

  Parser(List<Token> tokens) {
    this.tokens = tokens;
  }

  Expr_v3 parse() {
    try {
      return expression();
    } catch (ParseError error) {
      return null;
    }
  }
  
  private Expr_v3 expression() {
    return equality();
  }
  
  private Expr_v3 equality() {
    Expr_v3 expr = comparison();

    while (match(BANG_EQUAL, EQUAL_EQUAL)) {
      Token operator = previous();
      Expr_v3 right = comparison();
      expr = new Expr_v3.Binary(expr, operator, right);
    }

    return expr;
  }

  private Expr_v3 comma() {
    Expr_v3 expr = equality(); 

    while (match(COMMA)) {
      Expr_v3 right = equality(); 
      expr = new Expr_v3.Comma(expr, right); 
    }
    
    return expr;
  }
  
  private Expr_v3 comparison() {
    Expr_v3 expr = term();

    while (match(GREATER, GREATER_EQUAL, LESS, LESS_EQUAL)) {
      Token operator = previous();
      Expr_v3 right = term();
      expr = new Expr_v3.Binary(expr, operator, right);
    }

    return expr;
  }

  private Expr_v3 term() {
    Expr_v3 expr = factor();

    while (match(MINUS, PLUS)) {
      Token operator = previous();
      Expr_v3 right = factor();
      expr = new Expr_v3.Binary(expr, operator, right);
    }

    return expr;
  }

  private Expr_v3 factor() {
    Expr_v3 expr = unary();

    while (match(SLASH, STAR)) {
      Token operator = previous();
      Expr_v3 right = unary();
      expr = new Expr_v3.Binary(expr, operator, right);
    }

    return expr;
  }
  
  private Expr_v3 unary() {
    if (match(BANG, MINUS)) {
      Token operator = previous();
      Expr_v3 right = unary();
      return new Expr_v3.Unary(operator, right);
    }

    return primary();
  }
  
  private Expr_v3 primary() {
    if (match(FALSE)) return new Expr_v3.Literal(false);
    if (match(TRUE)) return new Expr_v3.Literal(true);
    if (match(NIL)) return new Expr_v3.Literal(null);

    if (match(NUMBER, STRING)) {
      return new Expr_v3.Literal(previous().literal);
    }

    if (match(LEFT_PAREN)) {
      Expr_v3 expr = expression();
      consume(RIGHT_PAREN, "Expect ')' after expression.");
      return new Expr_v3.Grouping(expr);
    }

    throw error(peek(), "Expect expression.");
  }

  private boolean match(TokenType... types) {
    for (TokenType type : types) {
      if (check(type)) {
        advance();
        return true;
      }
    }

    return false;
  }

  private Token consume(TokenType type, String message) {
    if (check(type)) return advance();

    throw error(peek(), message);
  }
  
  private boolean check(TokenType type) {
    if (isAtEnd()) return false;
    return peek().type == type;
  }
  
  private Token advance() {
    if (!isAtEnd()) current++;
    return previous();
  }
  
  private boolean isAtEnd() {
    return peek().type == EOF;
  }

  private Token peek() {
    return tokens.get(current);
  }

  private Token previous() {
    return tokens.get(current - 1);
  }
  
  private ParseError error(Token token, String message) {
    Lox.error(token, message);
    return new ParseError();
  }

  private void synchronize() {
    advance();

    while (!isAtEnd()) {
      if (previous().type == SEMICOLON) return;

      switch (peek().type) {
        case CLASS:
        case FUN:
        case VAR:
        case FOR:
        case IF:
        case WHILE:
        case PRINT:
        case RETURN:
          return;
      }

      advance();
    }
  }
}
