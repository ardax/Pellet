#ifndef _ROLE_
#define _ROLE_

#include "Utils.h"
#include "ExpressionNode.h"
#include "DependencySet.h"

class TransitionGraph;
class Role;
int isEqual(const Role* pRole1, const Role* pRole2);
struct strCmpRole 
{
  bool operator()( const Role* pR1, const Role* pR2 ) const {
    return isEqual( pR1, pR2 ) < 0;
  }
};
typedef set<Role*, strCmpRole> RoleSet;
void intersectRoleSets(RoleSet* pSet1, RoleSet* pSet2, RoleSet* pTarget);
void differenceRoleSets(RoleSet* pSet1, RoleSet* pSet2, RoleSet* pTarget);
bool isEqualSet(RoleSet* pSet1, RoleSet* pSet2);

class Role
{
 public:
  Role(ExprNode* pRoleExpr = NULL);
  Role(ExprNode* pRoleExpr, int iRoleType);
  
  enum RoleTypes
    {
      UNTYPED = 0,
      OBJECT,
      DATATYPE,
      ANNOTATION,
      ONTOLOGY
    };

  enum Flags
    {
      TRANSITIVE     = 0x01,
      FUNCTIONAL     = 0x02,
      INV_FUNCTIONAL = 0x04,
      REFLEXIVE      = 0x08,
      IRREFLEXIVE    = 0x10,
      ANTI_SYM       = 0x20,
      SIMPLE         = 0x40,    
      COMPLEX_SUB    = 0x80,
      FORCE_SIMPLE   = 0x100,
    };

  void addDomain(ExprNode* pC, DependencySet* pDS = NULL);
  void addDomains(ExprNodeSet* pSet);
  void addRange(ExprNode* pC, DependencySet* pDS = NULL);
  void addRanges(ExprNodeSet* pSet);
  void addSubRole(Role* p, DependencySet* pDS = NULL);
  void addSubRoleChain(ExprNodeList* paChain, DependencySet* pDS = NULL);
  void addSuperRole(Role* p, DependencySet* pDS = NULL);
  void addTransitiveSubRole(Role* p);
  void addFunctionalSuper(Role* p);

  void removeSubRoleChain(ExprNodeList* paChain);
  void removeSubRoleChains();

  void normalizeRole();

  void setSubRolesAndChains(RoleSet* pSubRoles, ExprNodeListSet* pSubRoleChains, Expr2DependencySetMap* pDependencies);
  void setInverse(Role* pTerm, DependencySet* pDS = NULL);
  void setFunctional(bool b, DependencySet* pDS = NULL);
  void setTransitive(bool b);
  void setTransitive(bool b, DependencySet* pDS);
  void setInverseFunctional(bool b, DependencySet* pDS = NULL);
  void setAntisymmetric(bool b, DependencySet* pDS = NULL);
  void setReflexive(bool b, DependencySet* pDS = NULL);
  void setIrreflexive(bool b, DependencySet* pDS = NULL);
  void setSimple(bool b);
  void setForceSimple(bool b);
  void setHasComplexSubRole(bool b);

  void getProperSubRoles(RoleSet* pProperSubRoles);

  DependencySet* getExplainDomain(ExprNode* pRange);
  DependencySet* getExplainRange(ExprNode* pDomain);
  DependencySet* getExplainSub(ExprNode* pR);
  DependencySet* getExplainSuper(ExprNode* pR);

  bool isEquivalent(Role* pRole);
  
  bool isInExplainDomain(ExprNode* pRange);
  bool isInExplainRange(ExprNode* pDomain);

  bool isObjectRole();
  bool isDatatypeRole();

  bool isSubRoleOf(Role* pRole);
  bool isSuperRoleOf(Role* pRole);

  bool hasNamedInverse();
  bool hasComplexSubRole() { return (m_iFlags & COMPLEX_SUB) != 0; }
  bool isFunctional() { return (m_iFlags & FUNCTIONAL) != 0; }
  bool isInverseFunctional() { return (m_iFlags & INV_FUNCTIONAL) != 0; }
  bool isSymmetric() { (m_pInverse && isEqual(m_pInverse, this)); }
  bool isAntisymmetric() { return (m_iFlags & ANTI_SYM) != 0; }
  bool isTransitive() { return (m_iFlags & TRANSITIVE) != 0; }
  bool isReflexive() { return (m_iFlags & REFLEXIVE) != 0; }
  bool isIrreflexive() { return (m_iFlags & IRREFLEXIVE) != 0; }
  bool isAnon() { return (m_pName->m_iArity != 0); }
  bool isSimple() { return (m_iFlags & SIMPLE) != 0; }
  bool isForceSimple() { return (m_iFlags & FORCE_SIMPLE) != 0; }

  static ExprNode* getTop(Role* pRole);

  ExprNode* m_pDomain;
  ExprNode* m_pRange;
  ExprNode* m_pName;

  ExprNodeSet m_setDomains;
  ExprNodeSet m_setRanges;

  ExprNodeListSet m_setSubRoleChains;

  RoleSet m_setSubRoles;
  RoleSet m_setSuperRoles;

  RoleSet m_setDisjointRoles;
  RoleSet m_setTransitiveSubRoles;
  RoleSet m_setFunctionalSupers;

  Role* m_pInverse;

  int m_iRoleType;
  int m_iFlags;

  DependencySet* m_pExplainFunctional;
  DependencySet* m_pExplainInverse;
  DependencySet* m_pExplainDomain;
  DependencySet* m_pExplainRange;
  DependencySet* m_pExplainTransitive;
  DependencySet* m_pExplainAntisymmetric;
  DependencySet* m_pExplainReflexive;
  DependencySet* m_pExplainIrreflexive;
  DependencySet* m_pExplainInverseFunctional;

  Expr2DependencySetMap m_mapExplainDomains;
  Expr2DependencySetMap m_mapExplainRanges;
  Expr2DependencySetMap m_mapExplainSub;
  Expr2DependencySetMap m_mapExplainSup;

  TransitionGraph* m_pTG;
};

#endif
