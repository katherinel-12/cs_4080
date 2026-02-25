package lox;

// AstPrinter took in a syntax tree and recursively traversed it, building up a string which it returned
// interpreter does the same, except instead of concatenating strings, it computes values

// post-order traversal—each node evaluates its children before doing its own work

class Interpreter implements Expr.Visitor<Object> {
    //    takes in a syntax tree for an expression and evaluates it
    //    yes, evaluate() returns an object for the result value and
    //    interpret() converts that to a string and shows it to the user
    void interpret(Expr expression) {
        try {
            Object value = evaluate(expression);
            System.out.println(stringify(value));
        } catch (RuntimeError error) {
            Lox.runtimeError(error);
        }
    }
    //    we converted a literal token into a literal syntax tree node in the parser
    //    now we convert the literal tree node into a runtime value
    @Override
    public Object visitLiteralExpr(Expr.Literal expr) {
        return expr.value;
    }

    //    evaluate the operand expression, then apply the unary operator to the result
    @Override
    public Object visitUnaryExpr(Expr.Unary expr) {
        Object right = evaluate(expr.right);

        switch (expr.operator.type) {
            case BANG:
                return !isTruthy(right);
            case MINUS:
                //    Lox-specific cast failure so that we can handle it how we want
                checkNumberOperand(expr.operator, right);
                return -(double)right;
        }

        //    Unreachable.
        return null;
    }

    //    to check the operand
    private void checkNumberOperand(Token operator, Object operand) {
        if (operand instanceof Double) return;
        throw new RuntimeError(operator, "Operand must be a number.");
    }

    //    helper for visitBinaryExpr(Expr.Binary expr)
    private void checkNumberOperands(Token operator,
                                     Object left, Object right) {
        if (left instanceof Double && right instanceof Double) return;

        throw new RuntimeError(operator, "Operands must be numbers.");
    }

    //    false and nil are falsey, and everything else is truthy
    private boolean isTruthy(Object object) {
        if (object == null) return false;
        if (object instanceof Boolean) return (boolean)object;
        return true;
    }

    private boolean isEqual(Object a, Object b) {
        if (a == null && b == null) return true;
        if (a == null) return false;

        return a.equals(b);
    }

    //    convert a Lox value to a string
    private String stringify(Object object) {
        if (object == null) return "nil";

        if (object instanceof Double) {
            String text = object.toString();
            if (text.endsWith(".0")) {
                text = text.substring(0, text.length() - 2);
            }
            return text;
        }

        return object.toString();
    }

    //    grouping node has a reference to an inner node for the expression inside the parentheses
    //    to evaluate the grouping expression itself, recursively evaluate the subexpression and return it
    @Override
    public Object visitGroupingExpr(Expr.Grouping expr) {
        return evaluate(expr.expression);
    }

    //    helper method that sends the expression back into the interpreter’s visitor implementation
    private Object evaluate(Expr expr) {
        return expr.accept(this);
    }

    @Override
    public Object visitBinaryExpr(Expr.Binary expr) {
        Object left = evaluate(expr.left);
        Object right = evaluate(expr.right);

        switch (expr.operator.type) {
            //    comparison operators always produce a Boolean
            //    comparison operators require numbers
            case GREATER:
                checkNumberOperands(expr.operator, left, right);
                return (double)left > (double)right;
            case GREATER_EQUAL:
                checkNumberOperands(expr.operator, left, right);
                return (double)left >= (double)right;
            case LESS:
                checkNumberOperands(expr.operator, left, right);
                return (double)left < (double)right;
            case LESS_EQUAL:
                checkNumberOperands(expr.operator, left, right);
                return (double)left <= (double)right;
            //    arithmetic operators produce a value whose type is the same as the operands
            case MINUS:
                checkNumberOperands(expr.operator, left, right);
                return (double)left - (double)right;
            //    the + operator can also be used to concatenate two strings
            case PLUS:
                if (left instanceof Double && right instanceof Double) {
                    return (double)left + (double)right;
                }

                if (left instanceof String && right instanceof String) {
                    return (String)left + (String)right;
                }
                //    fail if neither of the two success cases match
                throw new RuntimeError(expr.operator,
                        "Operands must be two numbers or two strings.");
            case SLASH:
                checkNumberOperands(expr.operator, left, right);
                return (double)left / (double)right;
            case STAR:
                checkNumberOperands(expr.operator, left, right);
                return (double)left * (double)right;
            //    equality operators support operands of any type, even mixed ones
            case BANG_EQUAL: return !isEqual(left, right);
            case EQUAL_EQUAL: return isEqual(left, right);
        }

        //    Unreachable.
        return null;
    }
}
