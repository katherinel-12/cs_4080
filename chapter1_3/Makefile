# Compile the C file to executable
hello: src/hello.c
	@gcc -o hello src/hello.c

# Convenience target to build and run
run: hello
	@./hello

# Tell make that "run" is not a file
.PHONY: run

