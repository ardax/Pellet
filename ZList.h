#ifndef _ZLIST_
#define _ZLIST_

#include "Utils.h"
#include <vector>

typedef struct _ZCollection_
{
  void** m_pList;
  int m_iUsedSize;
  int m_iListSize;
  bool m_bIsMap;
} ZCollection;

typedef struct _IntCollection_
{
  int* m_pList;
  int m_iUsedSize;
  int m_iListSize;
} ICollection;

typedef vector<ZCollection*> ZCollections;
typedef vector<ICollection*> ICollections;

ZCollection* getNewZCollection();
void initZ(ZCollection* pList, bool bIsMap = FALSE, int iListSize = 0);
bool addZ(ZCollection* pList, void* pToBeAdded);
bool removeZ(ZCollection* pList, void* pToBeRemoved);
void* removeAtZ(ZCollection* pList, int iIndex);
void* getAtZ(ZCollection* pList, int iIndex);
bool setAtZ(ZCollection* pList, int iIndex, void* pToBeSetted);
bool setAtZAnyway(ZCollection* pList, int iIndex, void* pToBeSetted);
void copyZs(ZCollection* pTarget, ZCollection* pSource);
void expandZ(ZCollection* pList, int iIncrement = 10);

ICollection* getNewICollection();
ICollection* getNewICollection(ICollection* pICollection);
void initI(ICollection* pList, int iListSize = 0);
bool addI(ICollection* pList, int iToBeAdded);
int removeAtI(ICollection* pList, int iIndex);
int getAtI(ICollection* pList, int iIndex);
bool setAtI(ICollection* pList, int iInddex, int iToBeSetValue);
bool setAtIAnyway(ICollection* pList, int iIndex, int iToBeSetValue);
void copyIs(ICollection* pTarget, ICollection* pSource);
void expandI(ICollection* pList, int iIncrement = 10);


#endif
