#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "compiler.h"
#include "scanner.h"

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

typedef struct {
    Token current;
    Token previous;
    bool hadError;
    // to avoid error cascades
    // ex. if the user has a mistake in their code and the parser gets confused about where it is in the grammar
    bool panicMode;
} Parser;

// Lox’s precedence levels in order from lowest to highest
typedef enum {
    PREC_NONE,
    PREC_ASSIGNMENT,  // =
    PREC_OR,          // or
    PREC_AND,         // and
    PREC_EQUALITY,    // == !=
    PREC_COMPARISON,  // < > <= >=
    PREC_TERM,        // + -
    PREC_FACTOR,      // * /
    PREC_UNARY,       // ! -
    PREC_CALL,        // . ()
    PREC_PRIMARY
  } Precedence;

typedef void (*ParseFn)(bool canAssign);

typedef struct {
    ParseFn prefix;
    ParseFn infix;
    Precedence precedence;
} ParseRule;

// when resolving an identifier, compare the identifier’s lexeme
// with each local’s name to find a match
// depth records how deep into the block where the local variable was declared
typedef struct {
    Token name;
    int depth;
} Local;

// compiler keeps track of all locals in an array
typedef struct {
    Local locals[UINT8_COUNT];
    int localCount; // tracks how many of those 256 slots are currently used
    int scopeDepth; // tracks how deep the nesting { } is
} Compiler;

Parser parser;
Compiler* current = NULL;
Chunk* compilingChunk;

int innermostLoopStart = -1;
int innermostLoopScopeDepth = 0;

static Chunk* currentChunk() {
    return compilingChunk;
}

static void errorAt(Token* token, const char* message) {
    // while panic mode flag is set, suppress any other errors that get detected
    if (parser.panicMode) return;
    parser.panicMode = true;
    // print where the error occurred
    fprintf(stderr, "[line %d] Error", token->line);

    // show the lexeme if it’s human-readable
    if (token->type == TOKEN_EOF) {
        fprintf(stderr, " at end");
    } else if (token->type == TOKEN_ERROR) {
        // Nothing.
    } else {
        fprintf(stderr, " at '%.*s'", token->length, token->start);
    }

    // print the error message itself
    fprintf(stderr, ": %s\n", message);
    // hadError flag records whether any errors occurred during compilation
    parser.hadError = true;
}

// tell the user where the error occurred and forward it to errorAt()
static void error(const char* message) {
    errorAt(&parser.previous, message);
}

// tell the user there's an error
static void errorAtCurrent(const char* message) {
    errorAt(&parser.current, message);
}

// steps forward through the token stream by
// asks the scanner for the next token and stores it for later use
static void advance() {
    // takes the old current token and stashes that in a previous field
    parser.previous = parser.current;

    // loop to read tokens and report the errors, until hitting a non-error token or reaching the end
    // the rest of the parser sees only valid tokens
    for (;;) {
        parser.current = scanToken();
        if (parser.current.type != TOKEN_ERROR) break;

        errorAtCurrent(parser.current.start);
    }
}

// reads the next token
// validates that the token has an expected type
static void consume(TokenType type, const char* message) {
    if (parser.current.type == type) {
        advance();
        return;
    }

    errorAtCurrent(message);
}

static bool check(TokenType type) {
    return parser.current.type == type;
}

static bool match(TokenType type) {
    if (!check(type)) return false;
    advance();
    return true;
}

// already parsed and understand a piece of the user’s program
// next step is to append a single byte to the chunk
static void emitByte(uint8_t byte) {
    writeChunk(currentChunk(), byte, parser.previous.line);
}

static void emitBytes(uint8_t byte1, uint8_t byte2) {
    emitByte(byte1);
    emitByte(byte2);
}

static void emitLoop(int loopStart) {
    emitByte(OP_LOOP);

    int offset = currentChunk()->count - loopStart + 2;
    if (offset > UINT16_MAX) error("Loop body too large.");

    emitByte((offset >> 8) & 0xff);
    emitByte(offset & 0xff);
}

// emits a bytecode instruction and writes a placeholder operand for the jump offset
// returns the offset of the emitted instruction in the chunk
static int emitJump(uint8_t instruction) {
    emitByte(instruction);
    emitByte(0xff);
    emitByte(0xff);
    return currentChunk()->count - 2;
}

static void emitReturn() {
    emitByte(OP_RETURN);
}

static uint8_t makeConstant(Value value) {
    int constant = addConstant(currentChunk(), value);
    if (constant > UINT8_MAX) {
        error("Too many constants in one chunk.");
        return 0;
    }

    return (uint8_t)constant;
}

static void emitConstant(Value value) {
    emitBytes(OP_CONSTANT, makeConstant(value));
}

// goes back into the bytecode and replaces the operand at the given location with the calculated jump offset
static void patchJump(int offset) {
    // -2 to adjust for the bytecode for the jump offset itself.
    int jump = currentChunk()->count - offset - 2;

    if (jump > UINT16_MAX) {
        error("Too much code to jump over.");
    }

    currentChunk()->code[offset] = (jump >> 8) & 0xff;
    currentChunk()->code[offset + 1] = jump & 0xff;
}

// to initialize the compiler
static void initCompiler(Compiler* compiler) {
    compiler->localCount = 0;
    compiler->scopeDepth = 0;
    current = compiler;
}

static void endCompiler() {
    emitReturn();

    // do this only if the code was free of errors
#ifdef DEBUG_PRINT_CODE
    if (!parser.hadError) {
        disassembleChunk(currentChunk(), "code");
    }
#endif
}

// to enter a new local scope
static void beginScope() {
    current->scopeDepth++;
}

static void endScope() {
    current->scopeDepth--;

    // when a block ends, bring the variables to rest
    while (current->localCount > 0 &&
         current->locals[current->localCount - 1].depth > current->scopeDepth) {
        // pop a scope: go backward through the local array for any variables declared at scope depth currently leaving
        // discard the variables by decrementing the length of the local variable array
        emitByte(OP_POP);
        current->localCount--;
    }
}

static void expression();
static void statement();
static void declaration();
static ParseRule* getRule(TokenType type);
static void parsePrecedence(Precedence precedence);

static uint8_t identifierConstant(Token* name) {
    return makeConstant(OBJ_VAL(copyString(name->start,
                                           name->length)));
}

// to see if two identifiers are the same
static bool identifiersEqual(Token* a, Token* b) {
    if (a->length != b->length) return false;
    return memcmp(a->start, b->start, a->length) == 0;
}

/*
 * Resolves a local variable by searching the locals array
 * * 1. Search Backward: Walk the array from the most recent local to the first
 * ensures "shadowing" works (inner variables take precedence over
 * outer ones with the same name)
 * * 2. Map to Stack: The index in the compiler's 'locals' array perfectly
 * matches the variable's slot on the VM stack at runtime
 * * 3. Fallback: If no match is found, return -1 to indicate the variable
 * is likely a global
 */
static int resolveLocal(Compiler* compiler, Token* name) {
    for (int i = compiler->localCount - 1; i >= 0; i--) {
        Local* local = &compiler->locals[i];
        if (identifiersEqual(name, &local->name)) {
            if (local->depth == -1) {
                error("Can't use a variable to define itself.");
            }
            return i;
        }
    }

    return -1;
}

// declaring the local variable adds it to the compiler’s list of variables in the current scope
static void addLocal(Token name) {
    // prevent going over the 256 available local variables
    // else the compiler would overwrite its own locals array
    if (current->localCount == UINT8_COUNT) {
        error("Too many local variables in function.");
        return;
    }

    Local* local = &current->locals[current->localCount++];
    local->name = name;
    local->depth = -1;
}

// the compiler records the existence of the variable
// only do this for locals, if in the top-level global scope then exit
static void declareVariable() {
    if (current->scopeDepth == 0) return;

    Token* name = &parser.previous;

    // an error to have two variables with the same name in the same local scope
    // Note: shadowing is okay => { var a = "outer"; { var a = "inner"; } }
    for (int i = current->localCount - 1; i >= 0; i--) {
        Local* local = &current->locals[i];
        if (local->depth != -1 && local->depth < current->scopeDepth) {
            break;
        }

        if (identifiersEqual(name, &local->name)) {
            error("Already a variable with this name in this scope.");
        }
    }

    addLocal(*name);
}

static uint8_t parseVariable(const char* errorMessage) {
    consume(TOKEN_IDENTIFIER, errorMessage);

    // next 2 lines changes variables from global to optionally local
    declareVariable();
    if (current->scopeDepth > 0) return 0;

    return identifierConstant(&parser.previous);
}

// “declaring” and “defining” a variable in the compiler
// “declaring” is when the variable is added to the scope => var a;
// “defining” is when it becomes available for use => a = "outer";
static void markInitialized() {
    current->locals[current->localCount - 1].depth =
        current->scopeDepth;
}

static void defineVariable(uint8_t global) {
    // emit the code to store a local variable if in a local scope
    if (current->scopeDepth > 0) {
        markInitialized();
        return;
    }

    emitBytes(OP_DEFINE_GLOBAL, global);
}

static void and_(bool canAssign) {
    int endJump = emitJump(OP_JUMP_IF_FALSE);

    emitByte(OP_POP);
    parsePrecedence(PREC_AND);

    patchJump(endJump);
}

static void binary(bool canAssign) {
    TokenType operatorType = parser.previous.type;
    ParseRule* rule = getRule(operatorType);
    parsePrecedence((Precedence)(rule->precedence + 1));

    switch (operatorType) {
        case TOKEN_BANG_EQUAL:    emitBytes(OP_EQUAL, OP_NOT); break;
        case TOKEN_EQUAL_EQUAL:   emitByte(OP_EQUAL); break;
        case TOKEN_GREATER:       emitByte(OP_GREATER); break;
        case TOKEN_GREATER_EQUAL: emitBytes(OP_LESS, OP_NOT); break;
        case TOKEN_LESS:          emitByte(OP_LESS); break;
        case TOKEN_LESS_EQUAL:    emitBytes(OP_GREATER, OP_NOT); break;
        case TOKEN_PLUS:          emitByte(OP_ADD); break;
        case TOKEN_MINUS:         emitByte(OP_SUBTRACT); break;
        case TOKEN_STAR:          emitByte(OP_MULTIPLY); break;
        case TOKEN_SLASH:         emitByte(OP_DIVIDE); break;
        default: return; // Unreachable.
    }
}

// output the proper instruction, figure that out based on the type of token parsed
// can now compile Boolean and nil literals to bytecode
static void literal(bool canAssign) {
    switch (parser.previous.type) {
        case TOKEN_FALSE: emitByte(OP_FALSE); break;
        case TOKEN_NIL: emitByte(OP_NIL); break;
        case TOKEN_TRUE: emitByte(OP_TRUE); break;
        default: return; // Unreachable.
    }
}

static void grouping(bool canAssign) {
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static void number(bool canAssign) {
    double value = strtod(parser.previous.start, NULL);
    // constants generated when we compile number literals
    emitConstant(NUMBER_VAL(value)); // emitConstant(value); gets replaced after putting Value as a struct in value.h
}

// if the left-hand side is truthy, then skip over the right operand
static void or_(bool canAssign) {
    int elseJump = emitJump(OP_JUMP_IF_FALSE);
    int endJump = emitJump(OP_JUMP);

    patchJump(elseJump);
    emitByte(OP_POP);

    parsePrecedence(PREC_OR);
    patchJump(endJump);
}

// parse function to takes the string’s characters directly from the lexeme
// +1 and -2 remove the quotation marks
// then creates a string object, wraps it in a Value, and puts into the constant table
static void string(bool canAssign) {
    emitConstant(OBJ_VAL(copyString(parser.previous.start + 1,
                                    parser.previous.length - 2)));
}

static void namedVariable(Token name, bool canAssign) {
    // first, try to find a local variable with the given name
    // if found, use the instructions for working with locals
    // else, assume it’s a global variable and use the existing bytecode instructions for globals
    uint8_t getOp, setOp;
    int arg = resolveLocal(current, &name);
    if (arg != -1) {
        getOp = OP_GET_LOCAL;
        setOp = OP_SET_LOCAL;
    } else {
        arg = identifierConstant(&name);
        getOp = OP_GET_GLOBAL;
        setOp = OP_SET_GLOBAL;
    }

    if (canAssign && match(TOKEN_EQUAL)) {
        expression();
        // use those variables to emit the right instructions for assignment
        emitBytes(setOp, (uint8_t)arg);
    } else {
        // use those variables to emit the right instructions for access
        emitBytes(getOp, (uint8_t)arg);
    }
}

static void variable(bool canAssign) {
    namedVariable(parser.previous, canAssign);
}

static void unary(bool canAssign) {
    TokenType operatorType = parser.previous.type;

    // Compile the operand.
    parsePrecedence(PREC_UNARY);

    // Emit the operator instruction.
    switch (operatorType) {
        case TOKEN_BANG: emitByte(OP_NOT); break;
        case TOKEN_MINUS: emitByte(OP_NEGATE); break;
        default: return; // Unreachable.
    }
}

ParseRule rules[] = {
  [TOKEN_LEFT_PAREN]    = {grouping, NULL,   PREC_NONE},
  [TOKEN_RIGHT_PAREN]   = {NULL,     NULL,   PREC_NONE},
  [TOKEN_LEFT_BRACE]    = {NULL,     NULL,   PREC_NONE},
  [TOKEN_RIGHT_BRACE]   = {NULL,     NULL,   PREC_NONE},
  [TOKEN_COMMA]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_DOT]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_MINUS]         = {unary,    binary, PREC_TERM},
  [TOKEN_PLUS]          = {NULL,     binary, PREC_TERM},
  [TOKEN_SEMICOLON]     = {NULL,     NULL,   PREC_NONE},
  [TOKEN_SLASH]         = {NULL,     binary, PREC_FACTOR},
  [TOKEN_STAR]          = {NULL,     binary, PREC_FACTOR},
  [TOKEN_BANG]          = {unary,    NULL,   PREC_NONE},
  [TOKEN_BANG_EQUAL]    = {NULL,     binary, PREC_EQUALITY},
  [TOKEN_EQUAL]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_EQUAL_EQUAL]   = {NULL,     binary, PREC_EQUALITY},
  [TOKEN_GREATER]       = {NULL,     binary, PREC_COMPARISON},
  [TOKEN_GREATER_EQUAL] = {NULL,     binary, PREC_COMPARISON},
  [TOKEN_LESS]          = {NULL,     binary, PREC_COMPARISON},
  [TOKEN_LESS_EQUAL]    = {NULL,     binary, PREC_COMPARISON},
  [TOKEN_IDENTIFIER]    = {variable, NULL,   PREC_NONE},
  [TOKEN_STRING]        = {string,     NULL,   PREC_NONE},
  [TOKEN_NUMBER]        = {number,   NULL,   PREC_NONE},
  [TOKEN_AND]           = {NULL,     and_,   PREC_AND},
  [TOKEN_CLASS]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_ELSE]          = {NULL,     NULL,   PREC_NONE},
  [TOKEN_FALSE]         = {literal,  NULL,   PREC_NONE},
  [TOKEN_FOR]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_FUN]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_IF]            = {NULL,     NULL,   PREC_NONE},
  // [TOKEN_SWITCH]        = {NULL,     NULL,   PREC_NONE},
  // [TOKEN_CASE]          = {NULL,     NULL,   PREC_NONE},
  // [TOKEN_DEFAULT]       = {NULL,     NULL,   PREC_NONE},
  // [TOKEN_COLON]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_OR]           = {NULL,     or_,    PREC_OR},
  [TOKEN_PRINT]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_RETURN]        = {NULL,     NULL,   PREC_NONE},
  [TOKEN_SUPER]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_THIS]          = {NULL,     NULL,   PREC_NONE},
  [TOKEN_TRUE]          = {literal,     NULL,   PREC_NONE},
  [TOKEN_VAR]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_WHILE]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_ERROR]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_EOF]           = {NULL,     NULL,   PREC_NONE},
};

static void parsePrecedence(Precedence precedence) {
    advance();
    ParseFn prefixRule = getRule(parser.previous.type)->prefix;
    if (prefixRule == NULL) {
        error("Expect expression.");
        return;
    }

    bool canAssign = precedence <= PREC_ASSIGNMENT;
    prefixRule(canAssign);

    while (precedence <= getRule(parser.current.type)->precedence) {
        advance();
        ParseFn infixRule = getRule(parser.previous.type)->infix;
        infixRule(canAssign);
    }

    if (canAssign && match(TOKEN_EQUAL)) {
        error("Invalid assignment target.");
    }
}

static ParseRule* getRule(TokenType type) {
    return &rules[type];
}

static void expression() {
    parsePrecedence(PREC_ASSIGNMENT);
}

// for block statements
static void block() {
    while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
        declaration();
    }

    consume(TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}

static void varDeclaration() {
    uint8_t global = parseVariable("Expect variable name.");

    if (match(TOKEN_EQUAL)) {
        expression();
    } else {
        emitByte(OP_NIL);
    }
    consume(TOKEN_SEMICOLON,
            "Expect ';' after variable declaration.");

    defineVariable(global);
}

static void expressionStatement() {
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after expression.");
    emitByte(OP_POP);
}

#define MAX_CASES 256

static void switchStatement() {
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'switch'.");
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after value.");
    consume(TOKEN_LEFT_BRACE, "Expect '{' before switch cases.");

    int state = 0; // 0: before all cases, 1: before default, 2: after default.
    int caseEnds[MAX_CASES];
    int caseCount = 0;
    int previousCaseSkip = -1;

    while (!match(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
        if (match(TOKEN_CASE) || match(TOKEN_DEFAULT)) {
            TokenType caseType = parser.previous.type;

            if (state == 2) {
                error("Can't have another case or default after the default case.");
            }

            if (state == 1) {
                // At the end of the previous case, jump over the others.
                caseEnds[caseCount++] = emitJump(OP_JUMP);

                // Patch its condition to jump to the next case (this one).
                patchJump(previousCaseSkip);
                emitByte(OP_POP);
            }

            if (caseType == TOKEN_CASE) {
                state = 1;

                // See if the case is equal to the value.
                emitByte(OP_DUP);
                expression();

                consume(TOKEN_COLON, "Expect ':' after case value.");

                emitByte(OP_EQUAL);
                previousCaseSkip = emitJump(OP_JUMP_IF_FALSE);

                // Pop the comparison result.
                emitByte(OP_POP);
            } else {
                state = 2;
                consume(TOKEN_COLON, "Expect ':' after default.");
                previousCaseSkip = -1;
            }
        } else {
            // Otherwise, it's a statement inside the current case.
            if (state == 0) {
                error("Can't have statements before any case.");
            }
            statement();
        }
    }

    // If we ended without a default case, patch its condition jump.
    if (state == 1) {
        patchJump(previousCaseSkip);
        emitByte(OP_POP);
    }

    // Patch all the case jumps to the end.
    for (int i = 0; i < caseCount; i++) {
        patchJump(caseEnds[i]);
    }

    emitByte(OP_POP); // The switch value.
}

// static void forStatement() {
//     // if a for statement declares a variable, that variable should be scoped to the loop body
//     beginScope();
//     consume(TOKEN_LEFT_PAREN, "Expect '(' after 'for'.");
//     // the initializer clause
//     if (match(TOKEN_SEMICOLON)) {
//         // No initializer.
//     } else if (match(TOKEN_VAR)) {
//         varDeclaration();
//     } else {
//         expressionStatement();
//     }
//
//     int loopStart = currentChunk()->count;
//     // condition expression that can be used to exit the loop
//     int exitJump = -1;
//     if (!match(TOKEN_SEMICOLON)) {
//         expression();
//         consume(TOKEN_SEMICOLON, "Expect ';' after loop condition.");
//
//         // Jump out of the loop if the condition is false.
//         exitJump = emitJump(OP_JUMP_IF_FALSE);
//         emitByte(OP_POP); // Condition.
//     }
//
//     // the increment clause - textually before the body, but executes after it
//     if (!match(TOKEN_RIGHT_PAREN)) {
//         int bodyJump = emitJump(OP_JUMP);
//         int incrementStart = currentChunk()->count;
//         expression();
//         emitByte(OP_POP);
//         consume(TOKEN_RIGHT_PAREN, "Expect ')' after for clauses.");
//
//         emitLoop(loopStart);
//         loopStart = incrementStart;
//         patchJump(bodyJump);
//     }
//
//     statement();
//     emitLoop(loopStart);
//
//     // after the loop body, patch that jump
//     if (exitJump != -1) {
//         patchJump(exitJump);
//         emitByte(OP_POP); // Condition.
//     }
//
//     endScope();
// }

static void forStatement() {
    beginScope();

    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'for'.");
    if (match(TOKEN_VAR)) {
        varDeclaration();
    } else if (match(TOKEN_SEMICOLON)) {
        // No initializer.
    } else {
        expressionStatement();
    }

    int surroundingLoopStart = innermostLoopStart; // <--
    int surroundingLoopScopeDepth = innermostLoopScopeDepth; // <--
    innermostLoopStart = currentChunk()->count; // <--
    innermostLoopScopeDepth = current->scopeDepth; // <--

    int exitJump = -1;
    if (!match(TOKEN_SEMICOLON)) {
        expression();
        consume(TOKEN_SEMICOLON, "Expect ';' after loop condition.");

        // Jump out of the loop if the condition is false.
        exitJump = emitJump(OP_JUMP_IF_FALSE);
        emitByte(OP_POP); // Condition.
    }

    if (!match(TOKEN_RIGHT_PAREN)) {
        int bodyJump = emitJump(OP_JUMP);

        int incrementStart = currentChunk()->count;
        expression();
        emitByte(OP_POP);
        consume(TOKEN_RIGHT_PAREN, "Expect ')' after for clauses.");

        emitLoop(innermostLoopStart); // <--
        innermostLoopStart = incrementStart; // <--
        patchJump(bodyJump);
    }

    statement();

    emitLoop(innermostLoopStart); // <--

    if (exitJump != -1) {
        patchJump(exitJump);
        emitByte(OP_POP); // Condition.
    }

    innermostLoopStart = surroundingLoopStart; // <--
    innermostLoopScopeDepth = surroundingLoopScopeDepth; // <--

    endScope();
}

static void continueStatement() {
    if (innermostLoopStart == -1) {
        error("Can't use 'continue' outside of a loop.");
    }

    consume(TOKEN_SEMICOLON, "Expect ';' after 'continue'.");

    // Discard any locals created inside the loop.
    for (int i = current->localCount - 1;
         i >= 0 && current->locals[i].depth > innermostLoopScopeDepth;
         i--) {
        emitByte(OP_POP);
         }

    // Jump to top of current innermost loop.
    emitLoop(innermostLoopStart);
}

// see an if keyword, hand off compilation to this function
static void ifStatement() {
    // compile the condition expression, bracketed by parentheses
    // at runtime, the condition value will be on top of the stack
    // use that to determine whether to execute the then branch or skip it
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

    // OP_JUMP_IF_FALSE is an operand for how much to offset the ip (how many bytes of code to skip)
    int thenJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP);
    statement();

    int elseJump = emitJump(OP_JUMP);

    // every if statement has an implicit else branch even if the user doesn't write it
    // discard the condition value if we jumped the if branch and there is no else branch
    patchJump(thenJump);
    emitByte(OP_POP);

    // when the condition is true, run the then branch and jump over the else branch
    if (match(TOKEN_ELSE)) statement();

    patchJump(elseJump);
}

static void printStatement() {
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after value.");
    emitByte(OP_PRINT);
}

// compile the condition expression, surrounded by mandatory parentheses
// followed by a jump instruction that skips over the subsequent body statement if the condition is falsey
// patch the jump after compiling the body and take care to pop the condition value from the stack on either path
static void whileStatement() {
    int loopStart = currentChunk()->count;
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'while'.");
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

    int exitJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP);
    statement();

    emitLoop(loopStart);

    patchJump(exitJump);
    emitByte(OP_POP);

    // contains two jumps —
    // a conditional forward one to escape the loop when the condition is not met
    // and an unconditional loop backward after have executed the body to recheck the condition
}

static void synchronize() {
    parser.panicMode = false;

    while (parser.current.type != TOKEN_EOF) {
        if (parser.previous.type == TOKEN_SEMICOLON) return;
        switch (parser.current.type) {
            case TOKEN_CLASS:
            case TOKEN_FUN:
            case TOKEN_VAR:
            case TOKEN_FOR:
            case TOKEN_IF:
            case TOKEN_WHILE:
            case TOKEN_PRINT:
            case TOKEN_RETURN:
                return;

            default:
                ; // Do nothing.
        }

        advance();
    }
}

static void declaration() {
    if (match(TOKEN_VAR)) {
        varDeclaration();
    } else {
        statement();
    }

    if (parser.panicMode) synchronize();
}

static void statement() {
    if (match(TOKEN_PRINT)) {
        printStatement();
    } else if (match(TOKEN_SWITCH)) {
        switchStatement();
    } else if (match(TOKEN_FOR)) {
        forStatement();
    } else if (match(TOKEN_CONTINUE)) {
        continueStatement();
    } else if (match(TOKEN_IF)) {
        ifStatement();
    } else if (match(TOKEN_WHILE)) {
        whileStatement();
    } else if (match(TOKEN_LEFT_BRACE)) {
        beginScope();
        block();
        endScope();
    } else {
        expressionStatement();
    }
}

// the first phase of compilation is scanning
// the compiler is setting scanning up
bool compile(const char* source, Chunk* chunk) {
    initScanner(source);

    Compiler compiler;
    initCompiler(&compiler);

    compilingChunk = chunk;

    parser.hadError = false;
    parser.panicMode = false;

    advance();
    // // parse a single expression
    // expression();
    // // should be at the end of the source code, so check for EOF token
    // consume(TOKEN_EOF, "Expect end of expression.");

    while (!match(TOKEN_EOF)) {
        declaration();
    }

    endCompiler();

    // return false if an error occurred
    return !parser.hadError;
}