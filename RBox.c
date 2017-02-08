#include "RBox.h"
#include "Role.h"
#include "DependencySet.h"
#include "Params.h"
#include "TransitionGraph.h"
#include "Taxonomy.h"
#include "RoleTaxonomyBuilder.h"
#include "ReazienerUtils.h"
#include "State.h"

#include <cassert>

extern DependencySet DEPENDENCYSET_EMPTY;
extern DependencySet DEPENDENCYSET_INDEPENDENT;
extern int g_iCommentIndent;

RBox::RBox()
{
  m_bConsistent = TRUE;
  m_pTaxonomy = NULL;
}

Taxonomy* RBox::getTaxonomy()
{
  if( m_pTaxonomy == NULL )
    {
       RoleTaxonomyBuilder* pBuilder = new RoleTaxonomyBuilder(this);
       m_pTaxonomy = pBuilder->classify();
    }
  return m_pTaxonomy;
}

Role* RBox::addRole(ExprNode* pRoleExpr)
{
  START_DECOMMENT2("TaxonomyBuilder::classify");

  Role* pRole = getDefinedRole(pRoleExpr);
  if( pRole == NULL )
    {
      pRole = new Role(pRoleExpr, Role::UNTYPED);
      m_mRoles[pRoleExpr] = pRole;
    }

  END_DECOMMENT("TaxonomyBuilder::classify");
  return pRole;
}

bool RBox::addInverseRole(ExprNode* pS, ExprNode* pR, DependencySet* pDS)
{
  Role* pSRole = getRole(pS);
  Role* pRRole = getRole(pR);

  if( pSRole == NULL || pRRole == NULL || !pSRole->isObjectRole() || !pRRole->isObjectRole() )
    return FALSE;

  addEquivalentRole(pSRole->m_pInverse->m_pName, pR, pDS);
  return TRUE;
}

bool RBox::addEquivalentRole(ExprNode* pS, ExprNode* pR, DependencySet* pDS)
{
  if( pDS == NULL )
    {
      pDS = &DEPENDENCYSET_INDEPENDENT;
      if( PARAMS_USE_TRACING() )
	pDS = new DependencySet(createExprNode(EXPR_EQUIVALENTPROPERTY, pR, pS));
    }
  
  Role* pSRole = getRole(pS);
  Role* pRRole = getRole(pR);
  if( pSRole == NULL || pRRole == NULL )
    return FALSE;
  
  pRRole->addSubRole(pSRole, pDS);
  pRRole->addSuperRole(pSRole, pDS);
  pSRole->addSubRole(pRRole, pDS);
  pSRole->addSuperRole(pRRole, pDS);
  
  if( pRRole->m_pInverse )
    {
      pRRole->m_pInverse->addSubRole(pSRole->m_pInverse, pDS);
      pRRole->m_pInverse->addSuperRole(pSRole->m_pInverse, pDS);
      pSRole->m_pInverse->addSubRole(pRRole->m_pInverse, pDS);
      pSRole->m_pInverse->addSuperRole(pRRole->m_pInverse, pDS);
    }
  return TRUE;
}

Role* RBox::addDatatypeRole(ExprNode* p)
{
  Role* pRole = getRole(p);
  if( pRole == NULL )
    {
      pRole = new Role(p, Role::DATATYPE);
      m_mRoles[p] = pRole;
    }
  else
    {
      switch(pRole->m_iRoleType)
	{
	case Role::DATATYPE:
	  {
	    break;
	  }
	case Role::OBJECT:
	  {
	    pRole = NULL;
	    break;
	  }
	default:
	  {
	    pRole->m_iRoleType = Role::DATATYPE;
	    break;
	  }
	};
    }
  return pRole;
}

Role* RBox::addObjectRole(ExprNode* p)
{
  START_DECOMMENT2("TaxonomyBuilder::addObjectRole");

  Role* pRole = getRole(p);
  int iRoleType = (pRole==NULL)?Role::UNTYPED:pRole->m_iRoleType;
  
  switch(iRoleType)
    {
    case Role::DATATYPE:
      {
	pRole = NULL;
	break;
      }
    case Role::OBJECT:
      {
	break;
      }
    default:
      {
	if( pRole == NULL )
	  {
	    pRole = new Role(p, Role::OBJECT);
	    m_mRoles[p] = pRole;
	  }
	else
	  pRole->m_iRoleType = Role::OBJECT;
	
	ExprNode* pInvP = createInverse(p);
	Role* pInvRole = new Role(pInvP, Role::OBJECT);
	m_mRoles[pInvP] = pInvRole;
	
	pRole->m_pInverse = pInvRole;
	pInvRole->m_pInverse = pRole;
      }
    };

  END_DECOMMENT("TaxonomyBuilder::addObjectRole");
  return pRole;
}

bool RBox::addSubRole(ExprNode* pSub, ExprNode* pSup, DependencySet* pDS)
{
  if( pDS == NULL )
    {
      pDS = &DEPENDENCYSET_INDEPENDENT;
      if( PARAMS_USE_TRACING() )
	pDS = new DependencySet(createExprNode(EXPR_SUBPROPERTY, pSub, pSup));
    }

  Role* pSupRole = getRole(pSup);
  Role* pSubRole = getRole(pSub);

  if( pSupRole == NULL )
    return FALSE;
  else if( pSub->m_pArgList )
    pSubRole->addSubRoleChain((ExprNodeList*)pSub->m_pArgList, pDS);
  else if( pSubRole == NULL )
    return FALSE;
  else
    {
      pSupRole->addSubRole(pSubRole, pDS);
      pSubRole->addSuperRole(pSupRole, pDS);
    }

  // TODO Need to figure out what to do about about role lists
  // explanationTable.add(ATermUtils.makeSub(sub, sup), ds);
  return TRUE;
}

Role* RBox::getRole(ExprNode* p)
{
  return getDefinedRole(p);
}

Role* RBox::getDefinedRole(ExprNode* p)
{
  Expr2RoleMap::iterator i = m_mRoles.find(p);
  if( i != m_mRoles.end() )
    return (Role*)i->second;
  return NULL;
}

void RBox::prepare()
{
  // first pass - compute sub roles
  for(Expr2RoleMap::iterator i = m_mRoles.begin(); i != m_mRoles.end(); i++ )
    {
      Role* pRole = (Role*)i->second;
      if( pRole->m_iRoleType == Role::OBJECT || pRole->m_iRoleType == Role::DATATYPE )
	{
	  Expr2DependencySetMap subExplain;
	  RoleSet subRoles;
	  ExprNodeListSet subRoleChains;

	  computeSubRoles(pRole, &subRoles, &subRoleChains, &subExplain, &DEPENDENCYSET_INDEPENDENT);
	  pRole->setSubRolesAndChains(&subRoles, &subRoleChains, &subExplain);

	  bool bHasComplexSubRoles = FALSE;
	  for(ExprNodeListSet::iterator j = subRoleChains.begin(); j != subRoleChains.end(); j++) 
	    {
	      ExprNodeList* pChain = (ExprNodeList*)*j;
	      ExprNode* pFirstExpr = pChain->m_pExprNodes[0];
	      ExprNode* pLastExpr =  pChain->m_pExprNodes[pChain->m_iUsedSize-1];

	      if( pChain->m_iUsedSize != 2 || 
		  isEqual(pFirstExpr, pLastExpr) != 0 ||
		  subRoles.find( getRole(pFirstExpr) ) == subRoles.end() )
		{
		  bHasComplexSubRoles = TRUE;
		  break;
		}
	    }
	  
	  pRole->setHasComplexSubRole(bHasComplexSubRoles);
	  if( bHasComplexSubRoles )
	    buildDFA(pRole, (new RoleSet));
	}
    }

  // second pass - set super roles and propagate domain & range
  for(Expr2RoleMap::iterator i = m_mRoles.begin(); i != m_mRoles.end(); i++ )
    {
      Role* pRole = (Role*)i->second;
      Role* pInverseRole = pRole->m_pInverse;
      if( pInverseRole )
	{
	  DependencySet* pDS = &DEPENDENCYSET_INDEPENDENT;
	  if( PARAMS_USE_TRACING() )
	    pDS = pRole->m_pExplainInverse;
	  
	  // domain of inv role is the range of this role
	  if( pInverseRole->m_setDomains.size() > 0 )
	    {
	      if( PARAMS_USE_TRACING() )
		{
		  for(ExprNodeSet::iterator j = pInverseRole->m_setDomains.begin(); j != pInverseRole->m_setDomains.end(); j++ )
		    {
		      ExprNode* pDomain = (ExprNode*)*j;
		      if( !pRole->isInExplainRange(pDomain) )
			pRole->addRange( pDomain, pDS->unionNew(pInverseRole->getExplainDomain(pDomain), TRUE) );
		    }
		}
	      else
		pRole->addRanges(&(pInverseRole->m_setDomains));
	    }

	  if( pInverseRole->m_setRanges.size() > 0 )
	    {
	      if( PARAMS_USE_TRACING() )
		{
		  for(ExprNodeSet::iterator j = pInverseRole->m_setRanges.begin(); j != pInverseRole->m_setRanges.end(); j++ )
		    {
		      ExprNode* pRange = (ExprNode*)*j;
		      if( !pRole->isInExplainDomain(pRange) )
			pRole->addDomain( pRange, pDS->unionNew(pInverseRole->getExplainRange(pRange), TRUE) );
		    }
		}
	      else
		pRole->addDomains(&(pInverseRole->m_setRanges));
	    }
	  
	  if( pInverseRole->isTransitive() && !pRole->isTransitive() )
	    pRole->setTransitive(TRUE,  pInverseRole->m_pExplainTransitive->unionNew(pDS, TRUE));
	  else if( !pInverseRole->isTransitive() && pRole->isTransitive() )
	    pInverseRole->setTransitive(TRUE,  pRole->m_pExplainTransitive->unionNew(pDS, TRUE));

	  if( pInverseRole->isFunctional() && !pRole->isInverseFunctional() )
	    pRole->setInverseFunctional(TRUE, pInverseRole->m_pExplainFunctional->unionNew(pDS, TRUE));
	  if( !pInverseRole->isInverseFunctional() && pRole->isFunctional() )
	    pInverseRole->setInverseFunctional(TRUE, pRole->m_pExplainFunctional->unionNew(pDS, TRUE));

	  if( pInverseRole->isInverseFunctional() && !pRole->isFunctional() )
	    pRole->setFunctional(TRUE, pInverseRole->m_pExplainInverseFunctional->unionNew(pDS, TRUE));
	  
	  if( pInverseRole->isAntisymmetric() && !pRole->isAntisymmetric() )
	    pRole->setAntisymmetric(TRUE, pInverseRole->m_pExplainAntisymmetric->unionNew(pDS,TRUE));
	  if( !pInverseRole->isAntisymmetric() && pRole->isAntisymmetric() )
	    pInverseRole->setAntisymmetric(TRUE, pRole->m_pExplainAntisymmetric->unionNew(pDS,TRUE));
	 
	  if( pInverseRole->isReflexive() && !pRole->isReflexive() )
	    pRole->setReflexive(TRUE, pInverseRole->m_pExplainReflexive->unionNew(pDS, TRUE));
	  if( !pInverseRole->isReflexive() && pRole->isReflexive() )
	    pInverseRole->setReflexive(TRUE, pRole->m_pExplainReflexive->unionNew(pDS, TRUE));
	}

      for(RoleSet::iterator k = pRole->m_setSubRoles.begin(); k != pRole->m_setSubRoles.end(); k++ )
	{
	  Role* pSubRole = (Role*)*k;
	  pSubRole->addSuperRole(pRole, pRole->getExplainSub(pSubRole->m_pName));
	  
	  if( pSubRole->isForceSimple() )
	    pSubRole->setForceSimple(TRUE);
	  if( !pSubRole->isSimple() )
	    pRole->setSimple(FALSE);

	  if( pRole->m_setDomains.size() > 0 )
	    {
	      if( PARAMS_USE_TRACING() )
		{
		  for(ExprNodeSet::iterator j = pRole->m_setDomains.begin(); j != pRole->m_setDomains.end(); j++ )
		    {
		      ExprNode* pDomain = (ExprNode*)*j;
		      if( !pSubRole->isInExplainDomain(pDomain) )
			pSubRole->addDomain( pDomain, pRole->getExplainSub(pSubRole->m_pName)->unionNew(pRole->getExplainDomain(pDomain), TRUE));
		    }
		}
	      else
		pSubRole->addDomains(&(pRole->m_setDomains));
	    }

	  if( pRole->m_setRanges.size() > 0 )
	    {
	      if( PARAMS_USE_TRACING() )
		{
		  for(ExprNodeSet::iterator j = pRole->m_setRanges.begin(); j != pRole->m_setRanges.end(); j++ )
		    {
		      ExprNode* pRange = (ExprNode*)*j;
		      if( !pSubRole->isInExplainRange(pRange) )
			pSubRole->addRange( pRange, pRole->getExplainSub(pSubRole->m_pName)->unionNew(pRole->getExplainRange(pRange), TRUE));
		    }
		}
	      else
		pSubRole->addRanges(&(pRole->m_setRanges));
	    }
	}
    }

  // TODO propagate disjoint roles through sub/super roles
  // third pass - set transitivity and functionality
  for(Expr2RoleMap::iterator i = m_mRoles.begin(); i != m_mRoles.end(); i++ )
    {
      Role* pRole = (Role*)i->second;
      pRole->normalizeRole();

      if( pRole->isForceSimple() )
	{
	  if( !pRole->isSimple() )
	    ignoreTransitivity(pRole);
	}
      else
	{
	  bool bIsTransitive = pRole->isTransitive();
	  DependencySet* pTransitiveDS = pRole->m_pExplainTransitive;
	  for(RoleSet::iterator s = pRole->m_setSubRoles.begin(); s != pRole->m_setSubRoles.end(); s++)
	    {
	      Role* pSubRole = (Role*)*s;
	      if( pSubRole->isTransitive() )
		{
		  if( pRole->isSubRoleOf(pSubRole) && pRole != pSubRole )
		    {
		      bIsTransitive = TRUE;
		      pTransitiveDS = pRole->getExplainSub( pSubRole->m_pName )->unionNew(pSubRole->m_pExplainTransitive, TRUE);
		    }
		  pRole->addTransitiveSubRole(pSubRole);
		}
	    }

	  if( bIsTransitive != pRole->isTransitive() )
	    pRole->setTransitive(bIsTransitive, pTransitiveDS);
	}

      for(RoleSet::iterator s = pRole->m_setSuperRoles.begin(); s != pRole->m_setSuperRoles.end(); s++)
	{
	  Role* pSuperRole = (Role*)*s;
	  if( pSuperRole->isFunctional() )
	    {
	      DependencySet* pDS = &DEPENDENCYSET_INDEPENDENT;
	      if( PARAMS_USE_TRACING() )
		pDS = pRole->getExplainSuper(pSuperRole->m_pName)->unionNew(pSuperRole->m_pExplainFunctional, TRUE);
	      pRole->setFunctional(TRUE, pDS);
	      pRole->addFunctionalSuper(pSuperRole);
	    }
	  if( pSuperRole->isIrreflexive() && !pRole->isIrreflexive() )
	    {
	      DependencySet* pDS = &DEPENDENCYSET_INDEPENDENT;
	      if( PARAMS_USE_TRACING() )
		pDS = pRole->getExplainSuper(pSuperRole->m_pName)->unionNew(pSuperRole->m_pExplainIrreflexive, TRUE);
	      pRole->setIrreflexive(TRUE, pDS);
	    }
	}

      if( pRole->isReflexive() && !pRole->isAnon() )
	m_setReflexiveRoles.insert(pRole);
    }

  if( m_pTaxonomy )
    delete m_pTaxonomy;
  m_pTaxonomy = NULL;
}

ExprNodeList* RBox::inverseExprList(ExprNodeList* pList)
{
  ExprNodeList* pInversed = createExprNodeList(pList->m_iUsedSize);
  for(int i = 0; i < pList->m_iUsedSize; i++ )
    {
      ExprNode* pR = pList->m_pExprNodes[i];
      Role* pRole = getRole(pR);
      Role* pInvRole = pRole->m_pInverse;
      if( pInvRole == NULL )
	{
	  //System.err.println( "Property " + r+ " was supposed to be an ObjectProperty but it is not!" );
	}
      else
	pInversed->m_pExprNodes[pInversed->m_iUsedSize++] = pInvRole->m_pName;
    }
  return pInversed;
}

void RBox::computeImmediateSubRoles(Role* pRole, RoleSet* pSubRoles, ExprNodeListSet* pSubChains, Expr2DependencySetMap* pDependencies)
{
  Role* pInverseRole = pRole->m_pInverse;
  if( pInverseRole && pInverseRole != pRole )
    {
      DependencySet* pDS = &DEPENDENCYSET_INDEPENDENT;
      if( PARAMS_USE_TRACING() )
	pDS = pRole->m_pExplainInverse;

      for(RoleSet::iterator i = pInverseRole->m_setSubRoles.begin(); i != pInverseRole->m_setSubRoles.end(); i++ )
	{
	  Role* pInvSubRole = (Role*)*i;
	  Role* pSubRole = pInvSubRole->m_pInverse;
	  if( pSubRole == NULL )
	    {
	      //System.err.println( "Property " + invSubR+ " was supposed to be an ObjectProperty but it is not!" );
	    }
	  else if( pSubRole != pRole )
	    {
	      DependencySet* pSubDS = &DEPENDENCYSET_INDEPENDENT;
	      if( PARAMS_USE_TRACING() )
		pSubDS = pDS->unionNew(pInverseRole->getExplainSub(pInvSubRole->m_pName)->unionNew(pInvSubRole->m_pExplainInverse, TRUE), TRUE);
	      pSubRoles->insert(pSubRole);
	      (*pDependencies)[pSubRole->m_pName] = pSubDS;
	    }
	}

      for(ExprNodeListSet::iterator i = pInverseRole->m_setSubRoleChains.begin(); i != pInverseRole->m_setSubRoleChains.end(); i++ )
	{
	  ExprNodeList* pRoleChain = (ExprNodeList*)*i;
	  ExprNode* pRoleChainExpr = createExprNode(EXPR_LIST, pRoleChain);
	  
	  DependencySet* pSubDS = &DEPENDENCYSET_INDEPENDENT;
	  if( PARAMS_USE_TRACING() )
	    pSubDS = pDS->unionNew(pInverseRole->getExplainSub(pRoleChainExpr), TRUE);

	  ExprNodeList* pSubChain = inverseExprList(pRoleChain);
	  ExprNode* pSubChainExpr = createExprNode(EXPR_LIST, pSubChain);
	  pSubChains->insert(pSubChain);
	  (*pDependencies)[pSubChainExpr] = pSubDS;
	}
    }
  
  for(RoleSet::iterator i = pRole->m_setSubRoles.begin(); i != pRole->m_setSubRoles.end(); i++ )
    {
      Role* pSubRole = (Role*)*i;
      DependencySet* pSubDS = &DEPENDENCYSET_INDEPENDENT;
      if( PARAMS_USE_TRACING() )
	pSubDS = pRole->getExplainSub(pSubRole->m_pName);

      pSubRoles->insert(pSubRole);
      (*pDependencies)[pSubRole->m_pName] = pSubDS;
    }

  for(ExprNodeListSet::iterator i = pRole->m_setSubRoleChains.begin(); i != pRole->m_setSubRoleChains.end(); i++)
    {
      ExprNodeList* pSubChain = (ExprNodeList*)*i;
      ExprNode* pSubChainExpr = createExprNode(EXPR_LIST, pSubChain);
      DependencySet* pSubDS = &DEPENDENCYSET_INDEPENDENT;
      if( PARAMS_USE_TRACING() )
	pSubDS = pRole->getExplainSub(pSubChainExpr);
      
      pSubChains->insert(pSubChain);
      (*pDependencies)[pSubChainExpr] = pSubDS;
    }
}

void RBox::computeSubRoles(Role* pRole, RoleSet* pSubRoles, ExprNodeListSet* pSubRoleChains, Expr2DependencySetMap* pDependencies, DependencySet* pDS)
{
  // check for loops
  if( pSubRoles->find(pRole) != pSubRoles->end() )
    return;

  // reflexive
  pSubRoles->insert(pRole);
  (*pDependencies)[pRole->m_pName] = pDS;

  RoleSet immSubRoles;
  Expr2DependencySetMap immDeps;
  ExprNodeListSet immSubChains;
  computeImmediateSubRoles(pRole, &immSubRoles, &immSubChains, &immDeps);
  
  for(RoleSet::iterator i = immSubRoles.begin(); i != immSubRoles.end(); i++ )
    {
      Role* pSubRole = (Role*)*i;
      ExprNode* pName = pSubRole->m_pName;
      DependencySet* pSubDS = &DEPENDENCYSET_INDEPENDENT;
      if( PARAMS_USE_TRACING() )
	{
	  DependencySet* pImmDS = NULL;
	  Expr2DependencySetMap::iterator iFind = immDeps.find(pName);
	  if( iFind != immDeps.end() )
	    pImmDS = (DependencySet*)iFind->second;
	  pSubDS = pDS->unionNew(pImmDS, TRUE);
	}
      computeSubRoles(pSubRole, pSubRoles, pSubRoleChains, pDependencies, pSubDS);
    }

  for(ExprNodeListSet::iterator i = immSubChains.begin(); i != immSubChains.end(); i++ )
    {
      ExprNodeList* pList = (ExprNodeList*)*i;
      ExprNode* pListExpr = createExprNode(EXPR_LIST, pList);
      DependencySet* pSubDS = &DEPENDENCYSET_INDEPENDENT;
      if( PARAMS_USE_TRACING() )
	{
	  DependencySet* pImmDS = NULL;
	  Expr2DependencySetMap::iterator iFind = immDeps.find(pListExpr);
	  if( iFind != immDeps.end() )
	    pImmDS = (DependencySet*)iFind->second;
	  pSubDS = pDS->unionNew(pImmDS, TRUE);
	}

      pSubRoleChains->insert(pList);
      (*pDependencies)[pListExpr] = pSubDS;
    }
}

TransitionGraph* RBox::buildDFA(Role* pRole, RoleSet* pVisited)
{
  if( pVisited->find(pRole) != pVisited->end() )
    return NULL;
  pVisited->insert(pRole);

  TransitionGraph* pTG = pRole->m_pTG;
  if( pTG == NULL )
    {
      pTG = buildNFA(pRole, pVisited);
      if( pTG == NULL )
	{
	  //log.warn( "Cycle detected in the complex suproperty chain involving " + s );
	  pRole->setForceSimple(TRUE);
	  ignoreTransitivity(pRole);
	  return NULL;
	}

      assert(pTG->isConnected());
      pTG = handleSymmetry(pRole, pTG);
      assert(pTG->isConnected());
      pTG->determinize();
      assert(pTG->isConnected());
      assert(pTG->isDeterministic());
      pTG->minimize();
      assert(pTG->isConnected());
      pTG->renumber();
      assert(pTG->isConnected());
      setFSM(pRole, pTG);
      setFSM(pRole->m_pInverse, mirror(pTG)->determinize()->renumber());
    }

  pVisited->erase(pRole);
  return pTG;
}

TransitionGraph* RBox::handleSymmetry(Role* pRole, TransitionGraph* pTG)
{
  if( pRole->m_setSubRoles.find(pRole->m_pInverse) == pRole->m_setSubRoles.end() )
    return pTG;
  return pTG->choice( mirror(pTG) );
}

void RBox::setFSM(Role* pRole, TransitionGraph* pTG)
{
  pRole->m_pTG = pTG;
  
  RoleSet setEquivalentProperties;
  intersectRoleSets(&pRole->m_setSubRoles, &pRole->m_setSuperRoles, &setEquivalentProperties);
  setEquivalentProperties.erase(pRole);

  for(RoleSet::iterator i = setEquivalentProperties.begin(); i != setEquivalentProperties.end(); i++ )
    {
      Role* pRole = (Role*)*i;
      pRole->m_pTG = pTG;
    }
}

TransitionGraph* RBox::buildNFA(Role* pRole, RoleSet* pVisited)
{
  TransitionGraph* pTG = new TransitionGraph;

  State* pInitialState = pTG->createNewState();
  State* pFinalState = pTG->createNewState();

  pTG->setInitialState(pInitialState);
  pTG->addFinalState(pFinalState);
  pTG->addTransition(pInitialState, pRole, pFinalState);

  RoleSet setProperSubRoles;
  pRole->getProperSubRoles(&setProperSubRoles);
  for(RoleSet::iterator i = setProperSubRoles.begin(); i != setProperSubRoles.end(); i++ )
    pTG->addTransition(pInitialState, (Role*)*i, pFinalState);

  setProperSubRoles.clear();
  pRole->m_pInverse->getProperSubRoles(&setProperSubRoles);
  for(RoleSet::iterator i = setProperSubRoles.begin(); i != setProperSubRoles.end(); i++ )
    pTG->addTransition(pInitialState, ((Role*)*i)->m_pInverse, pFinalState);
  
  for(ExprNodeListSet::iterator i = pRole->m_setSubRoleChains.begin(); i != pRole->m_setSubRoleChains.end(); i++ )
    addTransition(pTG, pRole, ((ExprNodeList*)*i));

  for(ExprNodeListSet::iterator i = pRole->m_pInverse->m_setSubRoleChains.begin(); i != pRole->m_pInverse->m_setSubRoleChains.end(); i++ )
    addTransition(pTG, pRole, inverseExprList(((ExprNodeList*)*i)));

  PointerSet* pSetAlphabet = pTG->getAlphabet();
  for(PointerSet::iterator i = pSetAlphabet->begin(); i != pSetAlphabet->end(); i++ )
    {
      Role* pR = (Role*)*i;
      if( pRole->m_setSubRoles.find(pR) != pRole->m_setSubRoles.end() && pRole->m_setSuperRoles.find(pR) != pRole->m_setSuperRoles.end() )
	continue;

      StatePairList listStatePairs;
      pTG->findTransitions(pR, &listStatePairs);
      for(StatePairList::iterator l = listStatePairs.begin(); l != listStatePairs.end(); l++ )
	{
	  StatePair* pPair = (StatePair*)*l;
	  TransitionGraph* pNewGraph = buildDFA(pR, pVisited);
	  if( pNewGraph == NULL )
	    return NULL;
	  pTG->insert(pNewGraph, pPair->first, pPair->second);
	}
    }

  return pTG;
}

void RBox::addTransition(TransitionGraph* pTG, Role* pS, ExprNodeList* pChain)
{
  Role* pRole = getRole(pChain->m_pExprNodes[0]);
  State* pNext = pTG->createNewState();
  State* pPrev = pNext;
  
  int iExprIndex = 0;
  if( pS->isEquivalent(pRole) )
    {
      pTG->addTransition(pTG->getFinalState(), pNext);
      iExprIndex++;
    }
  else
    {
      pTG->addTransition(pTG->m_pInitialState, pNext);
    }

  for(; iExprIndex < pChain->m_iUsedSize; iExprIndex++)
    {
      pNext = pTG->createNewState();
      pRole = getRole(pChain->m_pExprNodes[iExprIndex]);
      pTG->addTransition(pPrev, pRole, pNext);
    }

  pRole = getRole(pChain->m_pExprNodes[0]);
  if( pS->isEquivalent(pRole) )
    pTG->addTransition(pPrev, pTG->m_pInitialState);
  else
    pTG->addTransition(pPrev, pRole, pTG->getFinalState());
}

TransitionGraph* RBox::mirror(TransitionGraph* pTG)
{
  StateMap mapNewStates;

  TransitionGraph* pMirror = new TransitionGraph();
  State* pOldInitialState = pTG->m_pInitialState;
  State* pNewFinalState = copyState(pOldInitialState, pMirror, &mapNewStates);

  pMirror->addFinalState(pNewFinalState);

  StateSet oldFinalStates = pTG->m_setFinalStates;
  State* pNewInitialState = NULL;
  if( oldFinalStates.size() == 1 )
    {
      State* pOldFinalState = (State*)(*oldFinalStates.begin());
      StateMap::iterator iFind = mapNewStates.find(pOldFinalState);
      if( iFind != mapNewStates.end() )
	pNewInitialState = (State*)iFind->second;
      else
	pNewInitialState = NULL;
    }
  else
    {
      pNewInitialState = pMirror->createNewState();
      for(StateSet::iterator i = oldFinalStates.begin(); i != oldFinalStates.end(); i++ )
	{
	  State* pOldFinalState = (State*)*i;
	  State* pS = NULL;
	  StateMap::iterator iFind = mapNewStates.find(pOldFinalState);
	  if( iFind != mapNewStates.end() )
	    pS = (State*)iFind->second;
	  pMirror->addTransition(pNewInitialState, pS);
	}
    }

  pMirror->m_pInitialState = pNewInitialState;

  return pMirror;
}

State* RBox::copyState(State* pOldState, TransitionGraph* pTG, StateMap* pmapNewStates)
{
  State* pNewState = NULL;
  StateMap::iterator iFind = pmapNewStates->find(pOldState);
  if( iFind != pmapNewStates->end() )
    pNewState = (State*)iFind->second;

  if( pNewState == NULL )
    {
      pNewState = pTG->createNewState();
      (*pmapNewStates)[pOldState] = pNewState;
      for(TransitionSet::iterator t = pOldState->m_setTransitions.begin(); t != pOldState->m_setTransitions.end(); t++ )
	{
	  Transition* pT = (Transition*)*t;
	  State* pOldTo = pT->m_pTo;
	  State* pNewFrom = copyState(pOldTo, pTG, pmapNewStates);
	  if( pT->m_pName == NULL )
	    pTG->addTransition(pNewFrom, pNewState);
	  else
	    pTG->addTransition(pNewFrom, (void*)(((Role*)pT->m_pName)->m_pInverse), pNewState);
	}
    }

  return pNewState;
}

void RBox::ignoreTransitivity(Role* pRole)
{
  Role* pNamedRole = pRole->isAnon()?pRole->m_pInverse:pRole;

  pRole->removeSubRoleChains();
  pRole->setHasComplexSubRole(FALSE);
  pRole->setSimple(TRUE);
  pRole->m_pTG = NULL;
  
  pRole->m_pInverse->removeSubRoleChains();
  pRole->m_pInverse->setHasComplexSubRole(FALSE);
  pRole->m_pInverse->setSimple(TRUE);
  pRole->m_pInverse->m_pTG = NULL;
}
