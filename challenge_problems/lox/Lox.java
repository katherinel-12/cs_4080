//package com.craftinginterpreters.lox;
package lox;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.nio.charset.Charset;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.List;

public class Lox {
    //    start using the interpreter
    private static final Interpreter interpreter = new Interpreter();
    //    flag for the error() function
    static boolean hadError = false;
    //    runtimeError() sets this field
    static boolean hadRuntimeError = false;

    public static void main(String[] args) throws IOException {
        if (args.length > 1) {
            System.out.println("Usage: jlox [script]");
            System.exit(64);
        } else if (args.length == 1) {
            runFile(args[0]);
        } else {
            runPrompt();
        }
    }

    //    start jlox from the command line and give it a path to a file, it reads the file and executes it
    private static void runFile(String path) throws IOException {
        byte[] bytes = Files.readAllBytes(Paths.get(path));
        run(new String(bytes, Charset.defaultCharset()));

        //    indicate an error in the exit code
        //    65: data format error - input data was incorrect, corrupted, or did not conform to the expected format
        if (hadError) System.exit(65);
        //    70: internal software error - bug or unexpected condition within the program itself
        //        not related to the input data or environment
        if (hadRuntimeError) System.exit(70);
    }

    //    reads a line of input from the user on the command line and returns the result
    private static void runPrompt() throws IOException {
        InputStreamReader input = new InputStreamReader(System.in);
        BufferedReader reader = new BufferedReader(input);

        for (;;) {
            System.out.print("> ");
            String line = reader.readLine();
            if (line == null) break;
            run(line);

            //    reset the error() flag
            //    if the user makes a mistake, it shouldn’t kill their entire session
            hadError = false;
        }
    }

    //    the prompt and the file runner are wrappers around this core function:
    private static void run(String source) {
        Scanner scanner = new Scanner(source);
        List<Token> tokens = scanner.scanTokens();

        Parser parser = new Parser(tokens);
        List<Stmt> statements = parser.parse();

        //    Stop if there was a syntax error.
        if (hadError) return;

        interpreter.interpret(statements);
    }

    //    this error() function and its report() helper tells the user some syntax error occurred on a given line
    static void error(int line, String message) {
        report(line, "", message);
    }

    private static void report(int line, String where, String message) {
        System.err.println(
                "[line " + line + "] Error" + where + ": " + message);
        hadError = true;
    }

    //    use the token associated with the RuntimeError to tell the user what line of code
    //    was executing when the error occurred
    static void runtimeError(RuntimeError error) {
        System.err.println(error.getMessage() +
                "\n[line " + error.token.line + "]");
        hadRuntimeError = true;
    }

    //    call that shows the error to the user using method error(Token token, String message) in lox/Parser.java
    //    this reports an error: shows the token’s location and the token itself
    static void error(Token token, String message) {
        if (token.type == TokenType.EOF) {
            report(token.line, " at end", message);
        } else {
            report(token.line, " at '" + token.lexeme + "'", message);
        }
    }
}
