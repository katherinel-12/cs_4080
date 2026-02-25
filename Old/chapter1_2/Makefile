# Compile the Java file to a class file in src/
src/Hello.class: src/Hello.java
	@ javac src/Hello.java

# Convenience target to build and run it
run: src/Hello.class
	@ java -cp src Hello

# Tell make that "run" is not a file
.PHONY: run

