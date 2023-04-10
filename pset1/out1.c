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
Reading a token: Next token is token VAR ()
Shifting token VAR ()
Entering state 39
Reading a token: Next token is token ')' ()
Shifting token ')' ()
Entering state 56
Reducing stack by rule 6 (line 34):
   $1 = token TYPE ()
   $2 = token VAR ()
   $3 = token '(' ()
   $4 = token TYPE ()
   $5 = token VAR ()
   $6 = token ')' ()
-> $$ = nterm function_definition ()
Stack now 0
Entering state 5
Reading a token: Next token is token '{' ()
Shifting token '{' ()
Entering state 9
Reducing stack by rule 14 (line 54):
   $1 = token '{' ()
-> $$ = nterm block_start ()
Stack now 0 5
Entering state 11
Reading a token: Next token is token TYPE ()
Shifting token TYPE ()
Entering state 14
Reading a token: Next token is token VAR ()
Shifting token VAR ()
Entering state 30
Reading a token: Next token is token ';' ()
Shifting token ';' ()
Entering state 51
Reducing stack by rule 24 (line 72):
   $1 = token TYPE ()
   $2 = token VAR ()
   $3 = token ';' ()
-> $$ = nterm variable_declaration ()
Stack now 0 5 11
Entering state 23
Reducing stack by rule 23 (line 69):
   $1 = nterm variable_declaration ()
-> $$ = nterm statement ()
Stack now 0 5 11
Entering state 22
Reducing stack by rule 17 (line 61):
   $1 = nterm statement ()
-> $$ = nterm statements ()
Stack now 0 5 11
Entering state 21
Reading a token: Next token is token TYPE ()
Shifting token TYPE ()
Entering state 14
Reading a token: Next token is token VAR ()
Shifting token VAR ()
Entering state 30
Reading a token: Next token is token ';' ()
Shifting token ';' ()
Entering state 51
Reducing stack by rule 24 (line 72):
   $1 = token TYPE ()
   $2 = token VAR ()
   $3 = token ';' ()
-> $$ = nterm variable_declaration ()
Stack now 0 5 11 21
Entering state 23
Reducing stack by rule 23 (line 69):
   $1 = nterm variable_declaration ()
-> $$ = nterm statement ()
Stack now 0 5 11 21
Entering state 37
Reducing stack by rule 16 (line 60):
   $1 = nterm statements ()
   $2 = nterm statement ()
-> $$ = nterm statements ()
Stack now 0 5 11
Entering state 21
Reading a token: Next token is token IF ()
Shifting token IF ()
Entering state 15
Reading a token: Next token is token '(' ()
Shifting token '(' ()
Entering state 31
Reading a token: Next token is token VAR ()
Shifting token VAR ()
Entering state 42
Reducing stack by rule 41 (line 100):
   $1 = token VAR ()
-> $$ = nterm operand ()
Stack now 0 5 11 21 15 31
Entering state 45
Reducing stack by rule 38 (line 95):
   $1 = nterm operand ()
-> $$ = nterm parameter ()
Stack now 0 5 11 21 15 31
Entering state 53
Reading a token: Next token is token LT ()
Shifting token LT ()
Entering state 67
Reading a token: Next token is token VAR ()
Shifting token VAR ()
Entering state 42
Reducing stack by rule 41 (line 100):
   $1 = token VAR ()
-> $$ = nterm operand ()
Stack now 0 5 11 21 15 31 53 67
Entering state 45
Reducing stack by rule 38 (line 95):
   $1 = nterm operand ()
-> $$ = nterm parameter ()
Stack now 0 5 11 21 15 31 53 67
Entering state 78
Reducing stack by rule 34 (line 90):
   $1 = nterm parameter ()
   $2 = token LT ()
   $3 = nterm parameter ()
-> $$ = nterm condition ()
Stack now 0 5 11 21 15 31
Entering state 52
Reading a token: Next token is token ')' ()
Shifting token ')' ()
Entering state 66
Reading a token: Next token is token '{' ()
Shifting token '{' ()
Entering state 9
Reducing stack by rule 14 (line 54):
   $1 = token '{' ()
-> $$ = nterm block_start ()
Stack now 0 5 11 21 15 31 52 66
Entering state 11
Reading a token: Next token is token WHILE ()
Shifting token WHILE ()
Entering state 16
Reading a token: Next token is token '(' ()
Shifting token '(' ()
Entering state 32
Reading a token: Next token is token VAR ()
Shifting token VAR ()
Entering state 42
Reducing stack by rule 41 (line 100):
   $1 = token VAR ()
-> $$ = nterm operand ()
Stack now 0 5 11 21 15 31 52 66 11 16 32
Entering state 45
Reducing stack by rule 38 (line 95):
   $1 = nterm operand ()
-> $$ = nterm parameter ()
Stack now 0 5 11 21 15 31 52 66 11 16 32
Entering state 53
Reading a token: Next token is token LT ()
Shifting token LT ()
Entering state 67
Reading a token: Next token is token VAR ()
Shifting token VAR ()
Entering state 42
Reducing stack by rule 41 (line 100):
   $1 = token VAR ()
-> $$ = nterm operand ()
Stack now 0 5 11 21 15 31 52 66 11 16 32 53 67
Entering state 45
Reducing stack by rule 38 (line 95):
   $1 = nterm operand ()
-> $$ = nterm parameter ()
Stack now 0 5 11 21 15 31 52 66 11 16 32 53 67
Entering state 78
Reducing stack by rule 34 (line 90):
   $1 = nterm parameter ()
   $2 = token LT ()
   $3 = nterm parameter ()
-> $$ = nterm condition ()
Stack now 0 5 11 21 15 31 52 66 11 16 32
Entering state 54
Reading a token: Next token is token ')' ()
Shifting token ')' ()
Entering state 72
Reading a token: Next token is token '{' ()
Shifting token '{' ()
Entering state 9
Reducing stack by rule 14 (line 54):
   $1 = token '{' ()
-> $$ = nterm block_start ()
Stack now 0 5 11 21 15 31 52 66 11 16 32 54 72
Entering state 11
Reading a token: Next token is token VAR ()
Shifting token VAR ()
Entering state 13
Reading a token: Next token is token '=' ()
Shifting token '=' ()
Entering state 29
Reading a token: Next token is token '-' ()
Shifting token '-' ()
Entering state 43
Reading a token: Next token is token '(' ()
Error: popping token '-' ()
Stack now 0 5 11 21 15 31 52 66 11 16 32 54 72 11 13 29
Error: popping token '=' ()
Stack now 0 5 11 21 15 31 52 66 11 16 32 54 72 11 13
Error: popping token VAR ()
Stack now 0 5 11 21 15 31 52 66 11 16 32 54 72 11
Error: popping nterm block_start ()
Stack now 0 5 11 21 15 31 52 66 11 16 32 54 72
Error: popping token ')' ()
Stack now 0 5 11 21 15 31 52 66 11 16 32 54
Error: popping nterm condition ()
Stack now 0 5 11 21 15 31 52 66 11 16 32
Error: popping token '(' ()
Stack now 0 5 11 21 15 31 52 66 11 16
Error: popping token WHILE ()
Stack now 0 5 11 21 15 31 52 66 11
Error: popping nterm block_start ()
Stack now 0 5 11 21 15 31 52 66
Error: popping token ')' ()
Stack now 0 5 11 21 15 31 52
Error: popping nterm condition ()
Stack now 0 5 11 21 15 31
Error: popping token '(' ()
Stack now 0 5 11 21 15
Error: popping token IF ()
Stack now 0 5 11 21
Error: popping nterm statements ()
Stack now 0 5 11
Error: popping nterm block_start ()
Stack now 0 5
Error: popping nterm function_definition ()
Stack now 0
Cleanup: discarding lookahead token '(' ()
Stack now 0
int func(int i){
	int a;
	int b;
	
  if (a < i){
		while (b < i){
			b = -([Syntax error 1]