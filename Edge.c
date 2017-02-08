#include "Edge.h"
#include "Role.h"
#include "Individual.h"
#include "Node.h"
#include "DependencySet.h"

Edge::Edge(Role* pName, Individual* pFrom, Node* pTo)
{
  m_pRole = pName;
  m_pFrom = pFrom;
  m_pTo = pTo;
  m_pDepends = NULL;

  computeHashCode();
}

Edge::Edge(Role* pName, Individual* pFrom, Node* pTo, DependencySet* pDS)
{
  m_pRole = pName;
  m_pFrom = pFrom;
  m_pTo = pTo;
  m_pDepends = pDS;

  computeHashCode();
}

Node* Edge::getNeighbor(Node* pNode)
{
  if( ::isEqual(m_pFrom, pNode) == 0 )
    return m_pTo;
  else if( ::isEqual(m_pTo, pNode) == 0 )
    return m_pFrom;
  return NULL;
}

void Edge::computeHashCode()
{

}

bool Edge::isEqual(const Edge* pEdge) const
{
  if( ::isEqual(m_pFrom, pEdge->m_pFrom) == 0 && 
      ::isEqual(m_pTo, pEdge->m_pTo) == 0 && 
      ::isEqual(m_pRole, pEdge->m_pRole) == 0 )
    return TRUE;
  return FALSE;
}
