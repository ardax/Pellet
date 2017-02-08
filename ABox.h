#ifndef _ABOX_
#define _ABOX_

#include "ExpressionNode.h"
#include "Node.h"
#include "Individual.h"
#include "Branch.h"
#include "DependencySet.h"
#include "NodeMerge.h"
#include "ZList.h"

typedef map<ExprNode*, Node*, strCmpExprNode> Expr2NodeMap;
class CompletionQueue;
class Individual;
class Literal;
class Clash;
class ConceptCache;
class CachedNode;
class State;
class TransitionGraph;

class ABox
{
 public:
  ABox();
  ABox(ABox* pABox);
  ABox(ABox* pABox, ExprNode* pExtraIndividual, bool bCopyIndividuals);
  ~ABox();

  void init(ABox* pABox, ExprNode* pExtraIndividual, bool bCopyIndividuals);

  Individual* addIndividual(ExprNode* pIndvidual);
  Individual* addFreshIndividual(bool bIsNominal);
  Individual* addIndividual(ExprNode* pIndvidual, int iNominalLevel);
  void  addType(ExprNode* pX, ExprNode* pC);
  void  addType(ExprNode* pX, ExprNode* pC, DependencySet* pDS);
  void  addDifferent(ExprNode* pX, ExprNode* pC);
  Literal* addLiteral();
  Literal* addLiteral(ExprNode* p, DependencySet* pDS = NULL);
  Literal* createLiteral(ExprNode* pDataValue, DependencySet* pDS = NULL);

  void addEffected(int iBranchIndex, ExprNode* pExprNode);

  ExprNode* createUniqueName(bool bIsNominal);

  void removeType(ExprNode* pX, ExprNode* pC);

  void cache(Individual* pRootNode, ExprNode* pC, bool bIsConsistent);
  void collectComplexPropertyValues(Individual* pSubj);
  void collectComplexPropertyValues(Individual* pSubj, Role* pRole);
  void getObjectPropertyValues(ExprNode* pS, Role* pRole, ExprNodeSet* pKnowns, ExprNodeSet* pUnknowns, bool bGetSames);
  void getSimpleObjectPropertyValues(Individual* pS, Role* pRole, ExprNodeSet* pKnowns, ExprNodeSet* pUnknowns, bool bGetSames);
  void getTransitivePropertyValues(Individual* pS, Role* pRole, ExprNodeSet* pKnowns, ExprNodeSet* pUnknowns, bool bGetSames, ExprNodeSet* pVisited, bool bIsIndependent);
  void getComplexObjectPropertyValues(Individual* pS, State* pState, TransitionGraph* pTG, ExprNodeSet* pKnowns, ExprNodeSet* pUnknowns, bool bGetSames, IndividualStatePairSet* pVisited, bool bIsIndependent);
  void getSames(Individual* pInd, ExprNodeSet* pKnowns, ExprNodeSet* pUnknowns);
  void clearCaches(bool clearSatCache);
  CachedNode* getCached(ExprNode* pC);

  Node* getNode(ExprNode* pNode);
  Individual* getIndividual(ExprNode* pNode);
  Literal* getLiteral(ExprNode* pNode);
  void  setSyntacticUpdate(bool bSyntacticUpdate = TRUE) { m_bSyntacticUpdate = bSyntacticUpdate; }
  ABox* getPseudoModel();
 
  void getObviousTypes(ExprNode* pX, ExprNodes* pTypes, ExprNodes* pNonTypes);

  void setClash(Clash* pClash);
  Clash* getClash() { return m_pClash; }

  int getBranch() { return m_iCurrentBranchIndex; }
  void setBranch(int i) { m_iCurrentBranchIndex = i; }
  void incrementBranch();

  int size() { return m_mNodes.size(); }

  bool doExplanation() { return m_bDoExplanation; }

  bool isInitialized();
  void setInitialized(bool bInitialized) { m_bInitialized = bInitialized; }

  int mergable(Individual* pRoot1, Individual* pRoot2, bool bIndependent);
  int isType(Individual* pNode, ExprNode* pC, bool bIsIndependent);
  int isKnownSubClassOf(ExprNode* pC1, ExprNode* pC2);
  bool isSubClassOf(ExprNode* pC1, ExprNode* pC2);

  bool isConsistent();
  bool isConsistent(ExprNodeSet* pIndividuals, ExprNode* pC, bool bCacheModel);
  bool isIncrementallyConsistent();

  bool isSatisfiable(ExprNode* pC);
  bool isSatisfiable(ExprNode* pC, bool bCacheModel);
  bool isType(ExprNode* pX, ExprNode* pC);
  bool isType(ExprNodes* pList, ExprNode* pC);
  int  isKnownType(ExprNode* pX, ExprNode* pC, ExprNodeSet* pSubs);
  int  isKnownType(Individual* pNode, ExprNode* pC, ExprNodeSet* pSubs);

  bool isEmpty() { return m_mNodes.size()==0; }
  bool isClosed();

  ABox* copy() { return new ABox(this); }
  ABox* copy(ExprNode* pExtraIndividual, bool bCopyIndividuals) { return new ABox(this, pExtraIndividual, bCopyIndividuals); }
  void copyOnWrite();

  void validate();
  void validate(Individual* pNode);

  int getCachedSat(ExprNode* pC);

  ABox* m_pPseudoModel;

  bool m_bDoExplanation;
  bool m_bChanged;

  BranchList m_aBranches;
  int  m_iCurrentBranchIndex;
  int  m_iBranchSize;
  // flag set when incrementally updating the abox with explicit assertions
  bool m_bSyntacticUpdate;

  bool m_bRulesNotApplied;

  CompletionQueue* m_pCompletionQueue;

  // Individuals updated
  NodeSet m_setUpdatedIndividuals;
  // New individuals added - used for adding UC to new individuals
  NodeSet m_setNewIndividuals;

  /**
   * This is a list of nodes. Each node has a name expressed as an ATerm which
   * is used as the key in the Hashtable. The value is the actual node object
   */
  Expr2NodeMap m_mNodes;

  /**
   * This is a list of node names. This list stores the individuals in the
   * order they are created
   */
  ExprNodes m_aNodeList;

  ExprNodeMap m_mTypeAssertions;

  Clash* m_pClash;

  // cached satisfiability results
  // the table maps every atomic concept A (and also its negation not(A))
  // to the root node of its completed tree. If a concept is mapped to
  // null value it means it is not satisfiable
  ConceptCache* m_pCache;

  // cache of the last completion. it may be different from the pseudo
  // model, e.g. type checking for individual adds one extra assertion
  // last completion is stored for caching the root nodes that was
  // the result of
  ABox* m_pLastCompletion;
  bool m_bKeepLastCompletion;
  Clash* m_pLastClash;

  long m_iConsistencyCount;
  long m_iSatisfiabilityCount;

  // return true if init() function is called. This indicates parsing
  // is completed and ABox is ready for completion
  bool m_bInitialized;

  // complete ABox means no more tableau rules are applicable
  bool m_bIsComplete;

  // following two variables are used to generate names
  // for newly generated individuals. so during rules are
  // applied anon1, anon2, etc. will be generated. This prefix
  // will also make sure that any node whose name starts with
  // this prefix is not a root node
  int m_iAnonCount;

  // if we are using copy on write, this is where to copy from
  ABox* m_pSourceABox;

  NodeMerges m_listToBeMerged;

  int m_iTreeDepth;

  map<ExprNode*, ICollection*> m_mDisjBranchStats;
};

#endif
