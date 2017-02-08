#include "Tg.h"
#include "DependencySet.h"
#include "KnowledgeBase.h"
#include "TBox.h"
#include "Tu.h"
#include "TermDefinition.h"
#include "DependencySet.h"
#include "ReazienerUtils.h"
#include "Role.h"
#include "Params.h"
#include <cassert>

extern int g_iCommentIndent;

TgBox::TgBox(KnowledgeBase* pKB) : TBoxBase(pKB)
{
}

TgBox::~TgBox()
{
}

void TgBox::internalize()
{
  START_DECOMMENT2("TgBox::internalize");
  if( m_mapTermHash.size() == 0 )
    {
      clearUC();
      END_DECOMMENT("TgBox::internalize");
      return;
    }

  clearUC();
  
  for(ExprNode2TermDefinition::iterator i = m_mapTermHash.begin(); i != m_mapTermHash.end(); i++ )
    {
      TermDefinition* pTD = (TermDefinition*)i->second;
      for(ExprNodeSet::iterator j = pTD->m_setSubClassAxioms.begin(); j != pTD->m_setSubClassAxioms.end(); j++ )
	{
	  ExprNode* pSubClassAxiom = (ExprNode*)*j;
	  printExprNodeWComment("SubAxiom=", pSubClassAxiom);
	  
	  ExprNode* pC1 = (ExprNode*)pSubClassAxiom->m_pArgs[0];
	  ExprNode* pC2 = (ExprNode*)pSubClassAxiom->m_pArgs[1];
	  ExprNode* pNotC1 = createExprNode(EXPR_NOT, pC1);
	  ExprNode* pNotC1OrC2 = createExprNode(EXPR_OR, pNotC1, pC2);

	  ExprNodeSet* pExplanation;
	  if( PARAMS_USE_TRACING() )
	    pExplanation = m_pKB->m_pTBox->getAxiomExplanation(pSubClassAxiom);
	  else
	    pExplanation = new ExprNodeSet;

	  Expr2ExprSetPair* pNewPair = new Expr2ExprSetPair;
	  pNewPair->first = normalize(pNotC1OrC2);
	  pNewPair->second = pExplanation;
	  m_UC.push_back(pNewPair);

	  printExprNodeWComment("New UC Entry(via SubClass)=", (ExprNode*)pNewPair->first);
	}

      for(ExprNodeSet::iterator j = pTD->m_setEqClassAxioms.begin(); j != pTD->m_setEqClassAxioms.end(); j++ )
	{
	  ExprNode* pEqClassAxiom = (ExprNode*)*j;
	  printExprNodeWComment("EqClassAxiom=", pEqClassAxiom);
	  
	  ExprNode* pC1 = (ExprNode*)pEqClassAxiom->m_pArgs[0];
	  ExprNode* pC2 = (ExprNode*)pEqClassAxiom->m_pArgs[1];
	  ExprNode* pNotC1 = createExprNode(EXPR_NOT, pC1);
	  ExprNode* pNotC2 = createExprNode(EXPR_NOT, pC2);
	  ExprNode* pNotC1OrC2 = createExprNode(EXPR_OR, pNotC1, pC2);
	  ExprNode* pNotC2OrC1 = createExprNode(EXPR_OR, pNotC2, pC1);

	  ExprNodeSet* pExplanation;
	  if( PARAMS_USE_TRACING() )
	    pExplanation = m_pKB->m_pTBox->getAxiomExplanation(pEqClassAxiom);
	  else
	    pExplanation = new ExprNodeSet;
	  
	  Expr2ExprSetPair* pNewPair = new Expr2ExprSetPair;
	  pNewPair->first = normalize(pNotC1OrC2);
	  pNewPair->second = pExplanation;
	  m_UC.push_back(pNewPair);
	  printExprNodeWComment("New UC Entry(via EqClass)=", (ExprNode*)pNewPair->first);

	  pNewPair = new Expr2ExprSetPair;
	  pNewPair->first = normalize(pNotC2OrC1);
	  pNewPair->second = pExplanation;
	  m_UC.push_back(pNewPair);
	  printExprNodeWComment("New UC Entry(via EqClass)=", (ExprNode*)pNewPair->first);
	}
    }
  END_DECOMMENT("TgBox::internalize");
}

void TgBox::clearUC()
{
  m_UC.clear();
}

void TgBox::absorb(TuBox* pTuBox, TBox* pTBox)
{
  START_DECOMMENT2("TgBox::absorb");

  m_pTuBox = pTuBox;
  m_pTBox = pTBox;

  TermDefinitions listTerms;
  for(ExprNode2TermDefinition::iterator i = m_mapTermHash.begin(); i != m_mapTermHash.end(); i++ )
    {
      TermDefinition* pTD = (TermDefinition*)i->second;
      listTerms.push_back(pTD);
    }
  m_mapTermHash.clear();

  DECOMMENT1("TermCount = %d", listTerms.size());
  for(TermDefinitions::iterator i = listTerms.begin(); i != listTerms.end(); i++ )
    {
      TermDefinition* pTD = (TermDefinition*)*i;
      
      for(ExprNodeSet::iterator j = pTD->m_setSubClassAxioms.begin(); j != pTD->m_setSubClassAxioms.end(); j++ )
	{
	  ExprNode* pSubClassAxiom = (ExprNode*)*j;
	  ExprNode* pC1 = (ExprNode*)pSubClassAxiom->m_pArgs[0];
	  ExprNode* pC2 = (ExprNode*)pSubClassAxiom->m_pArgs[1];
	  
	  absorbSubClass(pC1, pC2, pTBox->getAxiomExplanation(pSubClassAxiom));
	}
      for(ExprNodeSet::iterator j = pTD->m_setEqClassAxioms.begin(); j != pTD->m_setEqClassAxioms.end(); j++ )
	{
	  ExprNode* pEqClassAxiom = (ExprNode*)*j;
	  ExprNode* pC1 = (ExprNode*)pEqClassAxiom->m_pArgs[0];
	  ExprNode* pC2 = (ExprNode*)pEqClassAxiom->m_pArgs[1];
	  
	  absorbSubClass(pC1, pC2, pTBox->getAxiomExplanation(pEqClassAxiom));
	  absorbSubClass(pC2, pC1, pTBox->getAxiomExplanation(pEqClassAxiom));
	}
    }
  END_DECOMMENT("TgBox::absorb");
}

void TgBox::absorbSubClass(ExprNode* pSub, ExprNode* pSup, ExprNodeSet* pAxiomExplanation)
{
  START_DECOMMENT2("TgBox::absorbSubClass");
  printExprNodeWComment("Sub=", pSub);
  printExprNodeWComment("Sup=", pSup);

  ExprNodeSet set;
  set.insert( nnf(pSub) );
  printExprNodeWComment("nnf(pSub)=", nnf(pSub));
  set.insert( nnf(createExprNode(EXPR_NOT, pSup)) );
  printExprNodeWComment("nnf(createExprNode(EXPR_NOT, pSup))=", nnf(createExprNode(EXPR_NOT, pSup)));

  // ***********************************
  // Explanation-related axiom tracking:
  // This is used in absorbII() where actual absorption takes place
  // with primitive definition
  m_setExplanation.clear();
  if( pAxiomExplanation->size() > 0 )
    m_setExplanation.insert(pAxiomExplanation->begin(), pAxiomExplanation->end());

  absorbTerm(&set);
  END_DECOMMENT("TgBox::absorbSubClass");
}

bool TgBox::absorbTerm(ExprNodeSet* pSet)
{
  START_DECOMMENT2("TgBox::absorbTerm");

  for(ExprNodeSet::iterator i = pSet->begin(); i != pSet->end(); i++ )
    {
      ExprNode* pName = (ExprNode*)*i;
      printExprNodeWComment("Term=", pName);
    }

  do
    {
      if( !PARAMS_USE_PSEUDO_NOMINALS() && 
	  ( PARAMS_USE_NOMINAL_ABSORPTION() || PARAMS_USE_HASVALUE_ABSORPTION() ) &&
	  absorbNominal(pSet) )
	{
	  END_DECOMMENT("TgBox::absorbTerm");
	  return TRUE;
	}

      if( absorb2(pSet) )
	{
	  END_DECOMMENT("TgBox::absorbTerm");
	  return TRUE;
	}
      if( absorb3(pSet) )
	continue;
      if( absorb5(pSet) )
	continue;
      if( absorb6(pSet) )
	{
	  END_DECOMMENT("TgBox::absorbTerm");
	  return TRUE;
	}
      if( PARAMS_USE_ROLE_ABSORPTION() && absorbRole(pSet) )
	{
	  END_DECOMMENT("TgBox::absorbTerm");
	  return TRUE;
	}

      absorb7(pSet);
      END_DECOMMENT("TgBox::absorbTerm");
      return FALSE;
    }
  while(1);
  END_DECOMMENT("TgBox::absorbTerm");
}

bool TgBox::absorbNominal(ExprNodeSet* pSet)
{
  START_DECOMMENT2("TgBox::absorbNominal");
  for(ExprNodeSet::iterator i = pSet->begin(); i != pSet->end(); i++ )
    {
      ExprNode* pName = (ExprNode*)*i;
      if( PARAMS_USE_NOMINAL_ABSORPTION() && (isOneOf(pName) || isNominal(pName)) )
	{
	  pSet->erase(i);
	  
	  ExprNodeList* pList = NULL;
	  if( isNominal(pName) )
	    pList = createExprNodeList(pName, 1);
	  else
	    pList = (ExprNodeList*)pName->m_pArgList;

	  ExprNode* pC = createExprNode(EXPR_NOT, createExprNode(EXPR_AND, convertExprSet2List(pSet)));
	  absorbOneOf(pList, pC, &m_setExplanation);
	  END_DECOMMENT("TgBox::absorbNominal");
	  return TRUE;
	}
      else if( PARAMS_USE_HASVALUE_ABSORPTION() && pName->m_iExpression == EXPR_VALUE )
	{
	  ExprNode* pP = (ExprNode*)pP->m_pArgs[0];
	  if( !m_pKB->isObjectProperty(pP) )
	    continue;

	  pSet->erase(i);
	  ExprNode* pC = createExprNode(EXPR_NOT, createExprNode(EXPR_AND, convertExprSet2List(pSet)));
	  ExprNode* pNominal = (ExprNode*)pName->m_pArgs[1];
	  ExprNode* pInd = (ExprNode*)pNominal->m_pArgs[0];
	  
	  ExprNode* pInvP = NULL;
	  if( m_pKB->getProperty(pP) && m_pKB->getProperty(pP)->m_pInverse )
	    pInvP = m_pKB->getProperty(pP)->m_pInverse->m_pName;
	  ExprNode* pAllInvPC = createExprNode(EXPR_ALL, pInvP, pC);

	  m_pKB->addIndividual(pInd);
	  m_pKB->addType(pInd, pAllInvPC, (new DependencySet(&m_setExplanation)));
	  END_DECOMMENT("TgBox::absorbNominal");
	  return TRUE;
	}
    }
  END_DECOMMENT("TgBox::absorbNominal");
  return FALSE;
}

bool TgBox::absorbRole(ExprNodeSet* pSet)
{
  START_DECOMMENT2("TgBox::absorbRole");
  for(ExprNodeSet::iterator i = pSet->begin(); i != pSet->end(); i++ )
    {
      ExprNode* pName = (ExprNode*)*i;
      if( pName->m_iExpression == EXPR_SOME )
	{
	  ExprNode* pR = (ExprNode*)pName->m_pArgs[0];
	  ExprNode* pDomain = createExprNode(EXPR_NOT, createExprNode(EXPR_AND, convertExprSet2List(pSet)));
	  m_pKB->addDomain(pR, pDomain, (new DependencySet(&m_setExplanation)));
	  m_setAbsorbedOutsideTBox.insert(m_setExplanation.begin(), m_setExplanation.end());
	  END_DECOMMENT("TgBox::absorbRole");
	  return TRUE;
	}
      else if( pName->m_iExpression == EXPR_MIN )
	{
	  printExprNodeWComment("MIN=", pName);
	  ExprNode* pR = (ExprNode*)pName->m_pArgs[0];
	  int n = ((ExprNode*)pName->m_pArgs[1])->m_iTerm;

	  // if we have min(r,1) sub ... this is also equal to a domain
	  // restriction
	  if( n == 1 ) 
	    {
	      pSet->erase(i);
	      ExprNode* pDomain = createExprNode(EXPR_NOT, createExprNode(EXPR_AND, convertExprSet2List(pSet)));
	      m_pKB->addDomain(pR, pDomain, (new DependencySet(&m_setExplanation)));
	      m_setAbsorbedOutsideTBox.insert(m_setExplanation.begin(), m_setExplanation.end());
	      END_DECOMMENT("TgBox::absorbRole");
	      return TRUE;
	    }
	}
    }
  END_DECOMMENT("TgBox::absorbRole");
  return FALSE;
}

bool TgBox::absorb2(ExprNodeSet* pSet)
{
  START_DECOMMENT2("TgBox::absorb2");
  for(ExprNodeSet::iterator i = pSet->begin(); i != pSet->end(); i++ )
    {
      ExprNode* pTerm = (ExprNode*)*i;
      TermDefinition* pTD = m_pTuBox->getTermDef(pTerm);
      bool bCanAbsorb;
      if( pTD )
	bCanAbsorb = (pTD->m_setEqClassAxioms.size()==0);
      else
	bCanAbsorb = (pTerm->m_iArity==0 && pSet->size()>1);

      if( bCanAbsorb )
	{
	  pSet->erase(i);
	  
	  ExprNode* pConjunct = createExprNode(EXPR_AND, convertExprSet2List(pSet));
	  pConjunct = createExprNode(EXPR_NOT, pConjunct);
	  ExprNode* pSub = createExprNode(EXPR_SUBCLASSOF, pTerm, nnf(pConjunct));
	  m_pTuBox->addDef(pSub);
	  m_pTBox->addAxiomExplanation(pSub, &m_setExplanation);
	  END_DECOMMENT("TgBox::absorb2");
	  return TRUE;
	}
    }
  END_DECOMMENT("TgBox::absorb2");
  return FALSE;
}

bool TgBox::absorb3(ExprNodeSet* pSet)
{
  START_DECOMMENT2("TgBox::absorb3");
  for(ExprNodeSet::iterator i = pSet->begin(); i != pSet->end(); i++ )
    {
      ExprNode* pTerm = (ExprNode*)*i;
      ExprNode* pNegatedTerm = NULL;
      TermDefinition* pTD = m_pTuBox->getTermDef(pTerm);
      if( pTD == NULL && isNegatedPrimitive(pTerm) )
	{
	  pNegatedTerm = (ExprNode*)pTerm->m_pArgs[0];
	  pTD = m_pTuBox->getTermDef(pNegatedTerm);
	}
      if( pTD == NULL )
	continue;
      
      if( pTD->m_setEqClassAxioms.size() > 0 )
	{
	  ExprNode* pEqClassAxiom = (ExprNode*)(*pTD->m_setEqClassAxioms.begin());
	  ExprNode* pEqClass = (ExprNode*)pEqClassAxiom->m_pArgs[1];
	  printExprNodeWComment("RemoveFromSet=", pTerm);
	  pSet->erase(i);
	  if( pNegatedTerm == NULL )
	    {
	      printExprNodeWComment("Add2Set=", pEqClass);
	      pSet->insert(pEqClass);
	    }
	  else
	    {
	      printExprNodeWComment("Add2Set(neg)=", negate2(pEqClass));
	      pSet->insert(negate2(pEqClass));
	    }

	  // Explanation-related tracking of axioms
	  ExprNodeSet* pAxiomExplanation = m_pTBox->getAxiomExplanation(pEqClassAxiom);
	  m_setExplanation.insert(pAxiomExplanation->begin(), pAxiomExplanation->end());
	  END_DECOMMENT("TgBox::absorb3");
	  return TRUE;
	}
    }
  END_DECOMMENT("TgBox::absorb3");
  return FALSE;
}

bool TgBox::absorb5(ExprNodeSet* pSet)
{
  START_DECOMMENT2("TgBox::absorb5");
  for(ExprNodeSet::iterator i = pSet->begin(); i != pSet->end(); i++ )
    {
      ExprNode* pTerm = (ExprNode*)*i;
      ExprNode* pNNFTerm = nnf(pTerm);
      if( pNNFTerm->m_iExpression == EXPR_AND )
	{
	  pSet->erase(i);
	  ExprNodeList* pAndList = (ExprNodeList*)pNNFTerm->m_pArgList;
	  for(int j = 0; j < pAndList->m_iUsedSize; j++ )
	    {
	      printExprNodeWComment("Add2Set=", pAndList->m_pExprNodes[j]);
	      pSet->insert(pAndList->m_pExprNodes[j]);
	    }
	  END_DECOMMENT("TgBox::absorb5");
	  return TRUE;
	}
    }
  END_DECOMMENT("TgBox::absorb5");
  return FALSE;
}

bool TgBox::absorb6(ExprNodeSet* pSet)
{
  START_DECOMMENT2("TgBox::absorb6");
  for(ExprNodeSet::iterator i = pSet->begin(); i != pSet->end(); i++ )
    {
      ExprNode* pTerm = (ExprNode*)*i;
      ExprNode* pNNFTerm = nnf(pTerm);
      if( pNNFTerm->m_iExpression == EXPR_OR )
	{
	  pSet->erase(i);
	  printExprNodeWComment("Absorb OR=", pNNFTerm);

	  ExprNodeList* pOrList = (ExprNodeList*)pNNFTerm->m_pArgList;
	  for(int j = 0; j < pOrList->m_iUsedSize; j++ )
	    {
	      ExprNodeSet* pCloned = new ExprNodeSet;
	      pCloned->insert(pSet->begin(), pSet->end());
	      pCloned->insert(pOrList->m_pExprNodes[j]);
	      absorbTerm(pCloned);
	    }
	  END_DECOMMENT("TgBox::absorb6");
	  return TRUE;
	}
    }
  END_DECOMMENT("TgBox::absorb6");
  return FALSE;
}

bool TgBox::absorb7(ExprNodeSet* pSet)
{
  START_DECOMMENT2("TgBox::absorb7");
  ExprNodeList* pList = convertExprSet2List(pSet);
  ExprNode* pSub = nnf(pList->m_pExprNodes[0]);
  pop_front(pList);
  
  ExprNode* pSup = (pList->m_iUsedSize==0)?createExprNode(EXPR_NOT, pSub):createExprNode(EXPR_NOT, createExprNode(EXPR_AND, pList));

  pSup = nnf(pSup);

  ExprNode* pSubClassAxiom = createExprNode(EXPR_SUBCLASSOF, pSub, pSup);
  addDef(pSubClassAxiom);
  m_pTBox->addAxiomExplanation(pSubClassAxiom, &m_setExplanation);

  END_DECOMMENT("TgBox::absorb7");
  return TRUE;
}

void TgBox::absorbOneOf(ExprNode* pExpr, ExprNode* pExprC, ExprNodeSet* pExplanation)
{
  if( pExpr->m_pArgList )
    absorbOneOf((ExprNodeList*)pExpr->m_pArgList, pExprC, pExplanation);
}

void TgBox::absorbOneOf(ExprNodeList* pExprOneOf, ExprNode* pExprC, ExprNodeSet* pExplanation)
{
  if( PARAMS_USE_PSEUDO_NOMINALS() )
    {
      //log.warn( "Ignoring axiom involving nominals: " + explain );
      return;
    }
  
  m_setAbsorbedOutsideTBox.insert(pExplanation->begin(), pExplanation->end());

  DependencySet* pDS = new DependencySet(pExplanation);

  // choose either a list or an argument
  for(int i = 0; i < pExprOneOf->m_iUsedSize; i++ )
    {
      ExprNode* pE = pExprOneOf->m_pExprNodes[i];
      ExprNode* pNominal = pE;
      ExprNode* pIndividual = (ExprNode*)pE->m_pArgs[0];
      m_pKB->addIndividual(pIndividual);
      m_pKB->addType(pIndividual, pExprC, pDS);
    }
}
