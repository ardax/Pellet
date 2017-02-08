#include "Literal.h"
#include "DependencySet.h"
#include "KnowledgeBase.h"
#include "ABox.h"
#include "Clash.h"

extern KnowledgeBase* g_pKB;
extern ExprNode* EXPRNODE_BOTTOMLITERAL;

Literal::Literal(ExprNode* pName, ExprNode* pTerm, ABox* pABox, DependencySet* pDS) : Node(pName, pABox)
{
  m_sLang = "";
  m_bHasValue = FALSE;
  m_pValue = NULL;
  m_pExprNodeValue = NULL;
  m_pDatatype = NULL;
}

ExprNode* Literal::getTerm()
{
  if( m_bHasValue )
    return (ExprNode*)m_pExprNodeValue->m_pArgs[0];
  return NULL;
}

void Literal::checkClash()
{
  if( m_bHasValue && m_pValue == NULL )
    {
      m_pABox->setClash( Clash::invalidLiteral(this, getDepends(m_pName), getTerm()) );
      return;
    }

  if( hasType(EXPRNODE_BOTTOMLITERAL) )
    {
      m_pABox->setClash( Clash::emptyDatatype(this, getDepends(EXPRNODE_BOTTOMLITERAL)) );
      return;
    }

  if( m_mDepends.size() == 1 )
    {
      //datatype = RDFSLiteral.instance;
      return;
    }
}

void Literal::addAllTypes(ExprNode2DependencySetMap* pTypesMap, DependencySet* pDS)
{
  for(ExprNode2DependencySetMap::iterator i = pTypesMap->begin(); i != pTypesMap->end(); i++ )
    {
      ExprNode* pC = (ExprNode*)i->first;
      if( hasType(pC) )
	continue;
      
      DependencySet* pDepends = (DependencySet*)i->second;
      Node::addType(pC, pDepends->unionNew(pDS, m_pABox->m_bDoExplanation));
    }
  checkClash();
}

void Literal::prune(DependencySet* pDS)
{
  m_pPruned = pDS;
}

bool Literal::isBlockable()
{
  return (m_pValue==NULL);
}

int Literal::getNominalLevel()
{
  return (isNominal()?NODE_NOMINAL:NODE_BLOCKABLE);
}
