#ifndef _EDGELIST_
#define _EDGELIST_

#include "Edge.h"
#include "Role.h"
#include "ExpressionNode.h"

#include <vector>
using namespace std;

class Individual;
class Role;
class Node;
class DependencySet;

typedef vector<Edge*> EdgeVector;

class EdgeList
{
 public:
  EdgeList();

  void addEdge(Edge* pEdge);
  bool removeEdge(Edge* pEdge);

  void findEdges(Role* pRole, Individual* pFrom, Node* pTo, EdgeList* pEdgeList);
  void getEdges(Role* pRole, EdgeList* pEdgeList);
  void getEdgesTo(Node* pTo, EdgeList* pEdgeList);
  void getNeighbors(Node* pNode, void* pSetNeighbors);
  void getPredecessors(void* pPredecessors);
  void getSuccessors(void* pSuccessors);
  void getRoles(RoleSet* pRoles);
  DependencySet* getDepends(bool bDoExplanation);

  bool hasEdge(Role* pRole);
  bool hasEdge(Individual* pFrom, Role* pRole, Node* pTo);
  bool hasEdge(Edge* pEdge);
  bool hasEdgeTo(Node* pNode);

  void getFilteredNeighbors(Individual* pNode, ExprNode* pC, void* pNodes);
  
  int size() { return m_listEdge.size(); }

  EdgeVector m_listEdge;
};

#endif
