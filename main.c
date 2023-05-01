#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>

#define int long long 


int poolsize;
int line_number;

int token, //current token
    token_val; //value of current token


int *text, //text segment
    *old_text, // dump text segment
    *stack; // stack
char *data; // data segment


char *src, *old_src; // pointer to source code string

// registers
/* 
 * pc = program counter 
 * stack_ptr = stack_ptr
 * base_ptr = base_ptr
 * gen_reg = general_register
 */
int *pc, *stack_ptr, *base_ptr, gen_reg, cycle;

//tokens and classes
enum {
  Num = 128, Fun, Sysm,   Glo, Loc, Id, Char, Else, Enum, If, Int, Return, Sizeof, While, Assign, Cond, Lor, Lan, Or, Xor, And, Eq, Ne, Lt, Gt, Le, Ge, Shl, Shr, Add, Sub, Mul, Div, Mod, Inc, Dec, Brak };


//CPU instructions based on x86 
enum { LEA, IMM, JMP, CALL, JZ, JNZ, ENT, ADJ, LEV, LI, LC, SI, SC, PUSH, OR, XOR, AND, EQ, NE, LT, GT, LE, GE, SHL, SHR, ADD, SUB, MUL, DIV, MOD, OPEN, READ, CLOS, PRTF, MALC,MSET, MCMP, EXIT, SSLD, CSLD };


//interpreter does not accept struct therefore enum is used
enum { Token, Hash, Name, Type, Class, Value, BType, BClass, BValue, IdSize };

int token_val;      //val of current token
int *current_id,    // current parsed Id
    *symbols;       // symbol table

void next() {
  char *last_pos;
  int hash;
  while(token = *src){
    ++src;

    if(token == '\n'){
      ++line_number;
    }
    else if(token == '#'){
      //skip, macro not supported
      while(*src != 0 && *src != '\n'){
        src++;
      }
    }
    else if((token >= 'a' && token <= 'z') || (token >= 'A' && token <= 'Z') || (token == '_')){
      
      //parse id
      last_pos = src -1;
      hash = token;

      while((*src >= 'a' && *src <= 'z') || (*src >= 'A' && *src <= 'Z') || (*src >= '0' && *src <= '9') || (*src == '_')){
        hash = hash * 147 + *src;
        src++;
      }

      //look for existing identifier
      current_id = symbols;
      while(current_id[Token]){
        if(current_id[Hash] == hash && !memcmp((char *)current_id[Name], last_pos, src - last_pos)) {
          //found one, return
          token = current_id[Token];
          return;
        }
        current_id = current_id + IdSize;
      }

      //store new ID
      current_id[Name] = (int)last_pos;
      current_id[Hash] = hash;
      token = current_id[Token] = Id;
      return;
    }
    else if(token >= '0' && token <= '9'){
      //parse number, three types: dec(123) hex(0x123) oct(012)
      token_val = token - '0';
      if(token_val > 0){
        //dec starts with number [1-9]
        while(*src >= '0' && *src <= '9'){
          token_val = token_val*10 + *src++ - '0';
        }
      }
      else {
        // starts with number 0
        if(*src == 'x' || *src == 'X'){
          //hex
          token = *++src;
          while((token >= '0' && token <= '9') || (token >= 'a' && token <= 'f') || (token >= 'A' && token <= 'F')){
            token_val = token_val * 16 +  (token & 15) + (token >= 'A' ? 9 : 0);
            token = *++src;
          }
        }
        else{
          //oct
          while(*src >= '0' && *src <= '7'){
            token_val = token_val * 8 + *src++ - '0';
          }
        }
      }
      token = Num;
      return;
    }
  }
  return;
}


void* eval(){
  int op, *tmp;

 while(1){
  op = *pc++; // get next operation code

  
  if(op == IMM) {gen_reg = *pc++;} // load immediate value to gen_reg
  else if(op == LC) {gen_reg = *(char *)gen_reg;} // load character to gen_reg, address in gen_reg
  else if(op == LI) {gen_reg = *(int *)gen_reg;} // load integer to gen_reg, address in gen_reg
  else if(op == SC) {gen_reg = *(char *)*stack_ptr++ = gen_reg;} //save character to address, value in gen_reg, address on stack
  else if(op == SI) {*(int *)*stack_ptr++ = gen_reg;} // save integer to address, value in gen_reg, address on stack



  else if(op == PUSH) {*--stack_ptr = gen_reg;} // push value of gen_reg onto stack
  else if(op == JMP) {pc = (int *)*pc;} // jump to address
  else if(op == JZ) {pc = gen_reg ? pc +1: (int *)*pc;} // jump if gen_reg is zero
  else if(op == JNZ) {pc = gen_reg ? (int *)*pc : pc + 1;} // jump if gen_reg is not zero
  else if(op == CALL) {*--stack_ptr = (int)(pc + 1); pc = (int *)*pc;} // call subroutine
  else if(op == ENT) {*--stack_ptr = (int)base_ptr; base_ptr = stack_ptr; stack_ptr = stack_ptr - *pc++;} //make new stack frame 
  else if(op == ADJ) {stack_ptr = stack_ptr + *pc++;} // add estack_ptr, <size>
  else if(op == LEV) {stack_ptr = base_ptr; base_ptr = (int *)*stack_ptr++; pc = (int *)*stack_ptr++;} // restore call frame and pc
  else if(op == LEA) {gen_reg = (int)(base_ptr + *pc++);} //load address for arguments


  else if (op == OR)  gen_reg = *stack_ptr++ | gen_reg;
  else if (op == XOR) gen_reg = *stack_ptr++ ^ gen_reg;
  else if (op == AND) gen_reg = *stack_ptr++ & gen_reg;
  else if (op == EQ)  gen_reg = *stack_ptr++ == gen_reg;
  else if (op == NE)  gen_reg = *stack_ptr++ != gen_reg;
  else if (op == LT)  gen_reg = *stack_ptr++ < gen_reg;
  else if (op == LE)  gen_reg = *stack_ptr++ <= gen_reg;
  else if (op == GT)  gen_reg = *stack_ptr++ >  gen_reg;
  else if (op == GE)  gen_reg = *stack_ptr++ >= gen_reg;
  else if (op == SHL) gen_reg = *stack_ptr++ << gen_reg;
  else if (op == SHR) gen_reg = *stack_ptr++ >> gen_reg;
  else if (op == ADD) gen_reg = *stack_ptr++ + gen_reg;
  else if (op == SUB) gen_reg = *stack_ptr++ - gen_reg;
  else if (op == MUL) gen_reg = *stack_ptr++ * gen_reg;
  else if (op == DIV) gen_reg = *stack_ptr++ / gen_reg;
  else if (op == MOD) gen_reg = *stack_ptr++ % gen_reg;
  else if (op == EXIT) { return (void *)*stack_ptr;}
  else if (op == OPEN) { gen_reg = open((char *)stack_ptr[1], stack_ptr[0]); }
  else if (op == CLOS) { gen_reg = close(*stack_ptr);}
  else if (op == READ) { gen_reg = read(stack_ptr[2], (char *)stack_ptr[1], *stack_ptr); }
  else if (op == PRTF) { tmp = stack_ptr + pc[1]; gen_reg = printf((char *)tmp[-1], tmp[-2], tmp[-3], tmp[-4], tmp[-5], tmp[-6]); }
  else if (op == MALC) { gen_reg = (int)malloc(*stack_ptr);}
  else if (op == MSET) { gen_reg = (int)memset((char *)stack_ptr[2], stack_ptr[1], *stack_ptr);}
  else if (op == MCMP) { gen_reg = memcmp((char *)stack_ptr[2], (char *)stack_ptr[1], *stack_ptr);}


  else if(op == SSLD) { data = (char *)*pc++; } //save to data segment
  else if(op == CSLD) {
    //to be implemented
    *pc++;
  } //concat *pc++ to data segment
  else{
    printf("unknown instruction:%d\n", op);
    return (void *)-1;
  }
}
    return 0;
}


int main(int argc, char *argv[]) {

  #define int long long

  int i;

  poolsize = 256 * 1024;

  // alloc mem for vm
  if(!(text = old_text = malloc(poolsize))){
    printf("could not malloc(%d) for text area\n", poolsize);
    return -1;
  }

  if(!(data = malloc(poolsize))){
    printf("could not malloc(%d) for data area\n", poolsize);
    return -1;
  }

  if(!(stack = malloc(poolsize))){
    printf("could not malloc(%d) for stack area\n");
    return -1;
  }

  memset(text, 0, poolsize);
  memset(data, 0, poolsize);
  memset(stack, 0, poolsize);


  //init base_ptr, stack_ptr, gen_reg
  base_ptr = stack_ptr = (int *)((int)stack + poolsize);
  gen_reg = 0;


  i = 0;

  text[i++] = SSLD;
  text[i++] = (int)"hello";
  text[i++] = CSLD;
  text[i++] = (int)"world";
  text[i++] = EXIT;
  
  pc = text;

  //printf("next operation code: %d", *pc++);
  void* restack_ptronse = eval();
  printf("%s\n", data);
}


