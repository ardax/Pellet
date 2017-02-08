#include "QueueElement.h"

QueueElement::QueueElement(ExprNode* pN, ExprNode* pL)
{
  m_pLabel = pL;
  m_pNode = pN;
}
