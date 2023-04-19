#include <iostream>
#include <vector>
#include <set>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "scope.h"

using namespace std;

/**
 * @brief Create a Scope object
 *
 * @return scope*
 */
scope_t *createScope()
{
  scope_t *scope = (scope_t *)calloc(1, sizeof(scope_t));
  scope->tables = new vector<set<char *, comp> *>();
  return scope;
}

/**
 * @brief add a variable into local scope
 *
 * @param s scope
 * @param variable variable to add to local scope
 */
void addToScope(scope_t *s, char *variable)
{
  assert(variable != NULL);
  assert(s != NULL);

  char *var = (char *)calloc(strlen(variable) + 1, sizeof(char));
  strncpy(var, variable, strlen(variable));
  s->tables->back()->insert(var);
}

/**
 * @brief mark new local scope
 *
 * @param s scope
 */
void startScope(scope_t *s)
{
  assert(s != NULL);
  s->tables->push_back(new set<char *, comp>());
}

/**
 * @brief clear out current local scope
 *
 * @param s scope
 */
void endScope(scope_t *s)
{
  assert(s != NULL);
  set<char *, comp> *table = s->tables->back();
  s->tables->pop_back();
  for (char *variable: *(table)) {
    free(variable);
  }
  free(table);
}

/**
 * @brief search for variable in entire scope
 *
 * @param s scope
 * @param variable variable to seearch for
 *
 * @return if scope contains variable
 */
bool searchScope(scope_t *s, char *variable)
{
  assert(s != NULL);
  assert(variable != NULL);

  // check blocks incrementally from most local to most global scope
  for (int i = s->tables->size(); i > 0; i--)
  {
    set<char *, comp> *table = s->tables->at(i - 1);
    if (auto search = table->find(variable); search != table->end())
    {
      return true;
    }
  }
  return false;
}

/**
 * @brief
 *
 * @param s scope
 */
void deleteScope(scope_t *s)
{
  assert(s != NULL);
  for (set<char *, comp> *table: *(s->tables)) {
    for (char *variable: *(table)) {
      free(variable);
    }
    free(table);
  }
  delete (s->tables);
  free(s);
}
