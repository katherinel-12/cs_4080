package lox;

import java.util.HashMap;
import java.util.Map;

// storage space for the bindings that associate variables to values

class Environment {
    final Environment enclosing;
    private final Map<String, Object> values = new HashMap<>();

    //    constructors initializing Line 9
    //    global scope’s environment, which ends the nesting chain
    Environment() {
        enclosing = null;
    }

    //    works with Line 14
    //    creates a new local scope nested inside the given outer one
    Environment(Environment enclosing) {
        this.enclosing = enclosing;
    }

    //    a variable definition binds a new name to a value
    //    can also be used to redefine an existing variable
    void define(String name, Object value) {
        values.put(name, value);
    }

    // Environment ancestor(int distance) {
    //     Environment environment = this;
    //     for (int i = 0; i < distance; i++) {
    //     environment = environment.enclosing; 
    //     }

    //     return environment;
    // }

    //    both getAt() and assignAt()
    //    walks a fixed number of environments, then stuffs the new value in that map
    // Object getAt(int distance, String name) {
    //     return ancestor(distance).values.get(name);
    // }

    // void assignAt(int distance, Token name, Object value) {
    //     ancestor(distance).values.put(name.lexeme, value);
    // }

    //    once a variable exists, we need a way to look it up
    Object get(Token name) {
        if (values.containsKey(name.lexeme)) {
            return values.get(name.lexeme);
        }

        //    variable lookup and assignment work with existing variables
        //    so they need to walk the nested chain to find them
        if (enclosing != null) return enclosing.get(name);

        throw new RuntimeError(name,
                "Undefined variable '" + name.lexeme + "'.");
    }

    //    assign vs define: assignment is not allowed to CREATE a new variable
    void assign(Token name, Object value) {
        if (values.containsKey(name.lexeme)) {
            values.put(name.lexeme, value);
            return;
        }

        //    if the variable isn’t found in Line 38, simply try the outer environment
        //    which does the same thing recursively, walking the entire chain
        //    if we reach an environment with no outer one and still don’t
        //    find the variable, give up and report an error
        if (enclosing != null) {
            enclosing.assign(name, value);
            return;
        }

        throw new RuntimeError(name,
                "Undefined variable '" + name.lexeme + "'.");
    }


}
