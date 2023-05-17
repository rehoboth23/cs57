#include <iostream>
#include <vector>
#include <set>
#include <map>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <array>
#include <functional>
#include "sem.h"
#include "ast.h"

using namespace std;

typedef struct analyzer analyzer_t;

struct analyzer
{
  vector<map<string, var_type> *> *tables;
  map<string, var_type> *funcs;
};

/**
 * @brief Create analyzer analyzer object
 *
 * @return analyzer_t* new analyzer
 */
analyzer_t *createAnalyzer()
{
  analyzer_t *analyzer = (analyzer_t *)calloc(1, sizeof(analyzer_t));
  analyzer->tables = new vector<map<string, var_type> *>();
  analyzer->funcs = new map<string, var_type>();
  return analyzer;
}

/**
 * @brief add analyzer variable into most recent symbol table
 *
 * @param analyzer analyzer
 * @param variable variable to add to most recent symbol table
 */
void addToAnalyzer(analyzer_t *analyzer, char *variable, var_type type)
{
  assert(variable != NULL);
  assert(analyzer != NULL);

  string var = string{variable};
  map<string, var_type> &table = *(analyzer->tables->back());
  table[var] = type;
}

/**
 * @brief add new symbol table to analyzer stack
 *
 * @param analyzer analyzer
 */
void addNewTable(analyzer_t *analyzer)
{
  assert(analyzer != NULL);
  analyzer->tables->push_back(new map<string, var_type>());
}

/**
 * @brief remove table at the top of analyzer stack
 *
 * @param analyzer analyzer
 */
void removeTopTable(analyzer_t *analyzer)
{
  assert(analyzer != NULL);
  map<string, var_type> *table = analyzer->tables->back();
  analyzer->tables->pop_back();
  delete (table);
}

/**
 * @brief search for variable in analyzer tables
 *
 * @param analyzer analyzer
 * @param variable variable to search for
 *
 * @return if any analyzer table contains variable return variable
 */
var_type searchAnalyzer(analyzer_t *analyzer, char *variable)
{
  assert(analyzer != NULL && analyzer->tables != NULL);
  assert(variable != NULL);
  // check blocks incrementally from most local to most global analyzer
  for (int i = analyzer->tables->size(); i > 0; i--)
  {
    map<string, var_type> *table = analyzer->tables->at(i - 1);
    if (auto search = table->find(variable); search != table->end())
    {
      auto res = (*table->find(variable)).second;
      return res;
    }
  }
  return void_ty;
}

bool searchCall(analyzer_t *analyzer, char *call)
{
  map<string, var_type> *funcs = analyzer->funcs;
  if (auto search = funcs->find(call); search != funcs->end())
  {
    return true;
  }
  return false;
}

void addCall(analyzer_t *analyzer, char *call, var_type type = void_ty)
{
  string name = string{call};
  map<string, var_type> &funcs = *(analyzer->funcs);
  funcs[name] = type;
}

/**
 * @brief delete analyzer
 *
 * @param analyzer analyzer
 */
void deleteAnalyzer(analyzer_t *analyzer)
{
  assert(analyzer != NULL);
  for (map<string, var_type> *table : *(analyzer->tables))
  {
    delete (table);
  }
  delete (analyzer->tables);
  delete (analyzer->funcs);
  free(analyzer);
}

void analyze(analyzer_t *analyzer, astNode *tree)
{
  assert(analyzer != NULL);
  if (tree == NULL)
    return;

  switch (tree->type)
  {
  case ast_prog:
    for (astNode *ext : *tree->prog.exts)
    {
      analyze(analyzer, ext);
    }
    analyze(analyzer, tree->prog.func);
    break;
  case ast_func:
  {
    char *name = tree->func.name;
    addCall(analyzer, name);
    addNewTable(analyzer);
    if (tree->func.params != nullptr)
    {
      for (astNode *param : *tree->func.params)
      {
        addToAnalyzer(analyzer, param->var.name, param->var.type);
      }
    }
    analyze(analyzer, tree->func.body);
    removeTopTable(analyzer);
    break;
  }
  case ast_extern:
  {
    char *name = tree->ext.name;
    addCall(analyzer, name);
    break;
  }
  case ast_stmt:
    switch (tree->stmt.type)
    {
    case ast_block:
      addNewTable(analyzer);
      for (astNode *stmt : *(tree->stmt.block.stmt_list))
      {
        analyze(analyzer, stmt);
      }
      removeTopTable(analyzer);
      break;
    case ast_asgn:
      analyze(analyzer, tree->stmt.asgn.lhs);
      analyze(analyzer, tree->stmt.asgn.rhs);
      break;
    case ast_while:
      analyze(analyzer, tree->stmt.whilen.cond);
      analyze(analyzer, tree->stmt.whilen.body);
      break;
    case ast_if:
      analyze(analyzer, tree->stmt.ifn.cond);
      analyze(analyzer, tree->stmt.ifn.if_body);
      analyze(analyzer, tree->stmt.ifn.else_body);
      break;
    case ast_ret:
      analyze(analyzer, tree->stmt.ret.expr);
      break;
    case ast_call:
    {
      astCall call = tree->stmt.call;
      char *name = call.name;
      if (!searchCall(analyzer, name))
      {
        cerr << "Unknows call -> " << string{name} << endl;
        exit(1);
      }
      if (tree->stmt.call.params != nullptr)
      {
        for (astNode *param : *tree->stmt.call.params)
        {
          analyze(analyzer, param);
        }
      }

      break;
    }
    case ast_decl:
      if (searchAnalyzer(analyzer, tree->stmt.decl.name) != void_ty)
      {
        cerr << "variable " << tree->stmt.decl.name << " is already declared" << endl;
        exit(1);
      }
      addToAnalyzer(analyzer, tree->stmt.decl.name, tree->stmt.decl.type);
      break;
    default:
      break;
    }
    break;
  case ast_var:
  {
    if (!tree->var.declared)
    {
      var_type type = searchAnalyzer(analyzer, tree->var.name);
      if (type == void_ty)
      {
        cerr << "variable " << tree->var.name << " is not declared" << endl;
        exit(1);
      }
      if (tree->var.type != ptr_ty)
      {
        tree->var.type = type;
      }
    }
    else
    {
      addToAnalyzer(analyzer, tree->var.name, tree->var.type);
    }
    break;
  }
  case ast_bexpr:
    analyze(analyzer, tree->bexpr.lhs);
    analyze(analyzer, tree->bexpr.rhs);
    break;
  case ast_rexpr:
    analyze(analyzer, tree->rexpr.lhs);
    analyze(analyzer, tree->rexpr.rhs);
    break;
  case ast_uexpr:
    analyze(analyzer, tree->uexpr.expr);
    break;
  default:
    break;
  }
}

void fixExpr(astNode *&a, astNode *&b)
{
  if (a->type == ast_bexpr && b->type == ast_rexpr && a->rexpr.lhs == b)
  {
    astNode *nb = createBExpr(b->rexpr.rhs, a->bexpr.rhs, a->bexpr.op);
    astNode *na = createRExpr(b->rexpr.lhs, nb, b->rexpr.op);
    a = na;
    b = nb;
  }
  else if (a->type == ast_bexpr && b->type == ast_bexpr && a->bexpr.lhs == b)
  {
    if ((a->bexpr.op == mul || a->bexpr.op == divide) && (b->bexpr.op != mul || b->bexpr.op != divide))
    {
      astNode *nb = createBExpr(b->bexpr.rhs, a->bexpr.rhs, a->bexpr.op);
      astNode *na = createBExpr(b->bexpr.lhs, nb, b->bexpr.op);
      a = na;
      b = nb;
    }
  }
}
