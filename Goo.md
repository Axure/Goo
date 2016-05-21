
##寄存器
```C
int *pc, *sp, *bp, a, cycle; 
```
- pc 程序计数器/指令指针指令指针,当前指令地址
- sp stack pointer, 堆栈寄存器,指向栈顶,栈由高地质向低地址生长
- bp base pointer, 基址寄存器
- ax accumulator, 累加器
- cycle 执行指令计数

#指令集

1. LEA 去局部变量地址,以[PC+1]地址,以bp为基址,**地址**载入累加器ax
2. IMM [PC+1]作为为立即数载入累加器ax
3. JMP 跳转到[PC+1]
4. CALL PC+2入栈作为返回地址,跳转到[PC+1]
5. JZ  累加器为零时分支,当累加器ax为0时跳转到[PC+1],不为零时跳转到PC+2继续执行
6. JNZ 累加器不为零时分支,当累加器ax不为0时跳转到[PC+1],为零时跳转到PC+2
7. ENT 进入子程序,将bp压栈,基址bp指向栈顶,然后将栈顶生长[PC+1]字,作为参数传递空间
8. ADJ 
9. LEV 离开子程序,堆栈指针sp = bp,从堆栈中弹出基址bp,pc
10. LI 以ax为地址取int数
11. LC 以ax为地址取char
12. SI 以栈顶为地址存int数并弹栈[[sp++]]=ax
13. SC 以栈顶为地址存char并弹栈[[sp++]]=ax
14. PUSH 将ax压栈
15. OR ax = [sp++] | ax
16. XOR ax = [sp++] ^ ax
17. AND ax = [sp++] & ax
18. EQ  ax = [sp++] == ax
19. NE  ax = [sp++] != ax
20. LT  ax = [sp++]  ax
21. GT  ax = [sp++] > ax
22. LE  ax = [sp++] <= ax
23. GE  ax = [sp++] >= ax
24. SHL ax = [sp++] << ax
25. SHR ax = [sp++] >> ax
26. ADD ax = [sp++] + ax
27. SUB ax = [sp++] - ax
28. MUL ax = [sp++] * ax
29. DIV ax = [sp++] / ax
30. MOD ax = [sp++] % ax
31. OPEN 调用C库函数open,堆栈传递2个参数(第一个参数先进栈,返回值存入累加器a,下同)
32. READ 调用C库函数read,堆栈传递2个参数
33. CLOS 调用C库函数close,堆栈传递2个参数
34. PRINTF 调用C库函数printf,[pc+1]表明参数个数,传递至多六个参数
35. MALLOC 调用C胡函数malloc,堆栈传递一个参数
36. MEMSET 调用C库函数memset,堆栈传递3个参数
37. MEMCMP 调用C库函数memcmp,堆栈传递3个参数
38. EXIT 打印虚拟机执行情况,返回[sp]


ENBF
```
[] optional
{} zero or one or more
program ::= {global_declaration}+

global_declaration ::= enum_decl | variable_decl | function_decl

enum_decl ::= 'enum' [id] '{' 
                                id ['=' 'num']      
                                {',' id ['=' 'num']} 
                                                        '}'

variable_decl ::= type {'*'} id { ',' {'*'} id } ';'

function_decl ::= type {'*'} id '(' parameter_decl ')' '{' body_decl '}'

parameter_decl ::= type {'*'} id {',' type {'*'} id}

body_decl ::= {variable_decl}, {statement}

statement ::= non_empty_statement | empty_statement

non_empty_statement ::= if_statement | while_statement | '{' statement '}'
                     | 'return' expression | expression ';'

if_statement ::= 'if' '(' expression ')' statement ['else' non_empty_statement]

while_statement ::= 'while' '(' expression ')' non_empty_statement

EXPRESSIONS:
if (...) <statement> [else <statement>]
while (...) <statement>
{ <statement> }
return xxx;
<empty statement>;
expression; (expression end with semicolon)

IF:
if (...) <statement> [else <statement>]

  if (<cond>)                   <cond>
                                JZ a
    <true_statement>   ===>     <true_statement>
  else:                         JMP b
a:                           a:
    <false_statement>           <false_statement>
b:                           b:

WHILE:
a:                     a:
   while (<cond>)        <cond>
                         JZ b
    <statement>          <statement>
                         JMP a
b:                     b:
```
