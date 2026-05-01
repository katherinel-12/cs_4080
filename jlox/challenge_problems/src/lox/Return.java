package lox;

//    take the value from visitReturnStmt() in lox/Interpreter.java 
//    and wrap it in a custom exception class and throw it
class Return extends RuntimeException {
  final Object value;

  Return(Object value) {
    super(null, null, false, false);
    this.value = value;
  }
}


