//package com.craftinginterpreters.lox;
package lox;

// scan through the list of characters and group them together into the smallest sequences that still represent something
// at the point that we recognize a lexeme, we also remember which kind of lexeme it represents
enum TokenType {
    //    Single-character tokens.
    LEFT_PAREN, RIGHT_PAREN, LEFT_BRACE, RIGHT_BRACE,
    COMMA, DOT, MINUS, PLUS, SEMICOLON, SLASH, STAR,

    //    One or two character tokens.
    BANG, BANG_EQUAL,
    EQUAL, EQUAL_EQUAL,
    GREATER, GREATER_EQUAL,
    LESS, LESS_EQUAL,

    //    Literals.
    IDENTIFIER, STRING, NUMBER,

    //    Keywords.
    AND, CLASS, ELSE, FALSE, FUN, FOR, IF, NIL, OR,
    PRINT, RETURN, SUPER, THIS, TRUE, VAR, WHILE,

    EOF
}