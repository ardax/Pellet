#include "KRSSLoader.h"
#include "KnowledgeBase.h"
#include "ExpressionNode.h"

#include "TextUtils.h"
#include <cassert>


KRSSLoader::KRSSLoader()
{
  m_cReadBuffer[0] = '\0';
  m_iReadIndex = -1;
  m_bDoneReading = FALSE;
  m_pFile = NULL;
  m_iReadBufferSize = 0;
  m_pKRSSDefinitions = NULL;
}

KRSSLoader::~KRSSLoader()
{
  KRSSDef* pDef = m_pKRSSDefinitions;
  while(pDef)
    {
      KRSSDef* pNext = (KRSSDef*)pDef->m_pNext;
      freeKRSSDef(pDef);
      pDef = (KRSSDef*)pNext;
    }
}

void KRSSLoader::printKRSSDef(KRSSDef* pDef, int iDepth)
{
  if( iDepth == 0 && PARAMS_PRINT_DEBUGINFO_INHTML() )
    printf("<comment>");

  if( !pDef->m_pDefinitionArgs )
    {
      printf("%s ", pDef->m_cDefinition);
      return;
    }

  printf("(%s ", pDef->m_cDefinition);

  KRSSDef* pDefArg = (KRSSDef*)pDef->m_pDefinitionArgs;
  while(pDefArg)
    {
      printKRSSDef(pDefArg, iDepth+1);
      pDefArg = (KRSSDef*)pDefArg->m_pNext;
    }

  printf(") ", pDef->m_cDefinition);
  if( iDepth == 0 )
    {
      if( PARAMS_PRINT_DEBUGINFO_INHTML() ) printf("</comment>");
      printf("\n");
    }
}

void KRSSLoader::showComment(char* pComment)
{
  return;
  printf("[%d/%d] %s\n", m_iReadIndex, m_iReadBufferSize, pComment);
}

void KRSSLoader::openBuffer(char* pFileName)
{
  m_pFile = fopen(pFileName, "r");
}

bool KRSSLoader::updateBuffer()
{
  if( !m_pFile ) return FALSE;

  if( fgets(m_cReadBuffer, KRSSLOADER_BUFFERSIZE, m_pFile) )
    {
      m_iReadBufferSize = strlen(m_cReadBuffer);
      assert(m_iReadBufferSize);
      m_iReadIndex = 0;
      return TRUE;
    }

  fclose(m_pFile);
  m_pFile = NULL;

  return FALSE;
}

bool KRSSLoader::isCurrComment()
{
  int iPrevReadIndex = m_iReadIndex;
  char c = currChar();
  if( c == ';' && (iPrevReadIndex==-1 ||
		   iPrevReadIndex==0 ||
		   m_cReadBuffer[iPrevReadIndex] == ' ' ||
		   m_cReadBuffer[iPrevReadIndex] == '\n' ||
		   m_cReadBuffer[iPrevReadIndex] == '\r' ||
		   m_cReadBuffer[iPrevReadIndex] == '\t') )
    {
      return TRUE;
    }
  return FALSE;
}

void KRSSLoader::loadKRSSFile(char* pFileInKRSSFormat, KnowledgeBase* pKB)
{
  openBuffer(pFileInKRSSFormat);
  updateBuffer();

  do
    {
      char c;
      if( isCurrComment() )
	{
	  // pass the comment
	  showComment("Passing Comment");
	  passTill('\n');
	}
      else if( (c = currChar()) == '(' )
	{
	  // starting a new definition
	  showComment("Starting a new definition");
	  if( readNextName() )
	    {
	      KRSSDef* pNewDef = loadDefinition(m_cName);
	      if( pNewDef )
		{ 
		  if( PARAMS_PRINT_DEBUGINFO_INHTML() )
		    printKRSSDef(pNewDef);

		  // validate and add this KRSS definition into given KB
		  if( isValidDefinition(pNewDef) )
		    {
		      convertKRSSDef2ExprNode(pNewDef, pKB);
		      pNewDef->m_pNext = m_pKRSSDefinitions;
		      m_pKRSSDefinitions = pNewDef;
		    }
		}
	    }
	}
      else
	{
	  //?
	}
    }
  while(!m_bDoneReading);
}

bool KRSSLoader::isValidDefinition(KRSSDef* pDef)
{
  if( pDef->m_iDefinition == -1 && pDef->m_pDefinitionArgs )
    return FALSE;
  return TRUE;
}

bool KRSSLoader::loadDefinitionArg(KRSSDef* pDef)
{
  char c = currChar();
  if( c == '(' )
    {
      showComment("Loading nested definition argument");
      // argument is a definition
      if( readNextName() )
	{
	  KRSSDef* pNewDefArg = loadDefinition(m_cName);
	  if( pDef->m_pLastDefinitionArg )
	    ((KRSSDef*)pDef->m_pLastDefinitionArg)->m_pNext = pNewDefArg;
	  else
	    pDef->m_pDefinitionArgs = pNewDefArg;
	  pDef->m_pLastDefinitionArg = pNewDefArg;
	  return TRUE;
	}
      // failed
      return FALSE;
    }

  // argument is a plain string
  if( readNextName(FALSE) )
    {
      char cComment[1024];
      sprintf(cComment, "Loading definition argument=[%s]", m_cName);
      showComment(cComment);

      KRSSDef* pNewDefArg = getNewKRSSDef();
      setKRSSDefinition(pNewDefArg, m_cName);

      if( pDef->m_pLastDefinitionArg )
	((KRSSDef*)pDef->m_pLastDefinitionArg)->m_pNext = pNewDefArg;
      else
	pDef->m_pDefinitionArgs = pNewDefArg;
      pDef->m_pLastDefinitionArg = pNewDefArg;
      return TRUE;
    }
  return FALSE;
}

void KRSSLoader::setKRSSDefinition(KRSSDef* pDef, char* pDefTitle)
{
  pDef->m_iDefinition = -1;

  if( stricmp(pDefTitle, "and") == 0 ) pDef->m_iDefinition = DEFINE_AND;
  else if( stricmp(pDefTitle, "or") == 0 ) pDef->m_iDefinition = DEFINE_OR;
  else if( stricmp(pDefTitle, "not") == 0 ) pDef->m_iDefinition = DEFINE_NOT;
  else if( stricmp(pDefTitle, "top") == 0 || stricmp(pDefTitle, ":top") == 0 || stricmp(pDefTitle, "*top*") == 0 ) pDef->m_iDefinition = DEFINE_TOP;
  else if( stricmp(pDefTitle, "bottom") == 0 || stricmp(pDefTitle, "*bottom*") == 0 ) pDef->m_iDefinition = DEFINE_BOTTOM;
  else if( stricmp(pDefTitle, "one-of") == 0 ) pDef->m_iDefinition = DEFINE_ONEOF;
  else if( stricmp(pDefTitle, "all") == 0 ) pDef->m_iDefinition = DEFINE_ALL;
  else if( stricmp(pDefTitle, "some") == 0 ) pDef->m_iDefinition = DEFINE_SOME;
  else if( stricmp(pDefTitle, "exactly") == 0 ) pDef->m_iDefinition = DEFINE_EXACTLY;
  else if( stricmp(pDefTitle, "=") == 0 ) pDef->m_iDefinition = DEFINE_EQUAL;
  //else if( stricmp(pDefTitle, "a") == 0 ) pDef->m_iDefinition = DEFINE_A;
  else if( stricmp(pDefTitle, "inv") == 0 ) pDef->m_iDefinition = DEFINE_INV;
  else if( stricmp(pDefTitle, "min") == 0 || stricmp(pDefTitle, ">=") == 0 ) pDef->m_iDefinition = DEFINE_MIN;
  else if( stricmp(pDefTitle, "max") == 0 || stricmp(pDefTitle, "<=") == 0 ) pDef->m_iDefinition = DEFINE_MAX;
  else if( stricmp(pDefTitle, "atleast") == 0 || stricmp(pDefTitle, "at-least") == 0 ) pDef->m_iDefinition = DEFINE_ATLEAST;
  else if( stricmp(pDefTitle, "atmost") == 0 || stricmp(pDefTitle, "at-most") == 0 ) pDef->m_iDefinition = DEFINE_ATMOST;
  else if( strincmp(pDefTitle, "def", 3) == 0 )
    {
      if( stricmp(pDefTitle, "define-role") == 0 ||
	  stricmp(pDefTitle, "defprimrole") == 0 ||
	  stricmp(pDefTitle, "define-primitive-role") == 0 ||
	  stricmp(pDefTitle, "define-primitive-attribute") == 0 ||
	  stricmp(pDefTitle, "define-attribute") == 0 ||
	  stricmp(pDefTitle, "defprimattribute") == 0 ||
	  stricmp(pDefTitle, "define-datatype-property") == 0)
	{
	  pDef->m_iDefinition = DEFINE_ROLE;
	}
      else if( stricmp(pDefTitle, "define-primitive-concept") == 0 ||
	       stricmp(pDefTitle, "defprimconcept") == 0 ||
	       stricmp(pDefTitle, "equal_c") == 0 )
	{
	  pDef->m_iDefinition = DEFINE_PRIMITIVE_CONCEPT;
	}
      else if( stricmp(pDefTitle, "define-concept") == 0 ||
	       stricmp(pDefTitle, "defconcept") == 0 )
	{
	  pDef->m_iDefinition = DEFINE_CONCEPT;
	}
      else if( stricmp(pDefTitle, "define-disjoint-primitive-concept") == 0 )
	{
	  pDef->m_iDefinition = DEFINE_DISJOINT_PRIMITIVE_CONCEPT;
	}
    }
  else if( strincmp(pDefTitle, "implies", 7) == 0 )
    {
      if( stricmp(pDefTitle, "implies") == 0 ||
	  stricmp(pDefTitle, "implies_c") == 0 )
	{
	  pDef->m_iDefinition = DEFINE_CONCEPT_SUBSUMES;
	}
      else if( stricmp(pDefTitle, "implies_r") == 0 ) { pDef->m_iDefinition = DEFINE_ROLE_SUBSUMES; }
    }
  else if( stricmp(pDefTitle, "equal_r") == 0 ) { pDef->m_iDefinition = DEFINE_ROLE_EQUIVALANCE; }
  else if( stricmp(pDefTitle, "domain") == 0 ) { pDef->m_iDefinition = DEFINE_DOMAIN; }
  else if( stricmp(pDefTitle, "range") == 0 ) { pDef->m_iDefinition = DEFINE_RANGE; }
  else if( stricmp(pDefTitle, "functional") == 0 ) { pDef->m_iDefinition = DEFINE_FUNCTIONAL; }
  else if( stricmp(pDefTitle, "transitive") == 0 ) { pDef->m_iDefinition = DEFINE_TRANSITIVE; }
  else if( stricmp(pDefTitle, "disjoint") == 0 ) { pDef->m_iDefinition = DEFINE_DISJOINT; }
  else if( stricmp(pDefTitle, "defindividual") == 0 ) { pDef->m_iDefinition = DEFINE_INDIVIDUAL; }
  else if( stricmp(pDefTitle, "instance") == 0 ) { pDef->m_iDefinition = DEFINE_INSTANCE; }
  else if( stricmp(pDefTitle, "related") == 0 ) { pDef->m_iDefinition = DEFINE_RELATED; }
  else if( stricmp(pDefTitle, "different") == 0 ) { pDef->m_iDefinition = DEFINE_DIFFERENT; }
  else if( stricmp(pDefTitle, "datatype-role-filler") == 0 ) { pDef->m_iDefinition = DEFINE_DATATYPE_ROLE_FILLER; }
  
  moveStr(pDef->m_cDefinition, pDefTitle, MIN(KSSDEF_TITLESIZE, strlen(pDefTitle)));
}

KRSSDef* KRSSLoader::loadDefinition(char* pDefName)
{
  char cComment[1024];
  sprintf(cComment, "Loading definition=[%s]", pDefName);
  showComment(cComment);

  KRSSDef* pNewDef = getNewKRSSDef();
  setKRSSDefinition(pNewDef, pDefName);
      
  // load definition arguments one after another till seeing ')'
  int iIterCount = 0;
  do
    {
      if( isCurrComment() )
	{
	  // pass the comment line
	  passTill('\n');
	}
      else
	{
	  char c = currChar();
	  if( c == ')' )
	    {
	      // end of this definition
	      // pass this
	      m_iReadIndex++;
	      return pNewDef;
	    }
	  else
	    {
	      // read the next definition node
	      if( !loadDefinitionArg(pNewDef) )
		{
		  freeKRSSDef(pNewDef);
		  return NULL;
		}
	    }
	}
    }
  while(iIterCount++<10000);
  // timeout
  freeKRSSDef(pNewDef);
  return NULL;
}

bool KRSSLoader::passTill(char cPassChar)
{
  for(int i = m_iReadIndex; i < m_iReadBufferSize; i++ )
    {
      if( m_cReadBuffer[i] == cPassChar )
	{
	  m_iReadIndex = i+1;
	  return TRUE;
	}
    }

  // pass char not seen
  if( updateBuffer() )
    return passTill(cPassChar);
  return FALSE;
}

bool KRSSLoader::readNextName(bool bPassFirstChar /*=TRUE*/)
{
  m_cName[0] = '\0';
  int k = 0;
  int iStart = bPassFirstChar?(m_iReadIndex+1):m_iReadIndex;
  for(int i = iStart; i < m_iReadBufferSize; i++ )
    {
      if( m_cName[0] != '|' && 
	  (m_cReadBuffer[i] == ' ' ||
	   m_cReadBuffer[i] == '\t' ||
	   m_cReadBuffer[i] == '\r' ||
	   m_cReadBuffer[i] == '\n' ||
	   m_cReadBuffer[i] == ')') )
	{
	  if( k == 0 )
	      continue;

	  if( m_cName[0] == '|' )
	    {
	      // keep going till seeing another |
	      continue;
	    }
	  m_iReadIndex = i;
	  m_cName[k] = '\0';
	  return TRUE;
	}
      else if( m_cName[0] == '|' && m_cReadBuffer[i] == '|' )
	{
	  m_cName[k++] = m_cReadBuffer[i];
	  m_cName[k] = '\0';
	  m_iReadIndex = i+1;
	  return TRUE;
	}

      m_cName[k++] = m_cReadBuffer[i];
      if( k > KRSSLOADER_BUFFERSIZE-10 )
	assert(FALSE);
    }

  // not read?
  if( updateBuffer() )
    return readNextName();
  return FALSE;
}

