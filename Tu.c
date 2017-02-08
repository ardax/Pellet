#include "Tu.h"
#include "TermDefinition.h"
#include "TBox.h"
#include "Params.h"
#include "DependencySet.h"
#include "KnowledgeBase.h"
#include "ReazienerUtils.h"

extern ExprNode* EXPRNODE_TOP;
extern ExprNode* EXPRNODE_BOTTOM;
extern int g_iCommentIndent;

TuBox::TuBox(KnowledgeBase* pKB) :TBoxBase(pKB)
{
  m_paTermsToNormalize = NULL;
}

bool TuBox::addDef(ExprNode* pAxiom)
{
  START_DECOMMENT2("TuBox::addDef");
  printExprNodeWComment("Axiom=", pAxiom);
  
  bool bAdded = FALSE;
  ExprNode* pName = (ExprNode*)pAxiom->m_pArgs[0];
  TermDefinition* pTD = getTermDef(pName);
  if( pTD == NULL )
    {
      pTD = new TermDefinition();
      printExprNodeWComment("New TermHash Key=", pName);
      m_mapTermHash[pName] = pTD;
    }

  bAdded = pTD->addDef(pAxiom);
  if( bAdded && m_paTermsToNormalize )
    m_paTermsToNormalize->push_back(pName);

  END_DECOMMENT("TuBox::addDef");
  return bAdded;
}

bool TuBox::removeDef(ExprNode* pExpr)
{
  bool bRemoved = TBoxBase::removeDef(pExpr);
  if( bRemoved && m_paTermsToNormalize )
    m_paTermsToNormalize->push_back( (ExprNode*)pExpr->m_pArgs[0] );
  return bRemoved;
}

bool TuBox::addIfUnfoldable(ExprNode* pTerm)
{
  START_DECOMMENT2("TuBox::addIfUnfoldable");
  printExprNodeWComment("Term=", pTerm);

  ExprNode* pName = (ExprNode*)pTerm->m_pArgs[0];
  ExprNode* pBody = (ExprNode*)pTerm->m_pArgs[1];
  TermDefinition* pTD = getTermDef(pName);

  if( !isPrimitive(pName) )
    {
      END_DECOMMENT("TuBox::addIfUnfoldable");
      return FALSE;
    }

  if( !pTD )
    pTD = new TermDefinition();

  // Basic Checks
  TermDefinition* pTDCopy = new TermDefinition(pTD);
  pTDCopy->addDef(pTerm);

  if( !pTDCopy->isUnique() )
    {
      END_DECOMMENT("TuBox::addIfUnfoldable");
      return FALSE;
    }

  // Loop Checks
  ExprNodes lDependencies;
  ExprNodeSet hSeen;
  findPrimitives(pBody, &lDependencies);
  if( !containsAll(&(pTD->m_setDependencies), &lDependencies) )
    {
      // Fast check failed
      for(ExprNodes::iterator i = lDependencies.begin(); i != lDependencies.end(); i++ )
	{
	  ExprNode* pCurrent = *i;
	  if( findTarget(pCurrent, pName, &hSeen) )
	    {
	      END_DECOMMENT("TuBox::addIfUnfoldable");
	      return FALSE;
	    }
	}
    }

  bool bAddDef = addDef(pTerm);
  END_DECOMMENT("TuBox::addIfUnfoldable");
  return bAddDef;
}

bool TuBox::findTarget(ExprNode* pTerm, ExprNode* pTarget, ExprNodeSet* pSeenSet)
{
  ExprNodes aStack;
  aStack.push_back(pTerm);

  while(aStack.size() > 0)
    {
      ExprNode* pCurrent = *(aStack.begin());
      aStack.pop_front();

      if( pSeenSet->find(pCurrent) != pSeenSet->end() )
	continue;
      pSeenSet->insert(pCurrent);

      if( isEqual(pCurrent, pTarget) == 0 )
	return TRUE;
      
      TermDefinition* pTD = getTermDef(pCurrent);
      if( pTD )
	{
	  // Shortcut
	  if( pTD->m_setDependencies.find(pTarget) != pTD->m_setDependencies.end() )
	    return TRUE;

	  for(ExprNodeSet::iterator i = pTD->m_setDependencies.begin(); i != pTD->m_setDependencies.end(); i++ )
	    aStack.push_front((ExprNode*)*i);
	}
    }
  return FALSE;
}

void TuBox::clearUnfoldingMap()
{
  m_UnfoldingMap.clear();
}

// Normalize all the definitions in the Tu
void TuBox::normalizeDefinitions(TBox* pTBox)
{
  START_DECOMMENT2("TuBox::normalizeDefinitions");
  if( m_paTermsToNormalize == NULL )
    {
      m_paTermsToNormalize = new ExprNodes;
      for(ExprNode2TermDefinition::iterator i = m_mapTermHash.begin(); i != m_mapTermHash.end(); i++ )
	{
	  ExprNode* pExprNode = (ExprNode*)i->first;
	  m_paTermsToNormalize->push_back(pExprNode);
	}
      clearUnfoldingMap();
    }

  for(ExprNodes::iterator i = m_paTermsToNormalize->begin(); i != m_paTermsToNormalize->end(); i++ )
    {
      ExprNode* pC = (ExprNode*)*i;
      printExprNodeWComment("C=", pC);
      TermDefinition* pTD = getTermDef(pC);
      ExprNode* pNotC = createExprNode(EXPR_NOT, pC);
      
      Expr2ExprSetPairList* listUnfoldC = new Expr2ExprSetPairList;
      if( pTD->m_setEqClassAxioms.size() > 0 )
	{
	  Expr2ExprSetPairList* listUnfoldNotC = new Expr2ExprSetPairList;
	  for(ExprNodeSet::iterator j = pTD->m_setEqClassAxioms.begin(); j != pTD->m_setEqClassAxioms.end(); j++ )
	    {
	      ExprNode* pEqClassAxiom = (ExprNode*)*j;
	      ExprNode* pUnfolded = (ExprNode*)pEqClassAxiom->m_pArgs[1];
	      ExprNodeSet* pDS = pTBox->getAxiomExplanation(pEqClassAxiom);

	      printExprNodeWComment("Unfolded=", pUnfolded);
	      ExprNode* pNormalized = normalize(pUnfolded);
	      ExprNode* pNormalizedNot = negate2(pNormalized);

	      Expr2ExprSetPair* pairNew = new Expr2ExprSetPair;
	      pairNew->first = pNormalized;
	      printExprNodeWComment("Unfolding=", pNormalized);
	      pairNew->second = pDS;
	      listUnfoldC->push_back(pairNew);

	      pairNew = new Expr2ExprSetPair;
	      pairNew->first = pNormalizedNot;
	      printExprNodeWComment("Unfolding(not)=", pNormalizedNot);
	      pairNew->second = pDS;
	      listUnfoldNotC->push_back(pairNew);
	    }

	  m_UnfoldingMap[pNotC] = listUnfoldNotC;
	}
      else
	m_UnfoldingMap.erase(pNotC);

      for(ExprNodeSet::iterator j = pTD->m_setSubClassAxioms.begin(); j != pTD->m_setSubClassAxioms.end(); j++ )
	{
	  ExprNode* pSubClassAxiom = (ExprNode*)*j;
	  ExprNode* pUnfolded = (ExprNode*)pSubClassAxiom->m_pArgs[1];
	  ExprNodeSet* pDS = pTBox->getAxiomExplanation(pSubClassAxiom);

	  ExprNode* pNormalized = normalize(pUnfolded);

	  Expr2ExprSetPair* pairNew = new Expr2ExprSetPair;
	  pairNew->first = pNormalized;
	  printExprNodeWComment("Unfolding=", pNormalized);
	  pairNew->second = pDS;
	  listUnfoldC->push_back(pairNew);
	}

      if( listUnfoldC->size() > 0 )
	{
	  m_UnfoldingMap[pC] = listUnfoldC;
	}
      else
	{
	  m_UnfoldingMap.erase(pC);
	}
    }
  
  m_paTermsToNormalize = NULL;
  
  if( PARAMS_USE_ROLE_ABSORPTION() )
    absorbRanges();
  END_DECOMMENT("TuBox::normalizeDefinitions");
}

void TuBox::absorbRanges()
{
  Expr2ExprSetPairList* pUnfoldTop;
  Expr2Expr2ExprSetPairListMap::iterator i = m_UnfoldingMap.find(EXPRNODE_TOP);
  if( i == m_UnfoldingMap.end() )
    return ;
  pUnfoldTop = (Expr2ExprSetPairList*)i->second;
   
  Expr2ExprSetPairList* pNewUnfoldTop = new Expr2ExprSetPairList;
  for(Expr2ExprSetPairList::iterator j = pUnfoldTop->begin(); j != pUnfoldTop->end(); j++ )
    {
      Expr2ExprSetPair* pPair = (Expr2ExprSetPair*)*j;
      ExprNode* pUnfolded = (ExprNode*)pPair->first;
      ExprNodeSet* pExplain = (ExprNodeSet*)pPair->second;

      if( pUnfolded->m_iExpression == EXPR_ALL )
	{
	  ExprNode* pR = (ExprNode*)pUnfolded->m_pArgs[0];
	  ExprNode* pRange = (ExprNode*)pUnfolded->m_pArgs[1];

	  m_pKB->addRange(pR, pRange, (new DependencySet(pExplain)) );
	}
      else if( pUnfolded->m_iExpression == EXPR_AND )
	{
	  ExprNodeList* pExprList = (ExprNodeList*)pUnfolded->m_pArgList;
	  ExprNodeList* pNewList = NULL;

	  for(int h = 0; h < pExprList->m_iUsedSize; h++)
	    {
	      ExprNode* pL = pExprList->m_pExprNodes[h];
	      if( pL->m_iExpression == EXPR_ALL )
		{
		  ExprNode* pR = (ExprNode*)pUnfolded->m_pArgs[0];
		  ExprNode* pRange = (ExprNode*)pUnfolded->m_pArgs[1];

		  m_pKB->addRange(pR, pRange, (new DependencySet(pExplain)) );
		}
	      else
		{
		  if( !pNewList )
		    pNewList = createExprNodeList(5);
		  addExprToList(pNewList, pL);
		}
	    }

	  if( pNewList )
	    {
	      Expr2ExprSetPair* pNewPair = new Expr2ExprSetPair;
	      pNewPair->first = createExprNode(EXPR_AND, pNewList);
	      pNewPair->second = pExplain;
	      pNewUnfoldTop->push_back(pNewPair);
	    }
	}
      else
	{
	  pNewUnfoldTop->push_back(pPair);
	}
    }

  if( pNewUnfoldTop->size() == 0 )
    {
      m_UnfoldingMap.erase(EXPRNODE_TOP);
    }
}

Expr2ExprSetPairList* TuBox::unfold(ExprNode* pC)
{
  Expr2Expr2ExprSetPairListMap::iterator iFind = m_UnfoldingMap.find(pC);
  if( iFind != m_UnfoldingMap.end() )
    return (Expr2ExprSetPairList*)iFind->second;
  return NULL;
}
