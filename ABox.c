#include "ABox.h"
#include "Params.h"
#include "CompletionQueue.h"
#include "ReazienerUtils.h"
#include "KnowledgeBase.h"
#include "Expressivity.h"
#include "Individual.h"
#include "Clash.h"
#include "DependencyIndex.h"
#include "Literal.h"
#include "CachedNode.h"
#include "ConceptCache.h"
#include "CompletionStrategy.h"
#include "SROIQIncStrategy.h"
#include "Role.h"
#include "State.h"
#include "TransitionGraph.h"
#include "Taxonomy.h"
#include "TBox.h"

extern DependencySet DEPENDENCYSET_EMPTY;
extern DependencySet DEPENDENCYSET_DUMMY;
extern DependencySet DEPENDENCYSET_INDEPENDENT;
extern ExprNode* EXPRNODE_TOP;
extern ExprNode* EXPRNODE_BOTTOM;
extern ExprNode* EXPRNODE_CONCEPT_SAT_IND;
extern KnowledgeBase* g_pKB;

extern Individual* g_pTOPIND;
extern Individual* g_pBOTTOMIND;
extern Individual* g_pDUMMYIND;

extern int g_iCommentIndent;

ABox::ABox()
{
  m_iCurrentBranchIndex = 0;
  m_iBranchSize = 0;
  m_bSyntacticUpdate = FALSE;

  m_pCompletionQueue = new CompletionQueue;

  m_bDoExplanation = FALSE;
  m_bChanged = FALSE;

  m_pSourceABox = NULL;
  m_pPseudoModel = NULL; 
  m_pClash = NULL;
  m_pCache = new ConceptCache;

  m_pLastCompletion = NULL;
  m_bKeepLastCompletion = FALSE;
  m_pLastClash = NULL;

  m_bRulesNotApplied = FALSE;

  m_iConsistencyCount = 0;
  m_iSatisfiabilityCount = 0;
  m_bInitialized = FALSE;
  m_bIsComplete = FALSE;

  m_iAnonCount = 0;

  // size of the completion tree for statistical purposes
  m_iTreeDepth = 0;
}

ABox::ABox(ABox* pABox)
{
  init(pABox, NULL, TRUE);
}

ABox::ABox(ABox* pABox, ExprNode* pExtraIndividual, bool bCopyIndividuals)
{
  init(pABox, pExtraIndividual, bCopyIndividuals);
}

void ABox::init(ABox* pABox, ExprNode* pExtraIndividual, bool bCopyIndividuals)
{
  m_pSourceABox = NULL;

  m_bInitialized = pABox->m_bInitialized;
  m_bChanged = pABox->m_bChanged;
  m_iAnonCount = pABox->m_iAnonCount;
  m_pCache = pABox->m_pCache;
  m_pClash = pABox->m_pClash;
  m_bDoExplanation = pABox->m_bDoExplanation;
  
  // copy the queue - this must be done early so that the effects of
  // adding the extra individual do not get removed
  if( PARAMS_USE_COMPLETION_QUEUE() )
    {
      m_pCompletionQueue = pABox->m_pCompletionQueue->copy();
      m_pCompletionQueue->m_pABox = this;

      if( PARAMS_USE_INCREMENTAL_CONSISTENCE() )
	{
	  if( pABox->m_setUpdatedIndividuals.size() > 0 )
	    m_setUpdatedIndividuals.insert(pABox->m_setUpdatedIndividuals.begin(), pABox->m_setUpdatedIndividuals.end());

	  if(pABox->m_setNewIndividuals.size() > 0 )
	    m_setNewIndividuals.insert(pABox->m_setNewIndividuals.begin(), pABox->m_setNewIndividuals.end());
	}
    }
  else
    m_pCompletionQueue = new CompletionQueue;

  if( pExtraIndividual )
    {
      Individual* pN = new Individual(pExtraIndividual, this, NODE_BLOCKABLE);

      pN->m_bIsConceptRoot = TRUE;
      pN->m_iBranch = DependencySet::NO_BRANCH;
      pN->addType(EXPRNODE_TOP, &DEPENDENCYSET_INDEPENDENT);
      m_mNodes[pExtraIndividual] = pN;
      m_aNodeList.push_back(pExtraIndividual);

      if( PARAMS_COPY_ON_WRITE() )
	m_pSourceABox = pABox;
    }

  if( bCopyIndividuals )
    {
      m_listToBeMerged = pABox->m_listToBeMerged;

      if( m_pSourceABox == NULL )
	{
	  for(ExprNodes::iterator i = pABox->m_aNodeList.begin(); i != pABox->m_aNodeList.end(); i++ )
	    {
	      ExprNode* pX = (ExprNode*)*i;
	      Node* pNode = pABox->getNode(pX);
	      Node* pCopy = pNode->copyTo(this);

	      m_mNodes[pX] = pCopy;
	      m_aNodeList.push_back(pX);
	    }
	  for(Expr2NodeMap::iterator i = m_mNodes.begin(); i != m_mNodes.end(); i++ )
	    {
	      Node* pNode = (Node*)i->second;
	      pNode->updateNodeReferences();
	    }
	}
    }
  else
    {
      m_pSourceABox = NULL;
    }

  m_iCurrentBranchIndex = pABox->m_iCurrentBranchIndex;
  for(BranchList::iterator i = pABox->m_aBranches.begin(); i != pABox->m_aBranches.end(); i++ )
    {
      Branch* pBranch = (Branch*)*i;
      Branch* pCopy = NULL;
      if( m_pSourceABox == NULL )
	{
	  pCopy = pBranch->copyTo(this);
	  pCopy->m_iNodeCount = pBranch->m_iNodeCount + ((pExtraIndividual)?1:0);
	}
      else
	pCopy = pBranch;
      m_aBranches.push_back(pCopy);
    }
}

ABox::~ABox()
{

}

Literal* ABox::addLiteral()
{
  return createLiteral(createExprNode(EXPR_LITERAL, createUniqueName(FALSE)), &DEPENDENCYSET_INDEPENDENT);
}

Literal* ABox::addLiteral(ExprNode* pDataValue, DependencySet* pDS)
{
  if( pDS == NULL )
    pDS = &DEPENDENCYSET_INDEPENDENT;

  if( pDataValue == NULL || isLiteral(pDataValue) )
    assertFALSE("Invalid value to create a literal. Value: ");

  return createLiteral(pDataValue, pDS);
}

Literal* ABox::createLiteral(ExprNode* pDataValue, DependencySet* pDS)
{
  assert(FALSE);
  return NULL;
}

Individual* ABox::addIndividual(ExprNode* pX)
{
  START_DECOMMENT2("ABox::addIndividual");
  printExprNodeWComment("pX=", pX);

  Individual* pIndividual = addIndividual(pX, NODE_NOMINAL);
  
  if( !PARAMS_USE_PSEUDO_NOMINALS() )
    {
      // add value(x) for nominal node but do not apply UC yet
      // because it might not be complete. it will be added
      // by CompletionStrategy.initialize()
      ExprNode* pValueX = createExprNode(EXPR_VALUE, pX);
      pIndividual->addType(pValueX, &DEPENDENCYSET_INDEPENDENT);
    }

  // update affected inds for this branch
  if( m_iCurrentBranchIndex >= 0 && PARAMS_USE_COMPLETION_QUEUE() )
    m_pCompletionQueue->addEffected(m_iCurrentBranchIndex, pIndividual->m_pName);
  
  END_DECOMMENT("ABox::addIndividual");
  return pIndividual;
}

Individual* ABox::addFreshIndividual(bool bIsNominal)
{
  START_DECOMMENT2("ABox::addFreshIndividual");
  ExprNode* pName = createUniqueName(bIsNominal);
  Individual* pInd = addIndividual(pName, NODE_BLOCKABLE);

  if( bIsNominal )
    pInd->m_iNominalLevel = 1;
  END_DECOMMENT("ABox::addFreshIndividual");
  return pInd;
}

ExprNode* ABox::createUniqueName(bool bIsNominal)
{
  ExprNode* pName = getNewExprNode();
  sprintf(pName->m_cTerm, "anon%d", ++m_iAnonCount);
  if( bIsNominal )
    pName->m_iExpression = EXPR_ANON_NOMINAL;
  else
    pName->m_iExpression = EXPR_TERM;
  return pName;
}

Individual* ABox::addIndividual(ExprNode* pIndividual, int iNominalLevel)
{
  START_DECOMMENT2("ABox::addIndividual");
  printExprNodeWComment("pIndividual=", pIndividual);

  if( getNode(pIndividual) )
    assertFALSE("adding a node twice ");

  m_bChanged = TRUE;

  // if can use inc strategy, then set pseudomodel.changed
  if( g_pKB->canUseIncrementalConsistency() && m_pPseudoModel )
    m_pPseudoModel->m_bChanged = TRUE;
  
  Individual* pNode = new Individual(pIndividual, this, iNominalLevel);
  pNode->m_iBranch = m_iCurrentBranchIndex;
  pNode->addType(EXPRNODE_TOP, &DEPENDENCYSET_INDEPENDENT);

  m_mNodes[pIndividual] = pNode;
  m_aNodeList.push_back(pIndividual);

  if( m_iCurrentBranchIndex > 0 && PARAMS_USE_COMPLETION_QUEUE() )
    m_pCompletionQueue->addEffected(m_iCurrentBranchIndex, pNode->m_pName);  

  END_DECOMMENT("ABox::addIndividual");
  return pNode;
}

void ABox::addEffected(int iBranchIndex, ExprNode* pExprNode)
{
  m_pCompletionQueue->addEffected(iBranchIndex, pExprNode);
}

ABox* ABox::getPseudoModel()
{
  return m_pPseudoModel;
}

Node* ABox::getNode(ExprNode* pNode)
{
  Expr2NodeMap::iterator i = m_mNodes.find(pNode);
  if( i != m_mNodes.end() )
    return (Node*)i->second;
  return NULL;
}

Individual* ABox::getIndividual(ExprNode* pNode)
{
  Expr2NodeMap::iterator i = m_mNodes.find(pNode);
  if( i != m_mNodes.end() )
    return (Individual*)i->second;
  return NULL;
}

Literal* ABox::getLiteral(ExprNode* pNode)
{
  Expr2NodeMap::iterator i = m_mNodes.find(pNode);
  if( i != m_mNodes.end() )
    return (Literal*)i->second;
  return NULL;
}

void ABox::addType(ExprNode* pX, ExprNode* pC)
{
  START_DECOMMENT2("ABox::addType");
  ExprNode* pTypeAxiom = createExprNode(EXPR_TYPE, pX, pC);
  DependencySet* pDS = (PARAMS_USE_TRACING())?(new DependencySet(pTypeAxiom)):(&DEPENDENCYSET_INDEPENDENT);
  addType(pX, pC, pDS);
  END_DECOMMENT("ABox::addType");
}

void ABox::addType(ExprNode* pX, ExprNode* pC, DependencySet* pDS)
{
  START_DECOMMENT2("ABox::addType(2)");
  pC = normalize(pC);

  // when a type is being added to a pseudo model, i.e.
  // an ABox that has already been completed, the branch
  // of the dependency set will automatically be set to
  // the current branch. We need to set it to initial
  // branch number to make sure that this type assertion
  // is being added to the initial model
  int iBranch = m_iCurrentBranchIndex;
  m_iCurrentBranchIndex = DependencySet::NO_BRANCH;

  Node* pNode = getNode(pX);
  if( pNode->isMerged() )
    {
      pDS = pNode->getMergeDependency(TRUE);
      pNode = pNode->getSame();
      if( !pDS->isIndependent() )
	m_mTypeAssertions[pX] = pC;
    }

  pNode->addType(pC, pDS);
  m_iCurrentBranchIndex = iBranch;
  END_DECOMMENT("ABox::addType(2)");
}

void ABox::removeType(ExprNode* pX, ExprNode* pC)
{
  ExprNode* pNormalized = normalize(pC);
  Node* pNode = getNode(pX);
  pNode->removeType(pNormalized);
}

void ABox::addDifferent(ExprNode* pX, ExprNode* pY)
{
  START_DECOMMENT2("ABox::addDifferent");
  Individual* pInd1 = getIndividual(pX);
  Individual* pInd2 = getIndividual(pY);
  
  ExprNode* pDiffAxiom = createExprNode(EXPR_DIFF, pX, pY);

  // update syntactic assertions - currently i do not add this to the
  // dependency index
  // now, as it will simply be used during the completion strategy
  if( PARAMS_USE_INCREMENTAL_DELETION() )
    g_pKB->m_setSyntacticAssertions.insert(pDiffAxiom);
 
  DependencySet* pDS = &DEPENDENCYSET_INDEPENDENT;
  if( PARAMS_USE_TRACING() )
    pDS = new DependencySet(pDiffAxiom);
  pInd1->setDifferent(pInd2, pDS);
  END_DECOMMENT("ABox::addDifferent");
}

void ABox::setClash(Clash* pClash)
{
  START_DECOMMENT2("ABox::setClash");
  if( pClash && PARAMS_PRINT_DEBUGINFO_INHTML() )
    pClash->printClashInfo();

  if( pClash && m_pClash )
    {
      if( m_pClash->m_pDepends && 
	  pClash->m_pDepends &&
	  m_pClash->m_pDepends->getMax() < pClash->m_pDepends->getMax() )
	{
	  END_DECOMMENT("ABox::setClash");
	  return;
	}
    }

  if( m_pClash )
    delete m_pClash;
  m_pClash = pClash;
 
  if( PARAMS_USE_INCREMENTAL_DELETION() )
    g_pKB->m_pDependencyIndex->setClashDependencies(m_pClash);

  END_DECOMMENT("ABox::setClash");
}

void ABox::cache(Individual* pRootNode, ExprNode* pC, bool bIsConsistent)
{
  START_DECOMMENT2("ABox::cache");
  DECOMMENT1(": isConsistent=%d\n", bIsConsistent);
  printExprNodeWComment("pRootNode=", pRootNode->m_pName);
  printExprNodeWComment("pC=", pC);

  if( !bIsConsistent )
    {
      m_pCache->putSat(pC, FALSE);
    }
  else
    {
      // if a merge occurred due to one or more non-deterministic
      // branches then what we are caching depends on this set
      DependencySet* pDS = pRootNode->getMergeDependency(TRUE);
      // if it is merged, get the representative node
      pRootNode = (Individual*)pRootNode->getSame();

      // collect all transitive property values
      if( g_pKB->getExpressivity()->hasNominal() )
	pRootNode->m_pABox->collectComplexPropertyValues(pRootNode);
      
      // create a copy of the individual (copying the edges
      // but not the neighbors)
      pRootNode = (Individual*)pRootNode->copy();

      m_pCache->put(pC, CachedNode::createNode(pRootNode, pDS));
    }
  END_DECOMMENT("ABox::cache");
}

/**
 * Clear the pseudo model created for the ABox and concept satisfiability.
 */
void ABox::clearCaches(bool clearSatCache)
{
  if( m_pPseudoModel )
    {
      delete m_pPseudoModel;
      m_pPseudoModel = NULL;
    }

  if( m_pLastCompletion )
    {
      delete m_pLastCompletion;
      m_pLastCompletion = NULL;
    }

  if( clearSatCache )
    {
      if( m_pCache )
	delete m_pCache;
      m_pCache = new ConceptCache;
    }
}

/**
 * Return true if this ABox is consistent. Consistent ABox means after
 * applying all the tableau completion rules at least one branch with no
 * clashes was found
 */
bool ABox::isConsistent()
{
  bool bIsConsistent = FALSE;
  START_DECOMMENT2("ABox::isConsistent");

  // if we can use the incremental consistency checking approach, use it
  if( g_pKB->canUseIncrementalConsistency() )
    {
      bIsConsistent = isIncrementallyConsistent();
    }
  else
    {
      // if ABox is empty then we need to actually check the
      // satisfiability of the TOP class to make sure that 
      // universe is not empty
      ExprNode* pC = (m_mNodes.size()==0)? EXPRNODE_TOP:NULL;
      ExprNodeSet setEmpty;
      bIsConsistent = isConsistent(&setEmpty, pC, FALSE);
      if( bIsConsistent )
	{
	  if( pC )
	    printExprNodeWComment("Consistent pC=", pC);
	  // put the BOTTOM concept into the cache which will
	  // also put TOP in there
	  m_pCache->putSat(EXPRNODE_BOTTOM, FALSE);
	}
    }

  // reset the updated indviduals
  m_setUpdatedIndividuals.clear();
  m_setNewIndividuals.clear();

  DECOMMENT1("RETURN bIsConsistent=%d\n", bIsConsistent);
  END_DECOMMENT("ABox::isConsistent");

  return bIsConsistent;
}

/**
 * Check the consistency of this ABox possibly after adding some type
 * assertions. If <code>c</code> is null then nothing is added to ABox
 * (pure consistency test) and the individuals should be an empty
 * collection. If <code>c</code> is not null but <code>individuals</code>
 * is empty, this is a satisfiability check for concept <code>c</code> so
 * a new individual will be added with type <code>c</code>. If
 * individuals is not empty, this means we will add type <code>c</code> to
 * each of the individuals in the collection and check the consistency.
 *
 * The consistency checks will be done either on a copy of the ABox or its
 * pseudo model depending on the situation. In either case this ABox will
 * not be modified at all. After the consistency check lastCompletion points
 * to the modified ABox.
 */
bool ABox::isConsistent(ExprNodeSet* pIndividuals, ExprNode* pC, bool bCacheModel)
{
  START_DECOMMENT2("ABox::isConsistent(3)");
  if( pC )
    printExprNodeWComment("pC=", pC);
  else
    {
      DECOMMENT("pC=NULL");
    }

  Expressivity* pExpressivity = g_pKB->getExpressivity()->compute(pC);

  // if c is null we are checking the consistency of this ABox as
  // it is and we will not add anything extra
  bool bBuildPseudoModel = (pC==NULL);
  DECOMMENT1(": bBuildPseudoModel=%d\n", bBuildPseudoModel);

  // if individuals is empty and we are not building the pseudo
  // model then this is concept satisfiability
  bool bConceptSatisfiability = (!bBuildPseudoModel && (pIndividuals==NULL||pIndividuals->size()==0));
  DECOMMENT1(": bConceptSatisfiability=%d\n", bConceptSatisfiability);

  // Check if there are any nominals in the KB or nominal
  // reasoning is disabled
  bool bHasNominal = (pExpressivity->hasNominal() && !PARAMS_USE_PSEUDO_NOMINALS());
  DECOMMENT1(": bHasNominal=%d\n", bHasNominal);
  
  // Use empty model only if this is concept satisfiability for a KB
  // where there are no nominals (for Econn never use empty ABox)
  bool bCanUseEmptyModel = bConceptSatisfiability && !bHasNominal;
  DECOMMENT1(": bCanUseEmptyModel=%d\n", bCanUseEmptyModel);

  // Use pseudo model only if we are not building the pseudo model, pseudo model
  // option is enabled and the strategy used to build the pseduo model actually
  // supports pseudo model completion
  bool bCanUsePseudoModel = !bBuildPseudoModel && m_pPseudoModel && PARAMS_USE_PSEUDO_MODEL() && g_pKB->chooseStrategy(this)->supportsPseudoModelCompletion();
  DECOMMENT1(": bCanUsePseudoModel=%d\n", bCanUsePseudoModel);

  ExprNode* pX = NULL;
  if( bConceptSatisfiability )
    {
      pX = EXPRNODE_CONCEPT_SAT_IND;
      pIndividuals->clear();
      pIndividuals->insert(pX);
    }

  ABox* pABox = bCanUseEmptyModel?copy(pX, FALSE):(bCanUsePseudoModel?m_pPseudoModel->copy(pX, TRUE):copy(pX, TRUE));

  // Due to abox deletions, it could be the case that this is an abox
  // consistency check, the kb was previously inconsistent,
  // and there was a removal which invalidated the clash. If the clash is
  // independent, then we need to retract the clash
  if( pABox->getClash() && bBuildPseudoModel && g_pKB->m_bABoxDeletion )
    {
      if( pABox->getClash()->m_pDepends->isIndependent() )
	pABox->setClash(NULL);
    }

  for(ExprNodeSet::iterator i = pIndividuals->begin(); i != pIndividuals->end(); i++ )
    {
      ExprNode* pInd = (ExprNode*)*i;
      printExprNodeWComment("Individual=", pInd);

      pABox->setSyntacticUpdate();
      pABox->addType(pInd, pC);
      pABox->setSyntacticUpdate(FALSE);
    }

  ABox* pCompletion = NULL;
  if( pABox->isEmpty() )
    {
      pCompletion = pABox;
    }
  else
    {
      CompletionStrategy* pStrategy = g_pKB->chooseStrategy(pABox, pExpressivity);
      //pStrategy->printStrategyType();
      pCompletion = pStrategy->complete();
    }

  bool bConsistent = !pCompletion->isClosed();
  if( bBuildPseudoModel )
    m_pPseudoModel = pCompletion;
  
  if( pX && pC && bCacheModel )
    cache(pCompletion->getIndividual(pX), pC, bConsistent);

  if( !bConsistent )
    {
      m_pLastClash = pCompletion->getClash();
      if( m_bDoExplanation && PARAMS_USE_TRACING() )
	{
	  if( pIndividuals->size() == 1 )
	    {
	      ExprNode* pInd = (ExprNode*)(*pIndividuals->begin());
	      ExprNode* pTermAxiom = createExprNode(EXPR_TYPE, pInd, pC);
	      m_pLastClash->m_pDepends->m_setExplain.erase(pTermAxiom);
	    }
	}
    }

  m_iConsistencyCount++;
  if( m_bKeepLastCompletion )
    m_pLastCompletion = pCompletion;
  else
    m_pLastCompletion = NULL;

  DECOMMENT1("RETURN bConsistent=%d\n", bConsistent);
  END_DECOMMENT("ABox::isConsistent(3)");
  return bConsistent;
}

/**
 * Check the consistency of this ABox using the incremental consistency
 * checking approach
 */
bool ABox::isIncrementallyConsistent()
{
  // throw away old information to let gc do its work
  m_pLastCompletion = NULL;

  // as this is an inc. consistency check, simply use the pseudomodel
  ABox* pABox = m_pPseudoModel;

  // if this is a deletion then remove dependent structures, update
  // completion queue and update branches
  if( g_pKB->m_bABoxDeletion )
    g_pKB->restoreDependencies();

  // set updated individuals for incremental completion
  pABox->m_setUpdatedIndividuals = m_setUpdatedIndividuals;
  pABox->m_setNewIndividuals = m_setNewIndividuals;

  // CHW - It appears that there are cases that kb.status may be false
  // prior to an incremental deletion. I think this may pop up if the KB
  // is inconsistent
  // prior to the deletion (which certainly should be supported). In order
  // for Strategy.initialize to only perform the limited initialization
  // (ie., when the kb is initialized)
  // we explicitly set the status here.
  // TODO: this should be looked into furtherreturn FALSE;
  g_pKB->m_iKBStatus = KBSTATUS_UNCHANGED;
  pABox->setInitialized(TRUE);

  if( pABox->isEmpty() )
    m_pLastCompletion = pABox;
  else
    {
      // currently there is only one incremental consistency strategy
      CompletionStrategy* pIncStrategy = new SROIQIncStrategy(pABox);
      pABox->m_bIsComplete = FALSE;
      m_pLastCompletion = pIncStrategy->complete();
    }

  bool bConsistent = !m_pLastCompletion->isClosed();
  m_pPseudoModel = m_pLastCompletion;

  if( !bConsistent )
    m_pLastClash = m_pLastCompletion->getClash();

  m_iConsistencyCount++;

  if( !m_bKeepLastCompletion )
    m_pLastCompletion = NULL;

  return bConsistent;
}

bool ABox::isClosed()
{
  return (!PARAMS_SATURATE_TABLEAU() && m_pClash);
}

bool ABox::isInitialized()
{
  return (m_bInitialized && !g_pKB->isChanged());
}

void ABox::collectComplexPropertyValues(Individual* pSubj)
{
  RoleSet setCollected;
  for(EdgeVector::iterator i = pSubj->m_listOutEdges.m_listEdge.begin(); i != pSubj->m_listOutEdges.m_listEdge.end(); i++ )
    {
      Edge* pEdge = (Edge*)*i;
      Role* pRole = pEdge->m_pRole;
      // only collect non-simple, i.e. complex, roles
      // TODO we might not need to collect all non-simple roles
      // collecting only the base ones, i.e. minimal w.r.t. role
      // ordering, would be enough
      if( pRole->isSimple() || setCollected.find(pRole) != setCollected.end() )
	continue;
      collectComplexPropertyValues(pSubj, pRole);
    }
  
  for(EdgeVector::iterator i = pSubj->m_listInEdges.m_listEdge.begin(); i != pSubj->m_listInEdges.m_listEdge.end(); i++ )
    {
      Edge* pEdge = (Edge*)*i;
      Role* pRole = pEdge->m_pRole;
      pRole = pRole->m_pInverse;
      if( pRole->isSimple() || setCollected.find(pRole) != setCollected.end() )
	continue;
      collectComplexPropertyValues(pSubj, pRole);
    }
}

void ABox::collectComplexPropertyValues(Individual* pSubj, Role* pRole)
{
  ExprNodeSet setKnowns, setUnknowns;
  getObjectPropertyValues(pSubj->m_pName, pRole, &setKnowns, &setUnknowns, FALSE);
  for(ExprNodeSet::iterator i = setKnowns.begin(); i != setKnowns.end(); i++ )
    {
      ExprNode* pVal = (ExprNode*)*i;
      Individual* pInd = getIndividual(pVal);
      pSubj->addEdge(pRole, pInd, &DEPENDENCYSET_INDEPENDENT);
    }
  for(ExprNodeSet::iterator i = setUnknowns.begin(); i != setUnknowns.end(); i++ )
    {
      ExprNode* pVal = (ExprNode*)*i;
      Individual* pInd = getIndividual(pVal);
      pSubj->addEdge(pRole, pInd, &DEPENDENCYSET_DUMMY);
    }
}

void ABox::getObjectPropertyValues(ExprNode* pS, Role* pRole, ExprNodeSet* pKnowns, ExprNodeSet* pUnknowns, bool bGetSames)
{
  Individual* pSubj = NULL;
  bool bIsIndependent = TRUE;
  
  if( m_pPseudoModel )
    pSubj = m_pPseudoModel->getIndividual(pS);
  if( pSubj == NULL )
    pSubj = getIndividual(pS);

  if( pSubj->isMerged() )
    {
      bIsIndependent = pSubj->getMergeDependency(TRUE)->isIndependent();
      pSubj = (Individual*)pSubj->getSame();
    }

  if( pRole->isSimple() )
    getSimpleObjectPropertyValues(pSubj, pRole, pKnowns, pUnknowns, bGetSames);
  else if( !pRole->hasComplexSubRole() )
    {
      ExprNodeSet setVisited;
      getTransitivePropertyValues(pSubj, pRole, pKnowns, pUnknowns, bGetSames, &setVisited, TRUE);
    }
  else
    {
      TransitionGraph* pTG = pRole->m_pTG;
      IndividualStatePairSet mapVisited;
      getComplexObjectPropertyValues(pSubj, pTG->m_pInitialState, pTG, pKnowns, pUnknowns, bGetSames, &mapVisited, TRUE);
    }

  if( !bIsIndependent )
    {
      pUnknowns->insert(pKnowns->begin(), pKnowns->end());
      pKnowns->clear();
    }
}

void ABox::getSimpleObjectPropertyValues(Individual* pSubj, Role* pRole, ExprNodeSet* pKnowns, ExprNodeSet* pUnknowns, bool bGetSames)
{
  EdgeList listRNeighborEdges;
  pSubj->getRNeighborEdges(pRole, &listRNeighborEdges);
  
  for(EdgeVector::iterator i = listRNeighborEdges.m_listEdge.begin(); i != listRNeighborEdges.m_listEdge.end(); i++ )
    {
      Edge* pEdge = (Edge*)*i;
      DependencySet* pDS = pEdge->m_pDepends;
      Individual* pValue = (Individual*)pEdge->getNeighbor(pSubj);
      
      if( pValue->isRootNominal() )
	{
	  if( pDS->isIndependent() )
	    {
	      if( bGetSames )
		getSames(pValue, pKnowns, pUnknowns);
	      else
		pKnowns->insert(pValue->m_pName);
	    }
	  else
	    {
	      if( bGetSames )
		getSames(pValue, pUnknowns, pUnknowns);
	      else
		pUnknowns->insert(pValue->m_pName);
	    }
	}
    }
}

void ABox::getTransitivePropertyValues(Individual* pSubj, Role* pProp, ExprNodeSet* pKnowns, ExprNodeSet* pUnknowns, bool bGetSames, ExprNodeSet* pVisited, bool bIsIndependent)
{
  if( pVisited->find(pSubj->m_pName) != pVisited->end() )
    return;
  pVisited->insert(pSubj->m_pName);

  EdgeList listRNeighborEdges;
  pSubj->getRNeighborEdges(pProp, &listRNeighborEdges);
  
  for(EdgeVector::iterator i = listRNeighborEdges.m_listEdge.begin(); i != listRNeighborEdges.m_listEdge.end(); i++ )
    {
      Edge* pEdge = (Edge*)*i;
      DependencySet* pDS = pEdge->m_pDepends;
      Individual* pValue = (Individual*)pEdge->getNeighbor(pSubj);
      Role* pEdgeRole = (isEqual(pEdge->m_pFrom,pSubj)==0)?pEdge->m_pRole:pEdge->m_pRole->m_pInverse;
      
      if( pValue->isRootNominal() )
	{
	  if( bIsIndependent && pDS->isIndependent() )
	    {
	      if( bGetSames )
		getSames(pValue, pKnowns, pUnknowns);
	      else
		pKnowns->insert(pValue->m_pName);
	    }
	  else
	    {
	      if( bGetSames )
		getSames(pValue, pUnknowns, pUnknowns);
	      else
		pUnknowns->insert(pValue->m_pName);
	    }
	}

      if( !pProp->isSimple() )
	{
	  // all the following roles might cause this property to
	  // propagate
	  RoleSet commonSubRoles;
	  intersectRoleSets(&(pEdgeRole->m_setSuperRoles), &(pProp->m_setTransitiveSubRoles), &commonSubRoles);
	  for(RoleSet::iterator r = commonSubRoles.begin(); r != commonSubRoles.end(); r++ )
	    {
	      Role* pTransRole = (Role*)*r;
	      getTransitivePropertyValues(pValue, pTransRole, pKnowns, pUnknowns, bGetSames, pVisited, (bIsIndependent&&pDS->isIndependent()));
	    }
	}
    }
}

void ABox::getComplexObjectPropertyValues(Individual* pSubj, State* pState, TransitionGraph* pTG, ExprNodeSet* pKnowns, ExprNodeSet* pUnknowns, bool bGetSames, IndividualStatePairSet* pVisited, bool bIsIndependent)
{
  IndividualStatePair* indStatePair = new IndividualStatePair;
  indStatePair->first = pSubj;
  indStatePair->second = pState;  
  if( pVisited->find(indStatePair) != pVisited->end() )
    return;
  pVisited->insert(indStatePair);

  if( pTG->isFinal(pState) && pSubj->isRootNominal() )
    {
      if( bIsIndependent )
	{
	  if( bGetSames )
	    getSames(pSubj, pKnowns, pUnknowns);
	  else
	    pKnowns->insert(pSubj->m_pName);
	}
      else
	{
	  if( bGetSames )
	    getSames(pSubj, pUnknowns, pUnknowns);
	  else
	    pUnknowns->insert(pSubj->m_pName);
	}
    }

  for(TransitionSet::iterator t = pState->m_setTransitions.begin(); t != pState->m_setTransitions.end(); t++) 
    {
      Transition* pTransition = (Transition*)*t;
      Role* pRole = (Role*)pTransition->m_pName;
      
      EdgeList listRNeighborEdges;
      pSubj->getRNeighborEdges(pRole, &listRNeighborEdges);
      
      for(EdgeVector::iterator i = listRNeighborEdges.m_listEdge.begin(); i != listRNeighborEdges.m_listEdge.end(); i++ )
	{
	  Edge* pEdge = (Edge*)*i;
	  DependencySet* pDS = pEdge->m_pDepends;
	  Individual* pValue = (Individual*)pEdge->getNeighbor(pSubj);

	  getComplexObjectPropertyValues(pValue, pTransition->m_pTo, pTG, pKnowns, pUnknowns, bGetSames, pVisited, (bIsIndependent&&pDS->isIndependent()));
	}
    }
}

void ABox::getSames(Individual* pInd, ExprNodeSet* pKnowns, ExprNodeSet* pUnknowns)
{
  pKnowns->insert(pInd->m_pName);

  bool bThisMerged = (pInd->isMerged() && !pInd->getMergeDependency(TRUE)->isIndependent());
  for(NodeSet::iterator i = pInd->m_setMerged.begin(); i != pInd->m_setMerged.end(); i++ )
    {
      Individual* pOther = (Individual*)*i;
      if( !pOther->isRootNominal() )
	continue;

      bool bOtherMerged = (pOther->isMerged() && !pOther->getMergeDependency(TRUE)->isIndependent());
      if( bThisMerged || bOtherMerged )
	{
	  pUnknowns->insert(pOther->m_pName);
	  getSames(pOther, pUnknowns, pUnknowns);
	}
      else
	{
	  pKnowns->insert(pOther->m_pName);
	  getSames(pOther, pKnowns, pUnknowns);
	}
    }
}

int ABox::mergable(Individual* pRoot1, Individual* pRoot2, bool bIndependent)
{
  START_DECOMMENT2("ABox::mergable");
  printExprNodeWComment("pRoot1=", pRoot1->m_pName);
  printExprNodeWComment("pRoot2=", pRoot2->m_pName);

  // if a concept c has a cached node rootX == topNode then it means
  // not(c) has a cached node rootY == bottomNode. Only nodes whose
  // negation is unsatisfiable has topNode in their cache
  if( isEqual(pRoot1, g_pBOTTOMIND) == 0 || isEqual(pRoot2, g_pBOTTOMIND) == 0 )
    {
      END_DECOMMENT("ABox::mergable(1)");
      return 0;
    }
  else if( isEqual(pRoot1, g_pTOPIND) == 0 && isEqual(pRoot2, g_pBOTTOMIND) != 0 )
    {
      END_DECOMMENT("ABox::mergable(2)");
      return 1;
    }
  else if( isEqual(pRoot2, g_pTOPIND) == 0 && isEqual(pRoot1, g_pBOTTOMIND) != 0 )
    {
      END_DECOMMENT("ABox::mergable(3)");
      return 1;
    }
  else if( isEqual(pRoot1, g_pDUMMYIND) == 0 || isEqual(pRoot2, g_pDUMMYIND) == 0 )
    {
      END_DECOMMENT("ABox::mergable(4)");
      return -1;
    }

  int bResult = 1;
  // first test if there is an atomic clash between the types of two roots
  // we pick the root with lower number of types an iterate through all
  // the types in its label
  Individual* pRoot, *pOtherRoot;
  if( pRoot1->m_mDepends.size() < pRoot2->m_mDepends.size() )
    {
      pRoot = pRoot1;
      pOtherRoot = pRoot2;
    }
  else
    {
      pRoot = pRoot2;
      pOtherRoot = pRoot1;
    }

  for(ExprNode2DependencySetMap::iterator i = pRoot->m_mDepends.begin(); i != pRoot->m_mDepends.end(); i++ )
    {
      ExprNode* pC = (ExprNode*)i->first;
      ExprNode* pNotC = negate2(pC);

      if( pOtherRoot->hasType(pNotC) )
	{
	  DependencySet* pDS1 = NULL, *pDS2 = NULL;
	  ExprNode2DependencySetMap::iterator iFind = pRoot->m_mDepends.find(pC);
	  if( iFind != pRoot->m_mDepends.end() )
	    pDS1 = (DependencySet*)iFind->second;
	  iFind = pOtherRoot->m_mDepends.find(pNotC);
	  if( iFind != pOtherRoot->m_mDepends.end() )
	    pDS2 = (DependencySet*)iFind->second;

	  bool bAllIndependent = bIndependent && pDS1->isIndependent() && pDS2->isIndependent();
	  if( bAllIndependent )
	    {
	      END_DECOMMENT("ABox::mergable(5)");
	      return 0;
	    }
	  else
	    {
	      bResult = -1;
	    }
	}
    }

  // if there is a suspicion there is no way to fix it later so return now
  if( bResult == -1 )
    {
      END_DECOMMENT("ABox::mergable(6)");
      return -1;
    }

  for(int i = 0; i < 2; i++ )
    {
      Individual* pRoot = (i==0)?pRoot1:pRoot2;
      Individual* pOtherRoot = (i==0)?pRoot2:pRoot1;

      for(ExprNodes::iterator j = pRoot->m_aTypes[Node::ALL].begin(); j != pRoot->m_aTypes[Node::ALL].end(); j++ )
	{
	  ExprNode* pAV = (ExprNode*)*j;
	  Role* pRole = NULL;
	  if( pAV->m_pArgs[0] == NULL && pAV->m_pArgList )
	    pRole = g_pKB->getRole(((ExprNodeList*)pAV->m_pArgList)->m_pExprNodes[0]);
	  else
	    pRole = g_pKB->getRole((ExprNode*)pAV->m_pArgs[0]);

	  if( !pRole->hasComplexSubRole() )
	    {
	      if( pOtherRoot->hasRNeighbor(pRole) )
		{
		  END_DECOMMENT("ABox::mergable(7)");
		  return -1;
		}
	    }
	  else
	    {
	      TransitionGraph* pTG = pRole->m_pTG;
	      for(TransitionSet::iterator j = pTG->m_pInitialState->m_setTransitions.begin(); j != pTG->m_pInitialState->m_setTransitions.end(); j++ )
		{
		  Transition* pTR = (Transition*)*j;
		  if( pOtherRoot->hasRNeighbor((Role*)pTR->m_pName) )
		    {
		      END_DECOMMENT("ABox::mergable(8)");
		      return -1;
		    }
		}
	    }
	}

      for(ExprNodes::iterator j = pRoot->m_aTypes[Node::MAX].begin(); j != pRoot->m_aTypes[Node::MAX].end(); j++ )
	{
	  ExprNode* pMax = (ExprNode*)*j;
	  ExprNode* pMaxCard = (ExprNode*)pMax->m_pArgs[0];
	  
	  Role* pMaxRole = g_pKB->getRole((ExprNode*)pMaxCard->m_pArgs[0]);
	  int iMax = ((ExprNode*)pMaxCard->m_pArgs[1])->m_iTerm-1;

	  EdgeList aRNeighborEdges;
	  NodeSet setNodes;
	  pRoot->getRNeighborEdges(pMaxRole, &aRNeighborEdges);
	  aRNeighborEdges.getFilteredNeighbors(pRoot, Role::getTop(pMaxRole), &setNodes);
	  int iN1 = setNodes.size();

	  setNodes.clear();
	  aRNeighborEdges.m_listEdge.clear();
	  pOtherRoot->getRNeighborEdges(pMaxRole, &aRNeighborEdges);
	  aRNeighborEdges.getFilteredNeighbors(pOtherRoot, Role::getTop(pMaxRole), &setNodes);
	  int iN2 = setNodes.size();
	  if( iN1+iN2 > iMax )
	    {
	      END_DECOMMENT("ABox::mergable(9)");
	      return -1;
	    }
	}
    }

  if( g_pKB->getExpressivity()->hasFunctionality() )
    {
      
      if( (pRoot1->m_listOutEdges.size() + pRoot1->m_listInEdges.size()) < (pRoot2->m_listOutEdges.size() + pRoot2->m_listInEdges.size()) )
	{
	  pRoot = pRoot1;
	  pOtherRoot = pRoot2;
	}
      else
	{
	  pRoot = pRoot2;
	  pOtherRoot = pRoot1;
	}

      RoleSet setChecked;
      for(EdgeVector::iterator i = pRoot->m_listOutEdges.m_listEdge.begin(); i != pRoot->m_listOutEdges.m_listEdge.end(); i++)
	{
	  Edge* pEdge = (Edge*)*i;
	  Role* pRole = pEdge->m_pRole;

	  if( !pRole->isFunctional() )
	    continue;

	  for(RoleSet::iterator r = pRole->m_setFunctionalSupers.begin(); r != pRole->m_setFunctionalSupers.end(); r++ )
	    {
	      Role* pSupRole = (Role*)*r;
	      if( setChecked.find(pSupRole) != setChecked.end() )
		continue;
	      setChecked.insert(pSupRole);
	      if( pOtherRoot->hasRNeighbor(pSupRole) )
		{
		  END_DECOMMENT("ABox::mergable(10)");
		  return -1;
		}
	    }
	}
      for(EdgeVector::iterator i = pRoot->m_listInEdges.m_listEdge.begin(); i != pRoot->m_listInEdges.m_listEdge.end(); i++)
	{
	  Edge* pEdge = (Edge*)*i;
	  Role* pRole = pEdge->m_pRole->m_pInverse;

	  if( pRole == NULL || !pRole->isFunctional() )
	    continue;

	  for(RoleSet::iterator r = pRole->m_setFunctionalSupers.begin(); r != pRole->m_setFunctionalSupers.end(); r++ )
	    {
	      Role* pSupRole = (Role*)*r;
	      if( setChecked.find(pSupRole) != setChecked.end() )
		continue;
	      setChecked.insert(pSupRole);
	      if( pOtherRoot->hasRNeighbor(pSupRole) )
		{
		  END_DECOMMENT("ABox::mergable(11)");
		  return -1;
		}
	    }
	}
    }

  if( g_pKB->getExpressivity()->hasNominal() )
    {
      bool bNom1 = pRoot1->isNamedIndividual();
      for(ExprNodes::iterator j = pRoot1->m_aTypes[Node::NOM].begin(); j != pRoot1->m_aTypes[Node::NOM].end(); j++ )
	{
	  ExprNode* pNom = (ExprNode*)*j;
	  ExprNode* pName = (ExprNode*)pNom->m_pArgs[0];
	  bNom1 = !isAnon(pName);
	  if( bNom1 ) break;
	}
      bool bNom2 = pRoot2->isNamedIndividual();
      for(ExprNodes::iterator j = pRoot2->m_aTypes[Node::NOM].begin(); j != pRoot2->m_aTypes[Node::NOM].end(); j++ )
	{
	  ExprNode* pNom = (ExprNode*)*j;
	  ExprNode* pName = (ExprNode*)pNom->m_pArgs[0];
	  bNom2 = !isAnon(pName);
	  if( bNom2 ) break;
	}

      // FIXME it should be enough to check if named individuals are
      // different or not
      if( bNom1 && bNom2 )
	{
	  END_DECOMMENT("ABox::mergable(12)");
	  return -1;
	}
    }

  // if there is no obvious clash then c1 & not(c2) is satisfiable
  // therefore c1 is NOT a subclass of c2.
  END_DECOMMENT("ABox::mergable(13)");
  return 1;
}

int ABox::isType(Individual* pNode, ExprNode* pC, bool bIsIndependent)
{
  START_DECOMMENT2("ABox::isType");
  printExprNodeWComment("Node=", pNode->m_pName);
  printExprNodeWComment("pC=", pC);

  ExprNode* pNotC = negate2(pC);
  CachedNode* pCached = getCached(pNotC);
  if( pCached && pCached->isComplete() )
    {
      printExprNodeWComment("Cached exists for = ", pCached->m_pNode->m_pName);

      bIsIndependent &= pCached->m_pDepends->isIndependent();
      int iMergable = mergable(pNode, pCached->m_pNode, bIsIndependent);
      DECOMMENT1(": iMergable=%d\n", iMergable);
      if( iMergable != -1 )
	{
	  DECOMMENT1("RETURN %d\n", (iMergable==1)?0:1);
	  END_DECOMMENT("ABox::isType(1)");
	  return (iMergable==1)?0:1;
	}
    }

  if(  PARAMS_CHECK_NOMINAL_EDGES() )
    {
      pCached = getCached(pC);
      if( pCached && pCached->m_pDepends->isIndependent() )
	{
	  Individual* pCNode = pCached->m_pNode;
	  for(EdgeVector::iterator i = pCNode->m_listOutEdges.m_listEdge.begin(); i != pCNode->m_listOutEdges.m_listEdge.end(); i++ )
	    {
	      Edge* pEdge = (Edge*)*i;
	      Role* pRole = pEdge->m_pRole;

	      if( pEdge->m_pDepends->isIndependent() )
		{
		  bool bFound = FALSE;
		  Node* pVal = pEdge->m_pTo;

		  if( !pRole->isObjectRole() )
		    bFound = pNode->hasRSuccessor(pRole);
		  else if( !pVal->isRootNominal() )
		    {
		      if( !pRole->hasComplexSubRole() )
			bFound = pNode->hasRNeighbor(pRole);
		      else
			{
			  TransitionGraph* pTG = pRole->m_pTG;
			  for(TransitionSet::iterator j = pTG->m_pInitialState->m_setTransitions.begin(); j != pTG->m_pInitialState->m_setTransitions.end(); j++ )
			    {
			      Transition* pTR = (Transition*)*j;
			      if( (bFound = pNode->hasRNeighbor((Role*)pTR->m_pName)) )
				break;
			    }
			}
		    }
		  else
		    {
		      ExprNodeSet setNeighbors;
		      if( pRole->isSimple() || pNode->isConceptRoot() )
			pNode->getRNeighborNames(pRole, &setNeighbors);
		      else
			getObjectPropertyValues(pNode->m_pName, pRole, &setNeighbors, &setNeighbors, FALSE);

		      if( setNeighbors.find(pVal->m_pName) != setNeighbors.end() )
			bFound = TRUE;
		      else
			bFound = FALSE;
		    }

		  if( !bFound )
		    {
		      DECOMMENT("RETURN 0");
		      END_DECOMMENT("ABox::isType(2)");
		      return 0;
		    }
		}
	    }

	  for(EdgeVector::iterator i = pCNode->m_listInEdges.m_listEdge.begin(); i != pCNode->m_listInEdges.m_listEdge.end(); i++ )
	    {
	      Edge* pEdge = (Edge*)*i;
	      Role* pRole = pEdge->m_pRole;
	      Node* pVal = pEdge->m_pFrom;

	      if( pEdge->m_pDepends->isIndependent() )
		{
		  bool bFound = FALSE;

		  if( !pVal->isRootNominal() )
		    {
		      if( pRole->isSimple() )
			bFound = pNode->hasRNeighbor(pRole);
		      else
			{
			  TransitionGraph* pTG = pRole->m_pTG;
			  for(TransitionSet::iterator j = pTG->m_pInitialState->m_setTransitions.begin(); j != pTG->m_pInitialState->m_setTransitions.end(); j++ )
			    {
			      Transition* pTR = (Transition*)*j;
			      if( (bFound = pNode->hasRNeighbor((Role*)pTR->m_pName)) )
				break;
			    }
			}
		    }
		  else
		    {
		      ExprNodeSet setNeighbors;
		      if( pRole->isSimple() || pNode->isConceptRoot() )
			pNode->getRNeighborNames(pRole, &setNeighbors);
		      else
			getObjectPropertyValues(pNode->m_pName, pRole, &setNeighbors, &setNeighbors, FALSE);

		      if( setNeighbors.find(pVal->m_pName) != setNeighbors.end() )
			bFound = TRUE;
		      else
			bFound = FALSE;
		    }

		  if( !bFound )
		    {
		      DECOMMENT("RETURN 0");
		      END_DECOMMENT("ABox::isType(3)");
		      return 0;
		    }
		}
	    }
	}
    }
  DECOMMENT("RETURN -1");
  END_DECOMMENT("ABox::isType(4)");
  return -1;
}

int ABox::isKnownSubClassOf(ExprNode* pC1, ExprNode* pC2)
{
  CachedNode* pCached = getCached(pC1);
  if( pCached && !m_bDoExplanation )
    {
      int iType = isType(pCached->m_pNode, pC2, pCached->m_pDepends->isIndependent());
      if( iType != -1 )
	return iType;
    }
  return -1;
}

bool ABox::isSubClassOf(ExprNode* pC1, ExprNode* pC2)
{
  START_DECOMMENT2("ABox::isSubClassOf");
  printExprNodeWComment("pC1=", pC1);
  printExprNodeWComment("pC2=", pC2);

  int iKnown = isKnownSubClassOf(pC1, pC2);
  DECOMMENT1("iKnown = %d\n", iKnown);
  if( iKnown != -1 )
    {
      DECOMMENT1("RETURN isSubClass = %d\n", (iKnown==1));
      END_DECOMMENT("ABox::isSubClassOf");
      return (iKnown==1);
    }
  
  ExprNode* pNotC2 = negate2(pC2);
  ExprNode* pC = createExprNode(EXPR_AND, pC1, pNotC2);
  bool bIsSubClass = !isSatisfiable(pC, FALSE);
  DECOMMENT1("RETURN isSubClass = %d\n", bIsSubClass);
  END_DECOMMENT("ABox::isSubClassOf");
  return bIsSubClass;
}

bool ABox::isSatisfiable(ExprNode* pC)
{
  bool bCacheModel = (PARAMS_USE_CACHING() && (isPrimitiveOrNegated(pC) || PARAMS_USE_ADVANCED_CACHING()));
  return isSatisfiable(pC, bCacheModel);
}

bool ABox::isSatisfiable(ExprNode* pC, bool bCacheModel)
{
  START_DECOMMENT2("ABox::isSatisfiable");
  printExprNodeWComment("pC=", pC);

  pC = normalize(pC);
  
  // if normalization revealed an obvious unsatisfiability, return
  // immediately
  if( isEqual(pC, EXPRNODE_BOTTOM) == 0 )
    {
      DECOMMENT("RETURN FALSE\n");
      END_DECOMMENT("ABox::isSubClassOf(1)");
      return FALSE;
    }

  if( bCacheModel )
    {
      CachedNode* pCached = getCached(pC);
      if( pCached )
	{
	  bool bSatisfiable = !pCached->isBottom();
	  bool bNeedToCacheModel = bCacheModel && !pCached->isComplete();
	  // if explanation is enabled we should actually build the
	  // tableau again to generate the clash. we don't cache the
	  // explanation up front because generating explanation is costly
	  // and we only want to do it when explicitly asked note that
	  // when the concepts is satisfiable there is no explanation to
	  // be generated so we return the result immediately
	  if( !bNeedToCacheModel && (bSatisfiable || !m_bDoExplanation) )
	    {
	      DECOMMENT1("RETURN isConsistent=%d\n", bSatisfiable);
	      END_DECOMMENT("ABox::isSubClassOf(2)");
	      return bSatisfiable;
	    }
	}
    }

  m_iSatisfiabilityCount++;

  ExprNodeSet setIndividuals;
  bool bIsConsistent = isConsistent(&setIndividuals, pC, bCacheModel);

  DECOMMENT1("RETURN isConsistent=%d\n", bIsConsistent);
  END_DECOMMENT("ABox::isSubClassOf(3)");
  return bIsConsistent;
}

CachedNode* ABox::getCached(ExprNode* pC)
{ 
  return m_pCache->getCached(pC);
}

void ABox::getObviousTypes(ExprNode* pX, ExprNodes* pTypes, ExprNodes* pNonTypes)
{
  Individual* pNode = m_pPseudoModel->getIndividual(pX);
  if( !pNode->getMergeDependency(TRUE)->isIndependent() )
    pNode = getIndividual(pX);
  else
    pNode = (Individual*)pNode->getSame();

  pNode->getObviousTypes(pTypes, pNonTypes);
}

/**
 * Returns true if individual x belongs to type c. This is a logical
 * consequence of the KB if in all possible models x belongs to C. This is
 * checked by trying to construct a model where x belongs to not(c).
 */
bool ABox::isType(ExprNode* pX, ExprNode* pC)
{
  pC = normalize(pC);
  
  if( !m_bDoExplanation )
    {
      ExprNodeSet setSubs;
      if( g_pKB->isClassified() && g_pKB->m_pTaxonomy->contains(pC) )
	{
	  g_pKB->m_pTaxonomy->getFlattenedSubSupers(pC, FALSE, FALSE, &setSubs);
	  setSubs.erase(EXPRNODE_BOTTOM);
	}

      int bType = isKnownType(pX, pC, &setSubs);
      if( bType != TaxonomyNode::NOT_MARKED )
	return (bType==1);
    }

  ExprNode* pNotC = negate2(pC);
  ExprNodeSet setInds;
  setInds.insert(pX);
  return !isConsistent(&setInds, pNotC, FALSE);
}

bool ABox::isType(ExprNodes* pList, ExprNode* pC)
{
  pC = normalize(pC);
  ExprNode* pNotC = negate2(pC);
  ExprNodeSet setList;
  for(ExprNodes::iterator i = pList->begin(); i != pList->end(); i++)
    setList.insert((ExprNode*)*i);
  return !isConsistent(&setList, pNotC, FALSE);
}

int ABox::isKnownType(ExprNode* pX, ExprNode* pC, ExprNodeSet* pSubs)
{
  Individual* pNode = m_pPseudoModel->getIndividual(pX);
  bool bIsIndependent = TRUE;
  if( pNode->isMerged() )
    {
      bIsIndependent = pNode->getMergeDependency(TRUE)->isIndependent();
      pNode = (Individual*)pNode->getSame();
    }

  int iIsType = isKnownType(pNode, pC, pSubs);
  if( bIsIndependent )
    return iIsType;
  else if( iIsType == 1 )
    return -1;
  return iIsType;
}

int ABox::isKnownType(Individual* pNode, ExprNode* pConcept, ExprNodeSet* pSubs)
{
  int iIsType = isType(pNode, pConcept, TRUE);
  if( iIsType == -1 )
    {
      ExprNodeSet set;
      if( pConcept->m_iExpression == EXPR_AND )
	{
	  for(int i = 0; i < ((ExprNodeList*)pConcept->m_pArgList)->m_iUsedSize; i++ )
	    set.insert( ((ExprNodeList*)pConcept->m_pArgList)->m_pExprNodes[i] );
	}
      else
	set.insert(pConcept);

      for(ExprNodeSet::iterator i = set.begin(); i != set.end(); i++ )
	{
	  ExprNode* pC = (ExprNode*)*i;
	  if( pNode->m_pABox && (pNode->hasObviousType(pC) || pNode->hasObviousType(pSubs)) )
	    iIsType = 1;
	  else
	    {
	      iIsType = -1;
	      ExprNodeSet setAxioms;
	      g_pKB->m_pTBox->getAxioms(pC, &setAxioms);

	      for(ExprNodeSet::iterator j = setAxioms.begin(); j != setAxioms.end(); j++)
		{
		  ExprNode* pAxiom = (ExprNode*)*j;
		  ExprNode* pTerm = (ExprNode*)pAxiom->m_pArgs[1];

		  if( pAxiom->m_iExpression == EXPR_EQCLASSES )
		    {
		      int iKnownType = 1;
		      if( pTerm->m_iExpression == EXPR_AND )
			{
			  for(int k = 0; k < ((ExprNodeList*)pTerm->m_pArgList)->m_iUsedSize; k++ )
			    {
			      ExprNodeSet setSubs;
			      iKnownType = isKnownType(pNode, ((ExprNodeList*)pTerm->m_pArgList)->m_pExprNodes[k], &setSubs);
			      if( iKnownType != 1 )
				break;
			    }
			}
		      else
			{
			  ExprNodeSet setSubs;
			  iKnownType = isKnownType(pNode, pTerm, &setSubs);
			}

		      if( iKnownType == 1 )
			{
			  iIsType = 1;
			  break;
			}
		    }
		}
	      if( iIsType == -1 )
		return -1;
	    }
	}
    }

  return iIsType;
}

void ABox::copyOnWrite()
{
  if( m_pSourceABox == NULL )
    return;
  
  ExprNodes aNodeList;
  int iCurrentSize = m_aNodeList.size();
  int iNodeCount = m_pSourceABox->m_mNodes.size();
  
  m_aNodeList.clear();
  m_aNodeList.push_back(aNodeList.front());

  for(ExprNodes::iterator i = m_pSourceABox->m_aNodeList.begin(); i != m_pSourceABox->m_aNodeList.end(); i++ )
    {
      ExprNode* pX = (ExprNode*)*i;
      Node* pNode = m_pSourceABox->getNode(pX);
      Node* pCopyNode = pNode->copyTo(this);
      m_mNodes[pX] = pCopyNode;
      m_aNodeList.push_back(pX);
    }

  if( iCurrentSize > 1 )
    {
      int iIndex = 0;
      for(ExprNodes::iterator i = aNodeList.begin(); i != aNodeList.end(); i++ )
	{
	  if( iIndex >= 1 && iIndex < iCurrentSize )
	    m_aNodeList.push_back((ExprNode*)*i);
	  iIndex++;
	}
    }

  for(Expr2NodeMap::iterator i = m_mNodes.begin(); i != m_mNodes.end(); i++ )
    {
      Node* pNode = (Node*)i->second;
      if( m_pSourceABox->m_mNodes.find(pNode->m_pName) != m_pSourceABox->m_mNodes.end() )
	pNode->updateNodeReferences();
    }

  for(int i = 0, n = m_aBranches.size(); i < n; i++ )
    {
      Branch* pBranch = (Branch*)m_aBranches.at(i);
      Branch* pCopyBranch = pBranch->copyTo(this);
      m_aBranches[i] = pCopyBranch;
      
      if( i >= m_pSourceABox->m_aBranches.size() )
	pCopyBranch->m_iNodeCount += iNodeCount;
      else
	pCopyBranch->m_iNodeCount += 1;
    }

  m_pSourceABox = NULL;
}

/**
 * Validate all the edges in the ABox nodes. Used to find bugs in the copy
 * and detach/attach functions.
 */
void ABox::validate()
{
  if( !PARAMS_VALIDATE_ABOX() )
    return;

  for(Expr2NodeMap::iterator i = m_mNodes.begin(); i != m_mNodes.end(); i++ )
    {
      Individual* pNode = (Individual*)i->second;
      if( pNode->isPruned() )
	continue;
      validate(pNode);
    }
}

void ABox::validate(Individual* pNode)
{
  ints aNegatedTypes;
  aNegatedTypes.push_back(Node::ATOM);
  aNegatedTypes.push_back(Node::SOME);
  aNegatedTypes.push_back(Node::OR);
  aNegatedTypes.push_back(Node::MAX);
  
  for(int i = 0; i < aNegatedTypes.size(); i++ )
    {
      for(ExprNodes::iterator j = pNode->m_aTypes[i].begin(); j !=  pNode->m_aTypes[i].end(); j++ )
	{
	  ExprNode* pA = (ExprNode*)*j;
	  if( pA->m_iArity == 0 )
	    continue;
	  ExprNode* pNotA = (ExprNode*)pA->m_pArgs[0];
	  if( pNode->hasType(pNotA) )
	    {
	      if( !pNode->hasType(pA) )
		assertFALSE("Invalid type found: ");
	      assertFALSE("Clash found: ");
	    }
	}
    }

  if( !pNode->isRoot() )
    {
      NodeSet setAncestors;
      pNode->getPredecessors(&setAncestors);
      if( setAncestors.size() != 1 )
	assertFALSE("Invalid blockable node: ");
    }
  else if( pNode->isNominal() )
    {
      ExprNode* pNominal = createExprNode(EXPR_VALUE, pNode->m_pName);
      if( !isAnonNominal(pNode->m_pName) && !pNode->hasType(pNominal) )
	assertFALSE("Invalid nominal node: ");
    }

  for(ExprNode2DependencySetMap::iterator i = pNode->m_mDepends.begin(); i != pNode->m_mDepends.end(); i++ )
    {
      ExprNode* pC = (ExprNode*)i->first;
      DependencySet* pDS = (DependencySet*)i->second;
      if( pDS->getMax() > m_iCurrentBranchIndex )
	assertFALSE("Invalid DependencySet found: ");
    }
  
  for(Node2DependencySetMap::iterator i = pNode->m_mDifferents.begin(); i != pNode->m_mDifferents.end(); i++ )
    {
      Node* pInd = (Node*)i->first;
      DependencySet* pDS = pNode->getDifferenceDependency(pInd);
      if( pDS->getMax() > m_iCurrentBranchIndex )
	assertFALSE("Invalid DependencySet found: ");
      if( pInd->getDifferenceDependency(pNode) == NULL )
	assertFALSE("Invalid Difference: ");
    }

  for(EdgeVector::iterator i = pNode->m_listOutEdges.m_listEdge.begin(); i != pNode->m_listOutEdges.m_listEdge.end(); i++ )
    {
      Edge* pEdge = (Edge*)*i;
      Node* pSucc = pEdge->m_pTo;

      Node* pN = NULL;
      Expr2NodeMap::iterator iFind = m_mNodes.find(pSucc->m_pName);
      if( iFind != m_mNodes.end() )
	pN = (Node*)iFind->second;
      if( pN == NULL || isEqual(pN, pSucc) != 0 )
	{
	  printf("Invalid edge(%x) to a non-existing node: FROM=", pEdge);
	  printExprNode(pNode->m_pName, TRUE);
	  printf(" TO=");
	  printExprNode(pSucc->m_pName, TRUE);
	  assert(FALSE);
	}

      if( !pSucc->m_listInEdges.hasEdge(pEdge) )
	assertFALSE("Invalid edge: ");
      if( pSucc->isMerged() )
	{
	  printExprNodeWComment("RemovedNode=", pSucc->m_pName);
	  printExprNodeWComment("EdgeFrom=", pNode->m_pName);
	  assertFALSE("Invalid edge to a removed node: ");
	}

      DependencySet* pDS = pEdge->m_pDepends;
      if( pDS->getMax() > m_iCurrentBranchIndex || pDS->m_iBranch > m_iCurrentBranchIndex )
	assertFALSE("Invalid DependencySet found: ");
      
      EdgeList setEdgesTo;
      pNode->m_listOutEdges.getEdgesTo(pSucc, &setEdgesTo);
      RoleSet setRoles;
      setEdgesTo.getRoles(&setRoles);
      if( setRoles.size() != setEdgesTo.size() )
	assertFALSE("Duplicate edges: ");
    }

  for(EdgeVector::iterator i = pNode->m_listInEdges.m_listEdge.begin(); i != pNode->m_listInEdges.m_listEdge.end(); i++ )
    {
      Edge* pEdge = (Edge*)*i;
      DependencySet* pDS = pEdge->m_pDepends;
      if( pDS->getMax() > m_iCurrentBranchIndex || pDS->m_iBranch > m_iCurrentBranchIndex )
	assertFALSE("Invalid DependencySet found: ");
    }
}

void ABox::incrementBranch()
{
  START_DECOMMENT2("ABox::incrementBranch");

  if( PARAMS_USE_COMPLETION_QUEUE() )
    m_pCompletionQueue->incrementBranch(m_iCurrentBranchIndex);
  m_iCurrentBranchIndex++;

  DECOMMENT1("NewBranchIndex=%d", m_iCurrentBranchIndex);
  END_DECOMMENT("ABox::incrementBranch");
}

int ABox::getCachedSat(ExprNode* pC)
{
  return m_pCache->getSat(pC);
}

