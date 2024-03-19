grammar ifcc;

axiom : function* EOF ;

function : INT IDENT '(' (functionArg (',' functionArg)*)? ')' block;

functionArg : INT IDENT ;

stmt : expr ';' | declare_stmt | block | if | while | return_stmt | break | continue ;

if: IF '(' expr ')' then=stmt ( ELSE else=stmt )? ;

while: WHILE '(' expr ')' stmt ;

block : '{' stmt* '}' ;

break : BREAK ';' ;
continue : CONTINUE ';' ;

declare_stmt : INT idents+=initializer (',' idents+=initializer)* ';' ;

initializer : IDENT ('=' expr | ) ;

expr: IDENT '(' (expr (',' expr)*)? ')' # call
    | SUM_OP expr # unarySumOp
    | UNARY_OP expr # unaryOp
    | expr PRODUCT_OP expr # productOp
    | expr SUM_OP expr # sumOp
    | expr CMP_OP expr # cmpOp
    | expr EQ_OP expr # eqOp
    | expr LOGICAL_AND expr # logicalAnd
    | expr LOGICAL_OR expr # logicalOr
    | IDENT '=' expr # assign
    | CONST # const
    | CHAR  # charLiteral
    | IDENT # var 
    | '(' expr ')' # par
    ;

return_stmt: RETURN expr? ';' ;


RETURN : 'return' ;
INT : 'int' ;
IF : 'if' ;
WHILE : 'while' ;
ELSE : 'else' ;
BREAK : 'break' ;
CONTINUE : 'continue' ;

SUM_OP : '+' | '-' ;
UNARY_OP : '!' ;
PRODUCT_OP : '*' | '/' | '%';
CMP_OP : '>' | '<' | '>=' | '<=' ;
EQ_OP : '==' | '!=' ;
LOGICAL_AND : '&&' ;
LOGICAL_OR : '||' ;
CHAR : '\'' + (~[\\'] | ('\\' + ('n' | 't' | 'r' | '0' | '\'' | '\\'))) + '\'' ;
CONST : [0-9]+ ;
IDENT : [a-zA-Z][a-zA-Z0-9]* ;
COMMENT : '/*' .*? '*/' -> skip ;
DIRECTIVE : '#' .*? '\n' -> skip ;
WS    : [ \t\r\n] -> channel(HIDDEN);
