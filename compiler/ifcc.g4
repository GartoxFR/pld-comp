grammar ifcc;

axiom : function* EOF ;

function : INT IDENT '(' (functionArg (',' functionArg)*)? ')' block;

functionArg : INT IDENT ;

stmt : expr ';' | declare_stmt | block | if | while | return_stmt;

if: IF '(' expr ')' then=stmt ( ELSE else=stmt )? ;

while: WHILE '(' expr ')' stmt ;

block : '{' stmt* '}' ;

declare_stmt : INT idents+=initializer (',' idents+=initializer)* ';' ;

initializer : IDENT ('=' expr | ) ;

expr: SUM_OP expr # unarySumOp
    | UNARY_OP expr # unaryOp
    | expr PRODUCT_OP expr # productOp
    | expr SUM_OP expr # sumOp
    | expr CMP_OP expr # cmpOp
    | expr EQ_OP expr # eqOp
    | IDENT '=' expr # assign
    | CONST # const
    | IDENT # var 
    | '(' expr ')' # par
    ;

return_stmt: RETURN expr ';' ;


RETURN : 'return' ;
INT : 'int' ;
IF : 'if' ;
WHILE : 'while' ;
ELSE : 'else' ;

SUM_OP : '+' | '-' ;
UNARY_OP : '!' ;
PRODUCT_OP : '*' ;
CMP_OP : '>' | '<' | '>=' | '<=' ;
EQ_OP : '==' | '!=' ;
CONST : [0-9]+ ;
IDENT : [a-zA-Z][a-zA-Z0-9]* ;
COMMENT : '/*' .*? '*/' -> skip ;
DIRECTIVE : '#' .*? '\n' -> skip ;
WS    : [ \t\r\n] -> channel(HIDDEN);
