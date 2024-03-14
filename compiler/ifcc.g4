grammar ifcc;

axiom : prog EOF ;

prog : 'int' 'main' '(' ')' '{' stmt* return_stmt '}' ;


stmt : expr ';' | declare_stmt | block;

block : '{' stmt* '}' ;

declare_stmt : INT idents+=initializer (',' idents+=initializer)* ';' ;

initializer : IDENT ('=' expr | ) ;

expr: SUM_OP expr # unarySumOp
    | expr PRODUCT_OP expr # productOp
    | expr SUM_OP expr # sumOp
    | IDENT '=' expr # assign
    | CONST # const
    | IDENT # var 
    | '(' expr ')' # par
    ;

return_stmt: RETURN expr ';' ;


RETURN : 'return' ;
INT : 'int' ;

SUM_OP : '+' | '-' ;
PRODUCT_OP : '*' ;
CONST : [0-9]+ ;
IDENT : [a-zA-Z][a-zA-Z0-9]* ;
COMMENT : '/*' .*? '*/' -> skip ;
DIRECTIVE : '#' .*? '\n' -> skip ;
WS    : [ \t\r\n] -> channel(HIDDEN);
