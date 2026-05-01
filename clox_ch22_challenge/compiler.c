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
    bool isFinal; // added to support final/const variables
} Local;

// compiler keeps track of all locals in an array
typedef struct {
    // Local locals[UINT8_COUNT];
    Local* locals;
    int localCount; // tracks how many of those 256 slots are currently used
    int localCapacity;
    int scopeDepth; // tracks how deep the nesting { } is
} Compiler;

Parser parser;
Compiler* current = NULL;
Chunk* compilingChunk;

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

static void emitShort(uint16_t value) {
    emitByte((value >> 8) & 0xff);
    emitByte(value & 0xff);
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
    int constant = addConstant(currentChunk(), value);

    if (constant <= UINT8_MAX) {
        emitBytes(OP_CONSTANT, (uint8_t)constant);
    } else {
        emitByte(OP_CONSTANT_LONG);
        emitShort((uint16_t)constant);
    }
}

// to initialize the compiler
static void initCompiler(Compiler* compiler) {
    compiler->localCount = 0;
    compiler->scopeDepth = 0;

    compiler->localCapacity = 8;
    compiler->locals = malloc(sizeof(Local) * compiler->localCapacity);

    if (compiler->locals == NULL) {
        fprintf(stderr, "Out of memory.\n");
        exit(1);
    }

    current = compiler;
}

static void endCompiler() {
    emitReturn();

#ifdef DEBUG_PRINT_CODE
    if (!parser.hadError) {
        disassembleChunk(currentChunk(), "code");
    }
#endif

    // free locals array
    free(current->locals);
}

// to enter a new local scope
static void beginScope() {
    current->scopeDepth++;
}

// static void endScope() {
//     current->scopeDepth--;
//
//     // when a block ends, bring the variables to rest
//     while (current->localCount > 0 &&
//          current->locals[current->localCount - 1].depth > current->scopeDepth) {
//         // pop a scope: go backward through the local array for any variables declared at scope depth currently leaving
//         // discard the variables by decrementing the length of the local variable array
//         emitByte(OP_POP);
//         current->localCount--;
//     }
// }

static void endScope() {
    current->scopeDepth--;

    while (current->localCount > 0 &&
         current->locals[current->localCount - 1].depth >= current->scopeDepth) {

        emitByte(OP_POP);
        current->localCount--;
         }
}

static void expression();
static void statement();
static void declaration();
static ParseRule* getRule(TokenType type);
static void parsePrecedence(Precedence precedence);
static bool isFinalGlobal(Token* name);

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

        // skip uninitialized locals
        if (local->depth == -1) continue;

        if (identifiersEqual(name, &local->name)) {
            if (local->depth == -1) {
                error("Can't use a variable to define itself.");
            }
            return i;
        }
    }

    return -1;
}

// // declaring the local variable adds it to the compiler’s list of variables in the current scope
// static void addLocal(Token name) {
//     // prevent going over the 256 available local variables
//     // else the compiler would overwrite its own locals array
//     if (current->localCount == UINT8_COUNT) {
//         error("Too many local variables in function.");
//         return;
//     }
//
//     Local* local = &current->locals[current->localCount++];
//     local->name = name;
//     local->depth = -1;
// }

static void addLocal(Token name, bool isFinal) {
    printf("addLocal: %.*s (count=%d, cap=%d)\n",
           name.length, name.start,
           current->localCount,
           current->localCapacity);

    // grow array if needed
    if (current->localCount == current->localCapacity) {
        int oldCapacity = current->localCapacity;
        current->localCapacity *= 2;

        current->locals = realloc(
            current->locals,
            sizeof(Local) * current->localCapacity
        );

        if (current->locals == NULL) {
            fprintf(stderr, "Out of memory\n");
            exit(1);
        }

        printf("  resized locals: %d -> %d\n",
               oldCapacity, current->localCapacity);
    }

    Local* local = &current->locals[current->localCount++];
    local->name = name;
    local->depth = -1;
    local->isFinal = isFinal;
}

// // the compiler records the existence of the variable
// // only do this for locals, if in the top-level global scope then exit
// static void declareVariable() {
//     if (current->scopeDepth == 0) return;
//
//     Token* name = &parser.previous;
//
//     // an error to have two variables with the same name in the same local scope
//     // Note: shadowing is okay => { var a = "outer"; { var a = "inner"; } }
//     for (int i = current->localCount - 1; i >= 0; i--) {
//         Local* local = &current->locals[i];
//         if (local->depth != -1 && local->depth < current->scopeDepth) {
//             break;
//         }
//
//         if (identifiersEqual(name, &local->name)) {
//             error("Already a variable with this name in this scope.");
//         }
//     }
//
//     addLocal(*name);
// }

static void declareVariable(bool isFinal) {
    if (current->scopeDepth == 0) return;

    Token* name = &parser.previous;

    for (int i = current->localCount - 1; i >= 0; i--) {
        Local* local = &current->locals[i];
        if (local->depth != -1 && local->depth < current->scopeDepth) {
            break;
        }

        if (identifiersEqual(name, &local->name)) {
            error("Already a variable with this name in this scope.");
        }
    }

    addLocal(*name, isFinal); // Pass it here
}

// static uint8_t parseVariable(const char* errorMessage) {
//     consume(TOKEN_IDENTIFIER, errorMessage);
//
//     // next 2 lines changes variables from global to optionally local
//     declareVariable();
//     if (current->scopeDepth > 0) return 0;
//
//     return identifierConstant(&parser.previous);
// }

static uint8_t parseVariable(const char* errorMessage, bool isFinal) {
    consume(TOKEN_IDENTIFIER, errorMessage);

    declareVariable(isFinal); // Pass it here
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

static void defineVariable(uint8_t global, bool isFinal) {
    // Local variables don't need bytecode for definition
    if (current->scopeDepth > 0) {
        markInitialized();
        return;
    }

    if (isFinal) {
        emitBytes(OP_DEFINE_FINAL_GLOBAL, global);
    } else {
        emitBytes(OP_DEFINE_GLOBAL, global);
    }
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

// parse function to takes the string’s characters directly from the lexeme
// +1 and -2 remove the quotation marks
// then creates a string object, wraps it in a Value, and puts into the constant table
static void string(bool canAssign) {
    emitConstant(OBJ_VAL(copyString(parser.previous.start + 1,
                                    parser.previous.length - 2)));
}

// static void namedVariable(Token name, bool canAssign) {
//     // first, try to find a local variable with the given name
//     // if found, use the instructions for working with locals
//     // else, assume it’s a global variable and use the existing bytecode instructions for globals
//     uint8_t getOp, setOp;
//     int arg = resolveLocal(current, &name);
//     if (arg != -1) {
//         getOp = OP_GET_LOCAL;
//         setOp = OP_SET_LOCAL;
//     } else {
//         arg = identifierConstant(&name);
//         getOp = OP_GET_GLOBAL;
//         setOp = OP_SET_GLOBAL;
//     }
//
//     if (canAssign && match(TOKEN_EQUAL)) {
//         // 1. We ALWAYS parse the right-hand side, even if the left is final.
//         // This keeps the compiler synchronized.
//         expression();
//
//         if (arg != -1) {
//             if (current->locals[arg].isFinal) {
//                 error("Cannot assign to a FINAL variable.");
//             }
//         } else {
//             if (isFinalGlobal(&name)) {
//                 error("Cannot assign to a FINAL variable.");
//             }
//         }
//
//         // 2. Only emit the SET instruction if there wasn't an error,
//         // or simply emit it anyway since the 'hadError' flag will stop execution later.
//         emitBytes(setOp, (uint8_t)arg);
//     } else {
//         if (arg <= UINT8_MAX) {
//             emitBytes(getOp, (uint8_t)arg);
//         } else {
//             emitByte(getOp == OP_GET_LOCAL ? OP_GET_LOCAL_LONG : OP_SET_LOCAL_LONG);
//             emitShort((uint16_t)arg);
//         }
//     }
// }

static void namedVariable(Token name, bool canAssign) {
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

        // Final variable checks
        if (arg != -1) {
            if (current->locals[arg].isFinal) error("Cannot assign to a FINAL variable.");
        } else if (isFinalGlobal(&name)) {
            error("Cannot assign to a FINAL variable.");
        }

        if (arg <= UINT8_MAX) {
            emitBytes(setOp, (uint8_t)arg);
        } else {
            if (getOp == OP_GET_LOCAL) {
                // Correctly handles local index > 255
                emitByte(OP_SET_LOCAL_LONG);
                emitShort((uint16_t)arg);
            } else {
                // BUG FIX: You likely don't have OP_SET_GLOBAL_LONG
                // Globals are stored in the constant table, so indices > 255
                // for globals are rare, but you must handle the case.
                error("Too many globals for long assignment.");
            }
        }
    } else {
        if (arg <= UINT8_MAX) {
            emitBytes(getOp, (uint8_t)arg);
        } else {
            // FIX: If it's a local, use GET_LOCAL_LONG.
            // If it's a global, you must fetch the name from the constant table
            // using OP_CONSTANT_LONG so the VM knows which global to look up.
            if (getOp == OP_GET_LOCAL) {
                emitByte(OP_GET_LOCAL_LONG);
                emitShort((uint16_t)arg);
            } else {
                // For globals, Lox typically uses the constant table to store the name.
                // You need to emit the instruction that gets a long constant index.
                emitByte(OP_CONSTANT_LONG);
                emitShort((uint16_t)arg);
                emitByte(OP_GET_GLOBAL); // Then tell the VM to treat that constant as a global name
            }
        }
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
  [TOKEN_AND]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_CLASS]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_ELSE]          = {NULL,     NULL,   PREC_NONE},
  [TOKEN_FALSE]         = {literal,  NULL,   PREC_NONE},
  [TOKEN_FOR]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_FUN]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_IF]            = {NULL,     NULL,   PREC_NONE},
  [TOKEN_NIL]           = {literal,     NULL,   PREC_NONE},
  [TOKEN_OR]            = {NULL,     NULL,   PREC_NONE},
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

static bool isFinalGlobal(Token* name) {
    // for (int i = 0; i < current->finalGlobalCount; i++) {
    //     if (identifiersEqual(name, &current->finalGlobals[i])) {
    //         return true;
    //     }
    // }
    // return false;
    // 1. Convert the token name to a Lox String
    ObjString* string = copyString(name->start, name->length);

    // 2. Check if this string exists as a key in the VM's finalGlobals table
    Value dummy;
    return tableGet(&vm.finalGlobals, string, &dummy);
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

// static void varDeclaration(bool isFinal) {
//     uint8_t global = parseVariable("Expect variable name.");
//
//     if (match(TOKEN_EQUAL)) {
//         expression();
//     } else {
//         if (isFinal) {
//             error("Final variable must be initialized.");
//         }
//         emitByte(OP_NIL);
//     }
//     consume(TOKEN_SEMICOLON,
//             "Expect ';' after variable declaration.");
//
//     defineVariable(global);
// }

/*
static void varDeclaration(bool isFinal) {
    // Pass isFinal to parseVariable
    uint8_t global = parseVariable("Expect variable name.", isFinal);

    if (match(TOKEN_EQUAL)) {
        expression();
    } else {
        if (isFinal) {
            error("Final variable must be initialized.");
        }
        emitByte(OP_NIL);
    }
    consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration.");

    defineVariable(global);
}
*/

static void varDeclaration(bool isFinal) {
    uint8_t global = parseVariable("Expect variable name.", isFinal);

    if (match(TOKEN_EQUAL)) {
        expression();
    } else {
        if (isFinal) error("Final variable must be initialized.");
        emitByte(OP_NIL);
    }

    consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration.");

    defineVariable(global, isFinal);
}

static void expressionStatement() {
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after expression.");
    emitByte(OP_POP);
}

static void printStatement() {
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after value.");
    emitByte(OP_PRINT);
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
        varDeclaration(false);
    } else if (match(TOKEN_FINAL)) {
        varDeclaration(true);
    } else {
        statement();
    }

    if (parser.panicMode) synchronize();
}

static void statement() {
    if (match(TOKEN_PRINT)) {
        printStatement();
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