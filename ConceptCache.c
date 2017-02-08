#include "ConceptCache.h"
#include "ReazienerUtils.h"
#include "Individual.h"

extern int g_iCommentIndent;

ConceptCache::ConceptCache()
{

}

bool ConceptCache::putSat(ExprNode* pC, bool bIsSatisfiable)
{
  START_DECOMMENT2("ConceptCache::putSat");
  DECOMMENT1("isSatisfiable=%d\n", bIsSatisfiable);
  printExprNodeWComment("[pC]=", pC);

  CachedNode* pCached = NULL;
  ExprNode2CachedNodeMap::iterator iFind = m_mCache.find(pC);
  if( iFind != m_mCache.end() )
    pCached = (CachedNode*)iFind->second;

  if( pCached )
    {
      if( bIsSatisfiable != !pCached->isBottom() )
	assertFALSE("Caching inconsistent results for ");
      END_DECOMMENT("ConceptCache::putSat");
      return FALSE;
    }
  else if( bIsSatisfiable )
    {
      m_mCache[pC] = CachedNode::createSatisfiableNode();
    }
  else
    {
      ExprNode* pNotC = negate2(pC);
      m_mCache[pC] = CachedNode::createBottomNode();
      m_mCache[pNotC] = CachedNode::createTopNode();
    }

  END_DECOMMENT("ConceptCache::putSat");
  return TRUE;
}

bool ConceptCache::put(ExprNode* pC, CachedNode* pCachedNode)
{
  START_DECOMMENT2("ConceptCache::put");
  printExprNodeWComment("[pC]=", pC);
  printExprNodeWComment("CachedNode=", pCachedNode->m_pNode->m_pName);

  if( m_mCache.find(pC) != m_mCache.end() )
    {
      END_DECOMMENT("ConceptCache::putSat");
      return FALSE;
    }

  m_mCache[pC] = pCachedNode;
  END_DECOMMENT("ConceptCache::putSat");
  return TRUE;
}

CachedNode* ConceptCache::getCached(ExprNode* pC)
{
  ExprNode2CachedNodeMap::iterator i = m_mCache.find(pC);
  if( i != m_mCache.end() )
    return (CachedNode*)i->second;
  return NULL;
}

int ConceptCache::getSat(ExprNode* pC)
{
  CachedNode* pCached = getCached(pC);
  return (pCached==NULL)?-1:(pCached->isBottom()?0:1);
}
