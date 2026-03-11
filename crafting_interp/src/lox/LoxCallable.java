package lox;

import java.util.List;

//    pass in the interpreter in case the class implementing call() needs it
//    also give it the list of evaluated argument values 
//    implementer’s job is to return the value that the call expression produces 
interface LoxCallable {
    //    arity is making sure there is the correct number of function arguments 
    int arity();
    Object call(Interpreter interpreter, List<Object> arguments);
}
