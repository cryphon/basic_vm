# Basic VM

Learning how to make a virtual machine and understanding the components.  <br>
Recreating a very simple JVM like machine with its own set of instructions.
The instructions are based of x86 assembly instructions but much more simplefied

This project serves as a way for me to understand and build more advanced machines,
my ulterior motive is to be able to write my own interpreter or compiler.


following instructions from:  <br>
https://github.com/lotabout/write-a-C-interpreter/tree/master

## The VM
The virtual machine is made out of three main components, CPU, memory and registers
Assembly will be stored in the memory as binary, CPU receives and executes commands one by one 


### Memory

the three main components of the memory used are:
1. `text` the text segment where instructions will be stored
2. `data` where initialized data is stored for example `y = 20`
3. `stack` for calling frames, local variables and functions

```c
int *text, //text segment
    *old_text, // dump text segment
    *stack; // stack
char *data; // data segment
```


### Registers
Registers are used to store the running states of computers.
The VM uses 4 registers,

1. `PC`: program counter, stores an mem addresses in which the next instruction te be ran is stored.
2. `SP`: stack pointer, which always points to the *top* of the stack.
3. `BP`: base pointer, points to some elements on the stack. used in function calls to variables
4. `AX`: register where a result is stored

```c
int *pc, *bp, *sp, ax, cycle; // virtual machine registers
```

## CPU

the CPU follows a minimal set of instructions based of of the x86 CPU from intel assembly instructions. 

they were provided by a tuturial from a user: @lotabout who has made a comprehensive guide about VMs, interpreters and in general low-level concepts. <br>
https://github.com/lotabout/write-a-C-interpreter/tree/master

 ```c
 //CPU instructions based on x86 
enum { LEA, IMM, JMP, CALL, JZ, JNZ, ENT, ADJ, LEV, LI, 
       LC, SI, SC, PUSH, OR, XOR, AND, EQ, NE, LT, GT,
       LE, GE, SHL, SHR, ADD, SUB, MUL, DIV, MOD, OPEN,
       READ, CLOS, PRTF, MALC, MSET, MCMP, EXIT };
```


#### Test

to test, a number of instructions were inserted into memory (`text segment`)

instructions:
```c
  text[i++] = IMM;
  text[i++] = 30;
  text[i++] = PUSH;
  text[i++] = IMM;
  text[i++] = 20;
  text[i++] = ADD;
  text[i++] = PUSH;
  text[i++] = EXIT;

  pc = text;
  ```
  
  this returns an `exit(50)` where the return value `50` is the result of the instructions. <br>
  In this case `30 + 20`




## TODO
- [ ] optimize the eval function, if more instructions were to be added, writing it would be tedious
- [ ] able to import asm from file and run it through VM
- [ ] possibly parse C to assembly instructions where it is possible to use this VM to run it
