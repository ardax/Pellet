#ifndef _KRSS_LOADER_
#define _KRSS_LOADER_

#include "Utils.h"

#define KRSSLOADER_BUFFERSIZE 50000
#define KSSDEF_TITLESIZE 200

typedef struct _KRSSDef_
{
  int m_iDefinition;
  char m_cDefinition[KSSDEF_TITLESIZE];

  void* m_pDefinitionArgs;
  void* m_pLastDefinitionArg;
  void* m_pNext;
} KRSSDef;

enum KRSSDefinitionNames
  {
    DEFINE_TOP = 0,
    DEFINE_BOTTOM,

    DEFINE_AND,
    DEFINE_OR,
    DEFINE_NOT,
    DEFINE_ATLEAST,
    DEFINE_ATMOST,
    DEFINE_ONEOF,
    DEFINE_ALL,
    DEFINE_SOME,
    DEFINE_EXACTLY,
    DEFINE_MIN,
    DEFINE_MAX,
    DEFINE_INV,
    DEFINE_EQUAL,
    DEFINE_A,

    DEFINE_CONCEPT,
    DEFINE_PRIMITIVE_CONCEPT,
    DEFINE_DISJOINT_PRIMITIVE_CONCEPT,
    DEFINE_ROLE,
    DEFINE_CONCEPT_SUBSUMES,
    DEFINE_ROLE_SUBSUMES,
    DEFINE_ROLE_EQUIVALANCE,
    DEFINE_INSTANCE,
    DEFINE_INDIVIDUAL,
    DEFINE_FUNCTIONAL,
    DEFINE_TRANSITIVE,
    DEFINE_DISJOINT,
    DEFINE_DOMAIN,
    DEFINE_RANGE,
    DEFINE_RELATED,
    DEFINE_DIFFERENT,
    DEFINE_DATATYPE_ROLE_FILLER,
  };

class KnowledgeBase;


class KRSSLoader
{
 public:
  KRSSLoader();
  ~KRSSLoader();

  void loadKRSSFile(char* pFileInKRSSFormat, KnowledgeBase* pKB);

 private:

  bool isValidDefinition(KRSSDef* pDef);

  void showComment(char* pComment);
  void openBuffer(char* pFileName);
  bool updateBuffer();

  bool loadDefinitionArg(KRSSDef* pDef);
  KRSSDef* loadDefinition(char* pDefName);
  void setKRSSDefinition(KRSSDef* pDef, char* pDefTitle);

  void printKRSSDef(KRSSDef* pDef, int iDepth = 0);
  KRSSDef* getNewKRSSDef()
    {
      KRSSDef* pNewDef = (KRSSDef*)calloc(1, sizeof(KRSSDef));
      pNewDef->m_pDefinitionArgs = NULL;
      pNewDef->m_pNext = NULL;
      pNewDef->m_pLastDefinitionArg = NULL;
      pNewDef->m_cDefinition[0] = '\0';
      return pNewDef;
    }
  void freeKRSSDefArg(KRSSDef* pDef)
  {
    if( pDef->m_pNext )
      freeKRSSDefArg((KRSSDef*)pDef->m_pNext);
    if( pDef->m_pDefinitionArgs )
      freeKRSSDef((KRSSDef*)pDef->m_pDefinitionArgs);
    free(pDef);
  }
  void freeKRSSDef(KRSSDef* pDef)
  {
    if( pDef->m_pDefinitionArgs )
      freeKRSSDefArg((KRSSDef*)pDef->m_pDefinitionArgs);
  }

  bool passTill(char cPassChar);
  bool readNextName(bool bPassFirstChar =TRUE);
  bool isCurrComment();
  char currChar()
  {
    if( m_iReadIndex == -1 )
      {
	if( !updateBuffer() )
	  {
	    showComment("Done reading[1]");
	    m_bDoneReading = TRUE;
	    return (char)NULL;
	  }
      }

    for(int i = m_iReadIndex; i < m_iReadBufferSize; i++ )
      {
	if( m_cReadBuffer[i] == ' ' || 
	    m_cReadBuffer[i] == '\t' || 
	    m_cReadBuffer[i] == '\r' || 
	    m_cReadBuffer[i] == '\n' )
	  {
	    // pass this
	  }
	else
	  {
	    m_iReadIndex = i;
	    return m_cReadBuffer[m_iReadIndex];
	  }
      }
    
    if( updateBuffer() )
      return currChar();

    m_bDoneReading = TRUE;
    showComment("Done reading[2]");
    return (char)NULL;
  }
  
  FILE* m_pFile;

  char m_cReadBuffer[KRSSLOADER_BUFFERSIZE];
  int  m_iReadBufferSize;

  char m_cName[KRSSLOADER_BUFFERSIZE];

  int  m_iReadIndex;
  bool m_bDoneReading;

  KRSSDef* m_pKRSSDefinitions;
};

#endif
