package lox;

// related to checkNumberOperand(Token operator, Object operand) in lox/Interpreter.java
class RuntimeError extends RuntimeException {
    final Token token;

    RuntimeError(Token token, String message) {
        super(message);
        this.token = token;
    }
}


