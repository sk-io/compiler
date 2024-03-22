# A Toy System Programming Language & Compiler 

Written in C. Outputs x64 nasm assembly. Uses the System V AMD64 calling convention. Is inefficient. Puts everything on the stack for now.

To build:
```
make
```

To run (Produces output.asm):
```
./compiler <source file>
```

Then to assemble and link the output:
```
nasm -felf64 output.asm
gcc -no-pie -o <executable> output.o
```