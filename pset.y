%{
#include <stdio.h>
#include "sem.h"
#include "ir_gen.h"

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
	var_type var_ty;
	char *vname;
	astNode *nptr;
	vector<astNode *> *node_vec_ptr;
	vector<var_type> *type_vec_ptr;
}

%token EXTERN RETURN COMMA PTR
%token IF ELSE WHILE
%token <ival> NUM CHAR
%token <vname> VAR TYPE OP

%type <type_vec_ptr> argTypes
%type <node_vec_ptr>  statements exprs func_args externs
%type <nptr> program func_arg ext function function_definition if_else loop variable_assignment
%type <nptr> if_statment else_statement code_block statement variable_declaration assignment
%type <nptr> function_call parameter operand unary
%type <var_ty> type

%left SUB


%nonassoc IFX
%nonassoc ELSE
%start program

%%
// final reduction state
program: externs function {
	$$ = createProg($1, $2);
	root = $$;
}

externs: externs ext {
	$$ = $1;
	$$->push_back($2);
} | {
	$$ = new vector<astNode *>();
}

ext: EXTERN type VAR '(' argTypes ')' ';' {
	$$ = createExtern($3);
	$$->ext.args = $5;
	$$->ext.type = $2;
	free($3);
}

argTypes: argTypes COMMA type {
	$$ = $1;
	$$->push_back($3);
} | type {
	$$ = new vector<var_type>();
	$$->push_back($1);
} | {
	$$ = new vector<var_type>();
}

// entire function
function: function_definition code_block {
	$$ = $1;
	$$->func.body = $2;
}

// a function definition
function_definition: type VAR '(' func_args ')' {
											$$ = createFunc($2, $1, $4, nullptr);
											free($2);
										}
										| type VAR '(' ')' {
											$$ = createFunc($2, $1, nullptr, nullptr);
											free($2);
										}

func_args: func_args COMMA func_arg {
	$$ = $1;
	$$->push_back($3);
}
| func_arg {{
	$$ = new vector<astNode *>();
	$$->push_back($1);
}}

func_arg: type VAR {
	$$ = createVar($2);
	$$->var.type = $1;
	free($2);
}

// a loop (while) block
loop: WHILE '(' parameter ')' code_block {
	astNode *cond = $3;
	if(cond->type != ast_rexpr) {
		astNode *tcnst = createCnst(0);
		tcnst->cnst.type = int_ty;
		cond = createRExpr(cond, tcnst, neq);
	}
	$$ = createWhile(cond, $5);
}

// if else block with proper precedence
if_else: if_statment %prec IFX {
					$$ = $1;
				}
				| if_statment else_statement {
					$$ = $1;
					$$->stmt.ifn.else_body = $2;
				}

// if statement
if_statment: IF '(' parameter ')' statement {
	astNode *cond = $3;
	if(cond->type != ast_rexpr) {
		astNode *tcnst = createCnst(0);
		tcnst->cnst.type = int_ty;
		cond = createRExpr(cond, tcnst, neq);
	}
	if($5->stmt.type != ast_block) {
		vector<astNode *> *vec = new vector<astNode *>();
		vec->push_back($5);
		$$ = createIf(cond, createBlock(vec), nullptr);
	} else {
		$$ = createIf(cond, $5, nullptr);
	}
}

// else statement
else_statement: ELSE statement {
	$$ = $2;
	if($$->stmt.type != ast_block) {
		vector<astNode *> *vec = new vector<astNode *>();
		vec->push_back($2);
		$$ = createBlock(vec);
	}
}

// code block with var declarations at the start
code_block: block_start statements block_end {
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
					| {$$ = new vector<astNode*>();}

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
					| RETURN parameter ';' {
						$$ = createRet($2);
					} 
					| variable_declaration {
						$$ = $1;
					} | variable_assignment {
						$$ = $1;
					}

// declare and assign variable at once
variable_assignment: type assignment {
		$$ = $2;
		$$->stmt.asgn.lhs->var.type = $1;
		$$->stmt.asgn.lhs->var.declared = true;
	}

// variable declaration with new line at the end
variable_declaration: type VAR ';' {
	$$ = createDecl($2, $1);
	free($2);
}

// assignment to var
assignment: VAR '=' parameter ';' {
						astNode *var = createVar($1);
						$$ = createAsgn(var, $3);
						free($1);
					}

// a function call
function_call: VAR '(' exprs ')' {
								$$ = createCall($1, $3);
								free($1);
							}
							| VAR '(' ')' {
								$$ = createCall($1, nullptr);
								free($1);
							}

exprs: exprs COMMA parameter {
			$$ = $1;
			$$->push_back($3);
		} 
		| parameter {
			$$ = new vector<astNode*>();
			$$->push_back($1);
		} 

parameter: operand {$$ = $1;}
					| unary {$$ = $1;}
					| function_call { $$ = $1;}
					| parameter OP operand {
						$$ = getExpr($1, string{$2}, $3);
						if ($1->type == ast_rexpr && !$1->rexpr.is_parenthesis || $1->type == ast_bexpr && !$1->bexpr.is_parenthesis) {
							fixExpr($$, $$->type == ast_rexpr ? $$->rexpr.lhs: $$->bexpr.lhs);
						}
					}
					| parameter OP '(' parameter ')' {
						$$ = getExpr($1, string{$2}, $4);
					}
					| '(' parameter ')' {
						$$ = $2;
						if ($$->type == ast_rexpr) {
							$$->rexpr.is_parenthesis = true;
						} else if ($$->type == ast_bexpr) {
							$$->bexpr.is_parenthesis = true;
						}
					}

// operand can be number or variable
operand: NUM {
	$$ = createCnst($1);
	$$->cnst.type = int_ty;
} 
| VAR {
		$$ = createVar($1);
		free($1);
	} 
| CHAR {
		$$ = createCnst($1);
		$$->cnst.type = char_ty;
	} 
| PTR VAR {
		$$ = createVar($2);
		$$->var.type = ptr_ty;
		free($2);
	}

type: TYPE {
	string ty_str = string{$1};
	if (ty_str == string{"int"}) {
		$$ = int_ty;
	} else if (ty_str == string{"char"}) {
		$$ = char_ty;
	} else {
		$$ = void_ty;
	}
} | PTR TYPE {
	$$ = ptr_ty;
}

// unary opwrations
unary: SUB operand {$$ = createUExpr($2, uminus);}
%%

int main(int argc, char** argv){
	// yydebug = 1;
	if (argc == 3){
		yyin = fopen(argv[1], "r");
	} else {
		fprintf(stderr, "Invalid number of argument ./? <input_file> [output_file]");
		exit(1);
	}
	root = nullptr;
	yyparse();
	if (root) {
		printNode(root);
		analyzer_t *analyzer = createAnalyzer();
		analyze(analyzer, root);
		deleteAnalyzer(analyzer);
		generateIR(root, string{argv[2]});
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
