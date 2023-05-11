#include <iostream>
#include <vector>
#include <set>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "sem.h"
#include "ast.h"

using namespace std;

typedef struct analyzer analyzer_t;

struct comp
{
  bool operator()(const char *a, const char *b) const
  {
    return strncmp(a, b, strlen(b)) < 0;
  }
};

struct analyzer
{
  vector<set<char *, comp> *> *tables;
};

/**
 * @brief Create analyzer analyzer object
 *
 * @return analyzer_t* new analyzer
 */
analyzer_t *createAnalyzer()
{
  analyzer_t *analyzer = (analyzer_t *)calloc(1, sizeof(analyzer_t));
  analyzer->tables = new vector<set<char *, comp> *>();
  return analyzer;
}

/**
 * @brief add analyzer variable into most recent symbol table
 *
 * @param analyzer analyzer
 * @param variable variable to add to most recent symbol table
 */
void addToAnalyzer(analyzer_t *analyzer, char *variable)
{
  assert(variable != NULL);
  assert(analyzer != NULL);

  char *var = (char *)calloc(strlen(variable) + 1, sizeof(char));
  strncpy(var, variable, strlen(variable));
  set<char *, comp> *table = analyzer->tables->back();
  table->insert(var);
}

/**
 * @brief add new symbol table to analyzer stack
 *
 * @param analyzer analyzer
 */
void addNewTable(analyzer_t *analyzer)
{
  assert(analyzer != NULL);
  analyzer->tables->push_back(new set<char *, comp>());
}

/**
 * @brief remove table at the top of analyzer stack
 *
 * @param analyzer analyzer
 */
void removeTopTable(analyzer_t *analyzer)
{
  assert(analyzer != NULL);
  set<char *, comp> *table = analyzer->tables->back();
  for (char *variable : *(table))
  {
    free(variable);
  }
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
bool searchAnalyzer(analyzer_t *analyzer, char *variable)
{
  assert(analyzer != NULL && analyzer->tables != NULL);
  assert(variable != NULL);
  // check blocks incrementally from most local to most global analyzer
  for (int i = analyzer->tables->size(); i > 0; i--)
  {
    set<char *, comp> *table = analyzer->tables->at(i - 1);
    if (auto search = table->find(variable); search != table->end())
    {
      return true;
    }
  }
  return false;
}

/**
 * @brief delete analyzer
 *
 * @param analyzer analyzer
 */
void deleteAnalyzer(analyzer_t *analyzer)
{
  assert(analyzer != NULL);
  for (set<char *, comp> *table : *(analyzer->tables))
  {
    for (char *variable : *(table))
    {
      free(variable);
    }
    delete (table);
  }
  delete (analyzer->tables);
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
    analyze(analyzer, tree->prog.func);
    break;
  case ast_func:
    addNewTable(analyzer);
    if (tree->func.param != NULL)
    {
      addToAnalyzer(analyzer, tree->func.param->var.name);
    }
    analyze(analyzer, tree->func.body);
    removeTopTable(analyzer);
    break;
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
      analyze(analyzer, tree->stmt.call.param);
      break;
    case ast_decl:
      if (searchAnalyzer(analyzer, tree->stmt.decl.name))
      {
        cerr << "variable " << tree->stmt.decl.name << " is already declared" << endl;
        exit(1);
      }
      addToAnalyzer(analyzer, tree->stmt.decl.name);
      break;
    default:
      break;
    }
    break;
  case ast_var:
    if (!searchAnalyzer(analyzer, tree->var.name))
    {
      cerr << "variable " << tree->var.name << " is not declared" << endl;
      exit(1);
    }
    break;
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
