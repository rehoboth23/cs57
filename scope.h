#ifndef SCOPE_H
#define SCOPE_H

#include <vector>
#include <set>
#include <string.h>

using namespace std;

struct scope;
typedef struct scope scope_t;

struct comp {
  bool operator()(const char* a, const char* b) const {
    return strncmp(a, b, strlen(b)) < 0;
  }
};

struct scope
{
  vector<set<char *, comp> *> *tables;
};

/**
 * @brief Create a Scope object
 *
 * @return scope_t* new scope
 */
scope_t *createScope();

/**
 * @brief add a node into local scope
 *
 * @param s scope
 * @param node node to add to local scope
 */
void addToScope(scope_t *s, char *node);

/**
 * @brief mark new local scope
 *
 * @param s scope
 */
void startScope(scope_t *s);

/**
 * @brief clear out current local scope
 *
 * @param s scope
 */
void endScope(scope_t *s);

/**
 * @brief search for node in entire scope
 *
 * @param s scope
 * @param node node to seearch for
 * 
 * @return if scope contains node return node
 */
bool searchScope(scope_t *s, char *name);

/**
 * @brief 
 * 
 * @param s scope
 */
void deleteScope(scope_t *s);

#endif
