#include "NodeMerge.h"
#include "Node.h"

NodeMerge::NodeMerge()
{
  m_pY = NULL;
  m_pZ = NULL;
  m_pDS = NULL;
}

NodeMerge::NodeMerge(Node* pY, Node* pZ, DependencySet* pDS = NULL)
{
  m_pY = pY->m_pName;
  m_pZ = pZ->m_pName;
  m_pDS = pDS;
}

NodeMerge::NodeMerge(Node* pY, Node* pZ)
{
  m_pY = pY->m_pName;
  m_pZ = pZ->m_pName;
  m_pDS = NULL;
}

NodeMerge::NodeMerge(ExprNode* pY, ExprNode* pZ)
{
  m_pY = pY;
  m_pZ = pZ;
  m_pDS = NULL;
}
