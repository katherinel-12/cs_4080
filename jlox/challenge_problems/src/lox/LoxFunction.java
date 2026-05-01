package lox;

import java.util.List;

class LoxFunction implements LoxCallable {
    private final String name;
    private final Expr.Function declaration;

    //    closure because it “closes over” and holds on to 
    //    the surrounding variables where the function is declared
    private final Environment closure;

    //    initialize the constructor
    LoxFunction(String name, Expr.Function declaration, Environment closure) {
        this.name = name;
        this.closure = closure;
        this.declaration = declaration;
    }

    @Override
    //    just gives nicer output if a user decides to print a function value
    public String toString() {
        if (name == null) return "<fn>";
        return "<fn " + name + ">";
    }

    @Override
    //    assume the parameter and argument lists have the same length 
    //    safe because visitCallExpr() checks the arity before calling call() 
    //    visitCallExpr() relies on this function to do that 
    public int arity() {
        return declaration.params.size();
    }

    //    each function gets its own environment where it stores the parameters
    //    the environment must be created dynamically 
    //    each function call gets its own environment, else recursion would break
    @Override
    public Object call(Interpreter interpreter, List<Object> arguments) {
        Environment environment = new Environment(closure);
        for (int i = 0; i < declaration.params.size(); i++) {
            environment.define(declaration.params.get(i).lexeme,
            arguments.get(i));
        }

        try {
            interpreter.executeBlock(declaration.body, environment);
        } catch (Return returnValue) {
            return returnValue.value;
        }

        return null;
    }


}


