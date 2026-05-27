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
> ./build/pamelo --help
USAGE: pamelo [OPTIONS] <file>

ABOUT:
    A Programming Language

OPTIONS:
    --help, -h       This screen
    --print-token    Print the token to the screen
    --print-ast      Print the ast to the screen
    --print-scope    Print the scope info to the screen

HINTS:
    1. If you want to read from stdin, then make filepath == '-'
       echo hello | orange -; this should read hello from the stdin

```

### Examples

#### 1. Printing the tokens

This will print the tokens in the source code

```
echo "fn f() void { var a = 1; var b = 13; var c u8 = b + a as u8 - 1; }" | ./build/pamelo --print-token -              
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
UNKNOWN(var)
ID(a)
UNKNOWN(=)
INT_LITERAL(1)
SEMICOLON(;)
UNKNOWN(var)
ID(b)
UNKNOWN(=)
INT_LITERAL(13)
SEMICOLON(;)
UNKNOWN(var)
ID(c)
ID(u8)
UNKNOWN(=)
ID(b)
PLUS(+)
ID(a)
UNKNOWN(as)
ID(u8)
MINUS(-)
INT_LITERAL(1)
SEMICOLON(;)
RBRACE(})
EOF()

```

#### 2. Printing the ast

This will print the ast

```
echo "fn f() void { var a = 1; var b = 13; var c u8 = b + a as u8 - 1; }" | ./build/pamelo --print-ast -  
```

Output:

```
file: -
+- AST_PROG
   +- AST_FN_DECL(type: ID | lexical: f)
      +- RETURN TYPE: AST_TYPE_SPECIFIER(type: ID | lexical: void) [void]
      +- FUNCTION BODY: AST_BLOCK_STMT
         +- AST_VAR_STMT(type: ID | lexical: a) [u32]
         |  +- AST_LITERAL_EXPR(type: INT_LITERAL | lexical: 1) [u32]
         +- AST_VAR_STMT(type: ID | lexical: b) [u32]
         |  +- AST_LITERAL_EXPR(type: INT_LITERAL | lexical: 13) [u32]
         +- AST_VAR_STMT(type: ID | lexical: c) [u8]
            +- AST_TYPE_SPECIFIER(type: ID | lexical: u8) [u8]
            +- AST_ADD_EXPR(type: MINUS | lexical: -) [u32]
               +- AST_ADD_EXPR(type: PLUS | lexical: +) [u32]
               |  +- AST_VAR_EXPR(type: ID | lexical: b) [u32]
               |  +- AST_CAST_EXPR [u8]
               |     +- CASTING EXPR: AST_VAR_EXPR(type: ID | lexical: a) [u32]
               |     +- CASTING TYPE: AST_TYPE_SPECIFIER(type: ID | lexical: u8) [u8]
               +- AST_LITERAL_EXPR(type: INT_LITERAL | lexical: 1) [u32]

```

This also has information of the types in square bracket

#### 3. Print the scope informations

This prints all the scope information like types per scope, symbols, etc.

```
echo "fn f() void { var a = 1; var b = 13; var c u8 = b + a as u8 - 1; }" | ./build/pamelo --print-scope -
```

Output:

```
id: 0x10159ae40
parent-id: 0x0
types:
    1. void
    2. u8
    3. u16
    4. u32
    5. u64
symbols:

id: 0x10159aed0
parent-id: 0x10159ae40
types:
    1. fn f () -> void
symbols:

id: 0x10159af00
parent-id: 0x10159aed0
types:
symbols:

id: 0x10159af60
parent-id: 0x10159af00
types:
symbols:
    1. name: a | type: u32
    2. name: b | type: u32
    3. name: c | type: u8

```

