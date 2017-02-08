#ifndef _CLASH_
#define _CLASH_

#include "Node.h"
#include "DependencySet.h"
#include "ExpressionNode.h"

class Clash
{
 public:
  Clash(Node* pNode, DependencySet* pDS, ExprNode* pExpr);
  Clash(Node* pNode, int iType, DependencySet* pDS);
  Clash(Node* pNode, int iType, DependencySet* pDS, string sMsg);
  Clash(Node* pNode, int iType, DependencySet* pDS, ExprNodes* pOthers);

  enum ClashTypes
    {
      ATOMIC = 0,
      MIN_MAX,
      MAX_CARD,
      FUNC_MAX_CARD,
      NOMINAL,
      EMPTY_DATATYPE,
      VALUE_DATATYPE,
      MISSING_DATATYPE,
      INVALID_LITERAL,
      UNEXPLAINED
    };

  static Clash* nominal(Node* pNode, DependencySet* pDS, ExprNode* pOther = NULL);
  static Clash* invalidLiteral(Node* pNode, DependencySet* pDS, ExprNode* pValue = NULL);
  static Clash* emptyDatatype(Node* pNode, DependencySet* pDS, ExprNode* pValue = NULL);
  static Clash* functionalCardinality(Node* pNode, DependencySet* pDS, ExprNode* pValue = NULL);
  static Clash* maxCardinality(Node* pNode, DependencySet* pDS);
  static Clash* maxCardinality(Node* pNode, DependencySet* pDS, ExprNode* pValue, int iN);
  static Clash* unexplained(Node* pNode, DependencySet* pDS, string sMsg = "");
  static Clash* atomic(Node* pNode, DependencySet* pDS);
  static Clash* atomic(Node* pNode, DependencySet* pDS, ExprNode* pR);

  void printClashInfo();

  DependencySet* m_pDepends;
  Node* m_pNode;
  int m_iType;
  ExprNodes m_aArgs;
  string m_sExplanation;
};


#endif
