#include "TBoxBase.h"
#include "KnowledgeBase.h"
#include "TermDefinition.h"
#include "ReazienerUtils.h"

extern int g_iCommentIndent;

TBoxBase::TBoxBase(KnowledgeBase* pKB)
{
  m_pKB = pKB;
}

bool TBoxBase::addDef(ExprNode* pDef)
{
  START_DECOMMENT2("TgBox::addDef");
  ExprNode* pName = (ExprNode*)pDef->m_pArgs[0];
  TermDefinition* pTD = getTermDef(pName);
  if( pTD )
    pTD->addDef(pDef);
  else
    {
      TermDefinition* pNewTD = new TermDefinition();
      printExprNodeWComment("New TermHash Key=", pName);
      pNewTD->addDef(pDef);
      m_mapTermHash[pName] = pNewTD;
    }
  END_DECOMMENT("TgBox::addDef");
}

bool TBoxBase::removeDef(ExprNode* pDef)
{
  bool bRemoved = FALSE;
  ExprNode* pName = (ExprNode*)pDef->m_pArgs[0];
  TermDefinition* pTD = getTermDef(pName);
  if( pTD )
    bRemoved = pTD->removeDef(pDef);
  return bRemoved;
}

TermDefinition* TBoxBase::getTermDef(ExprNode* pExpr)
{
  ExprNode2TermDefinition::iterator i = m_mapTermHash.find(pExpr);
  if( i != m_mapTermHash.end() )
    return (TermDefinition*)i->second;
  return NULL;
}
