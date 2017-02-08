#ifndef _LITERAL_
#define _LITERAL_

#include "Node.h"
#include "ExpressionNode.h"
#include "DependencySet.h"

class ABox;
class DependencySet;
class Datatype;

class Literal : public Node
{
 public:
  Literal(ExprNode* pName, ExprNode* pTerm, ABox* pABox, DependencySet* pDS);

  void checkClash();
  void addAllTypes(ExprNode2DependencySetMap* pTypesMap, DependencySet* pDS);
  ExprNode* getTerm();

  void prune(DependencySet* pDS);
  virtual bool hasSuccessor(Node* pX) { return FALSE; }
  virtual bool isBlockable();
  virtual int getNominalLevel();

  string m_sLang;
  bool m_bHasValue;
  void* m_pValue;
  ExprNode* m_pExprNodeValue;
  Datatype* m_pDatatype;
};

#endif
