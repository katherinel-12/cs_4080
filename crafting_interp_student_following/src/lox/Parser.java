package lox;

import java.util.List;

import static lox.TokenType.*;

// each grammar rule becomes a method inside this class
// consumes a flat input sequence like the Scanner, now reading tokens instead of characters
class Parser {
    //    error() method returns the error instead of throwing it with this
    private static class ParseError extends RuntimeException {}

    private final List<Token> tokens;
    private int current = 0;

    Parser(List<Token> tokens) {
        this.tokens = tokens;
    }

    //    define an initial method to kick the parser off
    Expr parse() {
        try {
            return expression();
        } catch (ParseError error) {
            return null;
        }
    }

    //    expression     → equality ;
    private Expr expression() {
        return equality();
    }

    //    equality       → comparison ( ( "!=" | "==" ) comparison )* ;
    private Expr equality() {
        Expr expr = comparison();

        while (match(BANG_EQUAL, EQUAL_EQUAL)) {
            Token operator = previous();
            Expr right = comparison();
            expr = new Expr.Binary(expr, operator, right);
        }

        return expr;
    }

    //    comparison     → term ( ( ">" | ">=" | "<" | "<=" ) term )* ;
    private Expr comparison() {
        Expr expr = term();

        while (match(GREATER, GREATER_EQUAL, LESS, LESS_EQUAL)) {
            Token operator = previous();
            Expr right = term();
            expr = new Expr.Binary(expr, operator, right);
        }

        return expr;
    }

    //    term           → factor ( ( "-" | "+" ) factor )* ;
    private Expr term() {
        Expr expr = factor();

        while (match(MINUS, PLUS)) {
            Token operator = previous();
            Expr right = factor();
            expr = new Expr.Binary(expr, operator, right);
        }

        return expr;
    }

    //    factor         → unary ( ( "/" | "*" ) unary )* ;
    //    multiplication and division
    private Expr factor() {
        Expr expr = unary();

        while (match(SLASH, STAR)) {
            Token operator = previous();
            Expr right = unary();
            expr = new Expr.Binary(expr, operator, right);
        }

        return expr;
    }

    //    unary          → ( "!" | "-" ) unary | primary ;
    private Expr unary() {
        if (match(BANG, MINUS)) {
            Token operator = previous();
            Expr right = unary();
            return new Expr.Unary(operator, right);
        }

        return primary();
    }

    //    primary        → NUMBER | STRING | "true" | "false" | "nil" | "(" expression ")" ;
    private Expr primary() {
        if (match(FALSE)) return new Expr.Literal(false);
        if (match(TRUE)) return new Expr.Literal(true);
        if (match(NIL)) return new Expr.Literal(null);

        if (match(NUMBER, STRING)) {
            return new Expr.Literal(previous().literal);
        }

        if (match(LEFT_PAREN)) {
            Expr expr = expression();
            consume(RIGHT_PAREN, "Expect ')' after expression.");
            return new Expr.Grouping(expr);
        }

        //    handle error: got to a token that can’t start an expression
        throw error(peek(), "Expect expression.");
    }

    //    checks to see if the current token has any of the given types
    //    yes, it consumes the token and returns true
    //    no, it returns false and leaves the current token alone
    private boolean match(TokenType... types) {
        for (TokenType type : types) {
            if (check(type)) {
                advance();
                return true;
            }
        }

        return false;
    }

    //    the parser looks for the closing ) by calling consume()
    private Token consume(TokenType type, String message) {
        if (check(type)) return advance();

        throw error(peek(), message);
    }

    //    check() returns true if the current token is of the given type
    private boolean check(TokenType type) {
        if (isAtEnd()) return false;
        return peek().type == type;
    }

    //    advance() consumes the current token and returns it
    private Token advance() {
        if (!isAtEnd()) current++;
        return previous();
    }

    //    isAtEnd() checks if we’ve run out of tokens to parse
    private boolean isAtEnd() {
        return peek().type == EOF;
    }

    //    peek() returns the current token we have YET to consume
    private Token peek() {
        return tokens.get(current);
    }

    //    previous() returns the most recently consumed token
    //    makes it easier to use match() and then access the just-matched token
    private Token previous() {
        return tokens.get(current - 1);
    }

    //    consume() checks to see if the next token is of the expected type
    //    yes, it consumes the token
    //    no, then error
    private ParseError error(Token token, String message) {
        Lox.error(token, message);
        return new ParseError();
    }

    //    Synchronizing (error recovery)
    //    We want to:
    //      Skip bad tokens
    //      Resume parsing at the next valid statement
    //    discard tokens until we reach the beginning of the next statement
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
