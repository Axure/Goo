#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
//#include <sys/io.h>
#include <fcntl.h>
#include <zconf.h>

// Update at 4.13.16, HZ, 217
// NOTE: NEVER use /**/ before I decide to add this feature, although it is easy ;)
// FORK it and EDIT it! And open PR in my github!
// I think I should write something like daily when I am refactoring.
// I write this before I read DragonBook systematically,
// when I read DragonBook recently, I am enlightened.
// I have solved some classic questions in the book
// (with the help of Google, StackOverflow, yeah, Google and StackOverflow Oriented Programming),
// so I can understand the "correct way" to solve them quickly.
// I think that's no a good way, but I really have some great thoughts and
// I can remember and understand the theory better.
// Is it a abstract stack machine? DragonBook mentions it and I think it is.

// Procedure Oriented can easily add some steps between steps.
// Object Oriented can easily add some "key word", like the Java Frontend.
// But it's too difficult to establish seg and keep it work properly.
// Maybe I should use C++? STL provides so many ds(s).
// Or Java, which we don't need to know the behavior exactly -- even if we SHOULD know it.
// An impression experience.

//current token
int token;
//pointer to source code
char *src, *previous_src;
//default size of data or stack
int poolsize;
int line_number;

int
// text segment
  *text,
// just for dump *text
  *previous_text,
// stack
  *stack;
// data segment
char *data;

//virtual machine registers
//use less, and divide the instructions
//sp:stack pointer, bp:base pointer, ax:accumulator, pc:next ins pointer
//TODO: cycle: count the instructions
int *pc, *bp, *sp, ax, cycle;

//instructions
enum {
    LEA, IMM, JMP, CALL, JZ, JNZ, ENT, ADJ, LEV, LI, LC, SI, SC, PUSH, OR, XOR, AND, EQ, NE, LT, GT, LE, GE, SHL, SHR,
    ADD, SUB, MUL, DIV, MOD, OPEN, READ, CLOS, PRINTF, MALLOC, MEMSET, MEMCMP, EXIT
};

// tokens and classes (operators last and in precedence order)
// 128 is larger than all defined ASCII
// order by priority
// id:identifier custom identifier
enum {
    Num = 128, Fun, Sys, Glo, Loc, Id, Char, Else, Enum, If, Int, Return, Sizeof, While, Assign, Cond, Lor, Lan, Or,
    Xor, And, Eq, Ne, Lt, Gt, Le, Ge, Shl, Shr, Add, Sub, Mul, Div, Mod, Inc, Dec, Brak
};

// value of current token (mainly for number)
int token_val;
// current parsed ID
int *current_id,
// symbol table
  *symbols;

// fields of identifier
enum {
    Token, Hash, Name, Type, Class, Value, BType, BClass, BValue, IdSize
};

// types of variable/function
enum {
    CHAR, INT, PTR
};
// the `main` function
int *idmain;

// the type of a declaration, make it global for convenience
int basetype;
// the type of an expression
int expression_type;
//index of bp on stack
int index_of_bp;

int is_uppercase(int c) {
  return c >= 'A' && c <= 'Z';
}

int is_lowercase(int c) {
  return c >= 'a' && c <= 'z';
}

int is_digit(int c) {
  return c >= '0' && c <= '9';
}

void next() {
  char *last_pos;
  int hash;

  // lazy while ;)
  while ((token = *(src++))) {
    // parse token here
    if (token == '\n') {
      ++line_number;
    } else if (token == '#') {
      // skip macro, because we will not support it (because the grammar
      while (*src != 0 && *src != '\n') {
        src++;
      }
    } else if (
      is_lowercase(token) ||
      is_uppercase(token) ||
      (token == '_')) {
      // parse identifier
      last_pos = src - 1;
      hash = token;

      while (is_lowercase(*src) || is_uppercase(*src) || is_digit(*src) ||
             (*src == '_')) {
        // 128 is random by me
        hash = hash * 128 + *src;
        src++;
      }

      // look for existing identifier, linear search
      current_id = symbols;
      while (current_id[Token]) {
        //
        if (current_id[Hash] == hash && !memcmp((char *) current_id[Name], last_pos, src - last_pos)) {
          //found one, return
          token = current_id[Token];
          return;
        }
        // offset
        current_id = current_id + IdSize;
      }


      // store new ID
      current_id[Name] = (int) last_pos;
      current_id[Hash] = hash;
      token = current_id[Token] = Id;
      return;
    } else if (is_digit(token)) {
      // parse number, three kinds: dec(123) hex(0x123) oct(017)
      token_val = token - '0';
      if (token_val > 0) {
        // dec, starts with [1-9]
        while (is_digit(*src)) {
          token_val = token_val * 10 + *src++ - '0';
        }
      } else {
        // starts with 0
        if (*src == 'x' || *src == 'X') {
          //hex
          token = *++src;
          while ((is_digit(token)) || (token >= 'a' && token <= 'f') ||
                 (token >= 'A' && token <= 'F')) {
            token_val = token_val * 16 + (token & 15) + (token >= 'A' ? 9 : 0);
            token = *++src;
          }
        } else {
          // oct
          while (*src >= '0' && *src <= '7') {
            token_val = token_val * 8 + *src++ - '0';
          }
        }
      }

      token = Num;
      return;
    } else if (token == '"' || token == '\'') {
      // " and ', like php or js?
      // parse string literal, the only supported escape
      // character is '\n', store the string literal into "data".
      last_pos = data;
      while (*src != 0 && *src != token) {
        token_val = *src++;
        if (token_val == '\\') {
          // escape character
          token_val = *src++;
          if (token_val == 'n') {
            token_val = '\n';
          }
        }

        if (token == '"') {
          *data++ = token_val;
        }
      }

      src++;
      // if it is a single character, return Num token
      if (token == '"') {
        token_val = (int) last_pos;
      } else {
        token = Num;
      }

      return;
    } else if (token == '/') {
      if (*src == '/') {
        // skip comments
        while (*src != 0 && *src != '\n') {
          ++src;
        }
      } else {
        // divide operator
        token = Div;
        return;
      }
    } else if (token == '=') {
      // parse '==' and '='
      if (*src == '=') {
        src++;
        token = Eq;
      } else {
        token = Assign;
      }
      return;
    } else if (token == '+') {
      // parse '+' and '++'
      if (*src == '+') {
        src++;
        token = Inc;
      } else {
        token = Add;
      }
      return;
    } else if (token == '-') {
      // parse '-' and '--'
      if (*src == '-') {
        src++;
        token = Dec;
      } else {
        token = Sub;
      }
      return;
    } else if (token == '!') {
      // parse '!='
      if (*src == '=') {
        src++;
        token = Ne;
      }
      return;
    } else if (token == '<') {
      // parse '<=', '<<' or '<'
      if (*src == '=') {
        src++;
        token = Le;
      } else if (*src == '<') {
        src++;
        token = Shl;
      } else {
        token = Lt;
      }
      return;
    } else if (token == '>') {
      // parse '>=', '>>' or '>'
      if (*src == '=') {
        src++;
        token = Ge;
      } else if (*src == '>') {
        src++;
        token = Shr;
      } else {
        token = Gt;
      }
      return;
    } else if (token == '|') {
      // parse '|' or '||'
      if (*src == '|') {
        src++;
        token = Lor;
      } else {
        token = Or;
      }
      return;
    } else if (token == '&') {
      // parse '&' and '&&'
      if (*src == '&') {
        src++;
        token = Lan;
      } else {
        token = And;
      }
      return;
    } else if (token == '^') {
      token = Xor;
      return;
    } else if (token == '%') {
      token = Mod;
      return;
    } else if (token == '*') {
      token = Mul;
      return;
    } else if (token == '[') {
      token = Brak;
      return;
    } else if (token == '?') {
      token = Cond;
      return;
    } else if (token == '~' || token == ';' || token == '{' || token == '}' || token == '(' || token == ')' ||
               token == ']' || token == ',' || token == ':') {
      // directly return the character as token
      return;
    }
  }
  return;
}

void match(int tk) {
  if (token != tk) {
    printf("expected %d(%c) but receive %d(%c)", tk, tk, token, token);
    exit(-1);
  }
  next();
}

void expression(int level) {
  int tmp;
  int *id;
  int *addr;

  if (!token) {
    printf("unexpected token EOF of expression in line %d\n", line_number);
    exit(-1);
  }
  if (token == Num) {
    match(Num);
    //emit code
    *++text = IMM;
    *++text = token_val;
    expression_type = INT;
  } else if (token == '"') {
    *++text = IMM;
    *++text = token_val;
    match('"');
    //for this"
    //"aaaa"
    //"bbbb"   -> "aaaabbbb"
    while (token == '"') {
      match('"');
    }
    //append EO string '\0'
    //but all the data are default to zero
    //so we just need to move data one position forward
    //MAGIC
    data = (char *) (((int) data + sizeof(int)) & (-sizeof(int)));
    expression_type = PTR;
  } else if (token == Sizeof) {
    match(Sizeof);
    match('(');
    expression_type = INT;
    if (token == Int) {
      match(Int);
    } else if (token == Char) {
      match(Char);
      expression_type = CHAR;
    }
    while (token == Mul) {
      match(Mul);
      expression_type = expression_type + PTR;
    }
    match(')');
    *++text = IMM;
    *++text = (expression_type == CHAR) ? sizeof(char) : sizeof(int);
    expression_type = INT;
  } else if (token == Id) {
    match(Id);
    id = current_id;
    if (token == '(') {
      match('(');
      //pass in arguments
      //number of arguments
      tmp = 0;
      while (token != ')') {
        expression(Assign);
        *++text = PUSH;
        tmp++;
        if (token == ',') {
          match(',');
        }
      }
      match(')');
      if (id[Class] == Sys) {
        //system functions
        *++text = id[Value];
      } else if (id[Class] == Fun) {
        *++text = CALL;
        *++text = id[Value];
      } else {
        printf("bad function calls in line %d\n", line_number);
        exit(-1);
      }

      //clean stack for arguments
      //what?
      if (tmp > 0) {
        *++text = ADJ;
        *++text = tmp;
      }
      expression_type = id[Type];
    } else if (id[Class] == Num) {
      // enum variable
      *++text = IMM;
      *++text = id[Value];
      expression_type = INT;
    } else {
      //variable
      if (id[Class] == Loc) {
        *++text = LEA;
        *++text = index_of_bp - id[Value];
      } else if (id[Class] == Glo) {
        *++text = IMM;
        *++text = id[Value];
      } else {
        printf("undefined variable in line %d\n", line_number);
        exit(-1);
      }
      // emit code, default behaviour is to load the value of the
      // address which is stored in `ax`
      expression_type = id[Type];
      *++text = (expression_type == Char) ? LC : LI;
    }
  } else if (token == '(') {
    //cast
    match('(');
    if (token == Int || token == Char) {
      tmp = (token == Char) ? CHAR : INT;
      match(token);
      while (token == Mul) {
        match(Mul);
        tmp = tmp + PTR;
      }
      match(')');
      // cast has the same priority as Inc(++)
      expression(Inc);
      expression_type = tmp;
    } else {
      expression(Assign);
      match(')');
    }
  } else if (token == Mul) {
    //dereference
    match(Mul);
    //dereference has the same priority as Inc(++)
    expression(Inc);
    if (expression_type >= PTR) {
      expression_type = expression_type - PTR;
    } else {
      printf("bad dereference in line %d\n", line_number);
      exit(-1);
    }
    *++text = (expression_type == CHAR) ? LC : LI;
  } else if (token == And) {
    match(And);
    expression(Inc);
    if (*text == LC || *text == LI) {
      text--;
    } else {
      printf("bad address of in line %d\n", line_number);
      exit(-1);
    }
    expression_type = expression_type + PTR;
  } else if (token == '!') {
    //==0?1:0;
    match('!');
    expression(Inc);
    // emit code, use <expr> == 0
    *++text = PUSH;
    *++text = IMM;
    *++text = 0;
    *++text = EQ;
    expression_type = INT;
  } else if (token == '~') {
    // bitwise not
    match('~');
    expression(Inc);
    // emit code, use <expr> XOR -1
    *++text = PUSH;
    *++text = IMM;
    *++text = -1;
    *++text = XOR;
    expression_type = INT;
  } else if (token == Add) {
    // +var, do nothing
    match(Add);
    expression(Inc);
    expression_type = INT;
  } else if (token == Sub) {
    // -var
    match(Sub);
    if (token == Num) {
      *++text = IMM;
      *++text = -token_val;
      match(Num);
    } else {
      *++text = IMM;
      *++text = -1;
      *++text = PUSH;
      expression(Inc);
      *++text = MUL;
    }
    expression_type = INT;
  } else if (token == Inc || token == Dec) {
    tmp = token;
    match(token);
    expression(Inc);
    // ++p use address of p twice, so we need PUSH
    if (*text == LC) {
      *text = PUSH;  // to duplicate the address
      *++text = LC;
    } else if (*text == LI) {
      *text = PUSH;
      *++text = LI;
    } else {
      printf("bad lvalue of pre-increment in line %d\n", line_number);
      exit(-1);
    }
    *++text = PUSH;
    *++text = IMM;
    // for pointer
    *++text = (expression_type > PTR) ? sizeof(int) : sizeof(char);
    *++text = (tmp == Inc) ? ADD : SUB;
    *++text = (expression_type == CHAR) ? SC : SI;
  }


  while (token >= level) {
    // handle according to current operator's precedence
    tmp = expression_type;
    if (token == Assign) {
      // var = expr;
      match(Assign);
      if (*text == LC || *text == LI) {
        *text = PUSH; // save the lvalue's pointer
      } else {
        printf("bad lvalue in assignment in line %d\n", line_number);
        exit(-1);
      }
      expression(Assign);

      expression_type = tmp;
      *++text = (expression_type == CHAR) ? SC : SI;
    } else if (token == Cond) {
      // expr ? a : b;
      match(Cond);
      *++text = JZ;
      addr = ++text;
      expression(Assign);
      if (token == ':') {
        match(':');
      } else {
        printf("missing colon in conditional in line %d\n", line_number);
        exit(-1);
      }
      *addr = (int) (text + 3);
      *++text = JMP;
      addr = ++text;
      expression(Cond);
      *addr = (int) (text + 1);
    } else if (token == Lor) {
      // logic or
      match(Lor);
      *++text = JNZ;
      addr = ++text;
      expression(Lan);
      *addr = (int) (text + 1);
      expression_type = INT;
    } else if (token == Lan) {
      // logic and
      match(Lan);
      *++text = JZ;
      addr = ++text;
      expression(Or);
      *addr = (int) (text + 1);
      expression_type = INT;
    } else if (token == Or) {
      // bitwise or
      match(Or);
      *++text = PUSH;
      expression(Xor);
      *++text = OR;
      expression_type = INT;
    } else if (token == Xor) {
      // bitwise xor
      match(Xor);
      *++text = PUSH;
      expression(And);
      *++text = XOR;
      expression_type = INT;
    } else if (token == And) {
      // bitwise and
      match(And);
      *++text = PUSH;
      expression(Eq);
      *++text = AND;
      expression_type = INT;
    } else if (token == Eq) {
      // equal ==
      match(Eq);
      *++text = PUSH;
      expression(Ne);
      *++text = EQ;
      expression_type = INT;
    } else if (token == Ne) {
      // not equal !=
      match(Ne);
      *++text = PUSH;
      expression(Lt);
      *++text = NE;
      expression_type = INT;
    } else if (token == Lt) {
      // less than
      match(Lt);
      *++text = PUSH;
      expression(Shl);
      *++text = LT;
      expression_type = INT;
    } else if (token == Gt) {
      // greater than
      match(Gt);
      *++text = PUSH;
      expression(Shl);
      *++text = GT;
      expression_type = INT;
    } else if (token == Le) {
      // less than or equal to
      match(Le);
      *++text = PUSH;
      expression(Shl);
      *++text = LE;
      expression_type = INT;
    } else if (token == Ge) {
      // greater than or equal to
      match(Ge);
      *++text = PUSH;
      expression(Shl);
      *++text = GE;
      expression_type = INT;
    } else if (token == Shl) {
      // shift left
      match(Shl);
      *++text = PUSH;
      expression(Add);
      *++text = SHL;
      expression_type = INT;
    } else if (token == Shr) {
      // shift right
      match(Shr);
      *++text = PUSH;
      expression(Add);
      *++text = SHR;
      expression_type = INT;
    } else if (token == Add) {
      // add
      match(Add);
      *++text = PUSH;
      expression(Mul);

      expression_type = tmp;
      if (expression_type > PTR) {
        // pointer type, and not `char *`
        *++text = PUSH;
        *++text = IMM;
        *++text = sizeof(int);
        *++text = MUL;
      }
      *++text = ADD;
    } else if (token == Sub) {
      // sub
      match(Sub);
      *++text = PUSH;
      expression(Mul);
      if (tmp > PTR && tmp == expression_type) {
        // pointer subtraction
        *++text = SUB;
        *++text = PUSH;
        *++text = IMM;
        *++text = sizeof(int);
        *++text = DIV;
        expression_type = INT;
      } else if (tmp > PTR) {
        // pointer movement
        *++text = PUSH;
        *++text = IMM;
        *++text = sizeof(int);
        *++text = MUL;
        *++text = SUB;
        expression_type = tmp;
      } else {
        // numeral subtraction
        *++text = SUB;
        expression_type = tmp;
      }
    } else if (token == Mul) {
      // multiply
      match(Mul);
      *++text = PUSH;
      expression(Inc);
      *++text = MUL;
      expression_type = tmp;
    } else if (token == Div) {
      // divide
      match(Div);
      *++text = PUSH;
      expression(Inc);
      *++text = DIV;
      expression_type = tmp;
    } else if (token == Mod) {
      // Modulo
      match(Mod);
      *++text = PUSH;
      expression(Inc);
      *++text = MOD;
      expression_type = tmp;
    } else if (token == Inc || token == Dec) {
      // postfix inc(++) and dec(--)
      // we will increase the value to the variable and decrease it
      // on `ax` to get its original value.
      if (*text == LI) {
        *text = PUSH;
        *++text = LI;
      } else if (*text == LC) {
        *text = PUSH;
        *++text = LC;
      } else {
        printf("bad value in increment in line %d\n", line_number);
        exit(-1);
      }

      *++text = PUSH;
      *++text = IMM;
      *++text = (expression_type > PTR) ? sizeof(int) : sizeof(char);
      *++text = (token == Inc) ? ADD : SUB;
      *++text = (expression_type == CHAR) ? SC : SI;
      *++text = PUSH;
      *++text = IMM;
      *++text = (expression_type > PTR) ? sizeof(int) : sizeof(char);
      *++text = (token == Inc) ? SUB : ADD;
      match(token);
    } else if (token == Brak) {
      // array access var[xx]
      match(Brak);
      *++text = PUSH;
      expression(Assign);
      match(']');

      if (tmp > PTR) {
        // pointer, `not char *`
        *++text = PUSH;
        *++text = IMM;
        *++text = sizeof(int);
        *++text = MUL;
      } else if (tmp < PTR) {
        printf("pointer type expected in line %d\n", line_number);
        exit(-1);
      }
      expression_type = tmp - PTR;
      *++text = ADD;
      *++text = (expression_type == CHAR) ? LC : LI;
    } else {
      printf("compiler error in line %d, token = %d\n", line_number, token);
      exit(-1);
    }
  }
}

void statement() {
  //just for branch control
  int *a, *b;


  if (token == If) {
    match(If);
    match('(');
    expression(Assign);
    match(')');

    *++text = JZ;
    b = ++text;
    statement();
    if (token == Else) {
      match(Else);
      //emit code for JMP B
      *b = (int) (text + 3);
      *++text = JMP;
      b = ++text;

      statement();
    }
    *b = (int) (text + 1);
  }


  else if (token == While) {
    match(While);
    a = text + 1;
    match('(');
    expression(Assign);
    match(')');

    *++text = JZ;
    b = ++text;
    statement();

    *++text = JMP;
    *++text = (int) a;
    *b = (int) (text + 1);
  } else if (token == Return) {
    match(Return);
    //return [expression]
    if (token != ';') {
      expression(Assign);
    }
    match(';');
    //emit code for return
    *++text = LEV;
  } else if (token == '{') {
    match('{');
    while (token != '}') {
      statement();
    }
    match('}');
  } else if (token == ';') {
    //just for ;;;;;;;
    match(';');
  } else {
    // a = b, or function()
    expression(Assign);
    match(';');
  }
}

void function_parameter() {
  int type;
  int params;
  params = 0;
  while (token != ')') {
    type = INT;
    if (token == Int) {
      match(Int);
    } else if (token == Char) {
      match(Char);
      type = CHAR;
    }
    while (token == Mul) {
      match(Mul);
      type = type + PTR;
    }
    if (token != Id) {
      printf("bad parameter declaration in line %d\n", line_number);
      exit(-1);
    }
    if (current_id[Class] == Loc) {
      printf("duplicate parameter declaration in line %d\n", line_number);
      exit(-1);
    }
    match(Id);
    //save information of global variable
    current_id[BClass] = current_id[Class];
    current_id[Class] = Loc;
    current_id[BType] = current_id[Type];
    current_id[Type] = type;
    // index of current parameter
    current_id[BValue] = current_id[Value];
    current_id[Value] = params++;
    if (token == ',') {
      match(',');
    }
  }
  //bp skip to function body
  index_of_bp = params + 1;
}

void function_body() {
  //our function body looks like
  //{
  // all local declarations
  // statements
  // }
  //local variables position on stack
  int pos_local;
  int type;
  pos_local = index_of_bp;

  while (token == Int || token == Char) {
    basetype = (token == Int) ? INT : CHAR;
    match(token);
    while (token != ';') {
      type = basetype;
      while (token == Mul) {
        match(Mul);
        type = type + PTR;
      }
      if (token != Id) {
        printf("bad local declaration in line %d\n", line_number);
        exit(-1);
      }
      if (current_id[Class] == Loc) {
        printf("duplicate local declaration in line %d\n", line_number);
        exit(-1);
      }
      match(Id);
      // store the local variable
      current_id[BClass] = current_id[Class];
      current_id[Class] = Loc;
      current_id[BType] = current_id[Type];
      current_id[Type] = type;
      current_id[BValue] = current_id[Value];
      // index of current parameter
      current_id[Value] = ++pos_local;
      if (token == ',') {
        match(',');
      }
    }
    match(';');
  }
  //generate assembly language
  //save the stack size
  *++text = ENT;
  *++text = pos_local - index_of_bp;

  while (token != '}') {
    statement();
  }
  //emit code for leaving the sub function
  *++text = LEV;
}

void function_declaration() {
  match('(');
  function_parameter();
  match(')');
  match('{');
  function_body();
  //    match('}');
  //function finished
  //replaced local variable by global variable
  current_id = symbols;
  while (current_id[Token]) {
    if (current_id[Class] == Loc) {
      current_id[Class] = current_id[BClass];
      current_id[Type] = current_id[BType];
      current_id[Value] = current_id[BValue];
    }
    current_id = current_id + IdSize;
  }
}

void enum_declaration() {
  int i;

  i = 0;
  while (token != '}') {
    if (token != Id) {
      printf("bad enum identifier in line %d\n", line_number);
      exit(-1);
    }
    next();
    if (token == Assign) {
      next();
      if (token != Num) {
        printf("bad enum initializer in line %d\n", line_number);
        exit(-1);
      }
      i = token_val;
      next();
    }

    current_id[Class] = Num;
    current_id[Type] = INT;
    current_id[Value] = i++;

    if (token == ',') {
      next();
    }
  }
}

void global_declaration() {
  int type, i;
  basetype = INT;
  //parse enum
  if (token == Enum) {
    match(Enum);
    if (token != '{') {
      match(Id);
    }
    if (token == '{') {
      match('{');
      enum_declaration();
      match('}');
    }
    match(';');
    return;
  }

  //Type id
  //such as int abc;char def;
  if (token == Int) {
    match(Int);
  } else if (token == Char) {
    match(Char);
    basetype = CHAR;
  }

  while (token != ';' && token != '}') {
    type = basetype;
    while (token == Mul) {
      match(Mul);
      //multilevel pointer
      type = type + PTR;
    }
    if (token != Id) {
      //invalid
      printf("bad global declaration in line %d\n", line_number);
      exit(-1);
    }
    if (current_id[Class]) {
      //exists
      printf("duplicate global declaration in line %d\n", line_number);
      exit(-1);
    }
    match(Id);
    current_id[Type] = type;
    if (token == '(') {
      //function
      current_id[Class] = Fun;
      current_id[Value] = (int) (text + 1);
      function_declaration();
    } else {
      //variable
      //Global
      current_id[Class] = Glo;
      current_id[Value] = (int) data;
      data = data + sizeof(int);
    }
    if (token == ',') {
      match(',');
    }
  }
  next();
}

void program() {
  // after this function, codes are emitted,
  // then call eval() to run
  //get next token
  next();

  while (token > 0) {
    global_declaration();
  }
}

int eval() {
  int op, *tmp;
  while (1) {
    // get next operation code
    op = *pc++;

    //set ax as current value
    if (op == IMM) { ax = *pc++; }
      //set ax as char, address in ax
    else if (op == LC) { ax = *(char *) ax; }
      //set ax as int, address in ax
    else if (op == LI) { ax = *(int *) ax; }
      //save char to sp, value in ax, address on stack
      //CHANGED
    else if (op == SC) { *(char *) *sp++ = (char) ax; }
      //save int to sp, value in ax, address on stack
    else if (op == SI) { *(int *) *sp++ = ax; }
      //set value of sp as ax
    else if (op == PUSH) { *--sp = ax; }
      //jump to address
    else if (op == JMP) { pc = (int *) *pc; }
      //jump zero/not zero
    else if (op == JZ) { pc = ax ? pc + 1 : (int *) *pc; } else if (op == JNZ) { pc = ax ? (int *) *pc : pc + 1; }
      //call
    else if (op == CALL) {
      //save next line after subroutine called
      *--sp = (int) (pc + 1);
      //jump
      pc = (int *) *pc;
    }
      //make new call frame
    else if (op == ENT) {
      *--sp = (int) bp;
      bp = sp;
      sp = sp - *pc++;
    }
      //add sp, size
    else if (op == ADJ) { sp = sp + *pc++; }
      //restore old call frame and pc
    else if (op == LEV) {
      sp = bp;
      bp = (int *) *sp++;
      //return from subroutine
      pc = (int *) *sp++;
    }
      //load arguments
    else if (op == LEA) { ax = (int) (bp + *pc++); }
    else if (op == OR) { ax = *sp++ | ax; }
    else if (op == XOR) { ax = *sp++ ^ ax; }
    else if (op == AND) { ax = *sp++ & ax; }
    else if (op == EQ) { ax = *sp++ == ax; }
    else if (op == NE) { ax = *sp++ != ax; }
    else if (op == LT) { ax = *sp++ < ax; }
    else if (op == GT) { ax = *sp++ > ax; }
    else if (op == LE) { ax = *sp++ <= ax; }
    else if (op == GE) { ax = *sp++ >= ax; }
    else if (op == SHL) { ax = *sp++ << ax; }
    else if (op == SHR) { ax = *sp++ >> ax; }
    else if (op == ADD) { ax = *sp++ + ax; }
    else if (op == SUB) { ax = *sp++ - ax; }
    else if (op == MUL) { ax = *sp++ * ax; }
    else if (op == DIV) { ax = *sp++ / ax; }
    else if (op == MOD) {
      ax = *sp++ % ax;

    }
    else if (op == OPEN) { ax = open((char *) sp[1], *sp); }
    else if (op == READ) { ax = read(sp[2], (char *) sp[1], *sp); }
    else if (op == CLOS) { ax = close(*sp); }
    else if (op == PRINTF) {
      tmp = sp + pc[1];
      ax = printf((char *) tmp[-1], tmp[-2], tmp[-3], tmp[-4], tmp[-5], tmp[-6]);
    }
    else if (op == MALLOC) { ax = (int) malloc(*sp); }
    else if (op == MEMSET) { ax = (int) memset((char *) sp[2], sp[1], *sp); }
    else if (op == MEMCMP) { ax = memcmp((char *) sp[2], (char *) sp[1], *sp); }
    else if (op == EXIT) {
      printf("exit(%d) cycle = %d\n", *sp, cycle);

      return *sp;
    } else {
      printf("%d :unknown instruction = %d! cycle = %d\n", line_number, op, cycle);
      return -1;
    }
    cycle++;
  }
}
//char keyword[1000] = "char while open exit void main";

int initKeyword() {
  int i;

  src = "char else enum if int return sizeof while"
    " open read close printf malloc memset memcmp exit void main";

  // add keywords to symbol table
  i = Char;
  while (i <= While) {
    next();
    current_id[Token] = i++;
  }

  // add library to symbol table
  // add functions like printf
  i = OPEN;
  while (i <= EXIT) {
    next();
    current_id[Class] = Sys;
    current_id[Type] = INT;
    current_id[Value] = i++;
  }

  next();
  current_id[Token] = Char; // handle void type, consider as char
  next();
  idmain = current_id; // keep track of main

  return 0;
}

int init() {
  //allocate memory for our virtual machine
  // position of emitted code
  if (!(text = previous_text = malloc(poolsize))) {
    printf("couldn't malloc for text\n");
    return -1;
  }
  // data seg
  if (!(data = malloc(poolsize))) {
    printf("couldn't malloc for data\n");
    return -1;
  }
  // stack
  if (!(stack = malloc(poolsize))) {
    printf("couldn't malloc for stack\n");
    return -1;
  }
  // symbols table
  if (!(symbols = malloc(poolsize))) {
    printf("could not malloc for symbol table\n");
    return -1;
  }
  memset(text, 0, poolsize);
  memset(data, 0, poolsize);
  memset(stack, 0, poolsize);
  memset(symbols, 0, poolsize);

  // init base pointer and stack pointer
  bp = sp = (int *) ((int) stack + poolsize);
  ax = 0;
  // lexer add key word
  initKeyword();
  return 1;
}

int main(int argc, char **argv) {
  int i, fd;
  int *tmp;
  cycle = 0;

  //avoid influence by useless data
  argc--;
  argv++;

  //random by me, memory alloc size
  poolsize = 1024 * 1024 * 5;
  line_number = 1;

  if (!init()) {
    return -1;
  }
  if ((fd = open(*argv, 0)) < 0) {
    printf("couldn't open (%s)\n", *argv);
    return -1;
  }
  if (!(src = previous_src = malloc(poolsize))) {
    printf("couldn't malloc (%d)\n", poolsize);
    return -1;
  }
  if ((i = read(fd, src, poolsize - 1)) <= 0) {
    printf("read() returned %d\n", i);
    return -1;
  }

  //add EOF
  src[i] = 0;
  close(fd);
  //    vmTest();
  program();

  if (!(pc = (int *) idmain[Value])) {
    printf("main() not defined\n");
    return -1;
  }
  // setup stack
  sp = (int *) ((int) stack + poolsize);
  *--sp = EXIT; // call exit if main returns
  *--sp = PUSH;
  tmp = sp;
  *--sp = argc;
  *--sp = (int) argv;
  *--sp = (int) tmp;

  return eval();
}
