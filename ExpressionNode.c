#include "ExpressionNode.h"
#include "KnowledgeBase.h"
#include "TextUtils.h"
#include "ReazienerUtils.h"
#include "Role.h"
#include <cassert>

ExprNode* g_pEmptyExprNodes = NULL;
extern int g_iCommentIndent;

ExprNode* EXPRNODE_TOP;
ExprNode* EXPRNODE_BOTTOM;
ExprNode* EXPRNODE_TOPLITERAL;
ExprNode* EXPRNODE_BOTTOMLITERAL;
ExprNode* EXPRNODE_CONCEPT_SAT_IND;

ExprNode* getNewExprNode()
{
  if( g_pEmptyExprNodes == 0 )
    {
      ExprNode* aNewExprNodes = (ExprNode*)calloc(1000, sizeof(ExprNode));
      for(int i = 0; i < 1000-1; i++ )
	aNewExprNodes[i].m_pArgs[0] = &(aNewExprNodes[i+1]);
      aNewExprNodes[999].m_pArgs[0] = NULL;
      g_pEmptyExprNodes = &(aNewExprNodes[0]);
    }

  ExprNode* pRet = g_pEmptyExprNodes;
  g_pEmptyExprNodes = (ExprNode*)g_pEmptyExprNodes->m_pArgs[0];

  pRet->m_iExpression = EXPR_INVALID;
  memset(pRet->m_cTerm, 200, '\0');
  pRet->m_pArgs[0] = NULL;
  pRet->m_pArgs[1] = NULL;
  pRet->m_pArgs[2] = NULL;
  pRet->m_iArity = 0;
  pRet->m_iTerm = 0;

  return pRet;
}

void initExprNodes()
{
  EXPRNODE_TOP = getNewExprNode();
  EXPRNODE_TOP->m_iExpression = EXPR_TERM;
  EXPRNODE_TOP->m_iArity = 0;
  sprintf(EXPRNODE_TOP->m_cTerm, "_TOP_");

  EXPRNODE_BOTTOM = getNewExprNode();
  EXPRNODE_BOTTOM->m_iExpression = EXPR_NOT;
  EXPRNODE_BOTTOM->m_pArgs[0] = EXPRNODE_TOP;
  EXPRNODE_BOTTOM->m_iArity = 1;

  EXPRNODE_TOPLITERAL = getNewExprNode();
  EXPRNODE_TOPLITERAL->m_iExpression = EXPR_TERM;
  sprintf(EXPRNODE_TOPLITERAL->m_cTerm, "Literal");
  EXPRNODE_TOPLITERAL->m_iArity = 0;

  EXPRNODE_BOTTOMLITERAL = getNewExprNode();
  EXPRNODE_BOTTOMLITERAL->m_iExpression = EXPR_NOT;
  EXPRNODE_BOTTOMLITERAL->m_pArgs[0] = EXPRNODE_TOPLITERAL;
  EXPRNODE_BOTTOMLITERAL->m_iArity = 1;

  EXPRNODE_CONCEPT_SAT_IND = getNewExprNode();
  EXPRNODE_CONCEPT_SAT_IND->m_iExpression = EXPR_TERM;
  sprintf(EXPRNODE_CONCEPT_SAT_IND->m_cTerm, "_C_");
}

ExprNode* createExprNode(int iExpressionType, ExprNode* pArg1, ExprNode* pArg2, ExprNode* pArg3)
{
  ExprNode* pExpr = getNewExprNode();
  pExpr->m_iExpression = iExpressionType;
  
  if( iExpressionType == EXPR_AND || iExpressionType == EXPR_OR )
    {
      ExprNodeList* pList = createExprNodeList();
      if( pArg1 ) addExprToList(pList, pArg1);
      if( pArg2 ) addExprToList(pList, pArg2);
      if( pArg3 ) addExprToList(pList, pArg3);
      pExpr->m_pArgList = pList;
      pExpr->m_iArity = pList->m_iUsedSize;
    }
  else
    {
      if( iExpressionType == EXPR_NOT && pArg1->m_iExpression == EXPR_NOT )
	{
	  assert(pArg1->m_iArity==1);
	  return (ExprNode*)pArg1->m_pArgs[0];
	}
      else
	{
	  pExpr->m_pArgs[0] = pArg1;
	  pExpr->m_pArgs[1] = pArg2;
	  pExpr->m_pArgs[2] = pArg3;
	  
	  if( pArg1 ) pExpr->m_iArity++;
	  if( pArg2 ) pExpr->m_iArity++;
	  if( pArg3 ) pExpr->m_iArity++;
	}
    }

  return pExpr;
}

ExprNode* createExprNode(int iExpressionType, ExprNode* pArg1, int iArg2, ExprNode* pArg3)
{
  ExprNode* pExpr = getNewExprNode();
  pExpr->m_iExpression = iExpressionType;
  pExpr->m_pArgs[0] = pArg1;
  pExpr->m_pArgs[1] = getNewExprNode();
  ((ExprNode*)pExpr->m_pArgs[1])->m_iTerm = iArg2;
  pExpr->m_pArgs[2] = pArg3;
  pExpr->m_iArity = 3;

  return pExpr;
}
ExprNode* createExprNode(int iExpressionType, ExprNodeList* pArgList, ExprNode* pArg2, ExprNode* pArg3)
{
  ExprNode* pExpr = getNewExprNode();
  pExpr->m_iExpression = iExpressionType;

  if( iExpressionType == EXPR_VALUE && pArgList->m_iUsedSize == 1 )
    {
      pExpr->m_pArgs[0] = pArgList->m_pExprNodes[0];
      pExpr->m_iArity = 1;
    }
  else if( (iExpressionType == EXPR_AND || iExpressionType == EXPR_OR) && pArgList->m_iUsedSize == 1 )
    return pArgList->m_pExprNodes[0];
  else
    {
      pExpr->m_pArgList = pArgList;
      if( iExpressionType == EXPR_AND || iExpressionType == EXPR_OR )
	pExpr->m_iArity = pArgList->m_iUsedSize;
      else
	pExpr->m_iArity = 1;
    }

  pExpr->m_pArgs[1] = pArg2;
  if( pArg2 ) pExpr->m_iArity++;
  pExpr->m_pArgs[2] = pArg3;
  if( pArg3 ) pExpr->m_iArity++;

  return pExpr;
}

ExprNode* createInverse(ExprNode* pArg)
{
  if( pArg->m_iExpression == EXPR_INV )
    return (ExprNode*)pArg->m_pArgs[0];
  return createExprNode(EXPR_INV, pArg);
}

ExprNode* createExactCard(ExprNode* pR, int n, ExprNode* pC)
{
  ExprNode* pN = getNewExprNode();
  pN->m_iExpression = EXPR_TERM;
  pN->m_iTerm = n;

  ExprNode* pMax = createExprNode(EXPR_MAX, pR, pN, pC);
  if( n == 0 )
    return pMax;

  ExprNode* pMin = createExprNode(EXPR_MIN, pR, pN, pC);
  return createExprNode(EXPR_AND, pMax, pMin);
}

ExprNode* createStrTerm(char* pTerm)
{
  ExprNode* pTermExpr = getNewExprNode();
  pTermExpr->m_iExpression = EXPR_TERM; 
  moveStr(pTermExpr->m_cTerm, pTerm, strlen(pTerm));
  return pTermExpr;
}

ExprNode* createIntTerm(char* pTerm)
{
  ExprNode* pTermExpr = getNewExprNode();
  pTermExpr->m_iExpression = EXPR_TERM; 
  pTermExpr->m_cTerm[0] = '\0';
  pTermExpr->m_iTerm = atoi(pTerm);
  return pTermExpr;
}

ExprNodeList* createExprNodeList(int iSize)
{
  ExprNodeList* pNewList = (ExprNodeList*)calloc(1, sizeof(ExprNodeList));

  pNewList->m_pExprNodes = NULL;
  pNewList->m_iListSize = 0;
  pNewList->m_iUsedSize = 0;

  if( iSize > 0 )
    {
      pNewList->m_pExprNodes = (ExprNode**)calloc(iSize, sizeof(ExprNode*));
      pNewList->m_iListSize = iSize;
    }
  return pNewList;
}

ExprNodeList* createExprNodeList(ExprNodeList* pList)
{
  ExprNodeList* pNewList = createExprNodeList(pList->m_iUsedSize);
  for(int i = 0; i < pList->m_iUsedSize; i++ )
    pNewList->m_pExprNodes[pNewList->m_iUsedSize++] = pList->m_pExprNodes[i];
  return pNewList;
}

ExprNodeList* createExprNodeList(ExprNode* pExprNode, int iSize)
{
  ExprNodeList* pList = createExprNodeList(iSize);
  pList->m_pExprNodes[pList->m_iUsedSize++] = pExprNode;
  return pList;
}

void pop_front(ExprNodeList* pList)
{
  if( pList->m_iUsedSize > 0 )
    {
      for(int i = 0; i < pList->m_iUsedSize-1; i++ )
	pList->m_pExprNodes[i] = pList->m_pExprNodes[i+1];
      pList->m_iUsedSize--;
    }
}

ExprNodeList* convertExprSet2List(ExprNodeSet* pSet)
{
  ExprNodeList* pList = createExprNodeList(pSet->size());
  for(ExprNodeSet::iterator i = pSet->begin(); i != pSet->end(); i++ )
    pList->m_pExprNodes[pList->m_iUsedSize++] = (ExprNode*)*i;
  return pList;
}

/**
 * Creates a simplified and assuming that all the elements have already been
 * normalized.
 */
ExprNode* createSimplifiedAnd(ExprNodeSet* pSet)
{
  ExprNodeSet setExprNodes;
  ExprNodes aNegations, aExprNodes;
  for(ExprNodeSet::iterator i = pSet->begin(); i != pSet->end(); i++ )
    aExprNodes.push_back((ExprNode*)*i);

  while(aExprNodes.size()>0)
    {
      ExprNode* pExprNode = aExprNodes.front();
      aExprNodes.pop_front();

      if( isEqual(pExprNode, EXPRNODE_TOP) == 0 )
	continue;
      else if( isEqual(pExprNode, EXPRNODE_BOTTOM) == 0 )
	return EXPRNODE_BOTTOM;
      else if( pExprNode->m_iExpression == EXPR_AND )
	{
	  for(int i = 0; i < ((ExprNodeList*)pExprNode->m_pArgList)->m_iUsedSize; i++ )
	    aExprNodes.push_back(((ExprNodeList*)pExprNode->m_pArgList)->m_pExprNodes[i]);
	}
      else if( pExprNode->m_iExpression == EXPR_NOT )
	{
	  aNegations.push_back(pExprNode);
	}
      else
	setExprNodes.insert(pExprNode);
    }

  for(ExprNodes::iterator i = aNegations.begin(); i != aNegations.end(); i++ )
    {
      ExprNode* pNotC = (ExprNode*)*i;
      ExprNode* pC = (ExprNode*)pNotC->m_pArgs[0];
      if( setExprNodes.find(pC) != setExprNodes.end() )
	return EXPRNODE_BOTTOM;
    }

  if( setExprNodes.size() == 0 )
    {
      if( aNegations.size() == 0 )
	return EXPRNODE_TOP;
      else if( aNegations.size() == 1 )
	return (ExprNode*)aNegations.front();
    }
  else if( setExprNodes.size() == 1 && aNegations.size() == 0 )
    return (ExprNode*)(*setExprNodes.begin());
  
  ExprNodeList* pAndList = createExprNodeList(aNegations.size()+setExprNodes.size());
  for(ExprNodes::iterator i = aNegations.begin(); i != aNegations.end(); i++ )
    addExprToList(pAndList, ((ExprNode*)*i));
  for(ExprNodeSet::iterator i = setExprNodes.begin(); i != setExprNodes.end(); i++ )
    addExprToList(pAndList, ((ExprNode*)*i));

  return createExprNode(EXPR_AND, pAndList);
}

ExprNode* createNormalizedMax(ExprNode* pR, int n, ExprNode* pC)
{
  assert(n>=0);
  return createExprNode(EXPR_NOT, createExprNode(EXPR_MIN, pR, (n+1), negate2(pC)));
}

void addExprToList(ExprNodeList* pList, ExprNode* pExpr)
{
  if( pList->m_iUsedSize == pList->m_iListSize )
    {
      // expand the list
      if( pList->m_iListSize == 0 )
	{
	  pList->m_pExprNodes = (ExprNode**)calloc(10, sizeof(ExprNode*));
	  pList->m_iListSize = 10;
	}
      else
	{
	  pList->m_pExprNodes = (ExprNode**)realloc(pList->m_pExprNodes, ((pList->m_iListSize+10)*sizeof(ExprNode*)));
	  pList->m_iListSize += 10;
	}
    }
  pList->m_pExprNodes[pList->m_iUsedSize++] = pExpr;
}

bool isTop(ExprNode* pExprNode)
{
  return (isEqual(pExprNode, EXPRNODE_TOP) == 0 || isEqual(pExprNode, EXPRNODE_TOPLITERAL) == 0 );
}

void getExpressionInStr(int iExpression, char* pExp)
{
  switch(iExpression)
    {
    case EXPR_LIST: sprintf(pExp, "LIST"); break;
    case EXPR_TERM: sprintf(pExp, "TERM"); break;
    case EXPR_AND: sprintf(pExp, "AND"); break;
    case EXPR_OR: sprintf(pExp, "OR"); break;
    case EXPR_NOT: sprintf(pExp, "NOT"); break;
    case EXPR_ALL: sprintf(pExp, "ALL"); break;
    case EXPR_SOME: sprintf(pExp, "SOME"); break;
    case EXPR_MIN: sprintf(pExp, "MIN"); break;
    case EXPR_MAX: sprintf(pExp, "MAX"); break;
    case EXPR_CARD: sprintf(pExp, "CARD"); break;
    case EXPR_SELF: sprintf(pExp, "SELF"); break;
    case EXPR_INV: sprintf(pExp, "INVERSE"); break;
    case EXPR_VALUE: sprintf(pExp, "VALUE"); break;
    case EXPR_SUBCLASSOF: sprintf(pExp, "SUBCLASS_OF"); break;
    case EXPR_EQCLASSES: sprintf(pExp, "EQ_CLASSES"); break;
    case EXPR_SAMEAS: sprintf(pExp, "SAME_AS"); break;
    case EXPR_DISJOINTWITH: sprintf(pExp, "DISJOINT_WITH"); break;
    case EXPR_DISJOINTCLASSES: sprintf(pExp, "DISJOINT_CLASSES"); break;
    case EXPR_COMPLEMENTOF: sprintf(pExp, "COMPLEMENT_OF"); break;
    case EXPR_DIFF: sprintf(pExp, "DIFF"); break;
    case EXPR_ALLDIFF: sprintf(pExp, "ALLDIFF"); break;
    case EXPR_ANTISYMMETRIC: sprintf(pExp, "ANTISYMMETRIC"); break;
    case EXPR_FUNCTIONAL: sprintf(pExp, "FUNCTIONAL"); break;
    case EXPR_INVFUNCTIONAL: sprintf(pExp, "INV_FUNCTIONAL"); break;
    case EXPR_IRREFLEXIVE: sprintf(pExp, "IRREFLEXIVE"); break;
    case EXPR_REFLEXIVE: sprintf(pExp, "REFLEXIVE"); break;
    case EXPR_SYMMETRIC: sprintf(pExp, "SYMMETRIC"); break;
    case EXPR_TRANSITIVE: sprintf(pExp, "TRANSITIVE"); break;
    case EXPR_SUBPROPERTY: sprintf(pExp, "SUBPROPERTY"); break;
    case EXPR_EQUIVALENTPROPERTY: sprintf(pExp, "EQUVALENT_PROPERTY"); break;
    case EXPR_INVERSEPROPERTY: sprintf(pExp, "INVERSE_PROPERTY"); break;
    case EXPR_DOMAIN: sprintf(pExp, "DOMAIN"); break;
    case EXPR_RANGE: sprintf(pExp, "RANGE"); break;
    };
}

void printExprNodeWComment(char* pComment, const ExprNode* pNode)
{
  if( PARAMS_PRINT_DEBUGINFO_INHTML() )
    {
      for(int i = 0; i < g_iCommentIndent; i++ )
	printf("   ");
      printf("<comment>%s", pComment);
      printExprNode(pNode, TRUE);
      printf("</comment>\n");
    }
}

void printExprNode(const ExprNode* pNode, bool bNoNewLine, int iDepth)
{
  if( pNode->m_iExpression == EXPR_TERM )
    {
      if( pNode->m_cTerm[0] != '\0' )
	printf("(TERM %s)", pNode->m_cTerm);
      else
	printf("(I-TERM %d)", pNode->m_iTerm);

      if( iDepth == 0 && !bNoNewLine )
	printf("\n");
      return;
    }

  char cExp[1024];
  cExp[0] = '\0';
  getExpressionInStr(pNode->m_iExpression, cExp);

  printf("(%s (%d) ", cExp, pNode->m_iArity);
  if( pNode->m_cTerm[0] != '\0' )
    {
      printf("T(%s) ", pNode->m_cTerm);
    }
  else if( pNode->m_iTerm != 0 )
    {
      printf("I(%d) ", pNode->m_iTerm);
    }

  if( pNode->m_pArgs[0] )
    printExprNode((ExprNode*)pNode->m_pArgs[0], bNoNewLine, iDepth+1);
  if( pNode->m_pArgList )
    {
      for(int i = 0; i < ((ExprNodeList*)pNode->m_pArgList)->m_iUsedSize; i++ )
	printExprNode(((ExprNodeList*)pNode->m_pArgList)->m_pExprNodes[i], bNoNewLine, iDepth+1);
    }
  if( pNode->m_pArgs[1] )
    printExprNode((ExprNode*)pNode->m_pArgs[1], bNoNewLine, iDepth+1);
  if( pNode->m_pArgs[2] )
    printExprNode((ExprNode*)pNode->m_pArgs[2], bNoNewLine, iDepth+1);

  printf(")");
  if( iDepth == 0 && !bNoNewLine )
    printf("\n");
}

int isEqual(const ExprNode* pExpr, const ExprNode* pExpr2)
{
  //printExprNode(pExpr);
  //printf("-vs-\n");
  //printExprNode(pExpr2);
  //printf("**************\n");

  if( pExpr->m_iExpression != pExpr2->m_iExpression )
    {
      return (pExpr->m_iExpression-pExpr2->m_iExpression);
    }
  else if( pExpr->m_iArity != pExpr2->m_iArity )
    {
      return (pExpr->m_iArity-pExpr2->m_iArity);
    }
  else
    {
      int iStrCmp = strcmp(pExpr->m_cTerm, pExpr2->m_cTerm);
      if( iStrCmp != 0 )
	return iStrCmp;
      else if( pExpr->m_iTerm != pExpr2->m_iTerm )
	return (pExpr->m_iTerm-pExpr2->m_iTerm);
    }

  for(int i = 0; i < 3; i++ )
    {
      if( pExpr->m_pArgs[i] == NULL && pExpr2->m_pArgs[i] )
	return 1;
      else if( pExpr->m_pArgs[i]&& pExpr2->m_pArgs[i] == NULL )
	return -1;

      if( pExpr->m_pArgs[i] )
	{
	  int iIsEqual = isEqual((ExprNode*)pExpr->m_pArgs[i], (ExprNode*)pExpr2->m_pArgs[i]);
	  if( iIsEqual != 0 )
	    return iIsEqual;
	}
    }

  if( pExpr->m_pArgList && pExpr2->m_pArgList == NULL )
    return 1;
  else if( pExpr->m_pArgList == NULL && pExpr2->m_pArgList )
    return -1;

  if( pExpr->m_pArgList )
    {
      if( ((ExprNodeList*)pExpr->m_pArgList)->m_iUsedSize != ((ExprNodeList*)pExpr2->m_pArgList)->m_iUsedSize )
	return (((ExprNodeList*)pExpr->m_pArgList)->m_iUsedSize-((ExprNodeList*)pExpr2->m_pArgList)->m_iUsedSize);

      ExprNodeList* pList = (ExprNodeList*)pExpr->m_pArgList;
      ExprNodeList* pList2 = (ExprNodeList*)pExpr2->m_pArgList;
      
      for(int i = 0; i < pList->m_iUsedSize; i++ )
	{
	  int iIsEqual = isEqual(pList->m_pExprNodes[i], pList2->m_pExprNodes[i]);
	  if( iIsEqual != 0 )
	    return iIsEqual;
	}
    }

  return 0;
}

int isEqualList(const ExprNodeList* pExprNodeList1, const ExprNodeList* pExprNodeList2)
{
  if( pExprNodeList1->m_iUsedSize != pExprNodeList2->m_iUsedSize )
    return (pExprNodeList1->m_iUsedSize-pExprNodeList2->m_iUsedSize);

  for(int i = 0; i < pExprNodeList1->m_iUsedSize; i++ )
    {
      int iIsEqual = isEqual(pExprNodeList1->m_pExprNodes[i], pExprNodeList2->m_pExprNodes[i]);
      if( iIsEqual != 0 )
	return iIsEqual;
    }

  return 0;
}

bool isHasValue(ExprNode* pExprNode)
{
  return (pExprNode->m_iExpression == EXPR_SELF && ((ExprNode*)pExprNode->m_pArgs[1])->m_iExpression == EXPR_VALUE);
}

bool isLiteral(ExprNode* pExprNode)
{
  return (pExprNode->m_iExpression==EXPR_LITERAL);
}

bool isPrimitive(ExprNode* pExprNode)
{
  if( pExprNode->m_iArity == 0 )
    return TRUE;
  return FALSE;
}

bool isNegatedPrimitive(ExprNode* pExprNode)
{
  return (pExprNode->m_iExpression == EXPR_NOT && isPrimitive((ExprNode*)pExprNode->m_pArgs[0]));
}

bool isPrimitiveOrNegated(ExprNode* pExprNode)
{
  return (isPrimitive(pExprNode) || isNegatedPrimitive(pExprNode));
}

bool isAnon(ExprNode* pExprNode)
{
  return (strincmp(pExprNode->m_cTerm, "anon", 4)==0);
}

bool isAnonNominal(ExprNode* pExprNode)
{
  return (pExprNode->m_iExpression==EXPR_ANON_NOMINAL);
}

bool isComplexClass(ExprNode* pExprNode)
{
  if( pExprNode->m_iExpression == EXPR_ALL ||
      pExprNode->m_iExpression == EXPR_SOME ||
      pExprNode->m_iExpression == EXPR_MAX ||
      pExprNode->m_iExpression == EXPR_MIN ||
      pExprNode->m_iExpression == EXPR_CARD ||
      pExprNode->m_iExpression == EXPR_AND ||
      pExprNode->m_iExpression == EXPR_OR ||
      pExprNode->m_iExpression == EXPR_NOT ||
      pExprNode->m_iExpression == EXPR_VALUE ||
      pExprNode->m_iExpression == EXPR_SELF )
    return TRUE;
  return FALSE;
}

bool isNominal(ExprNode* pExprNode)
{
  return (pExprNode->m_iExpression == EXPR_VALUE);
}

bool isOneOf(ExprNode* pExprNode)
{
  if( pExprNode->m_iExpression != EXPR_OR )
    return FALSE;

  assert(pExprNode->m_pArgList);
  ExprNodeList* pOrList = (ExprNodeList*)pExprNode->m_pArgList;
  for(int i = 0; i < pOrList->m_iUsedSize; i++ )
    {
      if( !isNominal(pOrList->m_pExprNodes[i]) )
	return FALSE;
    }
  return TRUE;
}

bool isTransitiveChain(ExprNodeList* pChain, ExprNode* pR)
{
  return (pChain->m_iUsedSize == 2 && isEqual(pChain->m_pExprNodes[0], pR) == 0 && isEqual(pChain->m_pExprNodes[pChain->m_iUsedSize-1], pR) == 0);
}

/**
 * Simplify the term by making following changes:
 * and([]) -> TOP
 * all(p, TOP) -> TOP
 * min(p, 0) -> TOP
 * and([a1, and([a2,...,an])]) -> and([a1, a2, ..., an]))
 * and([a, not(a), ...]) -> BOTTOM
 * not(C) -> not(simplify(C))
 */
ExprNode* simplify(ExprNode* pExprNode)
{
  ExprNode* pSimplified = pExprNode;
  ExprNode* pArg1 = (ExprNode*)pExprNode->m_pArgs[0];
  ExprNode* pArg2 = (ExprNode*)pExprNode->m_pArgs[1];
  ExprNode* pArg3 = (ExprNode*)pExprNode->m_pArgs[2];

  if( (pArg1 == NULL&&pExprNode->m_iExpression!=EXPR_AND) ||
      pExprNode->m_iExpression == EXPR_SELF ||
      pExprNode->m_iExpression == EXPR_VALUE )
    {
      // do nothing because term is primitive or self restriction
    }
  else if( pExprNode->m_iExpression == EXPR_NOT )
    {
      if( pArg1->m_iExpression == EXPR_NOT )
	pSimplified = simplify( (ExprNode*)pArg1->m_pArgs[0] );
      else if( pArg1->m_iExpression == EXPR_MIN )
	{
	  if( ((ExprNode*)pArg1->m_pArgs[1])->m_iTerm == 0 )
	    pSimplified = EXPRNODE_BOTTOM;
	}
    }
  else if( pExprNode->m_iExpression == EXPR_AND )
    {
      if( pExprNode->m_pArgList == NULL || ((ExprNodeList*)pExprNode->m_pArgList)->m_iUsedSize == 0 )
	pSimplified = EXPRNODE_TOP;
      else
	{
	  ExprNodeSet setNodes;
	  ExprNodes aConjExprNodes, aNegations;

	  ExprNodeList* pAndList = (ExprNodeList*)pExprNode->m_pArgList;
	  for(int i = 0; i < pAndList->m_iUsedSize; i++ )
	    aConjExprNodes.push_back(pAndList->m_pExprNodes[i]);

	  for(ExprNodes::iterator i = aConjExprNodes.begin(); i != aConjExprNodes.end(); i++ )
	    {
	      ExprNode* pConj = (ExprNode*)*i;

	      if( isEqual(pConj, EXPRNODE_TOP) == 0 )
		continue;
	      else if( isEqual(pConj, EXPRNODE_BOTTOM) == 0 )
		return EXPRNODE_BOTTOM;
	      else if( pConj->m_iExpression == EXPR_AND )
		{
		  ExprNodeList* pAndList = (ExprNodeList*)pConj->m_pArgList;
		  for(int i = 0; i < pAndList->m_iUsedSize; i++ )
		    aConjExprNodes.push_back(pAndList->m_pExprNodes[i]);
		}
	      else if( pConj->m_iExpression == EXPR_NOT )
		aNegations.push_back(pConj);
	      else
		setNodes.insert(pConj);
	    }

	  for(ExprNodes::iterator i = aNegations.begin(); i != aNegations.end(); i++ )
	    {
	      ExprNode* pNotC = (ExprNode*)*i;
	      ExprNode* pC = (ExprNode*)pNotC->m_pArgs[0];
	      if( setNodes.find(pC) != setNodes.end() )
		return EXPRNODE_BOTTOM;
	    }

	  if( setNodes.size() == 0 )
	    {
	      if( aNegations.size() == 0 )
		return EXPRNODE_TOP;
	      else if( aNegations.size() == 1 )
		 return (ExprNode*)(*aNegations.begin());
	    }
	  else if( setNodes.size() == 1 && aNegations.size() == 0 )
	    return (ExprNode*)(*setNodes.begin());

	  pSimplified = createExprNode(EXPR_AND, (ExprNode*)NULL);

	  aNegations.insert(aNegations.begin(), setNodes.begin(), setNodes.end());
	  ExprNodeList* pSimplifiedList = createExprNodeList(aNegations.size());
	  for(ExprNodes::iterator i = aNegations.begin(); i != aNegations.end(); i++ )
	    pSimplifiedList->m_pExprNodes[pSimplifiedList->m_iUsedSize++] = (ExprNode*)*i;
	  pSimplified->m_pArgList = pSimplifiedList;
	  pSimplified->m_iArity = pSimplifiedList->m_iUsedSize;
	}
    }
  else if( pExprNode->m_iExpression == EXPR_ALL )
    {
      if( isEqual(pArg2, EXPRNODE_TOP) == 0 )
	return EXPRNODE_TOP;
    }
  else if( pExprNode->m_iExpression == EXPR_MIN )
    {
      if( pArg2->m_iTerm == 0 )
	pSimplified = EXPRNODE_TOP;
      if( isEqual(pArg3, EXPRNODE_BOTTOM) == 0 )
	pSimplified = EXPRNODE_BOTTOM;
    }
  else if( pExprNode->m_iExpression == EXPR_MAX )
    {
      if( pArg2->m_iTerm > 0 && isEqual(pArg3, EXPRNODE_BOTTOM) == 0 )
	pSimplified = EXPRNODE_TOP;
    }
  else
    assertFALSE("Unknown term type: ");

  return pSimplified;
}

ExprNodeList* normalize_list(ExprNodeList* pExprNodeList)
{
  ExprNodeList* pNewExprNodeList = createExprNodeList(pExprNodeList->m_iUsedSize);
  for(int i = 0; i < pExprNodeList->m_iUsedSize; i++ )
    pNewExprNodeList->m_pExprNodes[i] = normalize(pExprNodeList->m_pExprNodes[i]);
  pNewExprNodeList->m_iUsedSize = pExprNodeList->m_iUsedSize;
  return pNewExprNodeList;
}

ExprNodeList* negate_list(ExprNodeList* pExprNodeList)
{
  ExprNodeList* pNewExprNodeList = createExprNodeList(pExprNodeList->m_iUsedSize);
  for(int i = 0; i < pExprNodeList->m_iUsedSize; i++ )
    {
      ExprNode* pExprNode = pExprNodeList->m_pExprNodes[i];
      if( pExprNode->m_iExpression == EXPR_NOT )
	pNewExprNodeList->m_pExprNodes[i] = (ExprNode*)pExprNode->m_pArgs[0];
      else
	pNewExprNodeList->m_pExprNodes[i] = createExprNode(EXPR_NOT, pExprNode);
    }
  pNewExprNodeList->m_iUsedSize = pExprNodeList->m_iUsedSize;
  return pNewExprNodeList;
}

ExprNode* negate2(ExprNode* pExprNode)
{
  if( pExprNode->m_iExpression == EXPR_NOT )
    return (ExprNode*)pExprNode->m_pArgs[0];
  return createExprNode(EXPR_NOT, pExprNode);
}

/**
 * Normalize the term by making following changes:
 * - or([a1, a2,..., an]) -> not(and[not(a1), not(a2), ..., not(an)]])
 * - some(p, c) -> all(p, not(c))
 * - max(p, n) -> not(min(p, n+1))
 */
ExprNode* normalize(ExprNode* pExprNode)
{
  ExprNode* pNormalized = pExprNode;
  ExprNode* pArg1 = (ExprNode*)pExprNode->m_pArgs[0];
  ExprNode* pArg2 = (ExprNode*)pExprNode->m_pArgs[1];
  ExprNode* pArg3 = (ExprNode*)pExprNode->m_pArgs[2];

  if( (pArg1 == NULL && pExprNode->m_iExpression != EXPR_AND && pExprNode->m_iExpression != EXPR_OR) || 
      (pExprNode->m_pArgList == NULL && (pExprNode->m_iExpression == EXPR_AND || pExprNode->m_iExpression == EXPR_OR)) ||
      pExprNode->m_iExpression == EXPR_SELF ||
      pExprNode->m_iExpression == EXPR_VALUE ||
      pExprNode->m_iExpression == EXPR_INV )
    {
      // do nothing because these terms cannot be decomposed any further
    }
  else if( pExprNode->m_iExpression == EXPR_NOT )
    {
      if( !isPrimitive(pArg1) )
	pNormalized = simplify( createExprNode(EXPR_NOT, normalize(pArg1)) );
    }
  else if( pExprNode->m_iExpression == EXPR_AND )
    {
      pNormalized = simplify( createExprNode(EXPR_AND,  normalize_list((ExprNodeList*)pExprNode->m_pArgList)) );
    }
  else if( pExprNode->m_iExpression == EXPR_OR )
    {
      ExprNode* pAnd = createExprNode(EXPR_AND, negate_list((ExprNodeList*)pExprNode->m_pArgList));
      ExprNode* pNotAnd = createExprNode(EXPR_NOT, pAnd);
      pNormalized = normalize(pNotAnd);
    }
  else if( pExprNode->m_iExpression == EXPR_ALL )
    {
      pNormalized = simplify( createExprNode(EXPR_ALL, pArg1, normalize(pArg2)) );
    }
  else if( pExprNode->m_iExpression == EXPR_SOME )
    {
      pNormalized = normalize( createExprNode(EXPR_NOT, createExprNode(EXPR_ALL, pArg1, createExprNode(EXPR_NOT, pArg2))) );
    }
  else if( pExprNode->m_iExpression == EXPR_MAX )
    {
      ExprNode* pNewN = getNewExprNode();
      pNewN->m_iExpression = EXPR_TERM;
      pNewN->m_iTerm = pArg2->m_iTerm+1;
      pNewN->m_cTerm[0] = '\0';

      pNormalized = normalize( createExprNode(EXPR_NOT, createExprNode(EXPR_MIN, pArg1, pNewN, pArg3)) );
    }
  else if( pExprNode->m_iExpression == EXPR_MIN )
    {
      pNormalized = simplify( createExprNode(EXPR_MIN, pArg1, pArg2, normalize(pArg3)) );
    }
  else if( pExprNode->m_iExpression == EXPR_CARD )
    {
      ExprNode* pNormMin = simplify( createExprNode(EXPR_MIN, pArg1, pArg2, normalize(pArg3)) );
      ExprNode* pNormMax = normalize( createExprNode(EXPR_MAX, pArg1, pArg2, pArg3) );

      pNormalized = simplify( createExprNode(EXPR_AND, pNormMin, pNormMax) );
    }
  else
    assertFALSE("Unknown concept type: ");

  return pNormalized;
}

// return the term in NNF form, i.e. negation only occurs in front of atomic
ExprNodeList* nnf_list(ExprNodeList* pList)
{
  ExprNodeList* pNewList = createExprNodeList(pList->m_iUsedSize);
  pNewList->m_iUsedSize = pList->m_iUsedSize;
  for(int i = 0; i < pList->m_iUsedSize; i++ )
    pNewList->m_pExprNodes[i] = nnf(pList->m_pExprNodes[i]);
  return pNewList;
}

ExprNode* nnf(ExprNode* pTerm)
{
  if( pTerm == NULL )
    return NULL;

  ExprNode* pNewTerm = NULL;
  if( pTerm->m_iExpression == EXPR_NOT )
    {
      // Take the first argument to the NOT, then check
      // the type of that argument to determine what needs to be done.
      assert(pTerm->m_iArity==1);
      ExprNode* pArg = (ExprNode*)pTerm->m_pArgs[0];
      if( pArg->m_iArity == 0 )
	pNewTerm = pTerm;
      else if( pArg->m_iExpression == EXPR_NOT )
	{
	  // double negation
	  pNewTerm = nnf((ExprNode*)pArg->m_pArgs[0]);
	}
      else if( pArg->m_iExpression == EXPR_VALUE || pArg->m_iExpression == EXPR_SELF )
	pNewTerm = pTerm;
      else if( pArg->m_iExpression == EXPR_MAX )
	{
	  ExprNode* pN = (ExprNode*)pArg->m_pArgs[1];
	  ExprNode* pNewN = getNewExprNode();
	  pNewN->m_iExpression = EXPR_TERM;
	  pNewN->m_iTerm = pN->m_iTerm+1;
	  pNewTerm = createExprNode(EXPR_MIN, (ExprNode*)pArg->m_pArgs[0], pNewN, nnf((ExprNode*)pArg->m_pArgs[2]));
	}
      else if( pArg->m_iExpression == EXPR_MIN )
	{
	  if( ((ExprNode*)pArg->m_pArgs[1])->m_iTerm == 0 )
	    pNewTerm = EXPRNODE_BOTTOM;
	  else
	    {
	      ExprNode* pNewN = getNewExprNode();
	      pNewN->m_iExpression = EXPR_TERM;
	      pNewN->m_iTerm = ((ExprNode*)pArg->m_pArgs[1])->m_iTerm-1;
	      pNewTerm = createExprNode(EXPR_MAX, (ExprNode*)pArg->m_pArgs[0], pNewN, nnf((ExprNode*)pArg->m_pArgs[2]));
	    }
	}
      else if( pArg->m_iExpression == EXPR_CARD )
	{
	  pNewTerm = nnf(createExprNode(EXPR_NOT, createExactCard((ExprNode*)pArg->m_pArgs[0], ((ExprNode*)pArg->m_pArgs[1])->m_iTerm, (ExprNode*)pArg->m_pArgs[2])));
	}
      else if( pArg->m_iExpression == EXPR_AND )
	{
	  if( pArg->m_pArgList )
	    pNewTerm = createExprNode(EXPR_OR, nnf_list(negate_list((ExprNodeList*)pArg->m_pArgList)));
	  else
	    pNewTerm = createExprNode(EXPR_OR, nnf(negate2((ExprNode*)pArg->m_pArgs[0])));
	}
      else if( pArg->m_iExpression == EXPR_OR )
	{
	  if( pArg->m_pArgList )
	    pNewTerm = createExprNode(EXPR_AND, nnf_list(negate_list((ExprNodeList*)pArg->m_pArgList)));
	  else
	    pNewTerm = createExprNode(EXPR_AND, nnf(negate2((ExprNode*)pArg->m_pArgs[0])));
	}
      else if( pArg->m_iExpression == EXPR_SOME )
	{
	  pNewTerm = createExprNode(EXPR_ALL, (ExprNode*)pArg->m_pArgs[0], nnf(createExprNode(EXPR_NOT, (ExprNode*)pArg->m_pArgs[1])));
	}
      else if( pArg->m_iExpression == EXPR_ALL)
	{
	  pNewTerm = createExprNode(EXPR_SOME, (ExprNode*)pArg->m_pArgs[0], nnf(createExprNode(EXPR_NOT, (ExprNode*)pArg->m_pArgs[1])));
	}
      else
	assertFALSE("Unknown term type: ");
    }
  else if( pTerm->m_iExpression == EXPR_MIN || pTerm->m_iExpression == EXPR_MAX || pTerm->m_iExpression == EXPR_SELF )
    {
      pNewTerm = pTerm;
    }
  else if( pTerm->m_iExpression == EXPR_CARD )
    {
      pNewTerm = nnf( createExactCard((ExprNode*)pTerm->m_pArgs[0], ((ExprNode*)pTerm->m_pArgs[1])->m_iTerm, (ExprNode*)pTerm->m_pArgs[2]) );
    }
  else
    {
      // Return the term with all of its arguments in nnf
      if( pTerm->m_iExpression == EXPR_AND || pTerm->m_iExpression == EXPR_OR )
	{
	  ExprNodeList* pNewList = createExprNodeList((ExprNodeList*)pTerm->m_pArgList);
	  pNewTerm = createExprNode(pTerm->m_iExpression, pNewList);
	}
      else
	{
	  pNewTerm = createExprNode(pTerm->m_iExpression, 
				    nnf((ExprNode*)pTerm->m_pArgs[0]),
				    nnf((ExprNode*)pTerm->m_pArgs[1]),
				    nnf((ExprNode*)pTerm->m_pArgs[2]));
	  if( pTerm->m_cTerm[0] != '\0' )
	    sprintf(pNewTerm->m_cTerm, "%s", pTerm->m_cTerm);
	  else
	    pNewTerm->m_cTerm[0] = '\0';
	  pNewTerm->m_iTerm = pTerm->m_iTerm;
	}
    }

  assert(pNewTerm);
  return pNewTerm;
}

ExprNode* parseExprNode(KRSSDef* pDef, KnowledgeBase* pKB)
{
  ExprNode* pENode = getNewExprNode();

  if( pDef->m_iDefinition == -1 )
    {
      pENode->m_iExpression = EXPR_TERM;
      moveStr(pENode->m_cTerm, pDef->m_cDefinition, strlen(pDef->m_cDefinition));
    }
  
  switch(pDef->m_iDefinition)
    {
    case DEFINE_TOP:
      {
	pENode = EXPRNODE_TOP;
	break;
      }
    case DEFINE_BOTTOM:
      {
	pENode = EXPRNODE_BOTTOM;
	break;
      }
    case DEFINE_NOT:
      {
	pENode->m_iExpression = EXPR_NOT;
	ExprNode* pEN = parseExprNode((KRSSDef*)pDef->m_pDefinitionArgs, pKB);
	if( isPrimitive(pEN) )
	  pKB->addClass(pEN);
	pENode->m_pArgs[0] = pEN;
	pENode->m_iArity = 1;
	break;
      }
    case DEFINE_AND:
    case DEFINE_OR:
    case DEFINE_ONEOF:
      // EXPR[C]
      {
	assert(pDef->m_pDefinitionArgs);

	if( pDef->m_iDefinition == DEFINE_OR || pDef->m_iDefinition == DEFINE_ONEOF )
	  pENode->m_iExpression = EXPR_OR;
	else
	  pENode->m_iExpression = EXPR_AND;

	// count args first
	int iArgCount = 0;
	KRSSDef* pDefArg = (KRSSDef*)pDef->m_pDefinitionArgs;
	while(pDefArg)
	  {
	    iArgCount++;
	    pDefArg = (KRSSDef*)pDefArg->m_pNext;
	  }	
	ExprNodeList* pArgList = createExprNodeList(iArgCount);
	pENode->m_pArgList = pArgList;

	pArgList->m_iUsedSize = 0;
	ExprNode* pExprArg = NULL;
	pDefArg = (KRSSDef*)pDef->m_pDefinitionArgs;
	while(pDefArg)
	  {
	    ExprNode* pEN = parseExprNode(pDefArg, pKB);
	    if( pDef->m_iDefinition == DEFINE_ONEOF )
	      {
		pKB->addIndividual(pEN);
		pEN = createExprNode(EXPR_VALUE, pEN);
	      }
	    else if( isPrimitive(pEN) )
	      pKB->addClass(pEN);

	    pArgList->m_pExprNodes[pArgList->m_iUsedSize++] = pEN;
	    pDefArg = (KRSSDef*)pDefArg->m_pNext;
	  }
	pENode->m_iArity = pArgList->m_iUsedSize;

	if( pArgList->m_iUsedSize == 1 )
	  pENode = pArgList->m_pExprNodes[0];
	break;
      }
    case DEFINE_ALL:
    case DEFINE_SOME:
      // EXPR[R C]
      {
	KRSSDef* pDefArg = (KRSSDef*)pDef->m_pDefinitionArgs;
	ExprNode* pRoleExpr = parseExprNode(pDefArg, pKB);
	pKB->addObjectProperty(pRoleExpr);

	pDefArg = (KRSSDef*)pDefArg->m_pNext;
	assert(pDefArg);

	ExprNode* pClassExpr = parseExprNode(pDefArg, pKB);
	if( isPrimitive(pClassExpr) )
	  pKB->addClass(pClassExpr);

	if( pDef->m_iDefinition == DEFINE_ALL )
	  pENode->m_iExpression = EXPR_ALL;
	else
	  pENode->m_iExpression = EXPR_SOME;
	  
	pENode->m_pArgs[0] = pRoleExpr;
	pENode->m_pArgs[1] = pClassExpr;
	pENode->m_iArity = 2;
	break;
      }
    case DEFINE_ATMOST:
    case DEFINE_ATLEAST:
    case DEFINE_EXACTLY:
      // EXPR[R N (C|TOP)]
      {
	if( pDef->m_iDefinition == DEFINE_ATMOST )
	  pENode->m_iExpression = EXPR_MAX;
	else if( pDef->m_iDefinition == DEFINE_ATLEAST )
	  pENode->m_iExpression = EXPR_MIN;
	else
	  pENode->m_iExpression = EXPR_CARD;

	KRSSDef* pDefArg = (KRSSDef*)pDef->m_pDefinitionArgs;
	ExprNode* pNumberExpr = createIntTerm(pDefArg->m_cDefinition);

	pDefArg = (KRSSDef*)pDefArg->m_pNext;
	assert(pDefArg);

	ExprNode* pRoleExpr = parseExprNode(pDefArg, pKB);
	pKB->addObjectProperty(pRoleExpr);

	pENode->m_pArgs[0] = pRoleExpr;
	pENode->m_pArgs[1] = pNumberExpr;
	pENode->m_pArgs[2] = EXPRNODE_TOP;
	pENode->m_iArity = 3;

	if(pDefArg->m_pNext )
	  {
	    pDefArg = (KRSSDef*)pDefArg->m_pNext;
	    pENode->m_pArgs[2] = parseExprNode(pDefArg, pKB);

	    // CHECK HERE : not included?
	    //if( isPrimitive(pClassExpr) )
	    //pKB->addClass(pClassExpr);
	  }
	break;
      }
    case DEFINE_A:
      {
	KRSSDef* pDefArg = (KRSSDef*)pDef->m_pDefinitionArgs;
	ExprNode* pRoleExpr = createStrTerm(pDefArg->m_cDefinition);
	pKB->addDatatypeProperty(pRoleExpr);
	pKB->addFunctionalProperty(pRoleExpr);

	pENode->m_iExpression = EXPR_MIN;
	pENode->m_iArity = 3;
	pENode->m_pArgs[0] = pRoleExpr;
	
	ExprNode* pNumTermExpr = getNewExprNode();
	pNumTermExpr->m_iExpression = EXPR_TERM;
	pNumTermExpr->m_iTerm = 1;
	pENode->m_pArgs[1] = pNumTermExpr;
	pENode->m_pArgs[2] = EXPRNODE_TOPLITERAL;

	break;
      }
    case DEFINE_MIN:
    case DEFINE_MAX:
    case DEFINE_EQUAL:
      {
	// REQUIRES DATATYPE
	break;
      }
    case DEFINE_INV:
      {
	KRSSDef* pDefArg = (KRSSDef*)pDef->m_pDefinitionArgs;
	ExprNode* pRoleExpr = parseExprNode(pDefArg, pKB);
	pKB->addObjectProperty(pRoleExpr);
	
	Role* pRole = pKB->getProperty(pRoleExpr);
	// CHECK HERE : ExpressionNode.c
	// Can we assign it directly or create a copy?
	pENode = pRole->m_pInverse->m_pName;
	break;
      }
    };

  return pENode;
}

void convertKRSSDef2ExprNode(KRSSDef* pDef, KnowledgeBase* pKB)
{
  switch(pDef->m_iDefinition)
    {
    case DEFINE_ROLE:
      {
	bool bDataProperty = (stricmp(pDef->m_cDefinition, "define-datatype-property") == 0);
	bool bFunctional = (stricmp(pDef->m_cDefinition, "define-primitive-attribute") == 0)||(stricmp(pDef->m_cDefinition, "defprimattribute") == 0);
	// CHECK HERE 
	// boolean primDef = str.indexOf( "PRIm" ) != -1;
	bool bPrimDef = FALSE;

	KRSSDef* pDefArg = (KRSSDef*)pDef->m_pDefinitionArgs;
	ExprNode* pRTerm = createStrTerm(pDefArg->m_cDefinition);

	if( bDataProperty )
	  pKB->addDatatypeProperty(pRTerm);
	else
	  {
	    pKB->addObjectProperty(pRTerm);
	    if( bFunctional )
	      pKB->addFunctionalProperty(pRTerm);
	  }

	pDefArg = (KRSSDef*)pDefArg->m_pNext;
	while(pDefArg)
	  {
	    if( pDefArg->m_cDefinition[0] == ':' )
	      {
		if( stricmp(pDefArg->m_cDefinition, ":parents") == 0 )
		  {
		    pDefArg = (KRSSDef*)pDefArg->m_pNext;
		    assert(pDefArg);
		    
		    ExprNode* pSTerm = createStrTerm(pDefArg->m_cDefinition);
		    pKB->addObjectProperty(pSTerm);
		    pKB->addSubProperty(pRTerm, pSTerm);
		    
		    KRSSDef* pInnerDefArg = (KRSSDef*)pDefArg->m_pDefinitionArgs;
		    while(pInnerDefArg)
		      {
			ExprNode* pTermExpr = createStrTerm(pInnerDefArg->m_cDefinition);
			
			pKB->addObjectProperty(pTermExpr);
			pKB->addSubProperty(pRTerm, pTermExpr);
			pInnerDefArg = (KRSSDef*)pInnerDefArg->m_pNext;
		      }

		    // pass the STerm
		    pDefArg = (KRSSDef*)pDefArg->m_pNext;
		  }
		else if( stricmp(pDefArg->m_cDefinition, ":feature") == 0 )
		  {
		    // pass ':feature'
		    pDefArg = (KRSSDef*)pDefArg->m_pNext;
		    assert( stricmp(pDefArg->m_cDefinition, "t") == 0 );

		    pKB->addFunctionalProperty(pRTerm);

		  }
		else if( stricmp(pDefArg->m_cDefinition, ":transitive") == 0 )
		  {
		    // pass ':transitive'
		    pDefArg = (KRSSDef*)pDefArg->m_pNext;
		    assert( stricmp(pDefArg->m_cDefinition, "t") == 0 );

		    pKB->addTransitiveProperty(pRTerm);
		  }
		else if( stricmp(pDefArg->m_cDefinition, ":range") == 0 )
		  {
		    // pass ':range'
		    pDefArg = (KRSSDef*)pDefArg->m_pNext;

		    ExprNode* pRangeExpr = parseExprNode(pDefArg, pKB);
		    pKB->addClass(pRangeExpr);
		    pKB->addRange(pRTerm, pRangeExpr);
		  }
		else if( stricmp(pDefArg->m_cDefinition, ":domain") == 0 )
		  {
		    // pass ':domain'
		    pDefArg = (KRSSDef*)pDefArg->m_pNext;

		    ExprNode* pDomainExpr = parseExprNode(pDefArg, pKB);
		    pKB->addClass(pDomainExpr);
		    pKB->addDomain(pRTerm, pDomainExpr);
		  }
		else if( stricmp(pDefArg->m_cDefinition, ":inverse") == 0 )
		  {
		    // pass ':inverse'
		    pDefArg = (KRSSDef*)pDefArg->m_pNext;

		    ExprNode* pInverseExpr = createStrTerm(pDefArg->m_cDefinition);
		    pKB->addInverseProperty(pRTerm, pInverseExpr);
		  }
		else
		  assertFALSE("Invalid role spec ");
	      }
	    else if( stricmp(pDefArg->m_cDefinition, "domain-range") == 0 )
	      {
		KRSSDef* pInnerDefArg = (KRSSDef*)pDefArg->m_pDefinitionArgs;
		assert(pInnerDefArg);

		ExprNode* pDomainExpr = createStrTerm(pInnerDefArg->m_cDefinition);
		pInnerDefArg = (KRSSDef*)pInnerDefArg->m_pNext;
		ExprNode* pRangeExpr = createStrTerm(pDefArg->m_cDefinition);

		pKB->addDomain(pRTerm, pDomainExpr);
		pKB->addRange(pRTerm, pRangeExpr);

		// pass 'domain-range'
		pDefArg = (KRSSDef*)pDefArg->m_pNext;
	      }
	    else
	      {
		ExprNode* pS = parseExprNode(pDefArg, pKB);

		if( bDataProperty )
		  pKB->addDatatypeProperty(pS);
		else
		  pKB->addObjectProperty(pRTerm);

		if( bPrimDef )
		  pKB->addSubProperty(pRTerm, pS);
		else
		  pKB->addEquivalentProperty(pRTerm, pS);

		// pass this 
		pDefArg = (KRSSDef*)pDefArg->m_pNext;
	      }
	  }
	  
	break;
      }
    case DEFINE_PRIMITIVE_CONCEPT:
      {
	KRSSDef* pDefArg = (KRSSDef*)pDef->m_pDefinitionArgs;
	ExprNode* pClassTerm = createStrTerm(pDefArg->m_cDefinition);
	pKB->addClass(pClassTerm);

	if( pDefArg->m_pNext )
	  {
	    ExprNode* pExpr = parseExprNode((KRSSDef*)pDefArg->m_pNext, pKB);
	    pKB->addClass(pExpr);
	    pKB->addSubClass(pClassTerm, pExpr);
	  }
	break;
      }
    case DEFINE_DISJOINT_PRIMITIVE_CONCEPT:
      {
	KRSSDef* pDefArg = (KRSSDef*)pDef->m_pDefinitionArgs;
	ExprNode* pClassTerm = createStrTerm(pDefArg->m_cDefinition);
	pKB->addClass(pClassTerm);

	if( pDefArg->m_pNext )
	  {
	    KRSSDef* pDisjointPrimConcepts = (KRSSDef*)pDefArg->m_pNext;
	    KRSSDef* pC = (KRSSDef*)pDisjointPrimConcepts->m_pNext;
	    assert(pDisjointPrimConcepts->m_iDefinition==-1&&pC);

	    ExprNode* pDExpr = parseExprNode(pDisjointPrimConcepts, pKB);
	    pKB->addDisjointClasses(pClassTerm, pDExpr);

	    pDisjointPrimConcepts = (KRSSDef*)pDisjointPrimConcepts->m_pDefinitionArgs;
	    while(pDisjointPrimConcepts)
	      {
		pDExpr = parseExprNode(pDisjointPrimConcepts, pKB);
		pKB->addDisjointClasses(pClassTerm, pDExpr);
		pDisjointPrimConcepts = (KRSSDef*)pDisjointPrimConcepts->m_pNext;
	      }

	    ExprNode* pExpr = parseExprNode(pC, pKB);
	    pKB->addSubClass(pClassTerm, pExpr);
	  }
	break;
      }
    case DEFINE_CONCEPT:
      {
	KRSSDef* pDefArg = (KRSSDef*)pDef->m_pDefinitionArgs;
	ExprNode* pClassTerm = createStrTerm(pDefArg->m_cDefinition);
	pKB->addClass(pClassTerm);

	pDefArg = (KRSSDef*)pDefArg->m_pNext;
	ExprNode* pCExpr = parseExprNode(pDefArg, pKB);
	pKB->addEquivalentClasses(pClassTerm, pCExpr);
	break;
      }
    case DEFINE_CONCEPT_SUBSUMES:
      {
	// IMPLIES, IMPLIES_C
	KRSSDef* pDefArg = (KRSSDef*)pDef->m_pDefinitionArgs;
	ExprNode* pC1 = parseExprNode(pDefArg, pKB);
	pDefArg = (KRSSDef*)pDefArg->m_pNext;
	ExprNode* pC2 = parseExprNode(pDefArg, pKB);

	pKB->addClass(pC1);
	pKB->addClass(pC2);
	pKB->addSubClass(pC1, pC2);
	break;
      }
    case DEFINE_ROLE_SUBSUMES:
      {
	// IMPLIES_R
	KRSSDef* pDefArg = (KRSSDef*)pDef->m_pDefinitionArgs;
	ExprNode* pP1 = parseExprNode(pDefArg, pKB);
	pDefArg = (KRSSDef*)pDefArg->m_pNext;
	ExprNode* pP2 = parseExprNode(pDefArg, pKB);

	pKB->addProperty(pP1);
	pKB->addProperty(pP2);
	pKB->addSubProperty(pP1, pP2);
	break;
      }
    case DEFINE_ROLE_EQUIVALANCE:
      {
	// EQUALS_R
	KRSSDef* pDefArg = (KRSSDef*)pDef->m_pDefinitionArgs;
	ExprNode* pP1 = parseExprNode(pDefArg, pKB);
	pDefArg = (KRSSDef*)pDefArg->m_pNext;
	ExprNode* pP2 = parseExprNode(pDefArg, pKB);

	pKB->addObjectProperty(pP1);
	pKB->addObjectProperty(pP2);
	pKB->addEquivalentProperty(pP1, pP2);
	break;
      }
    case DEFINE_DOMAIN:
    case DEFINE_RANGE:
      {
	KRSSDef* pDefArg = (KRSSDef*)pDef->m_pDefinitionArgs;
	ExprNode* pP = parseExprNode(pDefArg, pKB);
	pDefArg = (KRSSDef*)pDefArg->m_pNext;
	ExprNode* pC = parseExprNode(pDefArg, pKB);

	pKB->addProperty(pP);
	pKB->addClass(pC);

	if( pDef->m_iDefinition == DEFINE_DOMAIN )
	  pKB->addDomain(pP, pC);
	else
	  pKB->addRange(pP, pC);
	break;
      }
    case DEFINE_FUNCTIONAL:
      {
	KRSSDef* pDefArg = (KRSSDef*)pDef->m_pDefinitionArgs;
	ExprNode* pP = parseExprNode(pDefArg, pKB);

	pKB->addProperty(pP);
	pKB->addFunctionalProperty(pP);
	break;
      }
    case DEFINE_TRANSITIVE:
      {
	KRSSDef* pDefArg = (KRSSDef*)pDef->m_pDefinitionArgs;
	ExprNode* pP = parseExprNode(pDefArg, pKB);

	pKB->addObjectProperty(pP);
	pKB->addTransitiveProperty(pP);
	break;
      }
    case DEFINE_DISJOINT:
      {
	KRSSDef* pDefArg = (KRSSDef*)pDef->m_pDefinitionArgs;
	ExprNodesVector aDisjoints;
	ExprNode* pDisj = parseExprNode(pDefArg, pKB);
	while(pDisj)
	  {
	    aDisjoints.push_back(pDisj);
	    pDefArg = (KRSSDef*)pDefArg->m_pNext;
	    if( pDefArg )
	      pDisj = parseExprNode(pDefArg, pKB);
	    else
	      break;
	  }

	int iSize = aDisjoints.size();
	for(int i = 0; i < iSize; i++ )
	  {
	    ExprNode* pC1 = aDisjoints.at(i);
	    for(int j = i+1; j < iSize; j++ )
	      {
		ExprNode* pC2 = aDisjoints.at(j);
		pKB->addClass(pC2);
		pKB->addDisjointClass(pC1, pC2);
	      }
	  }

	break;
      }
    case DEFINE_INDIVIDUAL:
      {
	KRSSDef* pDefArg = (KRSSDef*)pDef->m_pDefinitionArgs;
	ExprNode* pIndividualTerm = createStrTerm(pDefArg->m_cDefinition);
	pKB->addIndividual(pIndividualTerm);
	break;
      }
    case DEFINE_INSTANCE:
      {
	KRSSDef* pDefArg = (KRSSDef*)pDef->m_pDefinitionArgs;
	ExprNode* pX = createStrTerm(pDefArg->m_cDefinition);
	pDefArg = (KRSSDef*)pDefArg->m_pNext;
	ExprNode* pC = parseExprNode(pDefArg, pKB);

	pKB->addIndividual(pX);
	pKB->addType(pX, pC);
	break;
      }
    case DEFINE_RELATED:
      {
	KRSSDef* pDefArg = (KRSSDef*)pDef->m_pDefinitionArgs;
	ExprNode* pX = createStrTerm(pDefArg->m_cDefinition);
	pDefArg = (KRSSDef*)pDefArg->m_pNext;
	ExprNode* pY = createStrTerm(pDefArg->m_cDefinition);
	pDefArg = (KRSSDef*)pDefArg->m_pNext;
	ExprNode* pR = createStrTerm(pDefArg->m_cDefinition);

	pKB->addIndividual(pX);
	pKB->addIndividual(pY);
	pKB->addPropertyValue(pR, pX, pY);
	break;
      }
    case DEFINE_DIFFERENT:
      {
	KRSSDef* pDefArg = (KRSSDef*)pDef->m_pDefinitionArgs;
	ExprNode* pX = createStrTerm(pDefArg->m_cDefinition);
	pDefArg = (KRSSDef*)pDefArg->m_pNext;
	ExprNode* pY = createStrTerm(pDefArg->m_cDefinition);

	pKB->addIndividual(pX);
	pKB->addIndividual(pY);
	pKB->addDifferent(pX, pY);
	break;
      }
    case DEFINE_DATATYPE_ROLE_FILLER:
      {
	KRSSDef* pDefArg = (KRSSDef*)pDef->m_pDefinitionArgs;
	ExprNode* pX = createStrTerm(pDefArg->m_cDefinition);
	pDefArg = (KRSSDef*)pDefArg->m_pNext;
	ExprNode* pY = createExprNode(EXPR_LITERAL, createStrTerm(pDefArg->m_cDefinition));
	ExprNode* pR = createStrTerm(pDefArg->m_cDefinition);

	pKB->addIndividual(pX);
	pKB->addIndividual(pY);
	pKB->addPropertyValue(pR, pX, pY);
	break;
      }
    default:
      {
	assertFALSE("Unknown command ");
      }
    };
}

void getPrimitives(ExprNodeList* pList, ExprNodeSet* pPrimitives)
{
  for(int i = 0; i < pList->m_iUsedSize; i++ )
    {
      if( isPrimitive(pList->m_pExprNodes[i]) )
	pPrimitives->insert(pList->m_pExprNodes[i]);
    }
}

void findPrimitives(ExprNode* pExpr, ExprNodes* pList, bool bSkipRestrictions, bool bSkipTopLevel)
{
  if( isPrimitive(pExpr) )
    pList->push_back(pExpr);
  else if( pExpr->m_iExpression == EXPR_SELF || pExpr->m_iExpression == EXPR_VALUE )
    {
      // do nothing because there is no atomic concept here
    }
  else if( pExpr->m_iExpression == EXPR_NOT )
    {
      ExprNode* pL = (ExprNode*)pExpr->m_pArgs[0];
      if( !isPrimitive(pL) || !bSkipTopLevel )
	findPrimitives(pL, pList, bSkipRestrictions, FALSE);
    }
  else if( pExpr->m_iExpression == EXPR_AND || pExpr->m_iExpression == EXPR_OR )
    {
      assert(pExpr->m_pArgList);
      ExprNodeList* pArgList = (ExprNodeList*)pExpr->m_pArgList;
      for(int i = 0; i < pArgList->m_iUsedSize; i++ )
	{
	  ExprNode* pArg = pArgList->m_pExprNodes[i];
	  if( !isNegatedPrimitive(pArg) || !bSkipTopLevel )
	    findPrimitives(pArg, pList, bSkipRestrictions, FALSE);
	}
    }
  else if( !bSkipRestrictions )
    {
      if( pExpr->m_iExpression == EXPR_ALL || pExpr->m_iExpression == EXPR_SOME )
	{
	  ExprNode* pArg = (ExprNode*)pExpr->m_pArgs[1];
	  findPrimitives(pArg, pList, bSkipRestrictions, FALSE);
	}
      else if( pExpr->m_iExpression == EXPR_MAX || pExpr->m_iExpression == EXPR_MIN || pExpr->m_iExpression == EXPR_CARD )
	{
	  ExprNode* pArg = (ExprNode*)pExpr->m_pArgs[2];
	  findPrimitives(pArg, pList, bSkipRestrictions, FALSE);
	}
      else
	assertFALSE("Unknown concept type ");
    }
}


/************************************************************************************/

bool containsAll(ExprNodeSet* pSet, ExprNodes* pExprList)
{
  for(ExprNodes::iterator i = pExprList->begin(); i != pExprList->end(); i++) 
    {
      ExprNode* pExpr = *i;
      if( pSet->find(pExpr) == pSet->end() )
	return FALSE;
    }
  return TRUE;
}

bool containsAll(ExprNodeSet* pSet1, ExprNodeSet* pSet2)
{
  for(ExprNodeSet::iterator i = pSet2->begin(); i != pSet2->end(); i++) 
    {
      ExprNode* pExpr = *i;
      if( pSet1->find(pExpr) == pSet1->end() )
	return FALSE;
    }
  return TRUE;
}

int isEqualSet(const ExprNodeSet* pExprNodeSet1, const ExprNodeSet* pExprNodeSet2)
{
  if( pExprNodeSet1->size() != pExprNodeSet2->size() )
    return (pExprNodeSet1->size() - pExprNodeSet2->size());

  if( pExprNodeSet1 > pExprNodeSet2 )
    {
      for(ExprNodeSet::iterator i = pExprNodeSet1->begin(); i != pExprNodeSet1->end(); i++) 
	{
	  ExprNode* pExpr = *i;
	  if( pExprNodeSet2->find(pExpr) == pExprNodeSet2->end() )
	    return 1;
	}
      for(ExprNodeSet::iterator i = pExprNodeSet2->begin(); i != pExprNodeSet2->end(); i++) 
	{
	  ExprNode* pExpr = *i;
	  if( pExprNodeSet1->find(pExpr) == pExprNodeSet1->end() )
	    return -1;
	}
    }
  else
    {
      for(ExprNodeSet::iterator i = pExprNodeSet2->begin(); i != pExprNodeSet2->end(); i++) 
	{
	  ExprNode* pExpr = *i;
	  if( pExprNodeSet1->find(pExpr) == pExprNodeSet1->end() )
	    return -1;
	}
      for(ExprNodeSet::iterator i = pExprNodeSet1->begin(); i != pExprNodeSet1->end(); i++) 
	{
	  ExprNode* pExpr = *i;
	  if( pExprNodeSet2->find(pExpr) == pExprNodeSet2->end() )
	    return 1;
	}
    }
  return 0;
}

bool isEqualSet(ExprNodeSet* pSet1, ExprNodeSet* pSet2)
{
  if( pSet1->size() != pSet2->size() )
    return FALSE;

  for(ExprNodeSet::iterator i = pSet1->begin(); i != pSet1->end(); i++) 
    {
      ExprNode* pExpr = *i;
      if( pSet2->find(pExpr) == pSet2->end() )
	return FALSE;
    }
  return TRUE;
}

bool contains(ExprNodes* pList, ExprNode* pExprNode)
{
  for(ExprNodes::iterator i = pList->begin(); i != pList->end(); i++ )
    {
      if( isEqual(((ExprNode*)*i), pExprNode) == 0 )
	return TRUE;
    }
  return FALSE;
}

void removeAll(ExprNodes* pList, ExprNodeSet* pToBeRemovedSet)
{
  for(ExprNodeSet::iterator i = pToBeRemovedSet->begin(); i != pToBeRemovedSet->end(); i++ )
    {
      ExprNode* pToBeRemoved = (ExprNode*)*i;
      for(ExprNodes::iterator j = pList->begin(); j != pList->end(); j++ )
	{
	  if( isEqual(pToBeRemoved, ((ExprNode*)*j)) == 0 )
	    {
	      pList->erase(j);
	      break;
	    }
	}
    }
}

void removeAll(ExprNodeSet* pSet, ExprNodeSet* pToBeRemovedSet)
{
  for(ExprNodeSet::iterator i = pToBeRemovedSet->begin(); i != pToBeRemovedSet->end(); i++ )
    pSet->erase((ExprNode*)*i);
}

void retainAll(ExprNodes* pList, ExprNodeSet* pToBeChecked)
{
  bool bRemoved = FALSE;
  do
    {
      bRemoved = FALSE;
      for(ExprNodes::iterator j = pList->begin(); j != pList->end(); j++ )
	{
	  ExprNode* pExprNode = (ExprNode*)*j;
	  if( pToBeChecked->find(pExprNode) == pToBeChecked->end() )
	    {
	      // remove it
	      pList->erase(j);
	      bRemoved = TRUE;
	      break;
	    }
	}
    }
  while(bRemoved==TRUE);
}

void copySet(ExprNodeSet* pTarget, ExprNodeSet* pSource)
{
  pTarget->insert(pSource->begin(), pSource->end());
}
