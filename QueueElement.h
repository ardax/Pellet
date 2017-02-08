#ifndef _QUEUE_ELEMENT_
#define _QUEUE_ELEMENT_

#include "ExpressionNode.h"

// Structured stored on the completion queue
class QueueElement
{
 public:
  QueueElement(ExprNode* pN, ExprNode* pL);

  ExprNode* m_pLabel;
  ExprNode* m_pNode;
};

#endif
