%token NUM VAR TYPE IF ELSE WHILE LT GT LTE GTE EQ

%{
#include <stdio.h>
void yyerror(const char *);
extern int yylex();
extern int yylex_destroy();
extern FILE *yyin;
extern int yylineno;
extern char* yytext;
extern int yydebug;
yydebug = 1;
%}

%left '+' '-'
%left '*' '/'
%nonassoc UNARY
%nonassoc IFX
%nonassoc ELSE
%start entry
%%
// final reduction state
entry: functions

// should allow the definition of multiple functions
functions: functions function
					| function

// entire function
function: function_definition code_block

// a function definition
function_definition: TYPE VAR '(' TYPE ')'
										| TYPE VAR '(' TYPE VAR ')'
										| TYPE VAR '(' ')'

// a loop (while) block
loop: WHILE '(' condition ')' code_block

// if else block with proper precedence
if_else: if_statment %prec IFX
				| if_statment else_statement

// if statement
if_statment: IF '(' condition ')' statement

// else statement
else_statement: ELSE statement

// code block with var declarations at the start
code_block: block_start statements block_end

// code block start
block_start: '{' 

// code block end
block_end: '}'

// multiple statements
statements: statements statement
					| statement

// statement can be assignmemnt function call if_else loop or code block
statement: assignment
					| function_call ';'
					| if_else
					| loop
					| code_block
					| variable_declaration

// variable declaration with new line at the end
variable_declaration: TYPE VAR ';' 

// assignment to var
assignment: VAR '=' arithmetic ';'
					| VAR '=' parameter ';'
					| VAR '=' function_call ';'

// a function call
function_call: VAR '(' parameter ')'

// arithmetic operations
arithmetic: parameter '+' parameter
					| parameter '-' parameter
					| parameter '*' parameter
					| parameter '/' parameter

// condition operation
condition: parameter EQ parameter
					| parameter LT parameter
					| parameter LTE parameter
					| parameter GT parameter
					| parameter GTE parameter

parameter: operand
					| unary

// operand can be number or variable
operand: NUM
				| VAR

// unary opwrations
unary: '-' operand %prec UNARY
%%

int main(int argc, char** argv){
	if (argc == 2){
		yyin = fopen(argv[1], "r");
	}
	yyparse();
	if (yyin != stdin)
		fclose(yyin);
	yylex_destroy();
	return 0;
}

void yyerror(const char * err){
	fprintf(stdout, "[Syntax error %d]", yylineno);
}
