// // This is probably the correct version to have in the book and can be put back after Homework 3 graded 
// for future reference to create the folder and the .class file 
// katherinele@MacBook-Pro chapter4 % javac -d tool_v2 tool/GenerateAst.java
// katherinele@MacBook-Pro chapter4  % java -cp tool_v2 tool.GenerateAst lox

// // Version 3 after adding Factory 
// package tool;

// import lox.Token;

// // import java.util.List;

// abstract class Expr_v3 {
//   interface Visitor<R> {
//     R visitBinaryExpr(Binary expr);
//     // R visitBinaryExpr(Comma expr);
//     R visitGroupingExpr(Grouping expr);
//     R visitLiteralExpr(Literal expr);
//     R visitUnaryExpr(Unary expr);
//   }
//   static class Binary extends Expr_v3 {
//     Binary(Expr_v3 left, Token operator, Expr_v3 right) {
//       this.left = left;
//       this.operator = operator;
//       this.right = right;
//     }

//     @Override
//     <R> R accept(Visitor<R> visitor) {
//       return visitor.visitBinaryExpr(this);
//     }

//     final Expr_v3 left;
//     final Token operator;
//     final Expr_v3 right;
//   }

//   static class Grouping extends Expr_v3 {
//     Grouping(Expr_v3 expression) {
//       this.expression = expression;
//     }

//     @Override
//     <R> R accept(Visitor<R> visitor) {
//       return visitor.visitGroupingExpr(this);
//     }

//     final Expr_v3 expression;
//   }

//   static class Literal extends Expr_v3 {
//     Literal(Object value) {
//       this.value = value;
//     }

//     @Override
//     <R> R accept(Visitor<R> visitor) {
//       return visitor.visitLiteralExpr(this);
//     }

//     final Object value;
//   }

//   static class Unary extends Expr_v3 {
//     Unary(Token operator, Expr_v3 right) {
//       this.operator = operator;
//       this.right = right;
//     }

//     @Override
//     <R> R accept(Visitor<R> visitor) {
//       return visitor.visitUnaryExpr(this);
//     }

//     final Token operator;
//     final Expr_v3 right;
//   }

//   abstract <R> R accept(Visitor<R> visitor);
// }
