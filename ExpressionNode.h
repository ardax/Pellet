#ifndef _EXPRESSION_NODE_
#define _EXPRESSION_NODE_

#include "KRSSLoader.h"
#include <set>
#include <vector>

typedef vector<void*> ObjectArray;
typedef vector<int> ints;

enum ExpressionTypes
  {
    EXPR_INVALID = 0,

    EXPR_LIST,
    EXPR_TERM,

    EXPR_AND,
    EXPR_OR,
    EXPR_NOT,
    EXPR_ALL,
    EXPR_SOME,
    //EXPR_SUB,

    EXPR_MIN,
    EXPR_MAX,
    EXPR_CARD,
    EXPR_SELF,
    EXPR_INV,
    EXPR_VALUE,
    
    EXPR_SUBCLASSOF,
    EXPR_EQCLASSES,
    EXPR_SAMEAS,
    EXPR_DISJOINTWITH,
    EXPR_DISJOINTCLASSES,
    EXPR_COMPLEMENTOF,

    EXPR_DIFF,
    EXPR_ALLDIFF,
    EXPR_ANTISYMMETRIC,
    EXPR_FUNCTIONAL,
    EXPR_INVFUNCTIONAL,
    EXPR_IRREFLEXIVE,
    EXPR_REFLEXIVE,
    EXPR_SYMMETRIC,
    EXPR_TRANSITIVE,
    EXPR_SUBPROPERTY,
    EXPR_EQUIVALENTPROPERTY,
    EXPR_INVERSEPROPERTY,
    EXPR_DOMAIN,
    EXPR_RANGE,

    //This is used to represent variables in queries
    EXPR_VAR,
    EXPR_TYPE,
    EXPR_PROP,
    EXPR_IPTRIPLE,
    EXPR_DPTRIPLE,

    EXPR_LITERAL,
    EXPR_TOPLITERAL,
    EXPR_BOTLITERAL,

    EXPR_ANON_NOMINAL,
  };

typedef struct _ExpressionNode_
{
  short m_iExpression;
  char  m_cTerm[200];
  int   m_iTerm;
  void* m_pArgs[3];
  void* m_pArgList;
  short m_iArity;
} ExprNode;

typedef struct _ExpressionNodes_
{
  ExprNode** m_pExprNodes;
  int m_iListSize;
  int m_iUsedSize;
} ExprNodeList;

typedef list<ExprNode*> ExprNodes;
typedef vector<ExprNode*> ExprNodesVector;

/*
typedef struct _ExpressionNodeSet_
{
  ExprNode** m_pExprNodes;
  int m_iSetFullSize;
  int m_iSetSize;
} ExprNodeSet;

typedef struct _SetOfExpressionNodeSet_
{
  ExprNodeSet** m_pExprNodeSets;
  int m_iSetFullSize;
  int m_iSetSize;
} SetOfExprNodeSet;
*/

class KnowledgeBase;

/* EXPRESSIONS */
void initExprNodes();

int isEqual(const ExprNode* pExprNode, const ExprNode* pOtherExprNode);

struct strCmpExprNode 
{
  bool operator()( const ExprNode* pEN1, const ExprNode* pEN2 ) const {
    return isEqual( pEN1, pEN2 ) < 0;
  }
};
typedef set<ExprNode*, strCmpExprNode> ExprNodeSet;
int isEqualSet(const ExprNodeSet* pExprNodeSet1, const ExprNodeSet* pExprNodeSet2);
struct strCmpExprNodeSet
{
  bool operator()( const ExprNodeSet* pENS1, const ExprNodeSet* pENS2 ) const {
    return isEqualSet( pENS1, pENS2 ) < 0;
  }
};
typedef set<ExprNodeSet*, strCmpExprNodeSet> SetOfExprNodeSet;
typedef map<ExprNode*, ExprNode*, strCmpExprNode> ExprNodeMap;
typedef map<ExprNode*, ExprNodeSet*, strCmpExprNode> ExprNode2ExprNodeSetMap;
typedef map<ExprNode*, ExprNodeList*, strCmpExprNode> ExprNode2ExprNodeListMap;
typedef map<ExprNode*, ExprNodes*, strCmpExprNode> ExprNode2ExprNodesMap;
typedef map<ExprNode*, bool, strCmpExprNode> ExprNode2Bool;
typedef map<ExprNode*, int, strCmpExprNode> ExprNode2Int;
typedef map<ExprNode*, SetOfExprNodeSet*, strCmpExprNode> Expr2SetOfExprNodeSets;
typedef map<ExprNode*, ExprNodeSet*, strCmpExprNode> Expr2ExprNodeSet;


int isEqualList(const ExprNodeList* pExprNodeList1, const ExprNodeList* pExprNodeList2);
struct strCmpExprNodeList
{
  bool operator()( const ExprNodeList* pENL1, const ExprNodeList* pENL2 ) const {
    return isEqualList( pENL1, pENL2 ) < 0;
  }
};
typedef set<ExprNodeList*, strCmpExprNodeList> ExprNodeListSet;
typedef pair<ExprNode*, ExprNodeSet*> Expr2ExprSetPair;
typedef list<Expr2ExprSetPair*> Expr2ExprSetPairList;

void printExprNode(const ExprNode* pNode, bool bNoNewLine = FALSE, int iDepth = 0);
void printExprNodeWComment(char* pComment, const ExprNode* pNode);

ExprNode* getNewExprNode();
ExprNode* createExprNode(int iExpressionType, ExprNode* pArg1, ExprNode* pArg2 = NULL, ExprNode* pArg3 = NULL);
ExprNode* createExprNode(int iExpressionType, ExprNode* pArg1, int iArg2, ExprNode* pArg3);
ExprNode* createStrTerm(char* pTerm);
ExprNode* createInverse(ExprNode* pArg);
ExprNode* createExprNode(int iExpressionType, ExprNodeList* pArgList, ExprNode* pArg2 = NULL, ExprNode* pArg3 = NULL);
ExprNode* createExactCard(ExprNode* pR, int n, ExprNode* pC);
ExprNode* createSimplifiedAnd(ExprNodeSet* pSet);
ExprNode* createNormalizedMax(ExprNode* pR, int n, ExprNode* pC);
ExprNodeList* createExprNodeList(int iSize = 0);
ExprNodeList* createExprNodeList(ExprNodeList* pList);
ExprNodeList* createExprNodeList(ExprNode* pExprNode, int iSize = 5);
ExprNodeList* convertExprSet2List(ExprNodeSet* pSet);
void pop_front(ExprNodeList* pList);

void addExprToList(ExprNodeList* pList, ExprNode* pExpr);


bool isPrimitive(ExprNode* pExprNode);
bool isPrimitiveOrNegated(ExprNode* pExprNode);
bool isNegatedPrimitive(ExprNode* pExprNode);
bool isComplexClass(ExprNode* pExprNode);
bool isOneOf(ExprNode* pExprNode);
bool isLiteral(ExprNode* pExprNode);
bool isNominal(ExprNode* pExprNode);
bool isTransitiveChain(ExprNodeList* pChain, ExprNode* pR);
bool isTop(ExprNode* pExprNode);
bool isHasValue(ExprNode* pExprNode);
bool isAnon(ExprNode* pExprNode);
bool isAnonNominal(ExprNode* pExprNode);


ExprNode* normalize(ExprNode* pExprNode);
ExprNode* simplify(ExprNode* pExprNode);
ExprNode* negate2(ExprNode* pExprNode);
ExprNodeList* negate2_list(ExprNodeList* pList);
ExprNode* nnf(ExprNode* pExprNode);
ExprNodeList* nnf_list(ExprNodeList* pList);
ExprNodeList* negate_list(ExprNodeList* pExprNodeList);
ExprNodeList* normalize_list(ExprNodeList* pExprNodeList);

void convertKRSSDef2ExprNode(KRSSDef* pDef, KnowledgeBase* pKB);

void findPrimitives(ExprNode* pExpr, ExprNodes* pList, bool bSkipRestrictions = FALSE, bool bSkipTopLevel = FALSE);
void getPrimitives(ExprNodeList* pList, ExprNodeSet* pPrimitives);

/* EXPRESSIONS SET & LIST */
bool isEqualSet(ExprNodeSet* pSet1, ExprNodeSet* pSet2);
bool containsAll(ExprNodeSet* pSet, ExprNodes* pExprList);
bool containsAll(ExprNodeSet* pSet1, ExprNodeSet* pSet2);
bool contains(ExprNodes* pList, ExprNode* pExprNode);
void removeAll(ExprNodes* pList, ExprNodeSet* pToBeRemovedSet);
void removeAll(ExprNodeSet* pSet, ExprNodeSet* pToBeRemovedSet);
void retainAll(ExprNodes* pList, ExprNodeSet* pToBeChecked);
void copySet(ExprNodeSet* pTarget, ExprNodeSet* pSource);

#endif
