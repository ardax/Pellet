#ifndef _EDGE_
#define _EDGE_


#include <set>
using namespace std;

class Role;
class Individual;
class Node;
class DependencySet;

class Edge
{
 public:
  Edge(Role* pName, Individual* pFrom, Node* pTo);
  Edge(Role* pName, Individual* pFrom, Node* pTto, DependencySet* pDS);

  Node* getNeighbor(Node* pNode);

  void computeHashCode();
  bool isEqual(const Edge* pEdge) const;

  Role* m_pRole;
  Individual* m_pFrom;
  Node* m_pTo;
  DependencySet* m_pDepends;

  int m_iHashCode;
};

struct strCmpEdge 
{
  bool operator()( const Edge* pN1, const Edge* pN2 ) const {
    return pN1->isEqual(pN2 );
  }
};

typedef set<Edge*, strCmpEdge> EdgeSet;


#endif
