#include "CachedNode.h"
#include "Individual.h"
#include "DependencySet.h"

Individual* g_pTOPIND = NULL;
Individual* g_pBOTTOMIND = NULL;
Individual* g_pDUMMYIND = NULL;

extern DependencySet DEPENDENCYSET_EMPTY;
extern DependencySet DEPENDENCYSET_INDEPENDENT;
extern ExprNode* EXPRNODE_TOP;
extern ExprNode* EXPRNODE_BOTTOM;

CachedNode::CachedNode(Individual* pNode, DependencySet* pDS)
{
  m_pNode = pNode;
  m_pDepends = pDS;
}

bool CachedNode::isBottom()
{
  if( g_pBOTTOMIND == NULL )
    g_pBOTTOMIND = new Individual(EXPRNODE_BOTTOM);
  return (m_pNode==g_pBOTTOMIND);
}

bool CachedNode::isComplete()
{
  if( g_pDUMMYIND == NULL )
      g_pDUMMYIND = new Individual(createStrTerm("_DUMMY_"));
  return (m_pNode!=g_pDUMMYIND);
}

CachedNode* CachedNode::createNode(Individual* pNode, DependencySet* pDepends)
{
  return (new CachedNode(pNode, pDepends));
}

CachedNode* CachedNode::createSatisfiableNode()
{
  if( g_pDUMMYIND == NULL )
    g_pDUMMYIND = new Individual(createStrTerm("_DUMMY_"));
  return (new CachedNode(g_pDUMMYIND, &DEPENDENCYSET_INDEPENDENT));
}

CachedNode* CachedNode::createBottomNode()
{
  if( g_pBOTTOMIND == NULL )
    g_pBOTTOMIND = new Individual(EXPRNODE_BOTTOM);
  return (new CachedNode(g_pBOTTOMIND, &DEPENDENCYSET_INDEPENDENT));
}

CachedNode* CachedNode::createTopNode()
{
  if( g_pTOPIND == NULL )
    g_pTOPIND = new Individual(EXPRNODE_TOP);
  return (new CachedNode(g_pTOPIND, &DEPENDENCYSET_INDEPENDENT));
}
