#include "ZList.h"

ZCollection* getNewZCollection()
{
  return (ZCollection*)calloc(1, sizeof(ZCollection));
}

void initZ(ZCollection* pList, bool bIsMap, int iListSize)
{
  pList->m_pList = NULL;
  pList->m_iListSize = 0;
  pList->m_iUsedSize = 0;
  pList->m_bIsMap = bIsMap;

  if( iListSize > 0 )
    expandZ(pList, iListSize);
}

bool addZ(ZCollection* pList, void* pToBeAdded)
{
  if( pList->m_bIsMap )
    {
      // first check for duplicate entries
      for(int i = 0; i < pList->m_iUsedSize; i++ )
	{
	  if( pList->m_pList[i] == pToBeAdded )
	    return FALSE;
	}
    }

  if( pList->m_iUsedSize == pList->m_iListSize )
    expandZ(pList);

  pList->m_pList[pList->m_iUsedSize++] = pToBeAdded;
  return TRUE;
}

bool removeZ(ZCollection* pList, void* pToBeRemoved)
{
  for(int i = 0; i < pList->m_iUsedSize; i++ )
    {
      if( pList->m_pList[i] == pToBeRemoved )
	{
	  for(int j = i; j < pList->m_iUsedSize-1; j++ )
	    pList->m_pList[j] = pList->m_pList[j+1];
	  pList->m_iUsedSize--;
	  return TRUE;
	}
    }
  return FALSE;
}

void* removeAtZ(ZCollection* pList, int iIndex)
{
  if( iIndex < pList->m_iUsedSize )
    {
      void* pItem = pList->m_pList[iIndex];

      for(int j = iIndex; j < pList->m_iUsedSize-1; j++ )
	pList->m_pList[j] = pList->m_pList[j+1];
      pList->m_iUsedSize--;

      return pItem;
    }
  return NULL;
}

void* getAtZ(ZCollection* pList, int iIndex)
{
  if( iIndex < pList->m_iUsedSize )
    return pList->m_pList[iIndex];
  return NULL;
}

bool setAtZ(ZCollection* pList, int iIndex, void* pToBeSetted)
{
  if( iIndex >= pList->m_iListSize )
    expandZ(pList);

  if( iIndex >= pList->m_iUsedSize )
    pList->m_iUsedSize = iIndex+1;
 
  pList->m_pList[iIndex] = pToBeSetted;
  return TRUE;
}

bool setAtZAnyway(ZCollection* pList, int iIndex, void* pToBeSetted)
{
  if( iIndex >= pList->m_iListSize )
    expandZ(pList);

  if( iIndex >= pList->m_iUsedSize )
    pList->m_iUsedSize = iIndex+1;
  pList->m_pList[iIndex] = pToBeSetted;
  return TRUE;
}

void copyZs(ZCollection* pTarget, ZCollection* pSource)
{
  if( pTarget->m_iListSize < pSource->m_iUsedSize )
    expandZ(pTarget);

  for(int i = 0; i < pSource->m_iUsedSize; i++ )
    pTarget->m_pList[i] = pSource->m_pList[i];
  pTarget->m_iUsedSize = pSource->m_iUsedSize;
}

void expandZ(ZCollection* pTarget, int iIncrement)
{
  if( pTarget->m_iListSize == 0 )
    pTarget->m_pList = (void**)calloc(iIncrement, sizeof(void*));
  else
    pTarget->m_pList = (void**)realloc(pTarget->m_pList, (pTarget->m_iListSize+iIncrement)*sizeof(void*));
  pTarget->m_iListSize += iIncrement;
}

/********************************************************************/

ICollection* getNewICollection()
{
  return (ICollection*)calloc(1, sizeof(ICollection));
}

ICollection* getNewICollection(ICollection* pICollection)
{
  ICollection* pNew = (ICollection*)calloc(1, sizeof(ICollection));
  copyIs(pNew, pICollection);
}

void initI(ICollection* pList, int iListSize)
{
  pList->m_pList = NULL;
  pList->m_iListSize = 0;
  pList->m_iUsedSize = 0;

  if( iListSize > 0 )
    expandI(pList, iListSize);
}

bool addI(ICollection* pList, int iToBeAdded)
{
  if( pList->m_iUsedSize == pList->m_iListSize )
    expandI(pList);

  pList->m_pList[pList->m_iUsedSize++] = iToBeAdded;
  return TRUE;
}

int removeAtI(ICollection* pList, int iIndex)
{
  if( iIndex < pList->m_iUsedSize )
    {
      int iValue = pList->m_pList[iIndex];

      for(int j = iIndex; j < pList->m_iUsedSize-1; j++ )
	pList->m_pList[j] = pList->m_pList[j+1];
      pList->m_iUsedSize--;

      return iValue;
    }
  return -1;
}

int getAtI(ICollection* pList, int iIndex)
{
  if( iIndex < pList->m_iUsedSize )
    return pList->m_pList[iIndex];
  return -1;
}

bool setAtI(ICollection* pList, int iIndex, int iToBeSetValue)
{
  if( iIndex >= pList->m_iListSize )
    expandI(pList);

  if( iIndex >= pList->m_iUsedSize )
    pList->m_iUsedSize = iIndex+1;

  pList->m_pList[iIndex] = iToBeSetValue;
  return TRUE;
}

bool setAtIAnyway(ICollection* pList, int iIndex, int iToBeSetValue)
{
  if( iIndex >= pList->m_iListSize )
    expandI(pList);

  if( iIndex >= pList->m_iUsedSize )
    pList->m_iUsedSize = iIndex+1;
  pList->m_pList[iIndex] = iToBeSetValue;
  return TRUE;
}

void copyIs(ICollection* pTarget, ICollection* pSource)
{
  if( pTarget->m_iListSize < pSource->m_iUsedSize )
    expandI(pTarget);

  for(int i = 0; i < pSource->m_iUsedSize; i++ )
    pTarget->m_pList[i] = pSource->m_pList[i];
  pTarget->m_iUsedSize = pSource->m_iUsedSize;
}

void expandI(ICollection* pTarget, int iIncrement)
{
  if( pTarget->m_iListSize == 0 )
    pTarget->m_pList = (int*)calloc(iIncrement, sizeof(int));
  else
    pTarget->m_pList = (int*)realloc(pTarget->m_pList, (pTarget->m_iListSize+iIncrement)*sizeof(int));
  pTarget->m_iListSize += iIncrement;
}
