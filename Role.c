#include "Role.h"
#include "Params.h"
#include "ReazienerUtils.h"
#include "TransitionGraph.h"

extern DependencySet DEPENDENCYSET_EMPTY;
extern DependencySet DEPENDENCYSET_INDEPENDENT;

extern ExprNode* EXPRNODE_TOP;
extern ExprNode* EXPRNODE_TOPLITERAL;
extern int g_iCommentIndent;

Role::Role(ExprNode* pRoleExpr) 
{
  m_pName = pRoleExpr;
  m_pDomain = NULL;
  m_pRange = NULL;
  m_iRoleType = Role::UNTYPED;
  m_pInverse = NULL;
  m_iFlags = SIMPLE;
  m_pTG = NULL;

  m_pExplainFunctional = &DEPENDENCYSET_INDEPENDENT;
  m_pExplainInverse = &DEPENDENCYSET_INDEPENDENT;
  m_pExplainDomain = &DEPENDENCYSET_INDEPENDENT;
  m_pExplainRange = &DEPENDENCYSET_INDEPENDENT;
  m_pExplainTransitive = &DEPENDENCYSET_INDEPENDENT;
  m_pExplainAntisymmetric = &DEPENDENCYSET_INDEPENDENT;
  m_pExplainReflexive = &DEPENDENCYSET_INDEPENDENT;
  m_pExplainIrreflexive = &DEPENDENCYSET_INDEPENDENT;
  m_pExplainInverseFunctional = &DEPENDENCYSET_INDEPENDENT;
}

Role::Role(ExprNode* pRoleExpr, int iRoleType)
{
  m_pName = pRoleExpr;
  m_pDomain = NULL;
  m_pRange = NULL;
  m_iRoleType = iRoleType;
  m_pInverse = NULL;
  m_iFlags = SIMPLE;
  m_pTG = NULL;

  m_pExplainFunctional = &DEPENDENCYSET_INDEPENDENT;
  m_pExplainInverse = &DEPENDENCYSET_INDEPENDENT;
  m_pExplainDomain = &DEPENDENCYSET_INDEPENDENT;
  m_pExplainRange = &DEPENDENCYSET_INDEPENDENT;
  m_pExplainTransitive = &DEPENDENCYSET_INDEPENDENT;
  m_pExplainAntisymmetric = &DEPENDENCYSET_INDEPENDENT;
  m_pExplainReflexive = &DEPENDENCYSET_INDEPENDENT;
  m_pExplainIrreflexive = &DEPENDENCYSET_INDEPENDENT;
  m_pExplainInverseFunctional = &DEPENDENCYSET_INDEPENDENT;
}

void Role::addDomain(ExprNode* pC, DependencySet* pDS)
{
  START_DECOMMENT2("Role::addDomain");
  printExprNodeWComment("this.Role=", m_pName);

  if( pDS == NULL )
    pDS = &DEPENDENCYSET_INDEPENDENT;

  pC = normalize(pC);
  printExprNodeWComment("pC=", pC);
  m_setDomains.insert(pC);
  m_mapExplainDomains[pC] = pDS;
  END_DECOMMENT("Role::addDomain");
}

void Role::addDomains(ExprNodeSet* pSet)
{
  m_setDomains.insert(pSet->begin(), pSet->end());
}

void Role::addRange(ExprNode* pC, DependencySet* pDS)
{
  START_DECOMMENT2("Role::addRange");
  printExprNodeWComment("this.Role=", m_pName);
  printExprNodeWComment("pC=", pC);

  if( pDS == NULL )
    pDS = &DEPENDENCYSET_INDEPENDENT;

  pC = normalize(pC);
  m_setRanges.insert(pC);
  m_mapExplainRanges[pC] = pDS;
  END_DECOMMENT("Role::addRange");
}

void Role::addRanges(ExprNodeSet* pSet)
{
  m_setRanges.insert(pSet->begin(), pSet->end());
}

bool Role::isObjectRole()
{
  return (m_iRoleType==Role::OBJECT);
}

bool Role::isDatatypeRole()
{
  return (m_iRoleType==Role::DATATYPE);
}

// pRole is subrole of this role
void Role::addSubRole(Role* pRole, DependencySet* pDS)
{
  START_DECOMMENT2("Role::addSubRole");
  printExprNodeWComment("this.role=", m_pName);
  printExprNodeWComment("SubRole=", pRole->m_pName);
  DECOMMENT1("This=%x", this);

  if( pDS == NULL )
    {
      pDS = &DEPENDENCYSET_INDEPENDENT;
      if( PARAMS_USE_TRACING() )
	pDS = new DependencySet(createExprNode(EXPR_SUBPROPERTY, pRole->m_pName, m_pName));
    }

  // CHECK HERE Role.c
  // Duplicate ??
  //if( PARAMS_USE_TRACING() && m_mapExplainSub.find(pRole->m_pName) == m_mapExplainSub.end() )
  //  m_mapExplainSub[pRole->m_pName] = pDS;

  m_setSubRoles.insert(pRole);
  m_mapExplainSub[pRole->m_pName] = pDS;
  END_DECOMMENT("Role::addSubRole");
}

void Role::addSuperRole(Role* pRole, DependencySet* pDS)
{
  START_DECOMMENT2("Role::addSuperRole");
  printExprNodeWComment("this.role=", m_pName);
  printExprNodeWComment("SuperRole=", pRole->m_pName);
  DECOMMENT1("This=%x", this);

  if( pDS == NULL )
    {
      pDS = &DEPENDENCYSET_INDEPENDENT;
      if( PARAMS_USE_TRACING() )
	pDS = new DependencySet(createExprNode(EXPR_SUBPROPERTY, pRole->m_pName, m_pName));
    }

  m_setSuperRoles.insert(pRole);
  m_mapExplainSup[pRole->m_pName] = pDS;
  END_DECOMMENT("Role::addSuperRole");
}

void Role::addTransitiveSubRole(Role* pR)
{
  setSimple(FALSE);
  
  if( m_setTransitiveSubRoles.size() == 0 )
    m_setTransitiveSubRoles.insert(pR);
  else if( m_setTransitiveSubRoles.size() == 1 )
    {
      Role* pTSRole = (Role*)(*m_setTransitiveSubRoles.begin());
      if( pTSRole->isSubRoleOf(pR) )
	{
	  m_setTransitiveSubRoles.clear();
	  m_setTransitiveSubRoles.insert(pR);
	}
      else if( !pR->isSubRoleOf(pTSRole) )
	{
	  m_setTransitiveSubRoles.clear();
	  m_setTransitiveSubRoles.insert(pR);
	  m_setTransitiveSubRoles.insert(pTSRole);
	}
    }
  else
    {
      for(RoleSet::iterator i = m_setTransitiveSubRoles.begin(); i != m_setTransitiveSubRoles.end(); i++ )
	{
	  Role* pTSRole = (Role*)*i;
	  if( pTSRole->isSubRoleOf(pR) )
	    {
	      m_setTransitiveSubRoles.erase(pTSRole);
	      m_setTransitiveSubRoles.insert(pR);
	      return;
	    }
	  else if( pR->isSubRoleOf(pTSRole) )
	    {
	      return;
	    }
	}
      m_setTransitiveSubRoles.insert(pR);
    }
}

void Role::addFunctionalSuper(Role* pR)
{
  for(RoleSet::iterator i = m_setFunctionalSupers.begin(); i != m_setFunctionalSupers.end(); i++ )
    {
      Role* pFSuper = (Role*)*i;
      if( pFSuper->isSubRoleOf(pR) )
	{
	  m_setFunctionalSupers.erase(pFSuper);
	  break;
	}
      else if( pR->isSubRoleOf(pFSuper) )
	return;
    }
  m_setFunctionalSupers.insert(pR);
}

void Role::addSubRoleChain(ExprNodeList* pChain, DependencySet* pDS)
{
  if( pChain->m_iUsedSize == 0 )
    assertFALSE("Adding a subproperty chain that is empty!");
  else if( pChain->m_iUsedSize == 1 )
    assertFALSE("Adding a subproperty chain that has a single element!");
  
  m_setSubRoleChains.insert(pChain);
  m_mapExplainSub[createExprNode(EXPR_LIST, pChain)] = pDS;
  setSimple(FALSE);
  
  if( !isTransitive() && isTransitiveChain(pChain, m_pName) )
    setTransitive(TRUE, pDS);
}

void Role::removeSubRoleChain(ExprNodeList* pChain)
{
  m_setSubRoleChains.erase(pChain);
  ExprNode* pChainNode = createExprNode(EXPR_LIST, pChain);
  m_mapExplainSub.erase(pChainNode);

  if( isTransitive() && isTransitiveChain(pChain, m_pName) )
    setTransitive(FALSE, NULL);
}

void Role::removeSubRoleChains()
{
  m_setSubRoleChains.clear();
  if( isTransitive() )
    setTransitive(FALSE, NULL);
}

void Role::normalizeRole()
{
  if( m_setDomains.size() > 0 )
    {
      if( m_setDomains.size() == 1 )
	{
	  m_pDomain = (ExprNode*)(*m_setDomains.begin());
	  if( PARAMS_USE_TRACING() )
	    m_pExplainDomain = getExplainDomain(m_pDomain);
	  else
	    m_pExplainDomain = &DEPENDENCYSET_INDEPENDENT;
	}
      else
	{
	  m_pDomain = createSimplifiedAnd(&m_setDomains);
	  m_pExplainDomain = &DEPENDENCYSET_INDEPENDENT;
	  if( PARAMS_USE_TRACING() )
	    {
	      for(ExprNodeSet::iterator i = m_setDomains.begin(); i != m_setDomains.end(); i++ )
		{
		  ExprNode* pD = (ExprNode*)*i;
		  m_pExplainDomain->unionDS(getExplainDomain(pD), TRUE);
		}
	    }
	}
    }

  if( m_setRanges.size() > 0 )
    {
      if( m_setRanges.size() == 1 )
	{
	  m_pRange = (ExprNode*)(*m_setRanges.begin());
	  if( PARAMS_USE_TRACING() )
	    m_pExplainRange = getExplainRange(m_pRange);
	  else
	    m_pExplainRange = &DEPENDENCYSET_INDEPENDENT;
	}
      else
	{
	  m_pRange = createSimplifiedAnd(&m_setRanges);
	  m_pExplainRange = &DEPENDENCYSET_INDEPENDENT;
	  if( PARAMS_USE_TRACING() )
	    {
	      for(ExprNodeSet::iterator i = m_setRanges.begin(); i != m_setRanges.end(); i++ )
		{
		  ExprNode* pR = (ExprNode*)*i;
		  m_pExplainRange->unionDS(getExplainRange(pR), TRUE);
		}
	    }
	}
    }
}

void Role::setInverse(Role* pTerm, DependencySet* pDS)
{
  m_pInverse = pTerm;
  if( pDS )
    m_pExplainInverse = pDS;
  else
    m_pExplainInverse = &DEPENDENCYSET_INDEPENDENT;
}

void Role::setFunctional(bool b, DependencySet* pDS)
{
  if( pDS == NULL )
    pDS = &DEPENDENCYSET_INDEPENDENT;

  if( b )
    {
      m_iFlags |= FUNCTIONAL;
      m_pExplainFunctional = pDS;
    }
  else
    {
      m_iFlags &= ~FUNCTIONAL;
      m_pExplainFunctional = &DEPENDENCYSET_INDEPENDENT;
    }
}

void Role::setAntisymmetric(bool b, DependencySet* pDS)
{
  if( b )
    m_iFlags |= ANTI_SYM;
  else
    m_iFlags &= ~ANTI_SYM;

  if( pDS )
    m_pExplainAntisymmetric = pDS;
  else
    m_pExplainAntisymmetric = &DEPENDENCYSET_INDEPENDENT;
}

void Role::setReflexive(bool b, DependencySet* pDS)
{
  if( b )
    m_iFlags |= REFLEXIVE;
  else
    m_iFlags &= ~REFLEXIVE;

  if( pDS )
    m_pExplainReflexive = pDS;
  else
    m_pExplainReflexive = &DEPENDENCYSET_INDEPENDENT;
}

void Role::setIrreflexive(bool b, DependencySet* pDS)
{
  if( b )
    m_iFlags |= IRREFLEXIVE;
  else
    m_iFlags &= ~IRREFLEXIVE;

  if( pDS )
    m_pExplainIrreflexive = pDS;
  else
    m_pExplainIrreflexive = &DEPENDENCYSET_INDEPENDENT;
}

void Role::setTransitive(bool b)
{
  DependencySet* pDS = &DEPENDENCYSET_INDEPENDENT;
  if( PARAMS_USE_TRACING() )
    pDS = new DependencySet(createExprNode(EXPR_TRANSITIVE, m_pName));
  setTransitive(b, pDS);
}

void Role::setTransitive(bool b, DependencySet* pDS)
{
  ExprNodeList* pList = createExprNodeList(2);
  addExprToList(pList, m_pName);
  addExprToList(pList, m_pName);

  if( b )
    {
      m_iFlags |= TRANSITIVE;
      m_pExplainTransitive = pDS;
      addSubRoleChain(pList, pDS);
    }
  else
    {
      m_iFlags &= ~TRANSITIVE;
      m_pExplainTransitive = pDS;
      removeSubRoleChain(pList);
    }
}

void Role::setInverseFunctional(bool b, DependencySet* pDS)
{
  if( pDS == NULL )
    pDS = &DEPENDENCYSET_INDEPENDENT;

  if( b )
    {
      m_iFlags |= INV_FUNCTIONAL;
      m_pExplainInverseFunctional = pDS;
    }
  else
    {
      m_iFlags &= ~INV_FUNCTIONAL;
      m_pExplainInverseFunctional = &DEPENDENCYSET_INDEPENDENT;
    }
}

void Role::setSimple(bool b)
{
  if( b == isSimple() )
    return;

  if( b )
    m_iFlags |= SIMPLE;
  else
    m_iFlags &= ~SIMPLE;

  if( m_pInverse )
    m_pInverse->setSimple(b);
}

void Role::setForceSimple(bool b)
{
  if( b == isForceSimple() )
    return;

  if( b )
    m_iFlags |= FORCE_SIMPLE;
  else
    m_iFlags &= ~FORCE_SIMPLE;

  if( m_pInverse )
    m_pInverse->setForceSimple(b);
}

void Role::setHasComplexSubRole(bool b)
{
  if( b == hasComplexSubRole() )
    return;

  if( b )
    m_iFlags |= COMPLEX_SUB;
  else
    m_iFlags &= ~COMPLEX_SUB;

  if( m_pInverse )
    m_pInverse->setHasComplexSubRole(b);
  if( b )
    setSimple(FALSE);
}


bool Role::isSubRoleOf(Role* pRole)
{
  return (m_setSuperRoles.find(pRole)!=m_setSuperRoles.end());
}

bool Role::isSuperRoleOf(Role* pRole)
{
  return (m_setSubRoles.find(pRole)!=m_setSubRoles.end());
}

DependencySet* Role::getExplainSub(ExprNode* pR)
{
  Expr2DependencySetMap::iterator iFind = m_mapExplainSub.find(pR);
  if( iFind != m_mapExplainSub.end() )
    return (DependencySet*)iFind->second;
  return &DEPENDENCYSET_INDEPENDENT;
}

DependencySet* Role::getExplainSuper(ExprNode* pR)
{
  Expr2DependencySetMap::iterator iFind = m_mapExplainSup.find(pR);
  if( iFind != m_mapExplainSup.end() )
    return (DependencySet*)iFind->second;
  return &DEPENDENCYSET_INDEPENDENT;
}

DependencySet* Role::getExplainDomain(ExprNode* pRange)
{
  Expr2DependencySetMap::iterator iFind = m_mapExplainDomains.find(pRange);
  if( iFind != m_mapExplainDomains.end() )
    return (DependencySet*)iFind->second;
  return NULL;
}

DependencySet* Role::getExplainRange(ExprNode* pDomain)
{
  Expr2DependencySetMap::iterator iFind = m_mapExplainRanges.find(pDomain);
  if( iFind != m_mapExplainRanges.end() )
    return (DependencySet*)iFind->second;
  return NULL;
}

bool Role::isInExplainDomain(ExprNode* pRange)
{
  return (m_mapExplainDomains.find(pRange) != m_mapExplainDomains.end());
}

bool Role::isInExplainRange(ExprNode* pDomain)
{
  return (m_mapExplainRanges.find(pDomain) != m_mapExplainRanges.end());
}

bool Role::isEquivalent(Role* pRole)
{
  return (m_setSubRoles.find(pRole)!=m_setSubRoles.end() && m_setSuperRoles.find(pRole) != m_setSuperRoles.end());
}

bool Role::hasNamedInverse()
{
  return (m_pInverse && !m_pInverse->isAnon());
}

void Role::getProperSubRoles(RoleSet* pProperSubRoles)
{
  differenceRoleSets(&m_setSubRoles, &m_setSuperRoles, pProperSubRoles);
}

void Role::setSubRolesAndChains(RoleSet* pSubRoles, ExprNodeListSet* pSubRoleChains, Expr2DependencySetMap* pDependencies)
{
  START_DECOMMENT2("Role::setSubRolesAndChains");
  m_setSubRoles.clear();
  for(RoleSet::iterator i = pSubRoles->begin(); i != pSubRoles->end(); i++ )
    m_setSubRoles.insert((Role*)*i);

  m_setSubRoleChains.clear();
  for(ExprNodeListSet::iterator i = pSubRoleChains->begin(); i != pSubRoleChains->end(); i++ )
    m_setSubRoleChains.insert((ExprNodeList*)*i);

  m_mapExplainSub.clear();
  for(Expr2DependencySetMap::iterator i = pDependencies->begin(); i != pDependencies->end(); i++ )
    m_mapExplainSub[((ExprNode*)i->first)] = (DependencySet*)i->second;
  END_DECOMMENT("Role::setSubRolesAndChains");
}

int isEqual(const Role* pRole1, const Role* pRole2)
{
  return isEqual(pRole1->m_pName, pRole2->m_pName);
}

ExprNode* Role::getTop(Role* pRole)
{
  if( pRole->isDatatypeRole() )
    return EXPRNODE_TOPLITERAL;
  return EXPRNODE_TOP;
}

void differenceRoleSets(RoleSet* pSet1, RoleSet* pSet2, RoleSet* pTarget)
{
  for(RoleSet::iterator i = pSet1->begin(); i != pSet1->end(); i++ )
    {
      Role* pRole = (Role*)*i;
      if( pSet2->find(pRole) == pSet2->end() )
	pTarget->insert(pRole);
    }
}

void intersectRoleSets(RoleSet* pSet1, RoleSet* pSet2, RoleSet* pTarget)
{
  for(RoleSet::iterator i = pSet1->begin(); i != pSet1->end(); i++ )
    {
      Role* pRole = (Role*)*i;
      if( pSet2->find(pRole) != pSet2->end() )
	pTarget->insert(pRole);
    }
}

bool isEqualSet(RoleSet* pSet1, RoleSet* pSet2)
{
  if( pSet1->size() != pSet2->size() )
    return FALSE;

  for(RoleSet::iterator i = pSet1->begin(); i != pSet1->end(); i++) 
    {
      Role* pRole = *i;
      if( pSet2->find(pRole) == pSet2->end() )
	return FALSE;
    }
  return TRUE;
}
