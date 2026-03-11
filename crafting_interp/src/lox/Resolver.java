package lox;

import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Stack;

//    visits every node in the syntax tree
class Resolver implements Expr.Visitor<Void>, Stmt.Visitor<Void> {
    private final Interpreter interpreter;

    //    keeps track of the stack of scopes currently, uh, in scope
    private final Stack<Map<String, Boolean>> scopes = new Stack<>();

    private FunctionType currentFunction = FunctionType.NONE;

    Resolver(Interpreter interpreter) {
        this.interpreter = interpreter;
    }

    private enum FunctionType {
        NONE,
        FUNCTION
    }

    //    walks a list of statements and resolves each one
    void resolve(List<Stmt> statements) {
        for (Stmt statement : statements) {
            resolve(statement);
        }
    }
    
    //    begins a new scope, traverses into the statements inside the block, 
    //    and then discards the scope to RESOLVE BLOCKS 
    @Override
    public Void visitBlockStmt(Stmt.Block stmt) {
        beginScope();
        resolve(stmt.statements);
        endScope();
        return null;
    }

    //    resolving other syntax tree nodes: 
    //    an expression statement contains a single expression to traverse 
    @Override
    public Void visitExpressionStmt(Stmt.Expression stmt) {
        resolve(stmt.expression);
        return null;
    }

    //    resolving function declarations: 
    //    functions both bind names and introduce a scope 
    //    the name of the function is bound in the surrounding scope where the function is declared 
    //    step into the function’s body and bind its parameters into that inner function scope 
    @Override
    public Void visitFunctionStmt(Stmt.Function stmt) {
        declare(stmt.name);
        define(stmt.name);

        resolveFunction(stmt, FunctionType.FUNCTION);
        return null;
    }

    //    resolving other syntax tree nodes: 
    //    if statement has an expression for its condition and 1 or 2 statements for the branches
    @Override
    public Void visitIfStmt(Stmt.If stmt) {
        resolve(stmt.condition);
        resolve(stmt.thenBranch);
        if (stmt.elseBranch != null) resolve(stmt.elseBranch);
        return null;
    }

    //    resolving other syntax tree nodes: 
    //    a print statement contains a single subexpression 
    @Override
    public Void visitPrintStmt(Stmt.Print stmt) {
        resolve(stmt.expression);
        return null;
    }

    //    resolving other syntax tree nodes: 
    //    a return statement has a single subexpression 
    @Override
    public Void visitReturnStmt(Stmt.Return stmt) {
        if (currentFunction == FunctionType.NONE) {
            Lox.error(stmt.keyword, "Can't return from top-level code.");
        }
        
        if (stmt.value != null) {
        resolve(stmt.value);
        }

        return null;
    }

    //    Resolving a variable declaration adds a new entry to the current innermost scope’s map
    @Override
    public Void visitVarStmt(Stmt.Var stmt) {
        //    split binding into two steps, declaring then defining 
        //    to handle funny edge cases like var a = "outer"; { var a = a; }
        declare(stmt.name);
        if (stmt.initializer != null) {
            resolve(stmt.initializer);
        }
        define(stmt.name);
        return null;
    }

    //    resolving other syntax tree nodes: 
    //    a while statement, resolve its condition and resolve the body exactly once
    @Override
    public Void visitWhileStmt(Stmt.While stmt) {
        resolve(stmt.condition);
        resolve(stmt.body);
        return null;
    }

    //    check to see if the variable is being accessed inside its own initializer
    @Override
    public Void visitVariableExpr(Expr.Variable expr) {
        if (!scopes.isEmpty() &&
            scopes.peek().get(expr.name.lexeme) == Boolean.FALSE) {
        Lox.error(expr.name,
            "Can't read local variable in its own initializer.");
        }

        resolveLocal(expr, expr.name);
        return null;
    }

    //    resolving assignment expressions: 
    //    resolve the expression for the assigned value 
    //    (in case it also contains references to other variables) 
    //    then use resolveLocal() to resolve the variable that’s being assigned to
    @Override
    public Void visitAssignExpr(Expr.Assign expr) {
        resolve(expr.value);
        resolveLocal(expr, expr.name);
        return null;
    }

    //    resolving other syntax tree nodes: 
    //    binary expression, traverse into and resolve both operands 
    @Override
    public Void visitBinaryExpr(Expr.Binary expr) {
        resolve(expr.left);
        resolve(expr.right);
        return null;
    }

    //    resolving other syntax tree nodes: 
    //    calls, walk the argument list and resolve them all 
    //    thing being called is also an expression (usually a variable expression), so resolve 
    @Override
    public Void visitCallExpr(Expr.Call expr) {
        resolve(expr.callee);

        for (Expr argument : expr.arguments) {
        resolve(argument);
        }

        return null;
    }

    //    resolving other syntax tree nodes: parentheses
    @Override
    public Void visitGroupingExpr(Expr.Grouping expr) {
        resolve(expr.expression);
        return null;
    }

    //    resolving other syntax tree nodes: literals 
    //    literal expression doesn’t mention any variables or contain any subexpressions
    @Override
    public Void visitLiteralExpr(Expr.Literal expr) {
        return null;
    }

    //    resolving other syntax tree nodes: same as binary 
    @Override
    public Void visitLogicalExpr(Expr.Logical expr) {
        resolve(expr.left);
        resolve(expr.right);
        return null;
    }

    //    resolving other syntax tree nodes: unaries, resolve its one operand 
    @Override
    public Void visitUnaryExpr(Expr.Unary expr) {
        resolve(expr.right);
        return null;
    }
    
    //    these methods are similar to the evaluate() and execute() methods in Interpreter
    private void resolve(Stmt stmt) {
        stmt.accept(this);
    }

    private void resolve(Expr expr) {
        expr.accept(this);
    }

    //    resolve the function’s body
    private void resolveFunction(Stmt.Function function, FunctionType type) {
        //    stash the previous value of the field in a local variable first 
        //    Lox has local functions, so you can nest function declarations arbitrarily deeply 
        //    need to track not just that we’re in a function, but how many we’re in 
        FunctionType enclosingFunction = currentFunction;
        currentFunction = type;
        beginScope();
        for (Token param : function.params) {
            declare(param);
            define(param);
        }
        resolve(function.body);
        endScope();
        currentFunction = enclosingFunction;
    }

    //    lexical scopes nest in both the interpreter and the resolver
    //    the interpreter implements that stack using a 
    //    linked list (the chain of Environment objects)
    private void beginScope() {
        scopes.push(new HashMap<String, Boolean>());
    }

    //    exit the scope 
    private void endScope() {
        scopes.pop();
    }

    //    What happens when the initializer for a local variable refers to a variable with the same name as the variable being declared? 
    //    make it an error to reference a variable in its initializer: 
    //    have the interpreter fail at compile time or runtime (we're choosing compile time)
    //    if an initializer mentions the variable being initialized 
    private void declare(Token name) {
        //    declaration adds the variable to the innermost scope so 
        //    it shadows any outer one and so we know the variable exists

        if (scopes.isEmpty()) return;

        Map<String, Boolean> scope = scopes.peek();

        //    detect some local mistake statically while resolving
        //    ex. var a = "first"; var a = "second"; (2 vars with the same name)
        if (scope.containsKey(name.lexeme)) {
        Lox.error(name,
            "Already a variable with this name in this scope.");
        }

        scope.put(name.lexeme, false);
    }

    //    set the variable’s value in the scope map to true 
    //    to mark it as fully initialized and available for use
    private void define(Token name) {
        if (scopes.isEmpty()) return;
        scopes.peek().put(name.lexeme, true);
    }

    //    resolve the variable itself
    private void resolveLocal(Expr expr, Token name) {
        for (int i = scopes.size() - 1; i >= 0; i--) {
            if (scopes.get(i).containsKey(name.lexeme)) {
            interpreter.resolve(expr, scopes.size() - 1 - i);
            return;
            }
        }
    }


}
