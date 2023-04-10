Starting parse
Entering state 0
Reading a token: Next token is token TYPE ()
Shifting token TYPE ()
Entering state 1
Reading a token: Next token is token VAR ()
Shifting token VAR ()
Entering state 6
Reading a token: Next token is token '(' ()
Shifting token '(' ()
Entering state 12
Reading a token: Next token is token TYPE ()
Shifting token TYPE ()
Entering state 26
Reading a token: Next token is token ')' ()
Shifting token ')' ()
Entering state 40
Reducing stack by rule 5 (line 33):
   $1 = token TYPE ()
   $2 = token VAR ()
   $3 = token '(' ()
   $4 = token TYPE ()
   $5 = token ')' ()
-> $$ = nterm function_definition ()
Stack now 0
Entering state 5
Reading a token: Next token is token ';' ()
Error: popping nterm function_definition ()
Stack now 0
Cleanup: discarding lookahead token ';' ()
Stack now 0
extern void print(int);[Syntax error 1]