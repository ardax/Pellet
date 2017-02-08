#ifndef _CACHED_NODE_
#define _CACHED_NODE_

class Individual;
class DependencySet;

class CachedNode
{
 public:
  CachedNode(Individual* pNode, DependencySet* pDS);

  void setMaxSize(int iMax);
  bool isBottom();
  bool isComplete();

  static CachedNode* createNode(Individual* pNode, DependencySet* pDepends);
  static CachedNode* createSatisfiableNode();
  static CachedNode* createBottomNode();
  static CachedNode* createTopNode();

  Individual* m_pNode;
  DependencySet* m_pDepends;
};

#endif
