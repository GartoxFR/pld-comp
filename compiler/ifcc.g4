grammar ifcc;

axiom : function* EOF ;

function : type IDENT '(' (functionArg (',' functionArg)*)? ')' block;

functionArg : type IDENT ;

stmt : expr ';' | declare_stmt | block | if | while | return_stmt | break | continue ;

if: IF '(' expr ')' then=stmt ( ELSE else=stmt )? ;

while: WHILE '(' expr ')' stmt ;

block : '{' stmt* '}' ;

break : BREAK ';' ;
continue : CONTINUE ';' ;

declare_stmt : type idents+=initializer (',' idents+=initializer)* ';' ;

initializer : IDENT ('=' expr | ) ;

expr: IDENT '(' (expr (',' expr)*)? ')' # call
    | postLvalue INCRDECR_OP # postIncrDecrOp
    | SUM_OP expr # unarySumOp
    | UNARY_OP expr # unaryOp
    | BIT_AND lvalue # addressOf
    | INCRDECR_OP lvalue # preIncrDecrOp
    | expr (STAR | PRODUCT_OP) expr # productOp
    | expr SUM_OP expr # sumOp
    | expr CMP_OP expr # cmpOp
    | expr EQ_OP expr # eqOp
    | expr BIT_AND expr # bitAnd
    | expr BIT_XOR expr # bitXor
    | expr BIT_OR expr # bitOr
    | expr LOGICAL_AND expr # logicalAnd
    | expr LOGICAL_OR expr # logicalOr
    | lvalue ('=' | ASSIGN_OP) expr # assign
    | CONST # const
    | CHAR  # charLiteral
    | STRING  # stringLiteral
    | lvalue # lvalueExpr
    | '(' expr ')' # par
    ;

lvalue : postLvalue | preLvalue ;
postLvalue: IDENT # lvalueVar
          | IDENT '[' expr ']' # lvalueIndex
          | '(' lvalue ')' # lvaluePar
          ;

preLvalue: STAR expr # lvalueDeref
      ;

return_stmt: RETURN expr? ';' ;

type : type STAR # pointerType
     | FLAT_TYPE # simpleType
     ;

RETURN : 'return' ;
FLAT_TYPE : 'int' | 'void' | 'char' | 'short' | 'long' | 'bool';
IF : 'if' ;
WHILE : 'while' ;
ELSE : 'else' ;
BREAK : 'break' ;
CONTINUE : 'continue' ;
STAR : '*' ;

ASSIGN_OP : '+=' | '-=' | '*=' | '/=' | '%=' | '&=' | '^=' | '|=' ;
SUM_OP : '+' | '-' ;
INCRDECR_OP : '++' | '--' ;
UNARY_OP : '!' ;
PRODUCT_OP : '/' | '%';
CMP_OP : '>' | '<' | '>=' | '<=' ;
EQ_OP : '==' | '!=' ;
BIT_AND : [&] ;
BIT_XOR : [^] ;
BIT_OR : [|] ;
LOGICAL_AND : '&&' ;
LOGICAL_OR : '||' ;
CHAR : '\'' (~[\\'] | ('\\' + ('n' | 't' | 'r' | '0' | '\'' | '\\'))) '\'' ;
STRING : '"' ((~[\\"] | ('\\' + ('n' | 't' | 'r' | '0' | '"' | '\\')))*?) '"' ;
CONST : [0-9]+ ;
IDENT : [a-zA-Z][a-zA-Z0-9]* ;
COMMENT : '/*' .*? '*/' -> skip ;
DIRECTIVE : '#' .*? '\n' -> skip ;
WS    : [ \t\r\n] -> channel(HIDDEN);
