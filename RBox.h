#ifndef _RBOX_
#define _RBOX_

#include "ExpressionNode.h"
#include "Role.h"
#include "State.h"

class DependencySet;
class TransitionGraph;
class Taxonomy;

typedef map<ExprNode*, Role*, strCmpExprNode> Expr2RoleMap;

class RBox
{
 public:
  RBox();

  Role* addRole(ExprNode* p);
  bool  addSubRole(ExprNode* pSub, ExprNode* pSup, DependencySet* pDS = NULL);
  Role* addDatatypeRole(ExprNode* p);
  Role* addObjectRole(ExprNode* p);
  bool  addInverseRole(ExprNode* pS, ExprNode* pR, DependencySet* pDS);
  bool  addEquivalentRole(ExprNode* pS, ExprNode* pR, DependencySet* pDS = NULL);

  Role* getRole(ExprNode* p);
  Role* getDefinedRole(ExprNode* p);

  void ignoreTransitivity(Role* pRole);
  ExprNodeList* inverseExprList(ExprNodeList* pList);

  Taxonomy* getTaxonomy();

  void prepare();
  void computeImmediateSubRoles(Role* pRole, RoleSet* pSubRoles, ExprNodeListSet* pSubChains, Expr2DependencySetMap* pDependencies);
  void computeSubRoles(Role* pRole, RoleSet* pSubRoles, ExprNodeListSet* pSubRoleChains, Expr2DependencySetMap* pDependencies, DependencySet* pDS);

  TransitionGraph* buildDFA(Role* pRole, RoleSet* pVisited);
  TransitionGraph* buildNFA(Role* pRole, RoleSet* pVisited);
  TransitionGraph* handleSymmetry(Role* pRole, TransitionGraph* pTG);
  void addTransition(TransitionGraph* pTG, Role* pS, ExprNodeList* pChain);
  void setFSM(Role* pRole, TransitionGraph* pTG);
  TransitionGraph* mirror(TransitionGraph* pTG);
  State* copyState(State* pOldState, TransitionGraph* pTG, StateMap* pNewStates);

  bool m_bConsistent;

  Expr2RoleMap m_mRoles;
  RoleSet m_setReflexiveRoles;
  Taxonomy* m_pTaxonomy;
};

#endif
