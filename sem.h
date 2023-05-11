#ifndef analyzer_H
#define analyzer_H

#include <vector>
#include <set>
#include <string.h>
#include "ast.h"

using namespace std;

typedef struct analyzer analyzer_t;

/**
 * @brief Create analyzer analyzer object
 *
 * @return analyzer_t* new analyzer
 */
analyzer_t *createAnalyzer();

/**
 * @brief analyze entire program
 * 
 * @param analyzer analyzer object
 * @param tree starting node
 */
void analyze(analyzer_t *analyzer, astNode *tree);

/**
 * @brief delete analyzer
 *
 * @param analyzer analyzer
 */
void deleteAnalyzer(analyzer_t *analyzer);

#endif
