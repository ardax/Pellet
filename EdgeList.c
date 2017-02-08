#include "EdgeList.h"
#include "Individual.h"
#include "Node.h"
#include "Role.h"
#include "Literal.h"
#include "DependencySet.h"

extern DependencySet DEPENDENCYSET_INDEPENDENT;

EdgeList::EdgeList()
{

}

void EdgeList::addEdge(Edge* pEdge)
{
  m_listEdge.push_back(pEdge);
}

bool EdgeList::removeEdge(Edge* pEdge)
{
  for(EdgeVector::iterator i = m_listEdge.begin(); i != m_listEdge.end(); i++ )
    {
      Edge* pEdge2 = (Edge*)*i;
      if( pEdge2 == pEdge )
	{
	  m_listEdge.erase(i);
	  return TRUE;
	}
    }
  return FALSE;
}

void EdgeList::findEdges(Role* pRole, Individual* pFrom, Node* pTo, EdgeList* pEdgeList)
{
  for(EdgeVector::iterator i = m_listEdge.begin(); i != m_listEdge.end(); i++ )
    {
      Edge* pEdge = (Edge*)*i;
      if( (pFrom == NULL || isEqual(pFrom, pEdge->m_pFrom) == 0) &&
	  (pRole == NULL || pEdge->m_pRole->isSubRoleOf(pRole)) &&
	  (pTo == NULL || isEqual(pTo, pEdge->m_pTo) == 0 ) )
	pEdgeList->addEdge(pEdge);
    }
}

void EdgeList::getEdges(Role* pRole, EdgeList* pEdgeList)
{
  for(EdgeVector::iterator i = m_listEdge.begin(); i != m_listEdge.end(); i++ )
    {
      Edge* pEdge = (Edge*)*i;
      if( pEdge->m_pRole->isSubRoleOf(pRole) )
	pEdgeList->addEdge(pEdge);
    }
}

void EdgeList::getEdgesTo(Node* pTo, EdgeList* pEdgeList)
{
  findEdges(NULL, NULL, pTo, pEdgeList);
}

void EdgeList::getNeighbors(Node* pNode, void* pSetNeighbors)
{
  for(EdgeVector::iterator i = m_listEdge.begin(); i != m_listEdge.end(); i++ )
    {
      Edge* pEdge = (Edge*)*i;
      ((NodeSet*)pSetNeighbors)->insert(pEdge->getNeighbor(pNode));
    }
}

bool EdgeList::hasEdge(Role* pRole)
{
  return hasEdge(NULL, pRole, NULL);
}

bool EdgeList::hasEdge(Edge* pEdge)
{
  return hasEdge(pEdge->m_pFrom, pEdge->m_pRole, pEdge->m_pTo);
}

bool EdgeList::hasEdge(Individual* pFrom, Role* pRole, Node* pTo)
{
  for(EdgeVector::iterator i = m_listEdge.begin(); i != m_listEdge.end(); i++ )
    {
      Edge* pEdge = (Edge*)*i;
      if( (pFrom == NULL || isEqual(pFrom, pEdge->m_pFrom) == 0) &&
	  (pRole == NULL || pEdge->m_pRole->isSubRoleOf(pRole)) &&
	  (pTo == NULL || isEqual(pTo, pEdge->m_pTo) == 0 ) )
	return TRUE;
    }
  return FALSE;
}

bool EdgeList::hasEdgeTo(Node* pNode)
{
  return hasEdge(NULL, NULL, pNode);
}

/**
 * Find the neighbors of a node that has a certain type. For literals, we collect
 * only the ones with the same language tag.
 */
void EdgeList::getFilteredNeighbors(Individual* pNode, ExprNode* pC, void* pNodes)
{
  NodeSet* pNodeSet = (NodeSet*)pNodes;

  string sLang = "";
  for(EdgeVector::iterator i = m_listEdge.begin(); i != m_listEdge.end(); i++ )
    {
      Edge* pEdge = (Edge*)*i;
      Node* pNeighbor = pEdge->getNeighbor(pNode);
      
      if( !isTop(pC) && !pNeighbor->hasType(pC) )
	continue;
      else if( !pNeighbor->isIndividual() ) // == IS LITERAL 
	{
	  Literal* pLiteral = (Literal*)pNeighbor;
	  if( sLang == "" )
	    {
	      sLang = pLiteral->m_sLang;
	      pNodeSet->insert(pNeighbor);
	    }
	  else if( sLang == pLiteral->m_sLang )
	    pNodeSet->insert(pNeighbor);
	}
      else
	pNodeSet->insert(pNeighbor);
    }
}

void EdgeList::getRoles(RoleSet* pRoles)
{
  for(EdgeVector::iterator i = m_listEdge.begin(); i != m_listEdge.end(); i++ )
    {
      Edge* pEdge = (Edge*)*i;
      pRoles->insert(pEdge->m_pRole);
    } 
}

void EdgeList::getPredecessors(void* pPredecessors)
{
  NodeSet* pPreds = (NodeSet*)pPredecessors;
  for(EdgeVector::iterator i = m_listEdge.begin(); i != m_listEdge.end(); i++ )
    {
      Edge* pEdge = (Edge*)*i;
      pPreds->insert(pEdge->m_pFrom);
    }
}

void EdgeList::getSuccessors(void* pSuccessors)
{
  NodeSet* pSuccs = (NodeSet*)pSuccessors;
  for(EdgeVector::iterator i = m_listEdge.begin(); i != m_listEdge.end(); i++ )
    {
      Edge* pEdge = (Edge*)*i;
      pSuccs->insert(pEdge->m_pTo);
    }
}

DependencySet* EdgeList::getDepends(bool bDoExplanation)
{
  DependencySet* pDS = &DEPENDENCYSET_INDEPENDENT;
  for(EdgeVector::iterator i = m_listEdge.begin(); i != m_listEdge.end(); i++ )
    {
      Edge* pEdge = (Edge*)*i;
      pDS = pDS->unionNew(pEdge->m_pDepends, bDoExplanation);
    }
  return pDS;
}
