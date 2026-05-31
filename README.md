# pamelo

A programming language created to understand compiler, where correctness and
understanding concepts is more important.

## building

Run the following to build the project

```bash
make
```

This should create a binary in `./build` folder

## usage

Below is the usage screen

```
USAGE: pamelo [OPTIONS] <file>

ABOUT:
    A Programming Language

OPTIONS:
    --help, -h       This screen
    --print-token    Print the token to the screen
    --print-ast      Print the ast to the screen
    --print-scope    Print the scope info to the screen
    --print-ir       Print the ir to the screen

HINTS:
    1. If you want to read from stdin, then make filepath == '-'
       echo hello | orange -; this should read hello from the stdin

```

### Examples

#### 1. Printing the tokens

This will print the tokens in the source code

```
echo "fn f() void { var a = 1; var b = 13; var c u8 = b + a as u8 - 1; } fn main() void { f(); }" | ./build/pamelo --print-token - 
```

Output:

```
file: -
FN_KEYWORD(fn)
ID(f)
LPAREN(()
RPAREN())
ID(void)
LBRACE({)
VAR_KEYWORD(var)
ID(a)
EQUAL(=)
INT_LITERAL(1)
SEMICOLON(;)
VAR_KEYWORD(var)
ID(b)
EQUAL(=)
INT_LITERAL(13)
SEMICOLON(;)
VAR_KEYWORD(var)
ID(c)
ID(u8)
EQUAL(=)
ID(b)
PLUS(+)
ID(a)
AS_KEYWORD(as)
ID(u8)
MINUS(-)
INT_LITERAL(1)
SEMICOLON(;)
RBRACE(})
FN_KEYWORD(fn)
ID(main)
LPAREN(()
RPAREN())
ID(void)
LBRACE({)
ID(f)
LPAREN(()
RPAREN())
SEMICOLON(;)
RBRACE(})
EOF()

```

#### 2. Printing the ast

This will print the ast

```
echo "fn f() void { var a = 1; var b = 13; var c u8 = b + a as u8 - 1; } fn main() void { f(); }" | ./build/pamelo --print-ast - 
```

Output:

```
file: -
+- AST_PROG
   +- AST_FN_DECL(type: ID | lexical: f)
   |  +- RETURN TYPE: AST_TYPE_SPECIFIER(type: ID | lexical: void) [void]
   |  +- FUNCTION BODY: AST_BLOCK_STMT
   |     +- AST_VAR_STMT(type: ID | lexical: a) [u32]
   |     |  +- AST_LITERAL_EXPR(type: INT_LITERAL | lexical: 1) [u32]
   |     +- AST_VAR_STMT(type: ID | lexical: b) [u32]
   |     |  +- AST_LITERAL_EXPR(type: INT_LITERAL | lexical: 13) [u32]
   |     +- AST_VAR_STMT(type: ID | lexical: c) [u8]
   |        +- AST_TYPE_SPECIFIER(type: ID | lexical: u8) [u8]
   |        +- AST_ADD_EXPR(type: MINUS | lexical: -) [u32]
   |           +- AST_ADD_EXPR(type: PLUS | lexical: +) [u32]
   |           |  +- AST_VAR_EXPR(type: ID | lexical: b) [u32]
   |           |  +- AST_CAST_EXPR [u8]
   |           |     +- CASTING EXPR: AST_VAR_EXPR(type: ID | lexical: a) [u32]
   |           |     +- CASTING TYPE: AST_TYPE_SPECIFIER(type: ID | lexical: u8) [u8]
   |           +- AST_LITERAL_EXPR(type: INT_LITERAL | lexical: 1) [u32]
   +- AST_FN_DECL(type: ID | lexical: main)
      +- RETURN TYPE: AST_TYPE_SPECIFIER(type: ID | lexical: void) [void]
      +- FUNCTION BODY: AST_BLOCK_STMT
         +- AST_EXPR_STMT
            +- AST_CALL_EXPR [void]
               +- AST_VAR_EXPR(type: ID | lexical: f) [fn f () -> void]
```

This also has information of the types in square bracket

#### 3. Print the scope informations

This prints all the scope information like types per scope, symbols, etc.

```
echo "fn f() void { var a = 1; var b = 13; var c u8 = b + a as u8 - 1; } fn main() void { f(); }" | ./build/pamelo --print-scope - 
```

Output:

```
id: 0x102fbedc0
parent-id: 0x0
types:
    1. void
    2. u8
    3. u16
    4. u32
    5. u64
symbols:

id: 0x102fbee20
parent-id: 0x102fbedc0
types:
    1. fn f () -> void
    2. fn main () -> void
symbols:
    1. name: f | type: fn f () -> void (0)
    2. name: main | type: fn main () -> void (1)

id: 0x102fbee50
parent-id: 0x102fbee20
types:
symbols:

id: 0x102fbef10
parent-id: 0x102fbee20
types:
symbols:

id: 0x102fbefc0
parent-id: 0x102fbee50
types:
symbols:
    1. name: a | type: u32 (0)
    2. name: b | type: u32 (1)
    3. name: c | type: u8 (2)

id: 0x102fbf100
parent-id: 0x102fbef10
types:
symbols:

```

#### 4. Print the ir information

This prints the converted IR representation of the source code

```
echo "fn f() void { var a = 1; var b = 13; var c u8 = b + a as u8 - 1; } fn main() void { f(); }" | ./build/pamelo --print-ir - 
```

Output:

```
$0: # f
@function_start
    %0 := ALLOCATE 4
    %4 := CONST 1
    STORE %0 4 := %4
    %1 := ALLOCATE 4
    %5 := CONST 13
    STORE %1 4 := %5
    %2 := ALLOCATE 1
    %6 := LOAD %1 4
    %7 := LOAD %0 4
    %9 := CONST 0
    %8 := ADD %7 %9 1
    %10 := ADD %6 %8 4
    %11 := CONST 1
    %12 := SUB %10 %11 4
    STORE %2 1 := %12
    DEALLOCATE 9
    RETURN
@function_end

$1: # main
@function_start
    BEGIN_CALL
    CALL $0
    DEALLOCATE 0
    END_CALL
    RETURN
@function_end

```

#### 5. Run source code

This prints the last set temp operation value

```
echo "fn f() void { var a = 1; var b = 13; var c u8 = b + a as u8 - 1; } fn main() void { f(); }" | ./build/pamelo -         
```

Output:

```
VALUE: 13
```
