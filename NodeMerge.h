#ifndef _NODE_MERGE_
#define _NODE_MERGE_

#include <list>
#include "ExpressionNode.h"
class DependencySet;
class Node;

class NodeMerge
{
 public:
  NodeMerge();
  NodeMerge(Node* pY, Node* pZ, DependencySet* pDS);
  NodeMerge(Node* pY, Node* pZ);
  NodeMerge(ExprNode* pY, ExprNode* pZ);

  ExprNode* m_pY;
  ExprNode* m_pZ;

  DependencySet* m_pDS;
};

typedef list<NodeMerge*> NodeMerges;

#endif
