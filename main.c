#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>

#define int long long 


int poolsize;
int line_number;
int line;
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

int index_of_bp; //index of bp pointer on stack
//tokens and classes
enum {
  Num = 128, Fun, Sys, Glo, Loc, Id, Char, Else, 
  Enum, If, Int, Return, Sizeof, While, Assign, Cond, 
  Lor, Lan, Or, Xor, And, Eq, Ne, Lt, Gt, Le, Ge, Shl, 
  Shr, Add, Sub, Mul, Div, Mod, Inc, Dec, Brak };


//CPU instructions based on x86 
enum { LEA, IMM, JMP, CALL, JZ, JNZ, ENT, ADJ, LEV, LI, 
      LC, SI, SC, PUSH, OR, XOR, AND, EQ, NE, LT, GT, LE, 
      GE, SHL, SHR, ADD, SUB, MUL, DIV, MOD, OPEN, READ, 
      CLOS, PRTF, MALC,MSET, MCMP, EXIT, SSLD, CSLD };

//interpreter does not accept struct therefore enum is used
enum { Token, Hash, Name, Type, Class, Value, BType, BClass, BValue, IdSize };

//types of vars / functions
enum {CHAR, INT, PTR};
int basetype; //type of declaration, global for convenience
int expr_type; //expression type

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
      } token = Num;
      return;
    }
    else if(token == '"' || token == '\''){
      //parse string literal    
      last_pos = data;
      while(*src != 0 && *src != token){
        token_val = *src++;
        if(token_val  == '\\'){
          //escape char
          token_val = *src++;
          if(token_val == 'n'){
            token_val = '\n';
          }
        }

        if(token == '"'){
          *data++ = token_val;
        }
      }
      src++;

      //if single char return Num token_val
      if(token == '"'){
        token_val = (int)last_pos;
      }
      else{
        token = Num;
      }
      return;
    }


    // comment or divide
    else if(token == '/'){
      if(*src  == '/'){
        //skip comments
        while(*src != 0 && *src != '\n'){
          ++src;
        }
      }
      else{
        //divide operator
        token = Div;
        return;
      }
    }

    // = , ==
    else if(token == '='){
      src++;

      if(*src == '='){
        src++;
        token = Eq;
      }
      else{
        token = Assign;
      }
      return;
    }

    // +, ++
    else if(token == '+'){
      if(*src == '+'){
        src++;
        token = Inc;
      }
      else{
        token = Add;
      }
      return;
    }

    // -, --
    else if(token == '-'){
      if(*src == '-'){
        src++;
        token = Dec;
      }
      else{
        token = Sub;
      }
      return;
    }

    // !=
    else if(token == '!'){
      //parse !=
      if(*src == '='){
        src++;
        token = Ne;
      }
      return;
    }

    // <=, <<, <
    else if(token == '<'){
      //parse '<=', '<<', '<'
      if(*src == '='){
        src++;
        token = Le;
      }
      else if(*src == '<'){
        src++;
        token = Shl;
      }
      else{
        token = Lt;
      }
      return;
    }
    else if(token == '>'){
      if(*src == '='){
        src++;
        token = Ge;
      }
      else if(*src == '>'){
        src++;
        token = Shr;
      }
      else{
        token = Gt;
      }
      return;
    }
    // |, ||
    else if(token == '|'){
      if(*src == '|'){
        src++;
        token = Lor;
      }
      else{
        token = Or;
      }
      return;
    }

    // &, &&
    else if(token == '&'){
      if(*src == '&'){
        src++;
        token = Lan;
      }
      else{
        token = And;
      }
      return;
    }


    //general conditions
    else if(token == '^'){
      token = Xor;
      return;
    }

    else if(token == '%'){
      token = Mod;
      return;
    }

    else if(token == '*'){
      token = Mul;
      return;
    }

    else if(token == '['){
      token = Brak;
      return;
    }

    else if(token == '?'){
      token = Cond;
      return;
    }

    else if(token == '~' || token == ';' || token == '{' || token == '}' || token == '(' || token == ')' || token == ']' || token == ',' || token == ':'){
      //return char as token
      return;
    }

  }
  return;
}




void match(int tkn){
  if(token == tkn){
    next();
  }
  printf("%d: expected token:%d\n", line, tkn);
  exit(-1);
}





void function_parameter(){
  int type;
  int params;
  params = 0;

  while(token != ')'){


    //int name
    type = INT;
    if(token == Int){
      match(Int);
    }
    else if(token == Char){
      type = CHAR;
      match(Char);
    }


    //pointer type
    while(token == Mul){
      match(Mul);
      type = type + PTR;
    }

    //param name
    if(token != Id){
      printf("%d: bad parameter declaration\n", line);
      exit(-1);
    }

    if(current_id[Class] == Loc){
      printf("%d: duplicate paramter declaration\n", line);
      exit(-1);
    }

    match(Id);


    //store local vars
    current_id[BClass] = current_id[Class];
    current_id[Class] = Loc;
    current_id[BType] = current_id[Type];
    current_id[Type] = type;
    current_id[BValue] = current_id[Value];
    current_id[Value] = params++;

    if(token == ','){
      match(',');
    }
  }

  index_of_bp = params + 1;
}


void function_body(){

  //interpreter requires all variables to be declared at start of function

  int pos_local; //pos of local vars on stack
  int type;
  pos_local = index_of_bp;

  while(token == Int || token == Char){

    //local var declaration
    basetype = (token == Int) ? INT : CHAR;
    match(token);

    while(token != ';'){
      type = basetype;
      while(token == Mul){
        type = type + PTR;
      }

      if(token != Id){
        printf("%d: bad local variable declaration\n", line);
        exit(-1);
      }

      if(current_id[Class] == Loc){
        printf("%d: duplicate local declaration\n", line);
        exit(-1);
      }

      match(Id);

      //store local vars
      current_id[BClass] = current_id[Class];
      current_id[Class] = Loc;
      current_id[BType] = current_id[Type];
      current_id[Type] = type;
      current_id[BValue] = current_id[Value];
      current_id[Value] = ++pos_local;

      if(token == ','){
        match(',');
      }
    }
    match(';');
  }

  //save stack size for local vars
  *++text  = ENT;
  *++text = pos_local - index_of_bp;


  //TODO: Statements
  while(token != '}'){
    statement();
  }


  //emit code for leaving sub function
  *++text = LEV;
}


void function_declaration(){
  match('(');
  function_parameter();
  match(')');
  match('{');
  function_body();
  match('}');


  //unwind local variable declarations for all local variables 
  current_id = symbols;
  while(current_id[Token]){
    if(current_id[Class] == Loc){
      current_id[Class] = current_id[BClass];
      current_id[Type] = current_id[BType];
      current_id[Value] = current_id[BValue];
    }
    current_id = current_id + IdSize;
  }
}

void enum_declaration(){
  //parse enum [id] {a = 10, b = 20, ...}
  int i;
  i = 0;
  while(token != '}'){
    if(token != Id){
      printf("%d: bad enum identifier", line);
      exit(-1);
    }
    next();
    if(token == Assign){
      next();
      if(token != Num){
        printf("%d: bad enum initializer", line);
        exit(-1);
      }
      i = token_val;
      next();
    }
    current_id[Class] = Num;
    current_id[Type] = INT;
    current_id[Value] = i++;

    if(token == ','){
      next();
    }
  }
}


int basetype;
int expression_type;

void global_declaration(){
  int type; // tmp, actual var type
  int i; //tmp
  basetype = INT;


  //parse enum, should treat alone
  if(token == Enum) {
    // enum [id] {a = 10, b = 20, ...}
    match(Enum);
    if(token != '{'){
      match(Id); // skip [id] part
    }
    if(token == '{'){
      //parse assign part
      match('{');
      enum_declaration();
      match('}');
    }

    match(';');
    return;
  }

  //parse type info
  if(token == Int){
    match(Int);
  }
  if(token == Char){
    match(Char);
    basetype = CHAR;
  }

  //parse comma separated variable declaration
  while(token != ';' && token != '}'){
    type = basetype;

    // pointer type parsing '****var' may exist
    while(token == Mul){
      match(Mul);
      type = type + PTR;
    }

    if(token != Id){
      //invalid declaration
      printf("%d: bad global declaration\n", line);
      exit(-1);
    }

    if(current_id[Class]){
      //ident exists
      printf("%d: duplicate global declaration\n", line);
      exit(-1);
    }

    match(Id);
    current_id[Type] = type;

    if(token == '('){
      current_id[Class] = Fun;
      current_id[Value] = (int)(text + 1); // address of function
      function_declaration();
    } else {
      //variable declaration
      current_id[Class] = Glo;
      current_id[Value] = (int)data; // memory address
      data = data + sizeof(int);
    }

    if(token == ','){
      match(',');
    }
  }
  next();
}


void program(){
  next();
  while(token > 0){
    global_declaration();
  }
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


int *id_main;



int main(int argc, char **argv) {

  #define int long long

  int i, f;

  argc--;
  argv++;

  poolsize = 256 * 1024; //size can be changed
  line = 1;

  if((f = open(*argv, 0)) < 0){
    printf("could not open(%s)\n", *argv);
    return -1;
  }

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

  src = "char else enum if int return sizeof while "
          "open read close printf malloc memset memcmp exit void main";


  //add keywords to sym table
  i = Char;

  while(i <= While){
    next();
    current_id[Token] = i++;
  }


  // add lib to sym table
  i = OPEN;
  while(i <= EXIT){
    next();
    current_id[Class] = Sys;
    current_id[Type] = INT;
    current_id[Value] = i++;
  }

  next(); current_id[Token] = Char;
  next(); id_main = current_id;



  if (!(src = old_src = malloc(poolsize))) {
        printf("malloc(%d) failed for source area\n", poolsize);
        return -1;
    }
    // read source file
    if ((i = read(f, src, poolsize-1)) <= 0) {
        printf("read() returned %d\n", i);
        return -1;
    }


  src[i] = 0; //EOF char 
  close(f);
  program();

  //printf("next operation code: %d", *pc++);
  void* response = eval();
  printf("%s\n", response);
}


