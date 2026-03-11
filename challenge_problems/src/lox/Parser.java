package lox;

import java.util.List;

import lox.Stmt.If;

import java.util.Arrays;
import java.util.ArrayList;

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

    //    program        → declaration* EOF ;
    //    statement      → exprStmt | printStmt ;
    List<Stmt> parse() {
        List<Stmt> statements = new ArrayList<>();
        while (!isAtEnd()) {
            statements.add(declaration());
        }

        return statements;
    }

    //    expression     → equality ;
    private Expr expression() {
        return assignment();
    }

    //    declaration    → funDecl | varDecl | statement ;
    //    funDecl        → "fun" function ;
    //    function       → IDENTIFIER "(" parameters? ")" block ;
    //    parameters     → IDENTIFIER ( "," IDENTIFIER )* ;
    private Stmt declaration() {
        try {
            if (check(FUN) && checkNext(IDENTIFIER)) {
                consume(FUN, null);
                return function("function");
            }
            if (match(VAR)) return varDeclaration();

            return statement();
        } catch (ParseError error) {
            synchronize();
            return null;
        }
    }

    //    statement      → exprStmt | forStmt | ifStmt | printStmt | returnStmt | whileStmt | block ;
    //    block          → "{" declaration* "}" ;
    //    a program is a list of statements
    //    a block is a (possibly empty) series of statements or declarations
    //    surrounded by curly braces and can appear anywhere a statement is allowed
    //    we parse one of those statements using this method:
    private Stmt statement() {
        if (match(FOR)) return forStatement();
        //    parser recognizes an if statement by the leading if keyword
        if (match(IF)) return ifStatement();
        if (match(PRINT)) return printStatement();
        if (match(RETURN)) return returnStatement();
        if (match(WHILE)) return whileStatement();
        if (match(LEFT_BRACE)) return new Stmt.Block(block());

        return expressionStatement();
    }

    //    forStmt        → "for" "(" ( varDecl | exprStmt | ";" )
    //                     expression? ";" expression? ")" statement ;
    //    our interpreter now supports C-style for loops
    private Stmt forStatement() {
        //    opening parenthesis before the clauses
        consume(LEFT_PAREN, "Expect '(' after 'for'.");

        //    the initializer
        //    if the token following the ( is a ; then the initializer has been omitted
        //    otherwise, check for a var keyword to see if it’s a variable declaration
        //    if neither, it must be an expression
        Stmt initializer;
        if (match(SEMICOLON)) {
            initializer = null;
        } else if (match(VAR)) {
            initializer = varDeclaration();
        } else {
            initializer = expressionStatement();
        }

        //    the condition
        //    again, look for a ; to see if the clause has been omitted
        Expr condition = null;
        if (!check(SEMICOLON)) {
            condition = expression();
        }
        consume(SEMICOLON, "Expect ';' after loop condition.");

        //    the last clause is the increment
        //    terminated by the closing parenthesis
        Expr increment = null;
        if (!check(RIGHT_PAREN)) {
            increment = expression();
        }
        consume(RIGHT_PAREN, "Expect ')' after for clauses.");

        Stmt body = statement();

        //    the increment, if there is one, executes after the body in each iteration of the loop:
        //    replace the body with a little block that contains the original body
        //    followed by an expression statement that evaluates the increment
        if (increment != null) {
            body = new Stmt.Block(
                    Arrays.asList(
                            body,
                            new Stmt.Expression(increment)));
        }

        if (initializer != null) {
            body = new Stmt.Block(Arrays.asList(initializer, body));
        }

        return body;
    }

    //    ifStmt         → "if" "(" expression ")" statement ( "else" statement )? ;
    private Stmt ifStatement() {
        consume(LEFT_PAREN, "Expect '(' after 'if'.");
        Expr condition = expression();
        consume(RIGHT_PAREN, "Expect ')' after if condition.");

        Stmt thenBranch = statement();
        Stmt elseBranch = null;
        if (match(ELSE)) {
            elseBranch = statement();
        }

        return new Stmt.If(condition, thenBranch, elseBranch);
    }

    //    a print statement evaluates an expression and displays the result to the user
    //    parse the subsequent expression, consume the terminating semicolon, & emit the syntax tree
    private Stmt printStatement() {
        Expr value = expression();
        consume(SEMICOLON, "Expect ';' after value.");
        return new Stmt.Print(value);
    }

    //    returnStmt     → "return" expression? ";" ;
    private Stmt returnStatement() {
        Token keyword = previous();
        Expr value = null;
        if (!check(SEMICOLON)) {
            value = expression();
        }

        consume(SEMICOLON, "Expect ';' after return value.");
        return new Stmt.Return(keyword, value);
    }

    //    varDecl        → "var" IDENTIFIER ( "=" expression )? ";" ;
    private Stmt varDeclaration() {
        Token name = consume(IDENTIFIER, "Expect variable name.");

        Expr initializer = null;
        if (match(EQUAL)) {
            initializer = expression();
        }

        consume(SEMICOLON, "Expect ';' after variable declaration.");
        return new Stmt.Var(name, initializer);
    }

    //    whileStmt      → "while" "(" expression ")" statement ;
    private Stmt whileStatement() {
        consume(LEFT_PAREN, "Expect '(' after 'while'.");
        Expr condition = expression();
        consume(RIGHT_PAREN, "Expect ')' after condition.");
        Stmt body = statement();

        return new Stmt.While(condition, body);
    }

    //    parse an expression followed by a semicolon,
    //    wrap that Expr in a Stmt of the correct type & return it
    private Stmt expressionStatement() {
        Expr expr = expression();
        consume(SEMICOLON, "Expect ';' after expression.");
        return new Stmt.Expression(expr);
    }

    //    consumes the identifier token for the function’s name
    private Stmt.Function function(String kind) {
        //    re: kind 
        //    reuse the function() method later to parse methods inside classes
        //    pass in “method” for kind so the error messages are specific to the kind of declaration being parsed 
        Token name = consume(IDENTIFIER, "Expect " + kind + " name.");
        return new Stmt.Function(name, functionBody(kind));

        // //    handling arguments in a call 
        // //    outer if statement handles the zero parameter case 
        // //    inner while loop parses parameters as long as we find commas to separate them
        // //    result is the list of tokens for each parameter’s name
        // consume(LEFT_PAREN, "Expect '(' after " + kind + " name.");
        // List<Token> parameters = new ArrayList<>();
        // if (!check(RIGHT_PAREN)) {
        // do {
        //     if (parameters.size() >= 255) {
        //     error(peek(), "Can't have more than 255 parameters.");
        //     }

        //     parameters.add(
        //         consume(IDENTIFIER, "Expect parameter name."));
        // } while (match(COMMA));
        // }
        // consume(RIGHT_PAREN, "Expect ')' after parameters.");

        // //    parse the body and wrap it all up in a function node
        // consume(LEFT_BRACE, "Expect '{' before " + kind + " body.");
        // List<Stmt> body = block();
        // return new Stmt.Function(name, parameters, body);
    }

    //    move the logic to handle anonymous functions into a new method
    private Expr.Function functionBody(String kind) {
        consume(LEFT_PAREN, "Expect '(' after " + kind + " name.");
        List<Token> parameters = new ArrayList<>();
        if (!check(RIGHT_PAREN)) {
            do {
            if (parameters.size() >= 8) {
                error(peek(), "Can't have more than 8 parameters.");
            }

            parameters.add(consume(IDENTIFIER, "Expect parameter name."));
            } while (match(COMMA));
        }
        consume(RIGHT_PAREN, "Expect ')' after parameters.");

        consume(LEFT_BRACE, "Expect '{' before " + kind + " body.");
        List<Stmt> body = block();
        return new Expr.Function(parameters, body);
    }

    //    create an empty list and then parse statements
    //    add them to the list until we reach the end of the block
    private List<Stmt> block() {
        List<Stmt> statements = new ArrayList<>();

        //    be careful to avoid infinite loops - need the isAtEnd()
        while (!check(RIGHT_BRACE) && !isAtEnd()) {
            statements.add(declaration());
        }

        consume(RIGHT_BRACE, "Expect '}' after block.");
        return statements;
    }

    //    parsing an assignment expression looks similar other binary operators like +
    //    parse the left-hand side (any expression of higher precedence)
    //    find an =, then parse the right-hand side
    //    wrap it all up in an assignment expression tree node
    //    NOTE: assignment is right-associative, so recursively call assignment() to parse the right-hand side
    private Expr assignment() {
        Expr expr = or();

        if (match(EQUAL)) {
            Token equals = previous();
            Expr value = assignment();

            if (expr instanceof Expr.Variable) {
                Token name = ((Expr.Variable)expr).name;
                return new Expr.Assign(name, value);
            }

            error(equals, "Invalid assignment target.");
        }

        return expr;
    }

    //    assignment     → IDENTIFIER "=" assignment | logic_or ;
    //    logic_or       → logic_and ( "or" logic_and )* ;
    //    logic_and      → equality ( "and" equality )* ;
    private Expr or() {
        Expr expr = and();

        while (match(OR)) {
            Token operator = previous();
            Expr right = and();
            expr = new Expr.Logical(expr, operator, right);
        }

        return expr;
    }

    private Expr and() {
        Expr expr = equality();

        while (match(AND)) {
            Token operator = previous();
            Expr right = equality();
            expr = new Expr.Logical(expr, operator, right);
        }

        return expr;
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
    //    changes to: 
    //    unary          → ( "!" | "-" ) unary | call ;
    private Expr unary() {
        if (match(BANG, MINUS)) {
            Token operator = previous();
            Expr right = unary();
            return new Expr.Unary(operator, right);
        }

        return call();
    }

    //    helper function to call(), handles the inside arguments
    private Expr finishCall(Expr callee) {
        List<Expr> arguments = new ArrayList<>();
        if (!check(RIGHT_PAREN)) {
            do {
                //    limit the number of arguments so it doesn't go forever 
                if (arguments.size() >= 255) {
                    //    reports the error but doesn't throw it 
                    error(peek(), "Can't have more than 255 arguments.");
                }
                arguments.add(expression());
            } while (match(COMMA));
        }

        Token paren = consume(RIGHT_PAREN,
                            "Expect ')' after arguments.");

        return new Expr.Call(callee, paren, arguments);
    }

    //    call           → primary ( "(" arguments? ")" )* ;
    //    if there are no parentheses, this parses a bare primary expression. 
    //    otherwise, each call is recognized by a pair of parentheses with an optional list of arguments inside
    //    arguments      → expression ( "," expression )* ;
    private Expr call() {
        Expr expr = primary();

        while (true) { 
            if (match(LEFT_PAREN)) {
                expr = finishCall(expr);
            } else {
                break;
            }
        }

        return expr;
        }

    //    primary        → NUMBER | STRING | "true" | "false" | "nil" | "(" expression ")" ;
    private Expr primary() {
        if (match(FALSE)) return new Expr.Literal(false);
        if (match(TRUE)) return new Expr.Literal(true);
        if (match(NIL)) return new Expr.Literal(null);

        if (match(NUMBER, STRING)) {
            return new Expr.Literal(previous().literal);
        }

        //    look for an identifier token
        if (match(IDENTIFIER)) {
            return new Expr.Variable(previous());
        }

        if (match(FUN)) return functionBody("function");

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

    private boolean checkNext(TokenType tokenType) {
        if (isAtEnd()) return false;
        if (tokens.get(current + 1).type == EOF) return false;
        return tokens.get(current + 1).type == tokenType;
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
