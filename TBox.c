#include "TBox.h"
#include "Tu.h"
#include "Tg.h"
#include "KnowledgeBase.h"
#include "ReazienerUtils.h"
#include "Params.h"

extern int g_iCommentIndent;
extern ExprNode* EXPRNODE_TOP;
extern ExprNode* EXPRNODE_BOTTOM;

TBox::TBox(KnowledgeBase* pKB)
{
  m_pTuBox = new TuBox(pKB);
  m_pTgBox = new TgBox(pKB);
}

bool TBox::addClass(ExprNode* pClassExpr)
{ 
  bool bAdded = m_setClasses.insert(pClassExpr).second;
  if( bAdded )
    m_setAllClasses.clear();
  return bAdded;
}

bool TBox::addAxiom(ExprNode* pAxiom, ExprNodeSet* pExplanation)
{
  START_DECOMMENT2("TBox::addAxiom");
  printExprNodeWComment("Axiom=", pAxiom);

  // absorb nominals on the fly because sometimes they might end up in the
  // Tu directly without going into Tg which is still less effective than absorbing
  if( PARAMS_USE_NOMINAL_ABSORPTION() || PARAMS_USE_PSEUDO_NOMINALS() )
    {
      if( pAxiom->m_iExpression == EXPR_EQCLASSES )
	{
	  ExprNode* pC1 = (ExprNode*)pAxiom->m_pArgs[0];
	  ExprNode* pC2 = (ExprNode*)pAxiom->m_pArgs[1];

	  // the first concept is oneOF
	  if( isOneOf(pC1) )
	    {
	      // absorb SubClassOf(c1,c2)
	      m_pTgBox->absorbOneOf(pC1, pC2, pExplanation);

	      if( isOneOf(pC2) )
		{
		  // absorb SubClassOf(c2,c1)
		  m_pTgBox->absorbOneOf(pC2, pC1, pExplanation);
		  // axioms completely absorbed so return
		  END_DECOMMENT("TBox::addAxiom");
		  return TRUE;
		}
	      else
		{
		  // SubClassOf(c2,c1) is not absorbed so continue with
		  // addAxiom function
		  pAxiom = createExprNode(EXPR_SUBCLASSOF, pC2, pC1);
		}
	    }
	  else if( isOneOf(pC2) )
	    {
	      // absorb SubClassOf(c2,c1)
	      m_pTgBox->absorbOneOf(pC2, pC1, pExplanation);

	      // SubClassOf(c1,c2) is not absorbed so continue with
	      // addAxiom function
	      pAxiom = createExprNode(EXPR_SUBCLASSOF, pC1, pC2);
	    }
	}
      else if( pAxiom->m_iExpression == EXPR_SUBCLASSOF )
	{
	  ExprNode* pSubExpr = (ExprNode*)pAxiom->m_pArgs[0];
	  if( isOneOf(pSubExpr) )
	    {
	      ExprNode* pSupExpr = (ExprNode*)pAxiom->m_pArgs[1];
	      m_pTgBox->absorbOneOf(pSubExpr, pSupExpr, pExplanation);
	      END_DECOMMENT("TBox::addAxiom");
	      return TRUE;
	    }
	}
    }
  
  bool bAddAxiom = addAxiom(pAxiom, pExplanation, FALSE);
  END_DECOMMENT("TBox::addAxiom");
  return bAddAxiom;
}

bool TBox::addAxiom(ExprNode* pAxiom, ExprNodeSet* pExplanation, bool bForceAddition)
{
  START_DECOMMENT2("TBox::addAxiom");
  printExprNodeWComment("Axiom=", pAxiom);

  bool bAxiomExpAdded = addAxiomExplanation(pAxiom, pExplanation);
  if( bAxiomExpAdded || bForceAddition )
    {
      if( !m_pTuBox->addIfUnfoldable(pAxiom) )
	{
	  if( pAxiom->m_iExpression == EXPR_EQCLASSES )
	    {
	      // Try reversing the term if it is a 'same' construct
	      ExprNode* pName = (ExprNode*)pAxiom->m_pArgs[0];
	      ExprNode* pDesc = (ExprNode*)pAxiom->m_pArgs[1];
	      ExprNode* pReversedAxiom = createExprNode(EXPR_EQCLASSES, pDesc, pName);

	      if( !m_pTuBox->addIfUnfoldable(pReversedAxiom) )
		m_pTgBox->addDef(pAxiom);
	      else
		addAxiomExplanation(pReversedAxiom, pExplanation);
	    }
	  else
	    m_pTgBox->addDef(pAxiom);
	}
    }
  END_DECOMMENT("TBox::addAxiom");
  return bAxiomExpAdded;
}

bool TBox::addAxiomExplanation(ExprNode* pAxiom, ExprNodeSet* pExplanation)
{
  SetOfExprNodeSet* pSetOfExprSet = NULL;
  Expr2SetOfExprNodeSets::iterator i = m_mapTBoxAxioms.find(pAxiom);
  if( i == m_mapTBoxAxioms.end() )
    {
      pSetOfExprSet = new SetOfExprNodeSet;
      m_mapTBoxAxioms[pAxiom] = pSetOfExprSet;
    }
  else
    pSetOfExprSet = (SetOfExprNodeSet*)i->second;
  
  bool bAdded = FALSE;
  if( pSetOfExprSet->find(pExplanation) == pSetOfExprSet->end() )
    {
      bAdded = TRUE;
      pSetOfExprSet->insert(pExplanation);
    }

  if( bAdded )
    {
      for(ExprNodeSet::iterator j = pExplanation->begin(); j != pExplanation->end(); j++ )
	{
	  ExprNode* pExplainAxiom = (ExprNode*)*j;
	  if( isEqual(pAxiom, pExplainAxiom) != 0 )
	    {
	      ExprNodeSet* pExprSet = NULL;
	      Expr2ExprNodeSet::iterator k = m_mapReverseExplain.find(pExplainAxiom);
	      if( k == m_mapReverseExplain.end() )
		{
		  pExprSet = new ExprNodeSet;
		  m_mapReverseExplain[pExplainAxiom] = pExprSet;
		}
	      else
		pExprSet = (ExprNodeSet*)i->second;
	      pExprSet->insert(pAxiom);
	    }
	}
    }
  return bAdded;
}

ExprNodeSet* TBox::getAxiomExplanation(ExprNode* pAxiom)
{
  SetOfExprNodeSet* pSetOfSets = NULL;
  Expr2SetOfExprNodeSets::iterator iFind = m_mapTBoxAxioms.find(pAxiom);
  if( iFind == m_mapTBoxAxioms.end() )
    {
      //log.warn( "No explanation for " + axiom );
      return NULL;
    }
  else
    pSetOfSets = (SetOfExprNodeSet*)iFind->second;

  // we won't be generating multiple explanations using axiom
  // tracing so we just pick one explanation. the other option
  // would be to return the union of all explanations which
  // would cause Pellet to return non-minimal explanations sets
  ExprNodeSet* pExplain = (ExprNodeSet*)*pSetOfSets->begin();
  return pExplain;
}

void TBox::getAxioms(ExprNode* pTerm, ExprNodeSet* pAxioms)
{
  TermDefinition* pTD = m_pTgBox->getTermDef(pTerm);
  if( pTD )
    {
      pAxioms->insert(pTD->m_setSubClassAxioms.begin(), pTD->m_setSubClassAxioms.end());
      pAxioms->insert(pTD->m_setEqClassAxioms.begin(), pTD->m_setEqClassAxioms.end());
    }
  pTD = m_pTuBox->getTermDef(pTerm);
  if( pTD )
    {
      pAxioms->insert(pTD->m_setSubClassAxioms.begin(), pTD->m_setSubClassAxioms.end());
      pAxioms->insert(pTD->m_setEqClassAxioms.begin(), pTD->m_setEqClassAxioms.end());
    }
}

void TBox::absorb()
{
  m_pTgBox->absorb(m_pTuBox, this);
}

void TBox::normalize()
{
  m_pTuBox->normalizeDefinitions(this);
}

void TBox::internalize()
{
  m_pTgBox->internalize();
}

Expr2ExprSetPairList* TBox::getUC()
{
  if( m_pTgBox )
    return m_pTgBox->getUC();
  return NULL;
}

Expr2ExprSetPairList* TBox::unfold(ExprNode* pC)
{
  if( m_pTuBox )
    return m_pTuBox->unfold(pC);
  return NULL;
}

void TBox::getAllClasses(ExprNodeSet* pSetAllClasses)
{
  if( m_setAllClasses.size() != 0 )
    {
      pSetAllClasses->insert(m_setAllClasses.begin(), m_setAllClasses.end());
    }
  else
    {
      pSetAllClasses->insert(m_setClasses.begin(), m_setClasses.end());
      pSetAllClasses->insert(EXPRNODE_TOP);
      pSetAllClasses->insert(EXPRNODE_BOTTOM);
    }
}
