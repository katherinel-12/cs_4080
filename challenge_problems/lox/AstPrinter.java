package lox;

// This file is giving me errors right now and making Lox unrunnable
// Line 8

import java.util.List;

class AstPrinter implements Expr.Visitor<String> {
//class AstPrinter implements Expr.Visitor<String>, Stmt.Visitor<String> {
    String print(Expr expr) {
        return expr.accept(this);
    }

    //    visit methods for each of the expression types we have so far
    @Override
    public String visitBinaryExpr(Expr.Binary expr) {
        return parenthesize(expr.operator.lexeme,
                expr.left, expr.right);
    }

    @Override
    public String visitGroupingExpr(Expr.Grouping expr) {
        return parenthesize("group", expr.expression);
    }

    @Override
    public String visitLiteralExpr(Expr.Literal expr) {
        if (expr.value == null) return "nil";
        return expr.value.toString();
    }

    @Override
    public String visitUnaryExpr(Expr.Unary expr) {
        return parenthesize(expr.operator.lexeme, expr.right);
    }

    //    parenthesize() helper method to help visitBinaryExpr, visitGroupingExpr, and visitUnaryExpr
    private String parenthesize(String name, Expr... exprs) {
        StringBuilder builder = new StringBuilder();

        builder.append("(").append(name);
        for (Expr expr : exprs) {
            builder.append(" ");
            //    calls accept() on each subexpression and passes in itself
            //    the recursive step that lets us print an entire tree
            builder.append(expr.accept(this));
        }
        builder.append(")");

        return builder.toString();
    }

    //    manually instantiates a tree and prints it
    public static void main(String[] args) {
        Expr expression = new Expr.Binary(
                new Expr.Unary(
                        new Token(TokenType.MINUS, "-", null, 1),
                        new Expr.Literal(123)),
                new Token(TokenType.STAR, "*", null, 1),
                new Expr.Grouping(
                        new Expr.Literal(45.67)));

        System.out.println(new AstPrinter().print(expression));
    }
}
// output: (* (- 123) (group 45.67))

// Right-click AstPrinter.java →
// Click Run 'AstPrinter.main()'
// this automatically create the correct config