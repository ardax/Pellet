#include "Expressivity.h"
#include "KnowledgeBase.h"
#include "TBox.h"
#include "ABox.h"
#include "RBox.h"
#include "Role.h"
#include "Node.h"
#include "Individual.h"

#include <cassert>

extern ExprNode* EXPRNODE_TOP;
extern ExprNode* EXPRNODE_BOTTOM;

Expressivity::Expressivity(KnowledgeBase* pKB)
{
  m_pKB = pKB;
  for(int i = 0; i < NUMOF_EXPRESSIVES; i++ )
    m_bExpressivity[i] = FALSE;
}

Expressivity::Expressivity(Expressivity* pExpressivity)
{
  m_pKB = pExpressivity->m_pKB;
  for(int i = 0; i < NUMOF_EXPRESSIVES; i++ )
    m_bExpressivity[i] = pExpressivity->m_bExpressivity[i];
}

bool Expressivity::hasNominal()
{
  return (m_setNominals.size()>0);
}

void Expressivity::compute()
{
  processIndividuals();
  processClasses();
  processRoles();
}

Expressivity* Expressivity::compute(ExprNode* pC)
{
  if( pC == NULL )
    return this;
  Expressivity* pExpressivity = new Expressivity(this);
  pExpressivity->visit(pC);
  return pExpressivity;
}

void Expressivity::processClasses()
{
  Expr2ExprSetPairList* pUC = m_pKB->m_pTBox->getUC();
  if( pUC )
    {
      m_bExpressivity[EX_NEGATION] = TRUE;
      for(Expr2ExprSetPairList::iterator i = pUC->begin(); i != pUC->end(); i++ )
	{
	  Expr2ExprSetPair* pPair = (Expr2ExprSetPair*)*i;
	  visit(pPair->first);
	}
    }

  ExprNodeSet setAllClasses;
  m_pKB->m_pTBox->getAllClasses(&setAllClasses);

  for(ExprNodeSet::iterator i = setAllClasses.begin(); i != setAllClasses.end(); i++ )
    {
      ExprNode* pC = (ExprNode*)*i;
      Expr2ExprSetPairList* pUnfoldC = m_pKB->m_pTBox->unfold(pC);
      if( pUnfoldC == NULL )
	continue;
      for(Expr2ExprSetPairList::iterator j = pUnfoldC->begin(); j != pUnfoldC->end(); j++ )
	{
	  Expr2ExprSetPair* pPair = (Expr2ExprSetPair*)*j;
	  visit(pPair->first);
	}
    }
}

void Expressivity::processIndividuals()
{
  for(ExprNodes::iterator i = m_pKB->m_pABox->m_aNodeList.begin(); i != m_pKB->m_pABox->m_aNodeList.end(); i++ )
    {
      ExprNode* pExprNode = (ExprNode*)*i;
      Individual* pIndividual = NULL;
      Expr2NodeMap::iterator iFind = m_pKB->m_pABox->m_mNodes.find(pExprNode);
      if( iFind != m_pKB->m_pABox->m_mNodes.end() )
	{
	  pIndividual = (Individual*)iFind->second;
	  ExprNode* pNominal = createExprNode(EXPR_VALUE, pIndividual->m_pName);
	  for(ExprNode2DependencySetMap::iterator j = pIndividual->m_mDepends.begin(); j != pIndividual->m_mDepends.end(); j++ )
	    {
	      ExprNode* pTerm = (ExprNode*)j->first;
	      if( isEqual(pTerm, pNominal) )
		continue;
	      visit(pTerm);
	    }
	}
    }
}

/**
 * Added for incremental reasoning. Given an aterm corresponding to an
 * individual and concept, the expressivity is updated accordingly.
 */
void Expressivity::processIndividual(ExprNode* pI, ExprNode* pConcept)
{
  ExprNode* pNominal = createExprNode(EXPR_VALUE, pI);
  if( isEqual(pConcept, pNominal) )
    return;
  visit(pConcept);
}

void Expressivity::processRoles()
{
  for(Expr2RoleMap::iterator i = m_pKB->m_pRBox->m_mRoles.begin(); i != m_pKB->m_pRBox->m_mRoles.end(); i++ )
    {
      Role* pRole = (Role*)i->second;
      if( pRole->isDatatypeRole() )
	{
	  m_bExpressivity[EX_DATATYPE] = TRUE;
	  if( pRole->isInverseFunctional() )
	    m_bExpressivity[EX_KEYS] = TRUE;
	}

      if( pRole->isAnon() && m_bExpressivity[EX_INVERSE] == FALSE )
	{
	  for(RoleSet::iterator j = pRole->m_setSubRoles.begin(); j != pRole->m_setSubRoles.end(); j++ )
	    {
	      Role* pSubRole = (Role*)*j;
	      if( !pSubRole->isAnon() )
		{
		  m_bExpressivity[EX_INVERSE] = TRUE;
		  break;
		}
	    }
	}

      // InverseFunctionalProperty declaration may mean that a named
      // property has an anonymous inverse property which is functional
      // The following condition checks this case
      if( pRole->isAnon() && pRole->isFunctional() )
	m_bExpressivity[EX_INVERSE] = TRUE;
      if( pRole->isFunctional() )
	m_bExpressivity[EX_FUNCTIONALITY] = TRUE;
      if( pRole->isTransitive() )
	m_bExpressivity[EX_TRANSITIVITY] = TRUE;
      if( pRole->isReflexive() )
	m_bExpressivity[EX_REFLEXIVITY] = TRUE;
      if( pRole->isIrreflexive() )
	m_bExpressivity[EX_IRREFLEXIVITY] = TRUE;
      if( pRole->isAntisymmetric() )
	m_bExpressivity[EX_ANTISYMMETRY] = TRUE;
      if( pRole->m_setDisjointRoles.size() != 0 )
	m_bExpressivity[EX_DISJOINTROLES] = TRUE;
      if( pRole->hasComplexSubRole() ) 
	m_bExpressivity[EX_COMPLEXSUBROLES] = TRUE;

      // Each property has itself included in the subroles set. We need
      // at least two properties in the set to conclude there is a role
      // hierarchy defined in the ontology
      if( pRole->m_setSubRoles.size() > 1 )
	m_bExpressivity[EX_ROLEHIERARCHY] = TRUE;

      if( pRole->m_pDomain )
	{
	  m_bExpressivity[EX_DOMAIN] |= !(isEqual(pRole->m_pDomain, EXPRNODE_TOP));
	  visit( pRole->m_pDomain );
	}

      if( pRole->m_pRange )
	{
	  m_bExpressivity[EX_RANGE] |= !(isEqual(pRole->m_pRange, EXPRNODE_TOP));
	  visit( pRole->m_pRange );
	}
    }
}

void Expressivity::visit(ExprNode* pExprNode)
{
  if( isEqual(pExprNode, EXPRNODE_TOP) )
    {
      // do nothing
      // visitTerm(OWL_THING);
    }
  else if( isEqual(pExprNode, EXPRNODE_BOTTOM) )
    {
      // do nothing
      // visitTerm(OWL_NOTHING);
    }
  else if( pExprNode->m_iArity == 0 )
    {
      // do nothing
      // visitTerm(pExprNode);
    }

  switch(pExprNode->m_iExpression)
    {
    case EXPR_AND:
      {
	visitList((ExprNodeList*)pExprNode->m_pArgList);
	break;
      }
    case EXPR_OR:
      {
	if( isOneOf(pExprNode) )
	  {
	    // visitOneOf
	    m_bExpressivity[EX_NEGATION] = TRUE;
	    visitList((ExprNodeList*)pExprNode->m_pArgList);
	  }
	else
	  {
	    // visitOr
	    m_bExpressivity[EX_NEGATION] = TRUE;
	    visitList((ExprNodeList*)pExprNode->m_pArgList);
	  }
	break;
      } 
    case EXPR_NOT:
      {
	//visitNot(pExprNode);
	m_bExpressivity[EX_NEGATION] = TRUE;
	visit((ExprNode*)pExprNode->m_pArgs[0]);
	break;
      }
    case EXPR_ALL:
      {
	//visitAll(pExprNode);
	visitRole((ExprNode*)pExprNode->m_pArgs[0]);
	visit((ExprNode*)pExprNode->m_pArgs[1]);
	break;
      }
    case EXPR_SOME:
      {
	if( isHasValue(pExprNode) )
	  {
	    //visitHasValue(pExprNode);
	    visitRole((ExprNode*)pExprNode->m_pArgs[0]);
	    visit((ExprNode*)pExprNode->m_pArgs[1]);
	  }
	else
	  {
	    //visitSome(pExprNode);
	    visitRole((ExprNode*)pExprNode->m_pArgs[0]);
	    visit((ExprNode*)pExprNode->m_pArgs[1]);
	  }
	break;
      }
    case EXPR_MIN:
      {
	visitMin(pExprNode);
	break;
      }
    case EXPR_MAX:
      {
	visitMax(pExprNode);
	break;
      }
    case EXPR_CARD:
      {
	//visitCard(pExprNode);
	visitMin(pExprNode);
	visitMax(pExprNode);
	break;
      }
    case EXPR_VALUE:
      {
	ExprNode* pNominal = (ExprNode*)pExprNode->m_pArgs[0];
	if( !isLiteral(pNominal) )
	  m_setNominals.insert(pNominal);
	break;
      }
    case EXPR_LITERAL:
      {
	// do nothing here
	break;
      }
    case EXPR_SELF:
      {
	m_bExpressivity[EX_REFLEXIVITY] = TRUE;
	m_bExpressivity[EX_IRREFLEXIVITY] = TRUE;
	break;
      }
    };
}

void Expressivity::visitList(ExprNodeList* pExprNodeList)
{
  for(int i = 0; i < pExprNodeList->m_iUsedSize; i++ )
    visit(pExprNodeList->m_pExprNodes[i]);
}

void Expressivity::visitRole(ExprNode* pExprNode)
{
  if( !isPrimitive(pExprNode) )
    m_bExpressivity[EX_INVERSE] = TRUE;
}

void Expressivity::visitMin(ExprNode* pExprNode)
{
  visitRole((ExprNode*)pExprNode->m_pArgs[0]);
  int iCardinality = ((ExprNode*)pExprNode->m_pArgs[1])->m_iTerm;
  ExprNode* pC = (ExprNode*)pExprNode->m_pArgs[2];
  if( !isTop(pC) )
    m_bExpressivity[EX_CARDINALITYQ] = TRUE;
  else if( iCardinality > 2 )
    {
      m_bExpressivity[EX_CARDINALITY] = TRUE;
      if( m_pKB->getRole((ExprNode*)pExprNode->m_pArgs[0])->isDatatypeRole() )
	m_bExpressivity[EX_CARDINALITYD] = TRUE;
    }
  else if( iCardinality > 0 )
    {
      m_bExpressivity[EX_FUNCTIONALITY] = TRUE;
      if( m_pKB->getRole((ExprNode*)pExprNode->m_pArgs[0])->isDatatypeRole() )
	m_bExpressivity[EX_FUNCTIONALITYD] = TRUE;
    }
}
void Expressivity::visitMax(ExprNode* pExprNode)
{
  visitRole((ExprNode*)pExprNode->m_pArgs[0]);
  int iCardinality = ((ExprNode*)pExprNode->m_pArgs[1])->m_iTerm;
  ExprNode* pC = (ExprNode*)pExprNode->m_pArgs[2];
  if( !isTop(pC) )
    m_bExpressivity[EX_CARDINALITYQ] = TRUE;
  else if( iCardinality > 1 )
    m_bExpressivity[EX_CARDINALITY] = TRUE;
  else if( iCardinality > 0 )
    m_bExpressivity[EX_FUNCTIONALITY] = TRUE;
}

void Expressivity::printExpressivity()
{
  string sDL = "";

  if( hasNegation() ) sDL = "ALC"; else sDL = "AL";
  if( hasTransitivity() ) sDL += "R+";
  if( sDL == "ALCR+" ) sDL = "S";
  if( hasComplexSubRoles() ) sDL = "SR"; else if( hasRoleHierarchy() ) sDL += "H";
  if( hasNominal() ) sDL += "O";
  if( hasInverse() ) sDL += "I";
  if( hasCardinalityQ() ) sDL += "Q"; else if( hasCardinality() ) sDL += "N"; else if( hasFunctionality() ) sDL += "F";

  if( hasDataType() )
    {
      if( hasKeys() )
	sDL += "(Dk)";
      else
	sDL += "(D)";
    }

  printf("Expressivity : %s\n", sDL.c_str());
}
