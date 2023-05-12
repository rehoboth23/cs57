%{
#include <stdio.h>
// #include <iostream>
// #include "ast.h"
// #include "vector"
#include "sem.h"
#include "optimizer.h"
// #include <assert.h>

void yyerror(const char *);
extern int yylex();
extern int yylex_destroy();
extern FILE *yyin;
extern int yylineno;
extern char* yytext;
extern int yydebug;
astNode *root;
%}

%union{
	int ival;
	char *vname;
	astNode *nptr;
	vector<astNode *> *vec_ptr;
}

%token IF ELSE WHILE LT GT LTE GTE EQ NEQ ADD SUB MUL DIV EXTERN RETURN
%token <ival> NUM
%token <vname> VAR TYPE

%type <vec_ptr>  statements variable_declarations
%type <nptr> program extern_print extern_read function function_definition if_else loop if_statment else_statement code_block statement variable_declaration assignment function_call expr arithmetic condition parameter operand unary

%left ADD SUB
%left MUL DIV

%nonassoc UNARY
%nonassoc IFX
%nonassoc ELSE
%start program

%%
// final reduction state
program: extern_print extern_read function {
	$$ = createProg($1, $2, $3);
	root = $$;
} | extern_read extern_print function {
	$$ = createProg($1, $2, $3);
	root = $$;
} | extern_print function {
	$$ = createProg($1, nullptr, $2);
	root = $$;
} | extern_read function {
	$$ = createProg($1, nullptr, $2);
	root = $$;
} | function {
	$$ = createProg(nullptr, nullptr, $1);
	root = $$;
}

extern_print: EXTERN TYPE VAR '(' TYPE ')' ';' {
	$$ = createExtern($3);
	free($3);
}

extern_read: EXTERN TYPE VAR '(' ')' ';' {
	$$ = createExtern($3);
	free($3);
}

// entire function
function: function_definition code_block {
	$$ = $1;
	$$->func.body = $2;
}

// a function definition
function_definition: TYPE VAR '(' TYPE VAR ')' {
											astNode *var = createVar($5);
											$$ = createFunc($2, $1, var, nullptr);
											free($2);
											// free($1);
											free($5);
										}
										| TYPE VAR '(' ')' {
											$$ = createFunc($2, $1, nullptr, nullptr);
											free($2);
											// free($1);
										}

// a loop (while) block
loop: WHILE '(' condition ')' code_block {$$ = createWhile($3, $5);}

// if else block with proper precedence
if_else: if_statment %prec IFX {
					$$ = $1;
				}
				| if_statment else_statement {
					$$ = $1;
					$$->stmt.ifn.else_body = $2;
				}

// if statement
if_statment: IF '(' condition ')' statement {
	$$ = createIf($3, $5, nullptr);
}

// else statement
else_statement: ELSE statement {
	$$ = $2;
}

// code block with var declarations at the start
code_block: block_start variable_declarations statements block_end {
	$2->insert($2->end(), $3->begin(), $3->end());
	$3->clear();
	delete($3);
	$$ = createBlock($2);
}

// code block start
block_start: '{' {
}

// code block end
block_end: '}' {
}

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
statement: assignment {
						$$ = $1;
					}
					| function_call ';' {
						$$ = $1;
					}
					| if_else {
						$$ = $1;
					}
					| loop {
						$$ = $1;
					}
					| code_block {
						$$ = $1;
					}
					| RETURN '(' expr ')' ';' {
						$$ = createRet($3);
					}

variable_declarations: variable_declarations variable_declaration {
												$$ = $1;
												$$->push_back($2);
											}
											| {$$ = new vector<astNode*>();}

// variable declaration with new line at the end
variable_declaration: TYPE VAR ';' {
	$$ = createDecl($2);
	free($2);
}

// assignment to var
assignment: VAR '=' expr ';' {
						astNode *var = createVar($1);
						$$ = createAsgn(var, $3);
						free($1);
					}
					| VAR '=' function_call ';' {
						astNode *var = createVar($1);
						$$ = createAsgn(var, $3);
						free($1);
					}

// a function call
function_call: VAR '(' expr ')' {
								$$ = createCall($1, $3);
								free($1);
							}
							| VAR '(' ')' {
								$$ = createCall($1, nullptr);
								free($1);
							}

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
				| VAR {
					$$ = createVar($1);
					free($1);
				}

// unary opwrations
unary: SUB operand %prec UNARY {$$ = createUExpr($2, uminus);}
%%

int main(int argc, char** argv){
	
	// yydebug = 1;
	if (argc == 2){
		yyin = fopen(argv[1], "r");
	}
	root = nullptr;
	yyparse();
	if (root) {
		analyzer_t *analyzer = createAnalyzer();
		analyze(analyzer, root);
		deleteAnalyzer(analyzer);
		llvmBackendCaller(root);
		freeNode(root);
	} else {
		
	}
	if (yyin != stdin)
		fclose(yyin);
	yylex_destroy();
	return 0;
}

void yyerror(const char * err){
	fprintf(stdout, "[Syntax error %d]\n", yylineno);
}
