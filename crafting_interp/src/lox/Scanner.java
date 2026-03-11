//package com.craftinginterpreters.lox;
package lox;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

//import static com.craftinginterpreters.lox.TokenType.*;
import static lox.TokenType.*;

class Scanner {
    //    To handle keywords, see if the identifier’s lexeme is one of the reserved words
    //    yes, then use a token type specific to that keyword
    private static final Map<String, TokenType> keywords;

    static {
        keywords = new HashMap<>();
        keywords.put("and",    AND);
        keywords.put("class",  CLASS);
        keywords.put("else",   ELSE);
        keywords.put("false",  FALSE);
        keywords.put("for",    FOR);
        keywords.put("fun",    FUN);
        keywords.put("if",     IF);
        keywords.put("nil",    NIL);
        keywords.put("or",     OR);
        keywords.put("print",  PRINT);
        keywords.put("return", RETURN);
        keywords.put("super",  SUPER);
        keywords.put("this",   THIS);
        keywords.put("true",   TRUE);
        keywords.put("var",    VAR);
        keywords.put("while",  WHILE);
    }

    private final String source;
    private final List<Token> tokens = new ArrayList<>();

    //    start and current fields are offsets that index into the string
    //    start points to the first character in the lexeme being scanned
    //    current points at the character currently being considered
    //    line tracks what source line current is on so we can produce tokens that know their location
    private int start = 0;
    private int current = 0;
    private int line = 1;

    Scanner(String source) {
        this.source = source;
    }

    //    a list ready to fill with tokens we’re going to generate
    List<Token> scanTokens() {
        while (!isAtEnd()) {
            // We are at the beginning of the next lexeme.
            start = current;
            scanToken();
        }

        tokens.add(new Token(EOF, "", null, line));
        return tokens;
    }

    //    we scan a single token
    //    if every lexeme were only a single character long, consume the next character and pick a token type for it
    private void scanToken() {
        char c = advance();
        switch (c) {
            //    single-character lexemes
            case '(': addToken(LEFT_PAREN); break;
            case ')': addToken(RIGHT_PAREN); break;
            case '{': addToken(LEFT_BRACE); break;
            case '}': addToken(RIGHT_BRACE); break;
            case ',': addToken(COMMA); break;
            case '.': addToken(DOT); break;
            case '-': addToken(MINUS); break;
            case '+': addToken(PLUS); break;
            case ';': addToken(SEMICOLON); break;
            case '*': addToken(STAR); break;

            //    look at the second character for things like !=, ==, <=, >=
            //    uses match()
            case '!':
                addToken(match('=') ? BANG_EQUAL : BANG);
                break;
            case '=':
                addToken(match('=') ? EQUAL_EQUAL : EQUAL);
                break;
            case '<':
                addToken(match('=') ? LESS_EQUAL : LESS);
                break;
            case '>':
                addToken(match('=') ? GREATER_EQUAL : GREATER);
                break;

            //    / for division
            //    special handling because comments begin with a / too
            //    when we find a second /, don’t end the token just keep consuming until the end of the line
            case '/':
                if (match('/')) {
                    // A comment goes until the end of the line.
                    while (peek() != '\n' && !isAtEnd()) advance();
                } else {
                    addToken(SLASH);
                }
                break;

            //    comments are meaningless lexemes, parser doesn’t want to deal with them
            //    when we reach the end of the comment, don’t call addToken()
            //    loop back around to start the next lexeme, start variable on line 45 gets reset
            //    the comment’s lexeme disappears
            case ' ':
            case '\r':
            case '\t':
                //    Ignore whitespace.
                break;

            case '\n':
                line++;
                break;

            //    string literal calling string()
            case '"': string(); break;

            default:
                //    recognize the beginning of a number lexeme
                if (isDigit(c)) {
                    number();
                    //    something about assuming any lexeme starting with a letter or underscore is an identifier
                } else if (isAlpha(c)) {
                    identifier();
                } else {
                    //    to handle some characters Lox doesn’t use right now like @#^
                    //    erroneous character is still consumed by the call to advance()
                    //    but gets silently discarded and interpreter tells us:
                    Lox.error(line, "Unexpected character.");
                }
                break;
        }
    }

    //    see line 133
    private void identifier() {
        while (isAlphaNumeric(peek())) advance();

        //    after we scan an identifier, we check to see if it matches anything in the map line 18-36
        String text = source.substring(start, current);
        TokenType type = keywords.get(text);
        if (type == null) type = IDENTIFIER;
        addToken(type);
    }

    //    method to consume the rest of the literal, like we do with strings
    private void number() {
        while (isDigit(peek())) advance();

        //    look for a fractional part
        if (peek() == '.' && isDigit(peekNext())) {
            //    consume the "."
            advance();

            while (isDigit(peek())) advance();
        }

        addToken(NUMBER,
                Double.parseDouble(source.substring(start, current)));
    }

    //    consume characters until we hit the " that ends the string
    private void string() {
        while (peek() != '"' && !isAtEnd()) {
            if (peek() == '\n') line++;
            advance();
        }

        //    handle running out of input before the string is closed and report an error for that
        if (isAtEnd()) {
            Lox.error(line, "Unterminated string.");
            return;
        }

        //    the closing "
        advance();

        //    create the token, and also produce the actual string VALUE later used by the interpreter
        //    trim the surrounding quotes
        String value = source.substring(start + 1, current - 1);
        addToken(STRING, value);
    }

    //    like a conditional advance(), only consume the current character if it’s what we’re looking for
    private boolean match(char expected) {
        if (isAtEnd()) return false;
        if (source.charAt(current) != expected) return false;

        current++;
        return true;
    }

    //    another helper
    //    like advance() but doesn’t consume the character, is a lookahead
    private char peek() {
        if (isAtEnd()) return '\0';
        return source.charAt(current);
    }

    //    to help with decimals in number()
    //    don't want to consume unless there is a number after the decimal
    //    0. vs 0.1
    private char peekNext() {
        if (current + 1 >= source.length()) return '\0';
        return source.charAt(current + 1);
    }

    //    something about isAlpha(char c) and isAlphaNumeric(char c) helping the identifier() method
    private boolean isAlpha(char c) {
        return (c >= 'a' && c <= 'z') ||
                (c >= 'A' && c <= 'Z') ||
                c == '_';
    }

    private boolean isAlphaNumeric(char c) {
        return isAlpha(c) || isDigit(c);
    }

    private boolean isDigit(char c) {
        return c >= '0' && c <= '9';
    }

    //    helper function that tells us if we’ve consumed all the characters
    private boolean isAtEnd() {
        return current >= source.length();
    }

    //    more helper methods
    //    advance() consumes the next character in the source file and returns it
    private char advance() {
        return source.charAt(current++);
    }

    //    where advance() is for input, addToken() is for output
    //    grabs the text of the current lexeme and creates a new token for it
    private void addToken(TokenType type) {
        addToken(type, null);
    }

    //    overload to handle tokens with literal values
    private void addToken(TokenType type, Object literal) {
        String text = source.substring(start, current);
        tokens.add(new Token(type, text, literal, line));
    }
}
