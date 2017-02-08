#include "KnowledgeBase.h"
#include "TBox.h"
#include "ABox.h"
#include "RBox.h"
#include "Expressivity.h"
#include "Branch.h"
#include "DependencySet.h"
#include "Params.h"
#include "SizeEstimate.h"
#include "Role.h"
#include "DependencyIndex.h"
#include "Individual.h"
#include "Literal.h"
#include "Edge.h"
#include "Taxonomy.h"
#include "TaxonomyBuilder.h"
#include "ConceptCache.h"
#include "CompletionQueue.h"
#include "CompletionStrategy.h"
#include "DependencyEntry.h"
#include "Dependency.h"
#include "Clash.h"

#include "ReazienerUtils.h"

#include "SHOINStrategy.h"
#include "SROIQStrategy.h"
#include "SHOIQStrategy.h"
#include "EmptySHNStrategy.h"
#include "CompletionStrategy.h"

extern DependencySet DEPENDENCYSET_EMPTY;
extern DependencySet DEPENDENCYSET_INDEPENDENT;
extern ExprNode* EXPRNODE_TOP;
extern ExprNode* EXPRNODE_BOTTOM;
extern int g_iCommentIndent;


KnowledgeBase::KnowledgeBase()
{
  initExprNodes();
  clear();

  m_pDependencyIndex = new DependencyIndex(this);
  m_pTaxonomy = NULL;

  m_iKBStatus = KBSTATUS_ALL_CHANGED;
}

KnowledgeBase::~KnowledgeBase()
{
  delete m_pTBox;
  delete m_pABox;
  if( m_pExpressivity )
    delete m_pExpressivity;
}

void KnowledgeBase::clear()
{
  m_pTBox = new TBox(this);
  m_pABox = new ABox();
  m_pRBox = new RBox();
  
  m_pExpressivity = new Expressivity(this);
  m_setIndividuals.clear();
  m_mABoxAssertions.clear();
  m_mapInstances.clear();

  m_bABoxAddition = FALSE;
  m_bABoxDeletion = FALSE;

  m_pBuilder = NULL;
  m_iKBStatus = KBSTATUS_ALL_CHANGED;
}

void KnowledgeBase::prepare()
{
  if( !isChanged() )
    return;
  START_DECOMMENT2("KnowledgeBase::prepare");

  bool bExplain = m_pABox->m_bDoExplanation;
  m_pABox->m_bDoExplanation = TRUE;

  bool bReuseTaxonomy = (m_pTaxonomy && !isTBoxChanged() && (m_pExpressivity->hasNominal()==FALSE || PARAMS_USE_PSEUDO_NOMINALS()));
  

  if( isTBoxChanged() )
    {
      if( PARAMS_USE_ABSORPTION() )
	m_pTBox->absorb();

      m_pTBox->normalize();
      m_pTBox->internalize();
    }

  if( isRBoxChanged() )
    m_pRBox->prepare();

  // The preparation of TBox and RBox is finished so we set the
  // status to UNCHANGED now. Expressivity check can only work
  // with prepared KB's
  m_iKBStatus = KBSTATUS_UNCHANGED;
  m_pExpressivity->compute();
  m_mapInstances.clear();
  m_pEstimate = new SizeEstimate(this);
  m_pABox->m_bDoExplanation = bExplain;
  m_pABox->clearCaches(!bReuseTaxonomy);
  m_pABox->m_pCache->setMaxSize( 2*m_pTBox->m_setClasses.size() );

  if( bReuseTaxonomy )
    m_iKBStatus |= KBSTATUS_CLASSIFICATION;
  else if( m_pTaxonomy )
    {
      delete m_pTaxonomy;
      m_pTaxonomy = NULL;
    }
  END_DECOMMENT("KnowledgeBase::prepare");
}

void KnowledgeBase::addClass(ExprNode* pClassExpr)
{
  if( isEqual(pClassExpr, EXPRNODE_TOP) == 0 || isComplexClass(pClassExpr) )
    return;

  if( m_pTBox->addClass(pClassExpr) )
    m_iKBStatus |= KBSTATUS_TBOX_CHANGED;
}

void KnowledgeBase::addDisjointClass(ExprNode* pC1, ExprNode* pC2)
{
  ExprNodeSet* pExplain = new ExprNodeSet;
  if( PARAMS_USE_TRACING() )
    pExplain->insert(createExprNode(EXPR_DISJOINTWITH, pC1, pC2));
  addDisjointClass(pC1, pC2, pExplain);
}

void KnowledgeBase::addDisjointClass(ExprNode* pC1, ExprNode* pC2, ExprNodeSet* pExplanation)
{
  m_iKBStatus |= KBSTATUS_TBOX_CHANGED;  
  
  ExprNode* pNotC1 = createExprNode(EXPR_NOT, pC1);
  ExprNode* pNotC2 = createExprNode(EXPR_NOT, pC2);

  m_pTBox->addAxiom(createExprNode(EXPR_SUBCLASSOF, pC1, pNotC2), pExplanation);
  m_pTBox->addAxiom(createExprNode(EXPR_SUBCLASSOF, pC2, pNotC1), pExplanation);
}

void KnowledgeBase::addSubClass(ExprNode* pSub, ExprNode* pSup)
{
  ExprNode* pSubAxiom = createExprNode(EXPR_SUBCLASSOF, pSub, pSup);
  ExprNodeSet* pSetExplanation = new ExprNodeSet;
  if( PARAMS_USE_TRACING() )
    pSetExplanation->insert(pSubAxiom);
  addSubClass(pSub, pSup, pSetExplanation);
}

void KnowledgeBase::addSubClass(ExprNode* pSub, ExprNode* pSup, ExprNodeSet* pExplanation)
{
  START_DECOMMENT2("KnowledgeBase::addSubClass");
  printExprNodeWComment("Sub=", pSub);
  printExprNodeWComment("Sup=", pSup);

  if( isEqual(pSub, pSup) == 0 )
    {
      END_DECOMMENT("KnowledgeBase::addSubClass");
      return;
    }

  ExprNode* pSubAxiom = createExprNode(EXPR_SUBCLASSOF, pSub, pSup);
  printExprNodeWComment("SubAxiom=", pSubAxiom);
  m_pTBox->addAxiom(pSubAxiom, pExplanation);
  m_iKBStatus |= KBSTATUS_TBOX_CHANGED;
  END_DECOMMENT("KnowledgeBase::addSubClass");
}

void KnowledgeBase::addSameClasses(ExprNode* pExpr, ExprNode* pSameExpr)
{
  addEquivalentClasses(pExpr, pSameExpr);
}

void KnowledgeBase::addEquivalentClasses(ExprNode* pExprNode1, ExprNode* pExprNode2)
{
  START_DECOMMENT2("KnowledgeBase::addEquivalentClasses");
  printExprNodeWComment("C1=", pExprNode1);
  printExprNodeWComment("C2=", pExprNode2);

  if( isEqual(pExprNode1, pExprNode2) == 0 )
    {
      END_DECOMMENT("KnowledgeBase::addEquivalentClasses");
      return;
    }

  ExprNode* pSameAxiom = createExprNode(EXPR_EQCLASSES, pExprNode1, pExprNode2);
  ExprNodeSet* pSetExplanation = new ExprNodeSet;
  if( PARAMS_USE_TRACING() )
    pSetExplanation->insert(pSameAxiom);
  
  m_pTBox->addAxiom(pSameAxiom, pSetExplanation);
  m_iKBStatus |= KBSTATUS_TBOX_CHANGED; 

  END_DECOMMENT("KnowledgeBase::addEquivalentClasses");
}

void KnowledgeBase::addDisjointClasses(ExprNode* pC1, ExprNode* pC2)
{
  ExprNodeSet setExplanation;
  if( PARAMS_USE_TRACING() )
    setExplanation.insert(createExprNode(EXPR_DISJOINTWITH, pC1, pC2));
  addDisjointClasses(pC1, pC2, &setExplanation);
}

void KnowledgeBase::addDisjointClasses(ExprNode* pC1, ExprNode* pC2, ExprNodeSet* pExplanation)
{
  ExprNode* pNotC1 = createExprNode(EXPR_NOT, pC1);
  ExprNode* pNotC2 = createExprNode(EXPR_NOT, pC2);

  m_pTBox->addAxiom(createExprNode(EXPR_SUBCLASSOF, pC1, pNotC2), pExplanation);
  m_pTBox->addAxiom(createExprNode(EXPR_SUBCLASSOF, pC2, pNotC1), pExplanation);

  m_iKBStatus |= KBSTATUS_TBOX_CHANGED;
}

void KnowledgeBase::addComplementClasses(ExprNode* pC1, ExprNode* pC2)
{
  ExprNode* pNotC2 = createExprNode(EXPR_NOT, pC2);

  m_iKBStatus |= KBSTATUS_TBOX_CHANGED;
  if( isEqual(pC1, pC2) == 0 )
    return;

  ExprNodeSet setExplanation;
  if( PARAMS_USE_TRACING() )
    setExplanation.insert(createExprNode(EXPR_COMPLEMENTOF, pC1, pC2));

  ExprNode* pNotC1 = createExprNode(EXPR_NOT, pC1);
  m_pTBox->addAxiom(createExprNode(EXPR_EQCLASSES, pC1, pNotC2), &setExplanation);
  m_pTBox->addAxiom(createExprNode(EXPR_EQCLASSES, pC2, pNotC1), &setExplanation);
}

Node* KnowledgeBase::addIndividual(ExprNode* pExprNode)
{
  Node* pNode = m_pABox->getNode(pExprNode);
  if( pNode == NULL )
    {
      m_iKBStatus |= KBSTATUS_ABOX_CHANGED;
      m_pABox->setSyntacticUpdate();
      pNode = m_pABox->addIndividual(pExprNode);
      m_setIndividuals.insert(pExprNode);
      m_pABox->setSyntacticUpdate(FALSE);
    }
  else if( pNode->m_bLiteralNode )
    assertFALSE("Trying to use a literal as an individual.");

  // set addition flag
  m_bABoxAddition = TRUE;

  // if we can use inc reasoning then update pseudomodel
  if( canUseIncrementalConsistency() )
    {
      m_pABox->getPseudoModel()->setSyntacticUpdate();
      Node* pPseudoModelNode = m_pABox->getPseudoModel()->getNode(pExprNode);
      if( pPseudoModelNode == NULL )
	{
	  m_iKBStatus |= KBSTATUS_ABOX_CHANGED;
	  // add to pseudomodel - note branch must be temporarily set to 0
	  // to ensure that asssertion
	  // will not be restored during backtracking
	  int iBranch = m_pABox->getPseudoModel()->getBranch();
	  m_pABox->getPseudoModel()->setBranch(0);
	  pPseudoModelNode = m_pABox->getPseudoModel()->addIndividual(pExprNode);
	  m_pABox->getPseudoModel()->setBranch(iBranch);

	  // need to update the branch node count as this is node has been
	  // added other wise during back jumping this node can be removed
	  for(int i = 0; m_pABox->getPseudoModel()->m_iBranchSize; i++ )
	    m_pABox->getPseudoModel()->m_aBranches[i]->m_iNodeCount++;
	}

      m_pABox->m_setUpdatedIndividuals.insert(m_pABox->getPseudoModel()->getNode(pExprNode));
      m_pABox->m_setNewIndividuals.insert(m_pABox->getPseudoModel()->getNode(pExprNode));
      m_pABox->getPseudoModel()->setSyntacticUpdate(FALSE);
    }

  return pNode;
}

void KnowledgeBase::addType(ExprNode* pIndvidualExpr, ExprNode* pCExpr)
{
  ExprNode* pTypeAxiom = createExprNode(EXPR_TYPE, pIndvidualExpr, pCExpr);
  DependencySet* pDS = (PARAMS_USE_TRACING())?(new DependencySet(pTypeAxiom)):(&DEPENDENCYSET_INDEPENDENT);
  
  // add type assertion to syntactic assertions and update dependency index
  if( PARAMS_USE_INCREMENTAL_DELETION() )
    {
      ExprNodeSet* pSet = NULL;
      map<int, ExprNodeSet*>::iterator i = m_mABoxAssertions.find(ASSERTION_TYPE);
      if( i == m_mABoxAssertions.end() )
	{
	  pSet = new ExprNodeSet;
	  m_mABoxAssertions[ASSERTION_TYPE] = pSet;
	}
      else
	pSet = (ExprNodeSet*)i->second;
      pSet->insert(pTypeAxiom);
    }

  addType(pIndvidualExpr, pCExpr, pDS);
}

void KnowledgeBase::addType(ExprNode* pIndividualExpr, ExprNode* pCExpr, DependencySet* pDS)
{
  m_iKBStatus |= KBSTATUS_ABOX_CHANGED;

  // set addition flag
  m_bABoxAddition = TRUE;

  m_pABox->setSyntacticUpdate();
  m_pABox->addType(pIndividualExpr, pCExpr, pDS);
  m_pABox->setSyntacticUpdate(FALSE);

  // if use incremental reasoning then update the cached pseudo model as well
  if( canUseIncrementalConsistency() )
    {
      // add this individuals to the affected list - used for inc.
      // consistency checking
      m_pABox->m_setUpdatedIndividuals.insert(m_pABox->getPseudoModel()->getNode(pIndividualExpr));

      // as there can possibly be many branches in the pseudomodel, we
      // need a workaround, so temporarily set branch to 0 and then add to
      // the pseudomdel
      int iBranch = m_pABox->getPseudoModel()->getBranch();
      m_pABox->getPseudoModel()->setSyntacticUpdate();
      m_pABox->getPseudoModel()->setBranch(0);
      m_pABox->getPseudoModel()->addType(pIndividualExpr, pCExpr);
      m_pABox->getPseudoModel()->setBranch(iBranch);
      m_pABox->getPseudoModel()->setSyntacticUpdate(FALSE);

      // incrementally update the expressivity of the KB, so that we do
      // not have to reperform if from scratch!
      updateExpressivity(pIndividualExpr, pCExpr);
    }
}

void KnowledgeBase::addDomain(ExprNode* pP, ExprNode* pC, DependencySet* pDS)
{
  START_DECOMMENT2("KnowledgeBase::addDomain");
  printExprNodeWComment("pRole=", pP);
  printExprNodeWComment("pDomain=", pC);

  if( pDS == NULL )
    {
      if( PARAMS_USE_TRACING() )
	pDS = new DependencySet(createExprNode(EXPR_DOMAIN, pP, pC));
      else
	pDS = &DEPENDENCYSET_INDEPENDENT;
    }

  m_iKBStatus |= KBSTATUS_RBOX_CHANGED;
  
  Role* pRole = m_pRBox->getDefinedRole(pP);
  // TODO Need to do something with the dependency set.
  pRole->addDomain(pC, pDS);
  END_DECOMMENT("KnowledgeBase::addDomain");
}

void KnowledgeBase::addRange(ExprNode* pP, ExprNode* pC, DependencySet* pDS)
{
  if( pDS == NULL )
    {
      if( PARAMS_USE_TRACING() )
	pDS = new DependencySet(createExprNode(EXPR_RANGE, pP, pC));
      else
	pDS = &DEPENDENCYSET_INDEPENDENT;
    }

  m_iKBStatus |= KBSTATUS_RBOX_CHANGED;

  // CHECK HERE
  Role* pRole = m_pRBox->getDefinedRole(pP);
  // TODO Do something with dependency...
  pRole->addRange(pC, pDS);
}

void KnowledgeBase::addDifferent(ExprNode* pExprNode1, ExprNode* pExprNode2)
{
  m_iKBStatus |= KBSTATUS_ABOX_CHANGED;

  // set addition flag
  m_bABoxAddition = TRUE;

  // if we can use incremental consistency checking then add to
  // pseudomodel
  if( canUseIncrementalConsistency() )
    {
      m_pABox->m_setUpdatedIndividuals.insert( (Node*)m_pABox->getPseudoModel()->getIndividual(pExprNode1) );
      m_pABox->m_setUpdatedIndividuals.insert( (Node*)m_pABox->getPseudoModel()->getIndividual(pExprNode2) );

      // add to pseudomodel - note branch must be temporarily set to 0 to
      // ensure that asssertion
      // will not be restored during backtracking
      int iBranch = m_pABox->getPseudoModel()->getBranch();
      m_pABox->getPseudoModel()->setBranch(0);
      m_pABox->getPseudoModel()->addDifferent(pExprNode1, pExprNode2);
      m_pABox->getPseudoModel()->setBranch(iBranch);
    }

  m_pABox->addDifferent(pExprNode1, pExprNode2);
}

void KnowledgeBase::addProperty(ExprNode* pRoleExpr)
{
  m_iKBStatus |= KBSTATUS_RBOX_CHANGED;
  m_pRBox->addRole(pRoleExpr);
}

bool KnowledgeBase::addPropertyValue(ExprNode* pP, ExprNode* pS, ExprNode* pO)
{
  Individual* pSubj = m_pABox->getIndividual(pS);
  Role* pRole = m_pRBox->getRole(pP);
  Node* pObj = NULL;

  if( pSubj == NULL )
    {
      //log.warn( s + " is not a known individual!" );
      return FALSE;
    }
  if( pRole == NULL )
    {
      //log.warn( p + " is not a known property!" );
      return FALSE;
    }

  if( !pRole->isObjectRole() && !pRole->isDatatypeRole() )
    return FALSE;
  
  ExprNode* pPropAxiom = createExprNode(EXPR_PROP, pP, pS, pO);
  DependencySet* pDS = &DEPENDENCYSET_INDEPENDENT;
  if( PARAMS_USE_TRACING() )
    pDS = new DependencySet(pPropAxiom);
  
  if( pRole->isObjectRole() )
    {
      pObj = m_pABox->getIndividual(pO);
      if( pObj == NULL )
	{
	  if( isLiteral(pO) )
	    {
	      //log.warn( "Ignoring literal value " + o + " for object property " + p );
	      return FALSE;
	    }
	  else
	    {
	      //log.warn( o + " is not a known individual!" );
	      return FALSE;
	    }
	}

      if( PARAMS_KEEP_ABOX_ASSERTIONS() )
	{
	  ExprNodeSet* pSet = NULL;
	  map<int, ExprNodeSet*>::iterator i = m_mABoxAssertions.find(ASSERTION_OBJ_ROLE);
	  if( i == m_mABoxAssertions.end() )
	    {
	      pSet = new ExprNodeSet;
	      m_mABoxAssertions[ASSERTION_OBJ_ROLE] = pSet;
	    }
	  else
	    pSet = (ExprNodeSet*)i->second;
	  pSet->insert(pPropAxiom);
	}
    }
  else if( pRole->isDatatypeRole() )
    {
      pObj = m_pABox->addLiteral(pO, pDS);
      if( PARAMS_KEEP_ABOX_ASSERTIONS() )
	{
	  ExprNodeSet* pSet = NULL;
	  map<int, ExprNodeSet*>::iterator i = m_mABoxAssertions.find(ASSERTION_DATA_ROLE);
	  if( i == m_mABoxAssertions.end() )
	    {
	      pSet = new ExprNodeSet;
	      m_mABoxAssertions[ASSERTION_DATA_ROLE] = pSet;
	    }
	  else
	    pSet = (ExprNodeSet*)i->second;
	  pSet->insert(pPropAxiom);
	}
    }

  m_iKBStatus |= KBSTATUS_ABOX_CHANGED;

  // set addition flag
  m_bABoxAddition = TRUE;

  Edge* pEdge = pSubj->addEdge(pRole, pObj, pDS);
  
  if( PARAMS_USE_INCREMENTAL_DELETION() )
    {
      // add to syntactic assertions
      m_setSyntacticAssertions.insert(pPropAxiom);

      // add to dependency index
      m_pDependencyIndex->addEdgeDependency(pEdge, pDS);
    }

  // if use inc. reasoning then we need to update the pseudomodel
  if( canUseIncrementalConsistency() )
    {
      // add this individual to the affected list
      m_pABox->m_setUpdatedIndividuals.insert( (Node*)m_pABox->getPseudoModel()->getIndividual(pS) );
      
      // if this is an object property then add the object to the affected list
      if( !pRole->isDatatypeRole() )
	m_pABox->m_setUpdatedIndividuals.insert( (Node*)m_pABox->getPseudoModel()->getIndividual(pO) );

      if( pRole->isObjectRole() )
	{
	  pObj = m_pABox->getPseudoModel()->getIndividual(pO);
	  if( pObj->isPruned() || pObj->isMerged() )
	    pObj = pObj->getSame();
	}
      else if( pRole->isDatatypeRole() )
	pObj = m_pABox->getPseudoModel()->addLiteral(pO);

      
      // get the subject
      Individual* pSubj2 = m_pABox->getPseudoModel()->getIndividual(pS);
      if( pSubj2->isPruned() || pSubj2->isMerged() )
	pSubj2 = (Individual*)pSubj2->getSame();

      pDS = &DEPENDENCYSET_INDEPENDENT;
      if( PARAMS_USE_TRACING() )
	pDS = new DependencySet(createExprNode(EXPR_PROP, pP, pS, pO));

      // add to pseudomodel - note branch must be temporarily set to 0 to
      // ensure that asssertion
      // will not be restored during backtracking
      int iBranch = m_pABox->getPseudoModel()->getBranch();
      m_pABox->getPseudoModel()->setBranch(0);
      pSubj2->addEdge(pRole, pObj, pDS);
      m_pABox->getPseudoModel()->setBranch(iBranch);
    }
  return TRUE;
}

void KnowledgeBase::addSubProperty(ExprNode* pSub, ExprNode* pSup)
{
  START_DECOMMENT2("KnowledgeBase::addSubProperty");
  m_iKBStatus |= KBSTATUS_RBOX_CHANGED;
  m_pRBox->addSubRole(pSub, pSup);
  END_DECOMMENT("KnowledgeBase::addSubProperty");
}

Role* KnowledgeBase::getRole(ExprNode* p)
{
  return m_pRBox->getRole(p);
}

Role* KnowledgeBase::getProperty(ExprNode* p)
{
  return m_pRBox->getRole(p);
}

int KnowledgeBase::getPropertyType(ExprNode* p)
{
  Role* pRole = m_pRBox->getRole(p);
  return (pRole==NULL)?Role::UNTYPED:pRole->m_iRoleType;
}

bool KnowledgeBase::isObjectProperty(ExprNode* p)
{
  return (getPropertyType(p) == Role::OBJECT);
}

/**
 * Add a new object property. If property was earlier defined to be a
 * datatype property then this function will simply return without changing
 * the KB.
 */
bool KnowledgeBase::addDatatypeProperty(ExprNode* pP)
{
  bool bExists = (getPropertyType(pP) == Role::DATATYPE);
  Role* pRole = m_pRBox->addDatatypeRole(pP);
  
  if( bExists )
    m_iKBStatus |= KBSTATUS_RBOX_CHANGED;

  return (pRole!=NULL);
}

/**
 * Add a new object property. If property was earlier defined to be a
 * datatype property then this function will simply return without changing
 * the KB.
 */
bool KnowledgeBase::addObjectProperty(ExprNode* pP)
{
  bool bExists = (getPropertyType(pP) == Role::OBJECT);
  Role* pRole = m_pRBox->addObjectRole(pP);
  
  if( bExists )
    m_iKBStatus |= KBSTATUS_RBOX_CHANGED;

  return (pRole!=NULL);
}

void KnowledgeBase::addFunctionalProperty(ExprNode* pP)
{
  m_iKBStatus |= KBSTATUS_RBOX_CHANGED;
  Role* pRole = m_pRBox->getDefinedRole(pP);
  
  DependencySet* pDS = &DEPENDENCYSET_INDEPENDENT;
  if( PARAMS_USE_TRACING() )
    pDS = new DependencySet(createExprNode(EXPR_FUNCTIONAL, pP));

  pRole->setFunctional(TRUE, pDS);
}

void KnowledgeBase::addTransitiveProperty(ExprNode* pP)
{
  m_iKBStatus |= KBSTATUS_RBOX_CHANGED;
  Role* pRole = m_pRBox->getDefinedRole(pP);

  DependencySet* pDS = &DEPENDENCYSET_INDEPENDENT;
  if( PARAMS_USE_TRACING() )
    pDS = new DependencySet(createExprNode(EXPR_TRANSITIVE, pP));

  ExprNodeList* pList = createExprNodeList();
  addExprToList(pList, pP);
  addExprToList(pList, pP);
  pRole->addSubRoleChain(pList, pDS);
}

void KnowledgeBase::addInverseProperty(ExprNode* pP1, ExprNode* pP2)
{
  if( PARAMS_IGNORE_INVERSES() )
    return ;

  m_iKBStatus |= KBSTATUS_RBOX_CHANGED;

  DependencySet* pDS = &DEPENDENCYSET_INDEPENDENT;
  if( PARAMS_USE_TRACING() )
    pDS = new DependencySet(createExprNode(EXPR_INVERSEPROPERTY, pP1, pP2));

  m_pRBox->addInverseRole(pP1, pP2, pDS);
}

void KnowledgeBase::addEquivalentProperty(ExprNode* pP1, ExprNode* pP2)
{
  m_iKBStatus |= KBSTATUS_RBOX_CHANGED;
  m_pRBox->addEquivalentRole(pP1, pP2);
}

/* Check if we can use incremental consistency checking */
bool KnowledgeBase::canUseIncrementalConsistency()
{
  if( m_pExpressivity == NULL )
    return FALSE;

  return (!(m_pExpressivity->hasNominal() && m_pExpressivity->hasInverse())) &&
    // !m_pExpressivity->hasAdvancedRoles() &&
    // !m_pExpressivity->hasCardinalityQ() &&
    !isTBoxChanged() && !isRBoxChanged() && (m_pABox->getPseudoModel() != NULL) && 
    PARAMS_USE_INCREMENTAL_CONSISTENCE() &&
    // support additions only; also support deletions with or with
    // additions, however tracing must be on to support incremental deletions
    ((!m_bABoxDeletion && m_bABoxAddition) || 
     (m_bABoxDeletion && PARAMS_USE_INCREMENTAL_DELETION()));
}

void KnowledgeBase::updateExpressivity(ExprNode* pIndividualExpr, ExprNode* pCExpr)
{
  // if the tbox or rbox changed then we cannot use incremental reasoning!
  if( !isChanged() || isTBoxChanged() || isRBoxChanged() )
    return;
  
  // update status
  m_iKBStatus = KBSTATUS_UNCHANGED;

  // update expressivity given this individual
  m_pExpressivity->processIndividual(pIndividualExpr, pCExpr);
  
  // update the size estimate as this could be a new individual
  m_pEstimate = new SizeEstimate(this);
}

bool KnowledgeBase::isConsistent()
{
  checkConsistency();
  return m_bConsistent;
}

/**
 * Returns true if the consistency check has been done and nothing in th KB
 * has changed after that.
 */
bool KnowledgeBase::isConsistencyDone()
{
  // check if consistency bit is set but none of the change bits
  return (m_iKBStatus & (KBSTATUS_CONSISTENCY|KBSTATUS_ALL_CHANGED) == KBSTATUS_CONSISTENCY);
}

void KnowledgeBase::checkConsistency()
{
  if( isConsistencyDone() )
    return;

  // always turn on explanations for the first consistency check
  bool bExplain = m_pABox->m_bDoExplanation;
  m_pABox->m_bDoExplanation = TRUE;

  // only prepare if we are not going to use the incremental 
  // consistency checking approach
  if( !canUseIncrementalConsistency() )
    prepare();
  
  m_bConsistent = m_pABox->isConsistent();
  m_pABox->m_bDoExplanation = bExplain;

  if( !m_bConsistent )
    {
      // log.warn( "Inconsistent ontology. Reason: " + getExplanation() );
    }

  m_iKBStatus |= KBSTATUS_CONSISTENCY;

  m_bABoxAddition = FALSE;
  m_bABoxDeletion = FALSE;

  m_setDeletedAssertions.clear();
}

Expressivity* KnowledgeBase::getExpressivity()
{
  // if we can use incremental reasoning then expressivity has been
  // updated as only the ABox was incrementally changed
  if( canUseIncrementalConsistency() )
    return m_pExpressivity;
  prepare();
  return m_pExpressivity;
}

CompletionStrategy* KnowledgeBase::chooseStrategy(ABox* pABox)
{
  return chooseStrategy(pABox, getExpressivity());
}

/**
 * Choose a completion strategy based on the expressivity of the KB. The
 * abox given is not necessarily the ABox that belongs to this KB but can be
 * a derivative.
 */
CompletionStrategy* KnowledgeBase::chooseStrategy(ABox* pABox, Expressivity* pExpressivity)
{
  // if there are dl-safe rules present, use RuleStrategy which is a subclass of SHOIN
  // only problem is, we're using SHOIN everytime there are rule- it is
  // faster to use SHN + Rules in some cases
#if 0
  if( getRules().size() > 0 && 
      ( expressivity.hasNominal() || 
	!( abox.size() == 1 && abox.getNodes().iterator().next().getName().equals( ATermUtils.CONCEPT_SAT_IND ) ) ) ) {
    if ( PelletOptions.USE_CONTINUOUS_RULES ) {
      return new ContinuousRulesStrategy( abox );
    }
    else {
      return new RuleStrategy( abox );
    }
  } 
  if( PelletOptions.DEFAULT_COMPLETION_STRATEGY != null ) {
    Class[] types = new Class[] { ABox.class };
    Object[] args = new Object[] { abox };
    try {
      Constructor<? extends CompletionStrategy> cons = PelletOptions.DEFAULT_COMPLETION_STRATEGY.getConstructor( types );
      return cons.newInstance( args );
    } catch( Exception e ) {
      e.printStackTrace();
      throw new InternalReasonerException(
					  "Failed to create the default completion strategy defined in PelletOptions!" );
    }
  }
#endif
  
  if( PARAMS_USE_COMPLETION_STRATEGY() )
    {
      bool bEmptyStrategy = FALSE;
      if( m_pABox->size() == 1 )
	{
	  Individual* pIndividual = (Individual*)(pABox->m_mNodes.begin())->second;
	  if( pIndividual->m_listOutEdges.m_listEdge.size() == 0 )
	    bEmptyStrategy = TRUE;
	}
      bool bFullDatatypeReasoning = (PARAMS_USE_FULL_DATATYPE_REASONING() && (pExpressivity->hasCardinalityD()||pExpressivity->hasKeys()));
      if( !bFullDatatypeReasoning )
	{
	  if( pExpressivity->hasComplexSubRoles() )
	    return (new SROIQStrategy(pABox));
	  else if( pExpressivity->hasCardinalityQ() )
	    return (new SHOIQStrategy(pABox));
	  else if( pExpressivity->hasNominal() )
	    {
	      if( pExpressivity->hasInverse() )
		return (new SHOINStrategy(pABox));
	      return (new SHONStrategy(pABox));
	    }
	  else if( pExpressivity->hasInverse() )
	    return (new SHINStrategy(pABox));
	  else if( bEmptyStrategy && !pExpressivity->hasCardinalityD() && !pExpressivity->hasKeys() )
	    return (new EmptySHNStrategy(pABox));
	  return (new SHNStrategy(pABox));
	}
    }

  return (new SROIQStrategy(pABox));
}

/**
 * Method to remove all stuctures dependent on an ABox assertion from the
 * abox. This is used for incremental reasoning under ABox deletions.
 */
void KnowledgeBase::restoreDependencies()
{
  // iterate over all removed assertions
  for(ExprNodeSet::iterator i = m_setDeletedAssertions.begin(); i != m_setDeletedAssertions.end(); i++ )
    {
      ExprNode* pAssertion = (ExprNode*)*i;
      
      // get the dependency entry
      DependencyEntry* pDE = m_pDependencyIndex->getDependencies(pAssertion);
      if( pDE )
	{
	  // restore the entry
	  restoreDependency(pAssertion, pDE);
	}
      
      // remove the entry in the index for this assertion
      m_pDependencyIndex->removeDependencies(pAssertion);
    }
}

/**
 * Perform the actual rollback of a depenedency entry
 */
void KnowledgeBase::restoreDependency(ExprNode* pAssertion, DependencyEntry* pEntry)
{
  START_DECOMMENT2("KnowledgeBase::restoreDependency");
  for(EdgeSet::iterator i = pEntry->m_setEdges.begin(); i != pEntry->m_setEdges.end(); i++ )
    restoreEdge(pAssertion, (Edge*)*i);
  
  for(SetOfDependency::iterator i = pEntry->m_setTypes.begin(); i != pEntry->m_setTypes.end(); i++ )
    restoreType(pAssertion, (TypeDependency*)*i);

  for(SetOfDependency::iterator i = pEntry->m_setMerges.begin(); i != pEntry->m_setMerges.end(); i++ )
    restoreMerge(pAssertion, (MergeDependency*)*i);
  
  for(SetOfDependency::iterator i = pEntry->m_setBranchAdds.begin(); i != pEntry->m_setBranchAdds.end(); i++ )
    restoreBranchAdd(pAssertion, (BranchAddDependency*)*i);

  for(SetOfDependency::iterator i = pEntry->m_setBranchCloses.begin(); i != pEntry->m_setBranchCloses.end(); i++ )
    restoreCloseBranch(pAssertion, (CloseBranchDependency*)*i);

  if( pEntry->m_pClashDependency )
    restoreClash(pAssertion, pEntry->m_pClashDependency);
  END_DECOMMENT("KnowledgeBase::restoreDependency");
}

void KnowledgeBase::restoreEdge(ExprNode* pAssertion, Edge* pEdge)
{
  START_DECOMMENT2("KnowledgeBase::restoreEdge");
  // the edge could have previously been removed so return
  if( pEdge == NULL )
    {
      END_DECOMMENT("KnowledgeBase::restoreEdge");
      return;
    }

  // get the object
  Individual* pSubj = m_pABox->m_pPseudoModel->getIndividual( pEdge->m_pFrom->m_pName );
  Node* pObj = m_pABox->m_pPseudoModel->getNode( pEdge->m_pTo->m_pName );
  Role* pRole = getRole( pEdge->m_pRole->m_pName );

  // loop over all edges for the subject
  EdgeList aEdgesTo;
  pSubj->getEdgesTo(pObj, pRole, &aEdgesTo);
  for(EdgeVector::iterator i = aEdgesTo.m_listEdge.begin(); i != aEdgesTo.m_listEdge.end(); i++ )
    {
      Edge* pE = (Edge*)*i;
      if( ::isEqual(pEdge->m_pRole, pRole) == 0 )
	{
	  // get dependency set for the edge
	  DependencySet* pDS = pE->m_pDepends;
	  
	  // clean it
	  pDS->removeExplain(pAssertion);

	  // remove if the dependency set is empty
	  if( pDS->m_setExplain.size() == 0 )
	    {
	      // need to check if the
	      pSubj->removeEdge(pE);
	      
	      // add to updated individuals
	      m_pABox->m_setUpdatedIndividuals.insert(pSubj);
	      
	      // TODO: Do we need to add literals?
	      if( pObj->isIndividual() )
		m_pABox->m_setUpdatedIndividuals.insert((Individual*)pObj);
	    }
	  break;
	}
    }
  END_DECOMMENT("KnowledgeBase::restoreEdge");
}

void KnowledgeBase::restoreType(ExprNode* pAssertion, TypeDependency* pType)
{
  START_DECOMMENT2("KnowledgeBase::restoreType");
  // get the dependency set - Note: we must normalize the concept
  DependencySet* pDS = NULL;
  ExprNode2DependencySetMap* pDSetMap = &(m_pABox->m_pPseudoModel->getNode(pType->m_pInd)->m_mDepends);
  ExprNode2DependencySetMap::iterator iFind = pDSetMap->find( normalize(pType->m_pType) );
  if( iFind != pDSetMap->end() )
    pDS = (DependencySet*)iFind->second;

  // return if null - this can happen as currently I have dupilicates in
  // the index
  if( pDS == NULL || isEqual(pType->m_pType, EXPRNODE_TOP) == 0 )
    {
      END_DECOMMENT("KnowledgeBase::restoreType");
      return;
    }

  // clean it
  pDS->removeExplain(pAssertion);

  // remove if the explanation set is empty
  if( pDS->m_setExplain.size() == 0 )
    {
      m_pABox->m_pPseudoModel->removeType(pType->m_pInd, pType->m_pType);

      // add to updated individuals
      Node* pN = m_pABox->m_pPseudoModel->getNode( pType->m_pInd );
      if( pN && pN->isIndividual() )
	{
	  Individual* pInd = (Individual*)pN;
	  m_pABox->m_setUpdatedIndividuals.insert(pInd);
	  
	  // also need to add all edge object to updated individuals -
	  // this is needed to fire allValues/domain/range rules etc.
	  for(EdgeVector::iterator i = pInd->m_listInEdges.m_listEdge.begin(); i != pInd->m_listInEdges.m_listEdge.end(); i++ )
	    {
	      Edge* pE = (Edge*)*i;
	      m_pABox->m_setUpdatedIndividuals.insert(pE->m_pFrom);
	    }

	  for(EdgeVector::iterator i = pInd->m_listOutEdges.m_listEdge.begin(); i != pInd->m_listOutEdges.m_listEdge.end(); i++ )
	    {
	      Edge* pE = (Edge*)*i;
	      if( pE->m_pTo->isIndividual() )
		m_pABox->m_setUpdatedIndividuals.insert(pE->m_pTo);
	    }
	}
    }
  END_DECOMMENT("KnowledgeBase::restoreType");
}

void KnowledgeBase::restoreMerge(ExprNode* pAssertion, MergeDependency* pMerge)
{
  START_DECOMMENT2("KnowledgeBase::restoreMerge");
  // get merge dependency
  DependencySet* pDS = m_pABox->m_pPseudoModel->getNode(pMerge->m_pInd)->m_pMergeDepends;
  
  // clean it
  pDS->removeExplain(pAssertion);

  // undo merge if empty
  if( pDS->m_setExplain.size() == 0 )
    {
      // get nodes
      Node* pInd = m_pABox->m_pPseudoModel->getNode(pMerge->m_pInd);
      Node* pMergedToInd = m_pABox->m_pPseudoModel->getNode(pMerge->m_pMergeIntoInd);
      
      // check that they are actually the same - else throw error
      if( !pInd->isSame(pMergedToInd) )
	assertFALSE("Restore merge error: not same");

      if( !pInd->isPruned() )
	assertFALSE("Restore merge error: not pruned");

      // unprune to prune branch
      pInd->unprune(pInd->m_pPruned->m_iBranch);
      
      // undo set same
      pInd->undoSetSame();

      // add to updated
      // Note that ind.unprune may add edges, however we do not need to
      // add them to the updated individuals as
      // they will be added when the edge is removed from the node which
      // this individual was merged to
      if( pInd->isIndividual() )
	m_pABox->m_setUpdatedIndividuals.insert((Individual*)pInd);
    }
  END_DECOMMENT("KnowledgeBase::restoreMerge");
}

void KnowledgeBase::restoreBranchAdd(ExprNode* pAssertion, BranchAddDependency* pBranchAdd)
{
  START_DECOMMENT2("KnowledgeBase::restoreBranchAdd");
  // get merge dependency
  DependencySet* pDS = pBranchAdd->m_pBranch->m_pTermDepends;

  // clean it
  pDS->removeExplain(pAssertion);

  // undo merge if empty
  if( pDS->m_setExplain.size() == 0 )
    {
      // !!!!!!!!!!!!!!!! First update completion queue branch effects
      // !!!!!!!!!!!!!!
      // need to update the completion queue
      // currently i track all nodes that are effected during the
      // expansion rules for a given branch
      ExprNodeSet allEffects;
      for( int i = pBranchAdd->m_pBranch->m_iBranch; i < m_pABox->m_pPseudoModel->m_pCompletionQueue->m_listBranchEffects.size();i++ )
	{
	  ExprNodeSet* pSet = (ExprNodeSet*)m_pABox->m_pPseudoModel->m_pCompletionQueue->m_listBranchEffects.at(i);
	  allEffects.insert(pSet->begin(), pSet->end());
	}
      
      for(ExprNodeSet::iterator i = allEffects.begin(); i != allEffects.end(); i++ )
	{
	  ExprNode* pATerm = (ExprNode*)*i;
	  
	  // get the actual node
	  Node* pNode = m_pABox->m_pPseudoModel->getNode(pATerm);
	  for(ExprNode2DependencySetMap::iterator j = pNode->m_mDepends.begin(); j != pNode->m_mDepends.end(); j++) 
	    {
	      // get next type
	      ExprNode* pType = (ExprNode*)j->first;
	      
	      // get ds for type
	      DependencySet* pTDS = pNode->getDepends(pType);

	      // update branch if necessary
	      if( pTDS->m_iBranch > pBranchAdd->m_pBranch->m_iBranch )
		pTDS->m_iBranch--;

	      for(int k = pBranchAdd->m_pBranch->m_iBranch; k <= m_pABox->m_pPseudoModel->m_aBranches.size(); k++ )
		{
		  // update dependency set
		  if( pTDS->contains(k) )
		    {
		      pTDS->remove(k);
		      pTDS->add(k-1);
		    }
		}
	    }

	  // update edge depdencies
	  for(EdgeVector::iterator i = pNode->m_listInEdges.m_listEdge.begin(); i != pNode->m_listInEdges.m_listEdge.end(); i++ )
	    {
	      Edge* pEdge = (Edge*)*i;
	      
	      // update branch if necessary
	      if( pEdge->m_pDepends->m_iBranch > pBranchAdd->m_pBranch->m_iBranch )
		pEdge->m_pDepends->m_iBranch--;

	      for(int k = pBranchAdd->m_pBranch->m_iBranch; k <= m_pABox->m_pPseudoModel->m_aBranches.size(); k++ )
		{
		  // update dependency set
		  if( pEdge->m_pDepends->contains(k) )
		    {
		      pEdge->m_pDepends->remove(k);
		      pEdge->m_pDepends->add(k-1);
		    }
		}
	    }
	}

      // remove the deleted branch from branch effects
      m_pABox->m_pPseudoModel->m_pCompletionQueue->m_listBranchEffects.erase( m_pABox->m_pPseudoModel->m_pCompletionQueue->m_listBranchEffects.begin()+pBranchAdd->m_pBranch->m_iBranch );

      // !!!!!!!!!!!!!!!! Next update abox branches !!!!!!!!!!!!!!
      // remove the branch from branches
      
      // decrease branch id for each branch after the branch we're
      // removing
      // also need to change the dependency set for each label
      for( int i = pBranchAdd->m_pBranch->m_iBranch; i < m_pABox->m_pPseudoModel->m_aBranches.size(); i++ )
	{
	  Branch* pBranch = (Branch*)m_pABox->m_pPseudoModel->m_aBranches.at(i);
	  
	  // update the term depends in the branch
	  if( pBranch->m_pTermDepends->m_iBranch > pBranchAdd->m_pBranch->m_iBranch )
	    pBranch->m_pTermDepends->m_iBranch--;

	  for(int k = pBranchAdd->m_pBranch->m_iBranch; k <= m_pABox->m_pPseudoModel->m_aBranches.size(); k++ )
	    {
	      // update dependency set
	      if( pBranch->m_pTermDepends->contains(k) )
		{
		  pBranch->m_pTermDepends->remove(k);
		  pBranch->m_pTermDepends->add(k-1);
		}
	    }

	  // also need to decrement the branch number
	  pBranch->m_iBranch--;
	}

      // remove the actual branch
      for(int i = 0; i < m_pABox->m_pPseudoModel->m_aBranches.size(); i++ )
	{
	  Branch* pBranch = (Branch*)m_pABox->m_pPseudoModel->m_aBranches.at(i);
	  if( pBranch == pBranchAdd->m_pBranch )
	    {
	      m_pABox->m_pPseudoModel->m_aBranches.erase(m_pABox->m_pPseudoModel->m_aBranches.begin()+i);
	      break;
	    }
	}

      // set the branch counter
      m_pABox->m_pPseudoModel->m_iCurrentBranchIndex = (m_pABox->m_pPseudoModel->m_iCurrentBranchIndex-1);
    }
  END_DECOMMENT("KnowledgeBase::restoreBranchAdd");
}

void KnowledgeBase::restoreCloseBranch(ExprNode* pAssertion, CloseBranchDependency* pCloseBranch)
{
  START_DECOMMENT2("KnowledgeBase::restoreCloseBranch");
  // only proceed if tryNext is larger than 1!
  if( pCloseBranch->m_pBranch->m_iTryNext > -1 )
    {
      pCloseBranch->m_pBranch->shiftTryNext(pCloseBranch->m_iTryNext);
    }
  END_DECOMMENT("KnowledgeBase::restoreCloseBranch");
}

void KnowledgeBase::restoreClash(ExprNode* pAssertion, ClashDependency* pClash)
{
  START_DECOMMENT2("KnowledgeBase::restoreClash");
  pClash->m_pClash->m_pDepends->removeExplain(pAssertion);
  if( pClash->m_pClash->m_pDepends->m_setExplain.size() == 0 && pClash->m_pClash->m_pDepends->isIndependent() )
    m_pABox->m_pPseudoModel->m_pClash = NULL;
  END_DECOMMENT("KnowledgeBase::restoreClash");
}

void KnowledgeBase::ensureConsistency()
{
  if( !isConsistent() )
    assertFALSE("Cannot do reasoning with inconsistent ontologies!");
}

bool KnowledgeBase::isClassified()
{
  return (m_iKBStatus & (KBSTATUS_CLASSIFICATION|KBSTATUS_ALL_CHANGED)) == KBSTATUS_CLASSIFICATION;
}

void KnowledgeBase::classify()
{
  START_DECOMMENT2("KnowledgeBase::classify");
  ensureConsistency();
  
  if( isClassified() )
    {
      END_DECOMMENT("KnowledgeBase::classify");
      return;
    }

  m_pBuilder = new TaxonomyBuilder();
  m_pBuilder->m_pKB = this;

  m_pTaxonomy = m_pBuilder->classify();
  if( m_pTaxonomy == NULL )
    {
      delete m_pBuilder;
      m_pBuilder = NULL;
      END_DECOMMENT("KnowledgeBase::classify");
      return;
    }

  m_iKBStatus |= KBSTATUS_CLASSIFICATION;
  END_DECOMMENT("KnowledgeBase::classify");
}

bool KnowledgeBase::isRealized()
{
  return (m_iKBStatus & (KBSTATUS_REALIZATION|KBSTATUS_ALL_CHANGED) == KBSTATUS_REALIZATION);
}

void KnowledgeBase::realize()
{
  START_DECOMMENT2("KnowledgeBase::realize");
  
  if( isRealized() )
    {
      END_DECOMMENT("KnowledgeBase::realize");
      return;
    }

  classify();
  if( !isClassified() )
    {
      END_DECOMMENT("KnowledgeBase::realize");
      return;
    }

  m_pTaxonomy = m_pBuilder->realize();
  if( m_pTaxonomy == NULL )
    {
      delete m_pBuilder;
      m_pBuilder = NULL;
      END_DECOMMENT("KnowledgeBase::realize");
      return;
    }

  m_iKBStatus |= KBSTATUS_REALIZATION;
  END_DECOMMENT("KnowledgeBase::realize");
}

ExprNodeSet* KnowledgeBase::getClasses()
{
  if( m_pTBox )
    return &(m_pTBox->m_setClasses);
  return NULL;
}

/**
 * Return the domain restrictions on the property. The results of this
 * function is not guaranteed to be complete. Use
 */
void KnowledgeBase::getDomains(ExprNode* pName, ExprNodeSet* pDomains)
{
  ensureConsistency();

  Role* pProp = m_pRBox->getRole(pName);
  if( pProp == NULL )
    return;

  ExprNode* pDomain = pProp->m_pDomain;
  if( pDomain )
    {
      if( pDomain->m_iExpression == EXPR_AND )
	getPrimitives((ExprNodeList*)pDomain->m_pArgList, pDomains);
      else if( isPrimitive(pDomain) )
	pDomains->insert(pDomain);
    }
}

bool KnowledgeBase::isType(ExprNode* pX, ExprNode* pC)
{
  ensureConsistency();
  
  if( m_setIndividuals.find(pX) != m_setIndividuals.end() )
    return FALSE;

  if( !isClass(pC) )
    return FALSE;

  return (isRealized() && m_pTaxonomy->contains(pC) && m_pABox->m_bDoExplanation)?m_pTaxonomy->isType(pX, pC):m_pABox->isType(pX, pC);
}

bool KnowledgeBase::isClass(ExprNode* pC)
{
  if( m_pTBox->m_setClasses.find(pC) != m_pTBox->m_setClasses.end() || isEqual(pC, EXPRNODE_TOP) == 0 )
    return TRUE;
  //else if( isComplexClass(pC) )
  //return fullyDefinedVisitor.isFullyDefined( (ATermAppl) c );
  return FALSE;
}

/**
 * Return all the individuals that belong to the given class which is not
 * necessarily a named class.
 */
void KnowledgeBase::retrieve(ExprNode* pD, ExprNodeSet* pIndividuals, ExprNodeSet* pSubInstances)
{
  ensureConsistency();
  
  ExprNode* pC = normalize(pD);
  ExprNode2ExprNodeSetMap::iterator iFind = m_mapInstances.find(pC);
  if( iFind != m_mapInstances.end() )
    {
      ExprNodeSet* pSet = (ExprNodeSet*)iFind->second;
      for(ExprNodeSet::iterator i = pSet->begin(); i != pSet->end(); i++ )
	pSubInstances->insert((ExprNode*)*i);
      return;
    }
  else if( isRealized() && m_pTaxonomy->contains(pC) )
    {
      ExprNodeSet setInstances;
      getInstances(pC, &setInstances);
      for(ExprNodeSet::iterator i = setInstances.begin(); i != setInstances.end(); i++ )
	pSubInstances->insert((ExprNode*)*i);
      return;
    }

  ExprNodes aKnowns;
  ExprNode* pNotC = negate2(pC);
  // this is mostly to ensure that a model for notC is cached
  if( !m_pABox->isSatisfiable(pNotC) )
    {
      // if negation is unsat c itself is TOP
      for(ExprNodeSet::iterator i = m_setIndividuals.begin(); i != m_setIndividuals.end(); i++ )
	aKnowns.push_back((ExprNode*)*i);
    }
  else if( m_pABox->isSatisfiable(pC) )
    {
      ExprNodeSet setSubs;
      if( isClassified() && m_pTaxonomy->contains(pC) )
	m_pTaxonomy->getFlattenedSubSupers(pC, FALSE, FALSE, &setSubs);
      
      ExprNodes aUnknowns;
      for(ExprNodeSet::iterator i = m_setIndividuals.begin(); i != m_setIndividuals.end(); i++ )
	{
	  ExprNode* pX = (ExprNode*)*i;
	  int iIsType = m_pABox->isKnownType(pX, pC, &setSubs);
	  if( iIsType == 1 )
	    aKnowns.push_back(pX);
	  else if( iIsType == -1 )
	    aUnknowns.push_back(pX);
	}

      if( aUnknowns.size() > 0 && m_pABox->isType(&aUnknowns, pC) )
	{
	  if( PARAMS_USE_BINARY_INSTANCE_RETRIEVAL() )
	    binaryInstanceRetrieval(pC, &aUnknowns, &aKnowns);
	  else
	    linearInstanceRetrieval(pC, &aUnknowns, &aKnowns);
	}
    }

  pSubInstances->clear();
  for(ExprNodes::iterator i = aKnowns.begin(); i != aKnowns.end(); i++ )
    pSubInstances->insert((ExprNode*)*i);

  //if( PelletOptions.CACHE_RETRIEVAL )
  //m_mapInstances[pC] = pResult;
}

/**
 * Returns all the instances of concept c. If TOP concept is used every
 * individual in the knowledge base will be returned
 */
void KnowledgeBase::getInstances(ExprNode* pC, ExprNodeSet* pSetInstances)
{
  if( !isClass(pC) )
    return;
  
  if( isRealized() && m_pTaxonomy->contains(pC) )
    {
      m_pTaxonomy->getInstances(pC, pSetInstances);
      return;
    }

  retrieve(pC, &m_setIndividuals, pSetInstances);
}

void KnowledgeBase::linearInstanceRetrieval(ExprNode* pC, ExprNodes* pCandidates, ExprNodes* pResults)
{
  for(ExprNodes::iterator i = pCandidates->begin(); i != pCandidates->end(); i++ )
    {
      ExprNode* pInd = (ExprNode*)*i;
      if( m_pABox->isType(pInd, pC) )
	pResults->push_back(pInd);
    }
}

void KnowledgeBase::binaryInstanceRetrieval(ExprNode* pC, ExprNodes* pCandidates, ExprNodes* pResults)
{

}

void KnowledgeBase::printClassTree()
{
  classify();
  m_pTaxonomy->print();
}

void KnowledgeBase::printClassTreeInFile(char* pOutputFile)
{
  classify();
  m_pTaxonomy->printInFile(pOutputFile);
}

void KnowledgeBase::printExpressivity()
{
  if( PARAMS_PRINT_DEBUGINFO_INHTML() )
    printf("<comment>");
  getExpressivity()->printExpressivity();
  if( PARAMS_PRINT_DEBUGINFO_INHTML() )
    printf("</comment>");
}
