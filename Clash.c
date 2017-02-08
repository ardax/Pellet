#include "Clash.h"

Clash::Clash(Node* pNode, DependencySet* pDS, ExprNode* pExpr)
{
  m_pNode = pNode;
  m_pDepends = pDS;
}

Clash::Clash(Node* pNode, int iType, DependencySet* pDS)
{
  m_pNode = pNode;
  m_iType = iType;
  m_pDepends = pDS;
}

Clash::Clash(Node* pNode, int iType, DependencySet* pDS, ExprNodes* pOthers)
{
  m_pNode = pNode;
  m_iType = iType;
  m_pDepends = pDS;

  for(ExprNodes::iterator i = pOthers->begin(); i != pOthers->end(); i++) 
    m_aArgs.push_back((ExprNode*)*i);
}

Clash::Clash(Node* pNode, int iType, DependencySet* pDS, string sMsg)
{
  m_pNode = pNode;
  m_iType = iType;
  m_pDepends = pDS;
  m_sExplanation = sMsg;
}

Clash* Clash::nominal(Node* pNode, DependencySet* pDS, ExprNode* pOther)
{
  if( pOther )
    {
      ExprNodes aOthers;
      aOthers.push_back(pOther);
      return (new Clash(pNode, NOMINAL, pDS, &aOthers));
    }
  return (new Clash(pNode, NOMINAL, pDS));
}

Clash* Clash::invalidLiteral(Node* pNode, DependencySet* pDS, ExprNode* pOther)
{
  if( pOther )
    {
      ExprNodes aOthers;
      aOthers.push_back(pOther);
      return (new Clash(pNode, INVALID_LITERAL, pDS, &aOthers));
    }
  return (new Clash(pNode, INVALID_LITERAL, pDS));
}

Clash* Clash::emptyDatatype(Node* pNode, DependencySet* pDS, ExprNode* pOther)
{
  if( pOther )
    {
      ExprNodes aOthers;
      aOthers.push_back(pOther);
      return (new Clash(pNode, EMPTY_DATATYPE, pDS, &aOthers));
    }
  return (new Clash(pNode, EMPTY_DATATYPE, pDS));
}

Clash* Clash::functionalCardinality(Node* pNode, DependencySet* pDS, ExprNode* pOther)
{
  if( pOther )
    {
      ExprNodes aOthers;
      aOthers.push_back(pOther);
      return (new Clash(pNode, FUNC_MAX_CARD, pDS, &aOthers));
    }
  return (new Clash(pNode, FUNC_MAX_CARD, pDS));
}

Clash* Clash::maxCardinality(Node* pNode, DependencySet* pDS)
{
  return (new Clash(pNode, MAX_CARD, pDS));
}

Clash* Clash::maxCardinality(Node* pNode, DependencySet* pDS, ExprNode* pOther, int iN)
{
  ExprNodes aOthers;
  aOthers.push_back(pOther);

  ExprNode* pN = getNewExprNode();
  pN->m_iExpression = EXPR_TERM;
  pN->m_iTerm = iN;
  aOthers.push_back(pN);

  return (new Clash(pNode, MAX_CARD, pDS, &aOthers));
}

Clash* Clash::unexplained(Node* pNode, DependencySet* pDS, string sMsg)
{
  return (new Clash(pNode, UNEXPLAINED, pDS, sMsg));
}

Clash* Clash::atomic(Node* pNode, DependencySet* pDS)
{
  return (new Clash(pNode, ATOMIC, pDS));
}

Clash* Clash::atomic(Node* pNode, DependencySet* pDS, ExprNode* pOther)
{
  ExprNodes aOthers;
  aOthers.push_back(pOther);
  return (new Clash(pNode, ATOMIC, pDS, &aOthers));
}

void Clash::printClashInfo()
{
  printf("<comment>");
  switch(m_iType)
    {
    case ATOMIC:
      printf("Atomic");
      break;
    case MIN_MAX:
      printf("MinMax");
      break;
    case MAX_CARD:
      printf("MaxCard");
      break;
    case FUNC_MAX_CARD:
      printf("FuncMaxCard");
      break;
    case NOMINAL:
      printf("Nominal");
      break;
    case EMPTY_DATATYPE:
      printf("EmptyDatatype");
      break;
    case VALUE_DATATYPE:
      printf("ValueDatatype");
      break;
    case MISSING_DATATYPE:
      printf("MissingDatatype");
      break;
    case INVALID_LITERAL:
      printf("InvalidLiteral");
      break;
    case UNEXPLAINED:
      printf("Unexplained");
      break;
    };
  printf("</comment>");
}
