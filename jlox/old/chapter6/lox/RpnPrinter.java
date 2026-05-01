// This is for Chapter 5 Challenge 3
// reverse Polish notation (RPN) 
// (1 + 2) * (4 - 3) 
// in RPN becomes:
// 1 2 + 4 3 - *

// Define a visitor class for our syntax tree classes that takes an expression, 
// converts it to RPN, and returns the resulting string.

package lox;

import lox.Expr;

class RpnPrinter implements Expr.Visitor<String> {
  String print(Expr expr) {
    return expr.accept(this);
  }

  @Override
  public String visitBinaryExpr(Expr.Binary expr) {
    return expr.left.accept(this) + " " +
           expr.right.accept(this) + " " +
           expr.operator.lexeme;
  }

  @Override
  public String visitGroupingExpr(Expr.Grouping expr) {
    return expr.expression.accept(this);
  }

  @Override
  public String visitLiteralExpr(Expr.Literal expr) {
    return expr.value.toString();
  }

  @Override
  public String visitUnaryExpr(Expr.Unary expr) {
    String operator = expr.operator.lexeme;
    if (expr.operator.type == TokenType.MINUS) {
      // Can't use same symbol for unary and binary.
      operator = "~";
    }

    return expr.right.accept(this) + " " + operator;
  }

  public static void main(String[] args) {
    Expr expression = new Expr.Binary(
        new Expr.Unary(
            new Token(TokenType.MINUS, "-", null, 1),
            new Expr.Literal(123)),
        new Token(TokenType.STAR, "*", null, 1),
        new Expr.Grouping(
            new Expr.Literal("str")));

    System.out.println(new RpnPrinter().print(expression));
  }
}
