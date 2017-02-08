#include "Blocking.h"
#include "Individual.h"
#include "Node.h"
#include "KnowledgeBase.h"
#include "TaxonomyNode.h"
#include "RBox.h"
#include "Taxonomy.h"

extern KnowledgeBase* g_pKB;

Blocking::Blocking()
{

}

bool Blocking::equals(Individual* pX, Individual* pY)
{
  ExprNodeSet setXTypes, setYTypes;

  for(ExprNode2DependencySetMap::iterator i = pX->m_mDepends.begin(); i != pX->m_mDepends.end(); i++ )
    setXTypes.insert((ExprNode*)i->first);
  for(ExprNode2DependencySetMap::iterator i = pY->m_mDepends.begin(); i != pY->m_mDepends.end(); i++ )
    setYTypes.insert((ExprNode*)i->first);
  
  return ::isEqualSet(&setXTypes, &setYTypes);
}

bool Blocking::subset(Individual* pX, Individual* pY)
{
  ExprNodeSet setXTypes, setYTypes;

  for(ExprNode2DependencySetMap::iterator i = pX->m_mDepends.begin(); i != pX->m_mDepends.end(); i++ )
    setXTypes.insert((ExprNode*)i->first);
  for(ExprNode2DependencySetMap::iterator i = pY->m_mDepends.begin(); i != pY->m_mDepends.end(); i++ )
    setYTypes.insert((ExprNode*)i->first);

  return containsAll(&setXTypes, &setYTypes);
}

bool Blocking::isBlocked(Individual* pX)
{
  if( pX->isNominal() )
    return FALSE;
  
  Nodes aAncestors;
  pX->getAncestors(&aAncestors);

  return isIndirectlyBlocked(&aAncestors) || isDirectlyBlocked(pX, &aAncestors);
}

bool Blocking::isIndirectlyBlocked(Individual* pX)
{
  if( pX->isNominal() )
    return FALSE;

  Nodes aAncestors;
  pX->getAncestors(&aAncestors);
  return isIndirectlyBlocked(&aAncestors);
}

bool Blocking::isIndirectlyBlocked(Nodes* pAncestors)
{
  for(Nodes::iterator i = pAncestors->begin(); i != pAncestors->end(); i++ )
    {
      Individual* pNode = (Individual*)*i;
      Nodes aAncestors;
      pNode->getAncestors(&aAncestors);
      if( isDirectlyBlocked(pNode, &aAncestors) )
	return TRUE;
    }
  return FALSE;
}

/*******************************************************/
/*******************************************************/
/*******************************************************/

bool SubsetBlocking::isDirectlyBlocked(Individual* pX, Nodes* pAncestors)
{
  for(Nodes::iterator i = pAncestors->begin(); i != pAncestors->end(); i++ )
    {
      Individual* pNode = (Individual*)*i;
      if( !pNode->isNominal() && subset(pX, pNode) )
	return TRUE;
    }
  return FALSE;
}

/*******************************************************/
/*******************************************************/
/*******************************************************/

bool DoubleBlocking::isDirectlyBlocked(Individual* pX, Nodes* pAncestors)
{
  // 1) x has ancestors x1, y and y1
  // 2) x is a successor of x1 and y is a successor of y1
  // 3) y, x and all nodes in between are blockable
  // 4) types(x) == types(y) && types(x1) == types(y1)
  // 5) edges(x1, x) == edges(y1, y)
  
  // FIXME can y1 be a nominal? (assumption: yes)
  // FIXME can y and x1 be same? (assumption: no)

  // we need at least two ancestors (y1 can be a nominal so it
  // does no need to be included in the ancestors list)
  if( pAncestors->size() < 2 )
    return FALSE;

  Individual* pX1 = NULL;
  for(Nodes::iterator i = pAncestors->begin(); i != pAncestors->end(); i++ )
    {
      Individual* pY = (Individual*)*i;

      if( pX1 == NULL )
	{
	  // first element is guaranteed to be x's predecessor 
	  pX1 = pY;
	  continue;
	}
      // if this is concept satisfiability then y might be a root node but
      // not necessarily a nominal and included in the ancestors list
      if( pY->isRoot() )
	return FALSE;

      // y1 is not necessarily in the ancestors list (it might be a nominal)
      Individual* pY1 = pY->getParent();
      
      RoleSet setXEdges, setYEdges;
      pX->m_listInEdges.getRoles(&setXEdges); // all the incoming edges should be coming from x1
      pY->m_listInEdges.getRoles(&setXEdges); // all the incoming edges should be coming from y1

      if( equals(pX,  pY) && equals(pY1, pX1) && ::isEqualSet(&setXEdges, &setYEdges) )
	return TRUE;
    }

  return FALSE;
}

/*******************************************************/
/*******************************************************/
/*******************************************************/

bool OptimizedDoubleBlocking::isDirectlyBlocked(Individual* pW, Nodes* pAncestors)
{
  NodeSet aPredecessors;
  pW->getPredecessors(&aPredecessors);
  
  Nodes::iterator iAncestors = pAncestors->begin();

  for(NodeSet::iterator i = aPredecessors.begin(); i != aPredecessors.end(); i++ )
    {
      Individual* pV = (Individual*)*i;
      for(; iAncestors != pAncestors->end(); iAncestors++ )
	{
	  Individual* pW1 = (Individual*)*iAncestors;
	  if( isEqual(pV, pW1) )
	    continue;
	  
	  bool b1Andb2 = block1(pW, pW1) && block2(pW, pV, pW1);
	  bool aBlock = b1Andb2 && block3(pW, pV, pW1) && block4(pW, pV, pW1);
	  
	  if( aBlock )
	    return TRUE;
	  
	  bool cBlock = b1Andb2 && block5(pW, pV, pW1) && block6(pW, pV);
	  if( cBlock )
	    return TRUE;
	}
    }
  return FALSE;
}

bool OptimizedDoubleBlocking::block1(Individual* pW, Individual* pW1)
{
  return subset(pW, pW1);
}

bool OptimizedDoubleBlocking::block2(Individual* pW, Individual* pV, Individual* pW1)
{
  for(ExprNodes::iterator j = pW1->m_aTypes[Node::ALL].begin(); j != pW1->m_aTypes[Node::ALL].end(); j++ )
    {
      ExprNode* pAV = (ExprNode*)*j;
      Role* pRole = g_pKB->getRole((ExprNode*)pAV->m_pArgs[0]);
      ExprNode* pC = (ExprNode*)pAV->m_pArgs[1];

      if( pRole->isDatatypeRole() )
	continue;

      Role* pInverseRole = pRole->m_pInverse;
      if( pV->hasRSuccessor(pInverseRole, pW) )
	{
	  if( !pV->hasType(pC) )
	    return FALSE;

	  for(RoleSet::iterator i = pRole->m_setSubRoles.begin(); i != pRole->m_setSubRoles.end(); i++ )
	    {
	      Role* pR = (Role*)*i;
	      if( !pR->isTransitive() )
		continue;

	      Role* pInvR = pR->m_pInverse;
	      if( pV->hasRSuccessor(pInvR, pW) )
		{
		  // we want to check if all(r,c) exists in v but it is possible that r has equivalent
		  // properties so we might find all(r1,c) where r1 equivalentProperty r. to get the 
		  // equivalents we need to use the hierarchy node because hierarchy.getAllEquivalents
		  // will filter anon roles that might be needed in this case 
		  bool bHasAllRC = FALSE;
		  TaxonomyNode* pTNode = g_pKB->m_pRBox->getTaxonomy()->getNode(pR->m_pName);
		  for(ExprNodeSet::iterator k = pTNode->m_setEquivalents.begin(); k != pTNode->m_setEquivalents.end(); k++ )
		    {
		      ExprNode* pEQR = (ExprNode*)*k;
		      ExprNode* pAllRC = createExprNode(EXPR_ALL, pEQR, pC);
		      if( pV->hasType(pAllRC) )
			{
			  bHasAllRC = TRUE;
			  break;
			}
		    }

		  if( !bHasAllRC )
		    return FALSE;
		}
	    }
	}
    }
  return TRUE;
}

bool OptimizedDoubleBlocking::block3(Individual* pW, Individual* pV, Individual* pW1)
{
  for(ExprNodes::iterator j = pW1->m_aTypes[Node::MAX].begin(); j != pW1->m_aTypes[Node::MAX].end(); j++ )
    {
      ExprNode* pNormMax = (ExprNode*)*j;
      ExprNode* pMax = (ExprNode*)pNormMax->m_pArgs[0];
      Role* pRole = g_pKB->getRole((ExprNode*)pMax->m_pArgs[0]);
      int iN = ((ExprNode*)pMax->m_pArgs[1])->m_iTerm-1;
      ExprNode* pC = (ExprNode*)pMax->m_pArgs[2];

      if( pRole->isDatatypeRole() )
	continue;

      Role* pInvRole = pRole->m_pInverse;
      if( pV->hasRSuccessor(pInvRole, pW) && pV->hasType(pC) )
	{
	  NodeSet setRSuccessors;
	  pW1->getRSuccessors(pRole, pC, &setRSuccessors);
	  if( setRSuccessors.size() >= iN )
	    return FALSE;
	}
    }
  return TRUE;
}

bool OptimizedDoubleBlocking::block4(Individual* pW, Individual* pV, Individual* pW1)
{
  for(ExprNodes::iterator j = pW1->m_aTypes[Node::MIN].begin(); j != pW1->m_aTypes[Node::MIN].end(); j++ )
    {
      ExprNode* pMin = (ExprNode*)*j;
      Role* pRole = g_pKB->getRole((ExprNode*)pMin->m_pArgs[0]);
      int iN = ((ExprNode*)pMin->m_pArgs[1])->m_iTerm;
      ExprNode* pC = (ExprNode*)pMin->m_pArgs[2];
      
      if( pRole->isDatatypeRole() )
	continue;

      Role* pInvRole = pRole->m_pInverse;
      NodeSet setRSuccessors;
      pW1->getRSuccessors(pRole, pC, &setRSuccessors);
      if( setRSuccessors.size() >= iN )
	continue;

      if( pV->hasRSuccessor(pInvRole, pW) && pV->hasType(pC) )
	continue;
      return FALSE;
    }
  
  for(ExprNodes::iterator j = pW1->m_aTypes[Node::SOME].begin(); j != pW1->m_aTypes[Node::SOME].end(); j++ )
    {
      ExprNode* pNormSome = (ExprNode*)*j;
      ExprNode* pSome = (ExprNode*)pNormSome->m_pArgs[0];
      Role* pRole = g_pKB->getRole((ExprNode*)pSome->m_pArgs[0]);
      ExprNode* pC = (ExprNode*)pSome->m_pArgs[1];
      pC = negate2(pC);

      if( pRole->isDatatypeRole() )
	continue;

      Role* pInvRole = pRole->m_pInverse;
      NodeSet setRSuccessors;
      pW1->getRSuccessors(pRole, pC, &setRSuccessors);
      if( setRSuccessors.size() >= 1 )
	continue;

      if( pV->hasRSuccessor(pInvRole, pW) && pV->hasType(pC) )
	continue;
      return FALSE;
    }

  return TRUE;
}

bool OptimizedDoubleBlocking::block5(Individual* pW, Individual* pV, Individual* pW1)
{
  for(ExprNodes::iterator j = pW1->m_aTypes[Node::MAX].begin(); j != pW1->m_aTypes[Node::MAX].end(); j++ )
    {
      ExprNode* pNormMax = (ExprNode*)*j;
      ExprNode* pMax = (ExprNode*)pNormMax->m_pArgs[0];
      Role* pRole = g_pKB->getRole((ExprNode*)pMax->m_pArgs[0]);
      ExprNode* pC = (ExprNode*)pMax->m_pArgs[2];

      if( pRole->isDatatypeRole() )
	continue;
      
      Role* pInvRole = pRole->m_pInverse;
      if( pV->hasRSuccessor(pInvRole, pW) && pV->hasType(pC) )
	return FALSE;
    }
  
  return TRUE;
}

bool OptimizedDoubleBlocking::block6(Individual* pW, Individual* pW1)
{
  for(ExprNodes::iterator j = pW1->m_aTypes[Node::MIN].begin(); j != pW1->m_aTypes[Node::MIN].end(); j++ )
    {
      ExprNode* pMin = (ExprNode*)*j;
      Role* pRole = g_pKB->getRole((ExprNode*)pMin->m_pArgs[0]);
      ExprNode* pC = (ExprNode*)pMin->m_pArgs[2];
      
      if( pRole->isDatatypeRole() )
	continue;

      if( pW1->hasRSuccessor(pRole, pW) && pW->hasType(pC) )
	return FALSE;
    }
  
  return TRUE;
}
