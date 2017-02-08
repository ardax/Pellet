#ifndef _NODE_
#define _NODE_

#include "Utils.h"
#include "TextUtils.h"
#include "DependencySet.h"
#include "ExpressionNode.h"
#include "EdgeList.h"
#include <limits.h>

#define NODE_BLOCKABLE INT_MAX
#define NODE_NOMINAL 0

#define NODE_CHANGED 0x7F
#define NODE_UNCHANGED 0x00

class Node;
int isEqual(const Node* pNode, const Node* pOtherNode);
struct strCmpNode 
{
  bool operator()( const Node* pN1, const Node* pN2 ) const {
    return isEqual(pN1, pN2) < 0;
  }
};
typedef list<Node*> Nodes;
typedef set<Node*, strCmpNode> NodeSet;
typedef map<ExprNode*, DependencySet*, strCmpExprNode> ExprNode2DependencySetMap;
typedef map<Node*, DependencySet*, strCmpNode> Node2DependencySetMap;
typedef map<Node*, ExprNode*, strCmpNode> Node2ExprNodeMap;



class ABox;

class Node
{
 public:
  Node(ExprNode* pName, ABox* pABox);
  Node(Node* pNode, ABox* pABox);

  enum 
    {
      ATOM = 0,
      OR,
      SOME,
      ALL,
      MIN,
      MAX,
      NOM,
      TYPES = 7,
    };
  
  virtual void addType(ExprNode* pC, DependencySet* pDS);
  virtual void removeType(ExprNode* pC);
  virtual bool restore(int iBranch);
  virtual void unprune(int iBranch);

  bool hasType(ExprNode* pC);
  void setDifferent(Node* pNode, DependencySet* pDS);


  virtual void addInEdge(Edge* pEdge);
  bool removeInEdge(Edge* pEdge);
  virtual Node* copyTo(ABox* pABox);
  virtual void prune(DependencySet* pDS) = 0;
  virtual bool hasSuccessor(Node* pX) = 0;
  virtual bool isBlockable() = 0;
  virtual int getNominalLevel() = 0;
  Node* copy();
  

  DependencySet* getMergeDependency(bool bAll);
  DependencySet* getDepends(ExprNode* pC);
  virtual DependencySet* getDifferenceDependency(Node* pNode);
  void getPredecessors(NodeSet* pAncestors);
  void updateNodeReferences();

  Node* getSame();

  void undoSetSame();
  void removeMerged(Node* pNode);

  virtual bool isNominal();
  virtual bool isIndividual() { return FALSE; }

  bool isPruned();
  bool isMerged();
  bool isDifferent(Node* pNode);
  bool isSame(Node* pNode);
  bool isRootNominal();
  bool isRoot();
  bool isConceptRoot() { return m_bIsConceptRoot; }
  bool isNamedIndividual() { return (m_bIsRoot && !m_bIsConceptRoot && !isBNode()); }
  bool isBNode(){ return (strincmp(m_pName->m_cTerm, "bNode", 5)==0); }

  bool isChanged(int iType);
  void setChanged(bool bChanged);
  void setChanged(int iType, bool bChanged);
  
  bool hasObviousType(ExprNode* pC);
  bool hasObviousType(ExprNodeSet* pSet);

  void setSame(Node* pNode, DependencySet* pDS);
  void inheritDifferents(Node* pY, DependencySet* pDS);

  Individual* getParent();
  void getPath(ExprNodes* pPath);

  void addMerged(Node* pNode);
  

  ABox* m_pABox;
  ExprNode* m_pName;

  bool m_bLiteralNode;
  bool m_bIsMerged;
  bool m_bIsRoot;
  bool m_bIsConceptRoot;

  int m_iStatus;
  int m_iBranch;
  int m_iDepth;

  EdgeList m_listInEdges;

  Node* m_pMergeTo;
  NodeSet m_setMerged;

  DependencySet* m_pMergeDepends;
  DependencySet* m_pPruned;

  ExprNode2DependencySetMap m_mDepends;
  Node2DependencySetMap m_mDifferents;
};

#endif
