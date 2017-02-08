#include "TermDefinition.h"
#include "ReazienerUtils.h"

extern int g_iCommentIndent;

TermDefinition::TermDefinition(TermDefinition* pTD)
{
  if( pTD )
    {
      m_setSubClassAxioms = pTD->m_setSubClassAxioms;
      m_setEqClassAxioms = pTD->m_setEqClassAxioms;
      m_setSeen = pTD->m_setSeen;
      m_setDependencies = pTD->m_setDependencies;
    }
}

bool TermDefinition::isUnique()
{
  if( m_setEqClassAxioms.size() == 0 )
    return TRUE;
  else if( m_setEqClassAxioms.size() == 1 && m_setSubClassAxioms.size() == 0 )
    return TRUE;
  return FALSE;
}

void TermDefinition::updateDependencies()
{
  m_setDependencies.clear();

  for(ExprNodeSet::iterator i = m_setSubClassAxioms.begin(); i != m_setSubClassAxioms.end(); i++ )
    {
      ExprNode* pSub = (ExprNode*)*i;
      ExprNodes aList;
      findPrimitives((ExprNode*)pSub->m_pArgs[1], &aList);
      for(ExprNodes::iterator i = aList.begin(); i != aList.end(); i++ )
	m_setDependencies.insert((ExprNode*)*i);
    }

  for(ExprNodeSet::iterator i = m_setEqClassAxioms.begin(); i != m_setEqClassAxioms.end(); i++ )
    {
      ExprNode* pSub = (ExprNode*)*i;
      ExprNodes aList;
      findPrimitives((ExprNode*)pSub->m_pArgs[1], &aList);
      for(ExprNodes::iterator i = aList.begin(); i != aList.end(); i++ )
	m_setDependencies.insert((ExprNode*)*i);
    }
}

bool TermDefinition::addDef(ExprNode* pExpr)
{
  START_DECOMMENT2("TermDefinition::addDef");
  printExprNodeWComment("Def=", pExpr);

  if( m_setSeen.find(pExpr) != m_setSeen.end() )
    {
      END_DECOMMENT("TermDefinition::addDef");
      return FALSE;
    }
  else
    m_setSeen.insert(pExpr);

  bool bAdded = FALSE;
  if( pExpr->m_iExpression == EXPR_SUBCLASSOF )
    bAdded = m_setSubClassAxioms.insert(pExpr).second;
  else if( pExpr->m_iExpression == EXPR_EQCLASSES )
    bAdded = m_setEqClassAxioms.insert(pExpr).second;
  else
    assertFALSE("Cannot add non-definition!");

  if( bAdded )
    updateDependencies();
  END_DECOMMENT("TermDefinition::addDef");
  return bAdded;
}

bool TermDefinition::removeDef(ExprNode* pExpr)
{
  bool bRemoved = FALSE;
  m_setSeen.erase(pExpr);

  if( pExpr->m_iExpression == EXPR_SUBCLASSOF )
    {
      ExprNodeSet::iterator i = m_setSubClassAxioms.find(pExpr);
      if( i != m_setSubClassAxioms.end() )
	{
	  m_setSubClassAxioms.erase(i);
	  bRemoved = TRUE;
	}
    }
  else if( pExpr->m_iExpression == EXPR_EQCLASSES )
    {
      ExprNodeSet::iterator i = m_setEqClassAxioms.find(pExpr);
      if( i != m_setEqClassAxioms.end() )
	{
	  m_setEqClassAxioms.erase(i);
	  bRemoved = TRUE;
	}
    }
  else
    assertFALSE("Cannot remove non-definition!");

  updateDependencies();
  return bRemoved;
}
