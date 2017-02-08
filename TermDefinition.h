#ifndef _TERM_DEFINITION_
#define _TERM_DEFINITION_

#include "ExpressionNode.h"
#include <vector>

class TermDefinition
{
 public:
  TermDefinition(TermDefinition* pTD = NULL);

  bool isUnique();
  void updateDependencies();
  bool addDef(ExprNode* pExpr);
  bool removeDef(ExprNode* pExpr);

  ExprNodeSet m_setSubClassAxioms;
  ExprNodeSet m_setEqClassAxioms;
  ExprNodeSet m_setSeen;
  ExprNodeSet m_setDependencies;
};

typedef vector<TermDefinition*> TermDefinitions;
typedef map<ExprNode*, TermDefinition*, strCmpExprNode> ExprNode2TermDefinition;

TermDefinition* createTermDefinition();


#endif
