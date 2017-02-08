#ifndef _COMPLETION_QUEUE_
#define _COMPLETION_QUEUE_

#include "ExpressionNode.h"
#include "ZList.h"
#include <vector>

class QueueElement;
class ABox;

class CompletionQueue
{
 public:
  CompletionQueue();
  ~CompletionQueue();
  
  // Indexes for the various queues
  enum QTypes
    {
      GUESSLIST = 0,
      NOMLIST,
      MAXLIST,
      DATATYPELIST,
      ATOMLIST,
      ORLIST,
      SOMELIST,
      MINLIST,
      LITERALLIST,
      ALLLIST,
      CHOOSELIST,
      SIZE = 11
    };

  void addEffected(int iBranchIndex, ExprNode* pExprNode);
  void removeEffect(int iBranchIndex, ExprNodeSet* pEffected);
  void add(QueueElement* pElement, int iType);

  void incrementBranch(int iBranch);

  void init(int iType);
  bool hasNext(int iType);
  void* getNext(int iType);
  void findNext(int iType);
  
  void restore(int iBranch);

  CompletionQueue* copy();

  ABox* m_pABox;

  vector<ExprNodeSet*> m_listBranchEffects;
  vector<ICollections*> m_aBranches;

  // This queue is the global queue which will be used for explicit types. 
  // During an incremental update, this is the queue which new elements 
  // will be pushed onto
  ObjectArray m_gQueue[11]; // as SIZE = 11

  //The queue - array - each entry is an arraylist for a particular rule type 
  ObjectArray m_Queue[11];  // as SIZE = 11

  // List of current index pointer for each global queue
  int m_gCurrent[11];
  // List of current index pointer for each queue
  int m_Current[11];
  // List of current index pointer for the stopping point at each queue 
  int m_CutOff[11];
};

#endif
