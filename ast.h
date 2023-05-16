#ifndef AST_H
#define AST_H

#include <cstddef>
#include <vector>
#include <string>
using namespace std;

struct ast_Node;
typedef struct ast_Node astNode;

struct ast_Stmt;
typedef struct ast_Stmt astStmt;

// enum to identify node type
typedef enum
{
	ast_prog,
	ast_func,
	ast_stmt,
	ast_extern,
	ast_var,
	ast_cnst,
	ast_rexpr, // For comparison expression
	ast_bexpr, // For binary arithmetic expression
	ast_uexpr, // For unary arithmetic exoression
} node_type;

// enum to identy stmt type
typedef enum
{
	ast_call,
	ast_ret,
	ast_block,
	ast_while,
	ast_if,
	ast_asgn,
	ast_decl
} stmt_type;

// enum to identify op type in expressions
// typedef enum
// {
// } op_type;

typedef enum
{
	add,		// +
	sub,		// -
	divide, // /
	mul,		// *
	uminus,	// -: unary minus
			lt, // <
	gt,			// >
	le,			// <=
	ge,			// >=
	eq,			// ==
	neq			// !=
} op_type;

typedef enum
{
	char_ty,
	int_ty,
	ptr_ty,
	void_ty,
} var_type;

/* structs for different node types */

typedef struct
{
	vector<astNode *> *exts; // extern functions
	astNode *func;					 // function defined in input miniC program
	char *name;
} astProg;

typedef struct
{
	char *name;								 // name of the function
	var_type type;						 // name of the function
	vector<astNode *> *params; // parameter, possibly NULL if the function doesn't take a param
	astNode *body;						 // function body
} astFunc;

typedef struct
{
	char *name; // For extern functions defined we will only save function names
	var_type type;
	vector<var_type> *args;
} astExtern;

typedef struct
{
	char *name;
	var_type type;
	bool declared;
} astVar;

typedef struct
{
	int value; // For integer contants we will store the value of the constant
	var_type type;
} astConst;

typedef struct
{
	astNode *lhs;
	astNode *rhs;
	op_type op;
} astRExpr;

typedef struct
{
	astNode *lhs;
	astNode *rhs;
	op_type op;
} astBExpr;

typedef struct
{
	astNode *expr;
	op_type op;
} astUExpr;

/* structs for different statement types */
typedef struct
{
	char *name;
	vector<astNode *> *params;
} astCall;

typedef struct
{
	astNode *expr; // Can be an expression/variable/constant
} astRet;

typedef struct
{
	vector<astNode *> *stmt_list;
} astBlock;

typedef struct
{
	astNode *cond;
	astNode *body;
} astWhile;

typedef struct
{
	astNode *cond;
	astNode *if_body;
	astNode *else_body; // possibly NULL
} astIf;

typedef struct
{
	char *name;
	var_type type;
} astDecl;

typedef struct
{
	astNode *lhs;
	astNode *rhs;
} astAsgn;

struct ast_Stmt
{
	stmt_type type;
	union
	{
		astCall call;
		astRet ret;
		astBlock block;
		astWhile whilen;
		astIf ifn;
		astDecl decl;
		astAsgn asgn;
		// astAsgn address;
		// astAsgn pointer;
	};
};

struct ast_Node
{
	node_type type;
	union
	{
		astProg prog;
		astFunc func;
		astExtern ext;
		astStmt stmt;
		astVar var;
		astConst cnst;
		astRExpr rexpr;
		astBExpr bexpr;
		astUExpr uexpr;
	};
};

/*
Declarations of create* functions for all the types of nodes
defined above. All the create* functions return a astNode*.
*/

astNode *createProg(vector<astNode *> *exts, astNode *func);
astNode *createFunc(const char *name, var_type type, vector<astNode *> *params, astNode *body);
astNode *createExtern(const char *name);
astNode *createVar(const char *name);
astNode *createCnst(int value);
astNode *createRExpr(astNode *lhs, astNode *rhs, op_type op);
astNode *createBExpr(astNode *lhs, astNode *rhs, op_type op);
astNode *createUExpr(astNode *expr, op_type op);

/*
Instead of one create for astNode of type ast_stmt, a separate
create function is given for each statement type. Like all
other create functions these functions return
a astNode*.
*/

astNode *createCall(const char *name, vector<astNode *> *params = NULL);
astNode *createRet(astNode *expr);
astNode *createBlock(vector<astNode *> *stmt_list);
astNode *createWhile(astNode *cond, astNode *body);
astNode *createIf(astNode *cond, astNode *if_body, astNode *else_body = NULL);
astNode *createDecl(const char *decl, var_type type);
astNode *createAsgn(astNode *lhs, astNode *rhs);

/*
Declarations for all free* functions. All these functions take a astNode* as parameter
as free the memory allocated by corresponding create functions.
*/

void freeProg(astNode *);
void freeFunc(astNode *);
void freeExtern(astNode *);
void freeVar(astNode *);
void freeCnst(astNode *);
void freeRExpr(astNode *);
void freeBExpr(astNode *);
void freeUExpr(astNode *);

/* free* for each type of statement */
void freeCall(astNode *);
void freeRet(astNode *);
void freeBlock(astNode *);
void freeWhile(astNode *);
void freeIf(astNode *);
void freeDecl(astNode *);
void freeAsgn(astNode *);

/* freeNode checks the node type and calls the corresponding free* function.*/
void freeNode(astNode *);

/* freeStmt checks the stmt type and calls the corresponding free* function.*/
void freeStmt(astNode *);

/* Function to print astNode and astStmt. The second parameter is to beautify the output.*/

void printNode(astNode *, int indent = 0);
void printStmt(astStmt *, int indent = 0);

// op_type getROpType(char *rop);
astNode* getExpr(astNode *lhs, string op, astNode *rhs);

#endif
