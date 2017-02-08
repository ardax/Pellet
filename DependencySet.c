#include "DependencySet.h"
#include "ReazienerUtils.h"


DependencySet DEPENDENCYSET_EMPTY;
DependencySet DEPENDENCYSET_DUMMY;
DependencySet DEPENDENCYSET_INDEPENDENT;

extern int g_iCommentIndent;

void bitsetOr(dynamic_bitset* pSetTarget, dynamic_bitset* pSetSource)
{
  if( pSetTarget->size() < pSetSource->size() )
    {
      int iDiff = pSetSource->size()-pSetTarget->size();
      for(int i = 0; i < iDiff; i++) 
	pSetTarget->push_back(FALSE);
    }

  int iSize = pSetSource->size();
  for(int i = 0; i < iSize; i++ )
    {
      bool b1 = pSetTarget->at(i);
      bool b2 = pSetSource->at(i);
      (*pSetTarget)[i] = b1|b2;
    }
}

DependencySet::DependencySet(ExprNode* pExpr)
{
  m_iBranch = NO_BRANCH;
  if( pExpr )
    m_setExplain.insert(pExpr);
}

DependencySet::DependencySet(ExprNodeSet* pExprSet)
{
  m_iBranch = NO_BRANCH;
  m_setExplain = (*pExprSet);
}

DependencySet::DependencySet(DependencySet* pDS)
{
  m_iBranch = pDS->m_iBranch;
  m_setExplain = pDS->m_setExplain;
  m_bitsetDepends = pDS->m_bitsetDepends;
}

DependencySet::DependencySet(int iBranch)
{
  m_iBranch = NO_BRANCH;
  add(iBranch);
}

DependencySet* DependencySet::addExplain(ExprNodeSet* pExplain, bool bDoExplanation)
{
  if( !bDoExplanation || pExplain->size() == 0 )
    return this;

  DependencySet* pNewDS = new DependencySet();
  for(dynamic_bitset::iterator i = m_bitsetDepends.begin(); i != m_bitsetDepends.end();i++ )
    pNewDS->m_bitsetDepends.push_back((bool)*i);

  for(ExprNodeSet::iterator i = m_setExplain.begin(); i != m_setExplain.end(); i++ )
    pNewDS->m_setExplain.insert((ExprNode*)*i);
  for(ExprNodeSet::iterator i = pExplain->begin(); i != pExplain->end(); i++ )
    pNewDS->m_setExplain.insert((ExprNode*)*i);

  return pNewDS;
}

void DependencySet::removeExplain(ExprNode* pExplain)
{
  if( m_setExplain.find(pExplain) != m_setExplain.end() )
    m_setExplain.clear();
}

bool DependencySet::isIndependent()
{
  return (getMax() <= 0);
}

/**
 * Create a new DependencySet and all the elements of <code>this</code>
 * and <code>ds</code> (Neither set is affected when the return the set is
 * modified).
 */
DependencySet* DependencySet::unionNew(DependencySet* pDS, bool bDoExplanation)
{
  DependencySet* pNewDS = new DependencySet();
  pNewDS->m_iBranch = NO_BRANCH;
  
  if( bDoExplanation )
    {
      pNewDS->m_setExplain = m_setExplain;
      pNewDS->m_setExplain.insert(pDS->m_setExplain.begin(), pDS->m_setExplain.end());
    }
  
  pNewDS->m_bitsetDepends = m_bitsetDepends;
  bitsetOr(&(pNewDS->m_bitsetDepends), &(pDS->m_bitsetDepends));

  return pNewDS;
}

DependencySet* DependencySet::unionDS(DependencySet* pDS, bool bDoExplanation)
{
  bitsetOr(&m_bitsetDepends, &(pDS->m_bitsetDepends));
  if( bDoExplanation )
    {
      // UNION (TargetDS.Explainations, SourceDS.Explainations)
      m_setExplain.insert(pDS->m_setExplain.begin(), pDS->m_setExplain.end());
    }
  else
    {
      m_setExplain.clear();
    }
  return this;
}

int DependencySet::getMax()
{
  for(int i = m_bitsetDepends.size()-1; i >= 0; i-- )
    {
      if( m_bitsetDepends.at(i) == TRUE )
	return i;
    }
  return -1;
}

bool DependencySet::contains(int b)
{
  if( b < m_bitsetDepends.size() )
    return (m_bitsetDepends.at(b)==TRUE);
  return FALSE;
}

void DependencySet::add(int b)
{
  START_DECOMMENT2("DependencySet::add");
  DECOMMENT1("b=%d", b);
  if( b >= m_bitsetDepends.size() )
    {
      int iNewSize = (b-m_bitsetDepends.size())+1;
      for(int i = 0; i < iNewSize; i++ )
	m_bitsetDepends.push_back(FALSE);
    }
  m_bitsetDepends[b] = TRUE;
  END_DECOMMENT("DependencySet::add");
}

void DependencySet::remove(int b)
{
  START_DECOMMENT2("DependencySet::remove");
  DECOMMENT1("b=%d", b);
  if( b >= m_bitsetDepends.size() )
    {
      int iNewSize = (b-m_bitsetDepends.size())+1;
      for(int i = 0; i < iNewSize; i++ )
	m_bitsetDepends.push_back(FALSE);
    }
  m_bitsetDepends[b] = FALSE;
  END_DECOMMENT("DependencySet::remove");
}

