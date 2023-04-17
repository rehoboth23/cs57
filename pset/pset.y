%{
#include <stdio.h>
#include "ast.h"
#include "vector"
void yyerror(const char *);
extern int yylex();
extern int yylex_destroy();
extern FILE *yyin;
extern int yylineno;
extern char* yytext;
extern int yydebug;
%}

%union{
	int ival;
	char *var_name;
	astNode *node_ptr;
	vector<astNode *> *vec_ptr;
}

%token TYPE IF ELSE WHILE LT GT LTE GTE EQ NEQ ADD SUB MUL DIV EXTERN
%token <ival> NUM
%token <var_name> VAR

%type <vec_ptr> program statements if_statment function_definition variable_declarations
%type <node_ptr> code function if_else loop else_statement code_block statement variable_declaration assignment function_call expr arithmetic condition parameter operand unary

%left ADD SUB
%left MUL DIV

%nonassoc UNARY
%nonassoc IFX
%nonassoc ELSE
%start program
%%
// final reduction state

program: program code {
					$$ = $1;
					$$->push_back($2);
				}
				| code {
					$$ = new vector<astNode*>();
					$$->push_back($1);
				}

code: function {$$ = $1;}
			| statement {$$ = $1;}

// entire function
function: function_definition code_block {$$ = createFunc(((astDecl *)$1->at(0))->name, $1->at(1), $2);}

// a function definition
function_definition: TYPE VAR '(' TYPE ')' {
											$$ = new vector<astNode*>();
											printf(" $2 -> %s\n", $2);
											astNode* decl = createDecl($2);
											$$->push_back(decl);
											astNode* var = createVar("");
											$$->push_back(var);
										}
										| TYPE VAR '(' TYPE VAR ')' {
											$$ = new vector<astNode*>();
											printf(" $2 -> %s\n", $$);
											astNode* decl = createDecl($2);
											$$->push_back(decl);
											astNode* var = createVar($5);
											$$->push_back(var);
										}
										| TYPE VAR '(' ')' {
											$$ = new vector<astNode*>();
											astNode* decl = createDecl($2);
											$$->push_back(decl);
											$$->push_back(nullptr);
										}

// a loop (while) block
loop: WHILE '(' condition ')' code_block {$$ = createWhile($3, $5);}

// if else block with proper precedence
if_else: if_statment %prec IFX {
					$$ = createIf($1->at(0), $1->at(1), nullptr);
				}
				| if_statment else_statement {
					$$ = createIf($1->at(0), $1->at(1), $2);
				}

// if statement
if_statment: IF '(' condition ')' statement {
	$$ = new vector<astNode*>();
	$$->push_back($3);
	$$->push_back($5);
}

// else statement
else_statement: ELSE statement {
	$$ = $2;
}

// code block with var declarations at the start
code_block: block_start variable_declarations statements block_end {
	$2->insert($2->end(), $3->begin(), $3->end());
	$$ = createBlock($2);
}

// code block start
block_start: '{'

// code block end
block_end: '}'

// multiple statements
statements: statements statement {
						$$ = $1;
						$$->push_back($2);
					}
					| statement {
						$$ = new vector<astNode*>();
						$$->push_back($1);
					}

// statement can be assignmemnt function call if_else loop or code block
statement: assignment {$$ = $1;}
					| function_call ';' {$$ = $1;}
					| if_else {$$ = $1;}
					| loop {$$ = $1;}
					| code_block {$$ = $1;}
					| EXTERN function_definition ';' {$$ = createFunc(((astDecl *)$2->at(0))->name, $2->at(1), nullptr);}

variable_declarations: variable_declarations variable_declaration {
												$$ = $1;
												$$->push_back($2);
											}
											| {$$ = new vector<astNode*>();}

// variable declaration with new line at the end
variable_declaration: TYPE VAR ';' {$$ = createDecl($2);}

// assignment to var
assignment: VAR '=' expr ';' {$$ = createAsgn(createVar($1), $3);}
					| VAR '=' function_call ';' {$$ = createAsgn(createVar($1), $3);}

// a function call
function_call: VAR '(' expr ')' {$$ = createCall($1, $3);}
							| VAR '(' ')' {$$ = createCall($1, nullptr);}

expr: arithmetic {$$ = $1;}
			| parameter {$$ = $1;}

// arithmetic operations
arithmetic: parameter ADD parameter {$$ = createBExpr($1, $3, add);}
					| parameter SUB parameter {$$ = createBExpr($1, $3, sub);}
					| parameter MUL parameter {$$ = createBExpr($1, $3, mul);}
					| parameter DIV parameter {$$ = createBExpr($1, $3, divide);}

// condition operation
condition: parameter EQ parameter {$$ = createRExpr($1, $3, eq);}
					| parameter LT parameter {$$ = createRExpr($1, $3, lt);}
					| parameter LTE parameter {$$ = createRExpr($1, $3, le);}
					| parameter GT parameter {$$ = createRExpr($1, $3, gt);}
					| parameter GTE parameter {$$ = createRExpr($1, $3, ge);}
					| parameter NEQ parameter {$$ = createRExpr($1, $3, neq);}

parameter: operand {$$ = $1;}
					| unary {$$ = $1;}

// operand can be number or variable
operand: NUM {$$ = createCnst($1);}
				| VAR {$$ = createVar($1);}

// unary opwrations
unary: SUB operand %prec UNARY {$$ = createUExpr($2, uminus);}
%%

int main(int argc, char** argv){
	yydebug = 1;
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
