#include "TaxonomyBuilder.h"
#include "KnowledgeBase.h"
#include "TBox.h"
#include "RBox.h"
#include "ABox.h"
#include "Expressivity.h"
#include "Taxonomy.h"
#include "TaxonomyNode.h"
#include "Params.h"
#include "ReazienerUtils.h"

extern ExprNode* EXPRNODE_TOP;
extern ExprNode* EXPRNODE_BOTTOM;
extern int g_iCommentIndent;

TaxonomyBuilder::TaxonomyBuilder()
{
  m_pKB = NULL;
  m_pTaxonomy = NULL;
  m_pSetClasses = NULL;
  m_bUseCD = FALSE;
}

// Classify the KB.
Taxonomy* TaxonomyBuilder::classify()
{
  START_DECOMMENT2("TaxonomyBuilder::classify");

  m_pSetClasses = m_pKB->getClasses();

  if( m_pSetClasses == NULL || m_pSetClasses->size() == 0 )
    {
      m_pTaxonomy = new Taxonomy();
      END_DECOMMENT("TaxonomyBuilder::classify");
      return m_pTaxonomy;
    }

  prepare();
  ExprNodes aPhaseOne, aPhaseTwo;

  if( m_bUseCD )
    {
      for(ExprNodes::iterator i = m_aOrderedClasses.begin(); i != m_aOrderedClasses.end(); i++ )
	{
	  ExprNode* pC = (ExprNode*)*i;
	  ExprNode2IntMap::iterator iFlag = m_mConceptFlags.find(pC);
	  if( iFlag != m_mConceptFlags.end() )
	    {
	      int iConceptFlag = (int)iFlag->second;
	      if( iConceptFlag == FLAG_COMPLETELY_DEFINED ||
		  iConceptFlag == FLAG_PRIMITIVE ||
		  iConceptFlag == FLAG_OTHER )
		{
		  aPhaseOne.push_back(pC);
		}
	      else
		{
		  aPhaseTwo.push_back(pC);
		}
	    }
	}
    }
  else
    {
      for(ExprNodes::iterator i = m_aOrderedClasses.begin(); i != m_aOrderedClasses.end(); i++ )
	{
	  ExprNode* pC = (ExprNode*)*i;
	  aPhaseTwo.push_back(pC);
	}
    }

  for(ExprNodes::iterator i = aPhaseOne.begin(); i != aPhaseOne.end(); i++ )
    {
      ExprNode* pC = (ExprNode*)*i;
      classify(pC, /* requireTopSearch = */FALSE);
    }
  aPhaseOne.clear();

  for(ExprNodes::iterator i = aPhaseTwo.begin(); i != aPhaseTwo.end(); i++ )
    {
      ExprNode* pC = (ExprNode*)*i;
      classify(pC, /* requireTopSearch = */TRUE);
    }
  aPhaseTwo.clear();

  m_pTaxonomy->assertValid();
  END_DECOMMENT("TaxonomyBuilder::classify");
  return m_pTaxonomy;
}

TaxonomyNode* TaxonomyBuilder::classify(ExprNode* pC, bool bRequireTopSearch/*=TRUE*/)
{
  START_DECOMMENT2("TaxonomyBuilder::classify(2)");
  printExprNodeWComment("pC=", pC);

  bool bSkipTopSearch = FALSE;
  bool bSkipBottomSearch = FALSE;

  TaxonomyNode* pNode = m_pTaxonomy->getNode(pC);
  if( pNode )
    {
      END_DECOMMENT("TaxonomyBuilder::classify(2)");
      return pNode;
    }
  pNode = checkSatisfiability(pC);
  if( pNode )
    {
      END_DECOMMENT("TaxonomyBuilder::classify(2)");
      return pNode;
    }

  clearMarks();

  TaxonomyNodes aSupers, aSubs;

  // FIXME: There may be a better thing to do here...
  int iConceptFlag = FLAG_OTHER;
  ExprNode2IntMap::iterator iFlag = m_mConceptFlags.find(pC);
  if( iFlag != m_mConceptFlags.end() )
    iConceptFlag = (int)iFlag->second;

  bSkipTopSearch = (!bRequireTopSearch && m_bUseCD && (iConceptFlag == FLAG_COMPLETELY_DEFINED));
  if( bSkipTopSearch )
    {
      getCDSupers(pC, &aSupers);
      bSkipBottomSearch = TRUE;
    }
  else
    {
      doTopSearch(pC, &aSupers);
      bSkipBottomSearch = m_bUseCD && ((iConceptFlag == FLAG_PRIMITIVE) && (iConceptFlag == FLAG_COMPLETELY_DEFINED));
    }

  if( bSkipBottomSearch )
    aSubs.push_back(m_pTaxonomy->getBottom());
  else
    {
      if( aSupers.size() == 1 )
	{
	  /*
	   * if i(pC) has only one super class j(pSup) and j is a subclass of i then
	   * it means i = j. There is no need to classify i since we
	   * already know everything about j
	   */
	  TaxonomyNode* pSupNode = ((TaxonomyNode*)(*aSupers.begin()));
	  ExprNode* pSup = pSupNode->m_pName;

	  DECOMMENT("CHECKING SUBSUMES");
	  printExprNodeWComment("pC=", pC);
	  printExprNodeWComment("pSup=", pSup);

	  // pC subsumes pSup
	  if( subsumes(pC, pSup) )
	    {
	      m_pTaxonomy->addEquivalentNode(pC, pSupNode);
	      DECOMMENT("SUBSUMED\n");
	      END_DECOMMENT("TaxonomyBuilder::classify(2)");
	      return pSupNode;
	    }
	}

      doBottomSearch(pC, &aSupers, &aSubs);
    }

  pNode = m_pTaxonomy->addNode(pC);

  DECOMMENT1("DETECTED SUPERS size=%d\n", aSupers.size());
  DECOMMENT1("DETECTED SUBS size=%d\n", aSubs.size());
  pNode->addSupers(&aSupers);
  pNode->addSubs(&aSubs);
  pNode->removeMultiplePaths();

  /*
   * For told relations maintain explanations.
   */
  TaxonomyNode* pToldNode = m_pToldTaxonomy->getNode(pC);
  if( pToldNode )
    {
      // Add the told equivalents to the taxonomy
      TaxonomyNode* pDefOrder = pToldNode;
      for(ExprNodeSet::iterator i = pDefOrder->m_setEquivalents.begin(); i != pDefOrder->m_setEquivalents.end(); i++ )
	{
	  ExprNode* pEq = (ExprNode*)*i;
	  m_pTaxonomy->addEquivalentNode(pEq, pNode);
	}
      
      for(TaxonomyNodes::iterator i = aSupers.begin(); i != aSupers.end(); i++ )
	{
	  TaxonomyNode* pN = (TaxonomyNode*)*i;
	  TaxonomyNode* pSupTold = m_pToldTaxonomy->getNode(pN->m_pName);

	  SetOfExprNodeSet* pExps = pToldNode->getSuperExplanations(pSupTold);
	  if( pExps )
	    {
	      for(SetOfExprNodeSet::iterator j = pExps->begin(); j != pExps->end(); j++ )
		{
		  ExprNodeSet* pExp = (ExprNodeSet*)*j;
		  pNode->addSuperExplanation(pN, pExp);
		}
	    }
	}
    }

  END_DECOMMENT("TaxonomyBuilder::classify(2)");
  return pNode;
}

// Find all of told subsumers already classified and not redundant
void TaxonomyBuilder::getCDSupers(ExprNode* pC, TaxonomyNodes* paSupers)
{
  // FIXME: Handle or rule out the null case
  TaxonomyNode* pDefOrder = m_pToldTaxonomy->getNode(pC);
  if( pDefOrder->m_aSupers.size() > 1 )
    {
      if( pDefOrder->m_aSupers.size() == 2 )
	{
	  for(TaxonomyNodes::iterator i = pDefOrder->m_aSupers.begin(); i != pDefOrder->m_aSupers.end(); i++ )
	    {
	      TaxonomyNode* pDef = (TaxonomyNode*)*i;
	      if( pDef == m_pToldTaxonomy->getTop())
		continue;
	      TaxonomyNode* pParent = m_pTaxonomy->getNode(pDef->m_pName);
	      if( pParent == NULL )
		{
		  printf("CD classification of getName(c) failed, told subsumer not classified = ");
		  printExprNode(pDef->m_pName);
		  assert(FALSE);
		}
	      paSupers->push_back(pParent);
	      break;
	    }
	}
      else
	{
	  for(TaxonomyNodes::iterator i = pDefOrder->m_aSupers.begin(); i != pDefOrder->m_aSupers.end(); i++ )
	    {
	      TaxonomyNode* pDef = (TaxonomyNode*)*i;
	      if( pDef == m_pToldTaxonomy->getTop())
		continue;
	      TaxonomyNode* pCandidate = m_pTaxonomy->getNode(pDef->m_pName);
	      if( pCandidate == NULL )
		assertFALSE("CD classification of getName(c) failed, told subsumer getNode(pDef->m_pName) not classified");
	      for(TaxonomyNodes::iterator j = pCandidate->m_aSupers.begin(); j != pCandidate->m_aSupers.end(); j++ )
		{
		  TaxonomyNode* pAncestor = (TaxonomyNode*)*j;
		  mark(pAncestor, TaxonomyNode::MARKED_TRUE, PROPAGATE_UP);
		}
	    }

	  for(TaxonomyNodes::iterator i = pDefOrder->m_aSupers.begin(); i != pDefOrder->m_aSupers.end(); i++ )
	    {
	      TaxonomyNode* pDef = (TaxonomyNode*)*i;
	      if( pDef == m_pToldTaxonomy->getTop())
		continue;
	      TaxonomyNode* pCandidate = m_pTaxonomy->getNode(pDef->m_pName);
	      if( pCandidate->m_bMark == TaxonomyNode::NOT_MARKED )
		  paSupers->push_back(pCandidate);
	    }
	}
    }

  if( paSupers->size() == 0 )
    paSupers->push_back(m_pTaxonomy->getTop());
}

void TaxonomyBuilder::mark(ExprNodeSet* pSet, ExprNode2Int* pMarked, int bMarkValue)
{
  for(ExprNodeSet::iterator i = pSet->begin(); i != pSet->end(); i++ )
    {
      ExprNode* pC = (ExprNode*)*i;
      (*pMarked)[pC] = bMarkValue;
    }
}

bool TaxonomyBuilder::mark(TaxonomyNode* pNode, int bMarkValue, int iPropagate)
{
  if( pNode->m_setEquivalents.find(EXPRNODE_BOTTOM) != pNode->m_setEquivalents.end() )
    return TRUE;
  if( pNode->m_bMark != TaxonomyNode::NOT_MARKED )
    {
      if( pNode->m_bMark != bMarkValue )
	assertFALSE("Inconsistent classification result ");
      else
	return FALSE;
    }

  pNode->m_bMark = bMarkValue;
  m_aMarkedNodes.push_back(pNode);

  if( iPropagate != NO_PROPAGATE )
    {
      TaxonomyNodes::iterator iStart, iEnd;
      if( iPropagate == PROPAGATE_UP )
	{
	  iStart = pNode->m_aSupers.begin();
	  iEnd = pNode->m_aSupers.begin();
	}
      else
	{
	  iStart = pNode->m_aSubs.begin();
	  iEnd = pNode->m_aSubs.end();
	}
      for(TaxonomyNodes::iterator i = iStart; i != iEnd; i++ )
	{
	  TaxonomyNode* pN = (TaxonomyNode*)*i;
	  mark(pN, bMarkValue, iPropagate);
	}
    }
  
  return TRUE;
}

void TaxonomyBuilder::doTopSearch(ExprNode* pC, TaxonomyNodes* paSupers)
{
  START_DECOMMENT2("TaxonomyBuilder::doTopSearch");
  mark(m_pTaxonomy->getTop(), TaxonomyNode::MARKED_TRUE, NO_PROPAGATE);
  m_pTaxonomy->getBottom()->m_bMark = TaxonomyNode::MARKED_FALSE;
  markToldSubsumers(pC);

  ExprNodes aDisjoints;
  aDisjoints.push_back(pC);
  markToldDisjoints(&aDisjoints, TRUE);
  
  TaxonomyNodeSet setVisited;
  printExprNodeWComment("Starting search from ", m_pTaxonomy->getTop()->m_pName);
  search(TRUE, pC, m_pTaxonomy->getTop(), &setVisited, paSupers);
  END_DECOMMENT("TaxonomyBuilder::doTopSearch");
}

void TaxonomyBuilder::markToldSubsumers(ExprNode* pC)
{
  TaxonomyNode* pNode = m_pTaxonomy->getNode(pC);
  if( pNode )
    {
      if( !mark(pNode, TaxonomyNode::MARKED_TRUE, PROPAGATE_UP) )
	return;
    }

  if( m_pToldTaxonomy->contains(pC) )
    {
      // TODO just getting direct supers and letting recursion handle rest
      // might be more efficient
      ExprNodeSet setSupers;
      m_pToldTaxonomy->getFlattenedSubSupers(pC, TRUE, TRUE, &setSupers);
      for(ExprNodeSet::iterator i = setSupers.begin(); i != setSupers.end(); i++ )
	{
	  ExprNode* pSup = (ExprNode*)*i;
	  markToldSubsumers(pSup);
	}
    }
}

void TaxonomyBuilder::markToldDisjoints(ExprNodes* pInputC, bool bTopSearch)
{
  ExprNodeSet setC;
  setC.insert(pInputC->begin(), pInputC->end());

  for(ExprNodes::iterator i = pInputC->begin(); i != pInputC->end(); i++ )
    {
      ExprNode* pC = (ExprNode*)*i;
      if( m_pTaxonomy->contains(pC) )
	{
	  ExprNodeSet setSupers;
	  m_pTaxonomy->getFlattenedSubSupers(pC, FALSE, TRUE, &setSupers);
	  setC.insert(setSupers.begin(), setSupers.end());
	}

      if( m_pToldTaxonomy->contains(pC) )
	{
	  ExprNodeSet setSupers;
	  m_pToldTaxonomy->getFlattenedSubSupers(pC, FALSE, TRUE, &setSupers);
	  setC.insert(setSupers.begin(), setSupers.end());
	}
    }

  ExprNodeSet setDisjoints;
  for(ExprNodeSet::iterator i = setC.begin(); i != setC.end(); i++ )
    {
      ExprNode* pA = (ExprNode*)*i;
      ExprNode2ExprNodeSetMap::iterator i = m_mapToldDisjoints.find(pA);
      if( i != m_mapToldDisjoints.end() )
	{
	  ExprNodeSet* pToldDisjoints = (ExprNodeSet*)i->second;
	  setDisjoints.insert(pToldDisjoints->begin(), pToldDisjoints->end());
	}
    }

  if( bTopSearch )
    {
      for(ExprNodeSet::iterator i = setDisjoints.begin(); i != setDisjoints.end(); i++ )
	{
	  ExprNode* pD = (ExprNode*)*i;
	  TaxonomyNode* pNode = m_pTaxonomy->getNode(pD);
	  if( pNode )
	    mark(pNode, TaxonomyNode::MARKED_FALSE, NO_PROPAGATE);
	}
    }
  else
    {
      for(ExprNodeSet::iterator i = setDisjoints.begin(); i != setDisjoints.end(); i++ )
	{
	  ExprNode* pD = (ExprNode*)*i;
	  markToldSubsumeds(pD, TaxonomyNode::MARKED_FALSE);
	}
    }
}

void TaxonomyBuilder::markToldSubsumeds(ExprNode* pC, int iMark)
{
  TaxonomyNode* pNode = m_pTaxonomy->getNode(pC);
  if( pNode )
    {
      if( !mark(pNode, iMark, PROPAGAET_DOWN) )
	return;
    }
  
  if( m_pToldTaxonomy->contains(pC) )
    {
      ExprNodeSet setSubs;
      m_pToldTaxonomy->getFlattenedSubSupers(pC, TRUE, FALSE, &setSubs);
      for(ExprNodeSet::iterator i = setSubs.begin(); i != setSubs.end(); i++ )
	{
	  ExprNode* pSub = (ExprNode*)*i;
	  markToldSubsumeds(pSub, iMark);
	}
    }
}

void TaxonomyBuilder::search(bool bTopSearch, ExprNode* pC, TaxonomyNode* pX, TaxonomyNodeSet* pVisited, TaxonomyNodes* pResult)
{
  START_DECOMMENT2("TaxonomyBuilder::search");
  printExprNodeWComment("pC=", pC);
  printExprNodeWComment("pX=", pX->m_pName);

  TaxonomyNodeSet setPosSucc;
  pVisited->insert(pX);

  TaxonomyNodes::iterator iStart, iEnd;
  if( bTopSearch )
    {
      iStart = pX->m_aSubs.begin();
      iEnd = pX->m_aSubs.end();
    }
  else
    {
      iStart = pX->m_aSupers.begin();
      iEnd = pX->m_aSupers.end();
    }

  for(TaxonomyNodes::iterator i = iStart; i != iEnd; i++ )
    {
      TaxonomyNode* pNext = (TaxonomyNode*)*i;
      if( bTopSearch )
	{
	  DECOMMENT("Checking Subsumes");
	  printExprNodeWComment("Next=", pNext->m_pName);
	  printExprNodeWComment("subsumes pC=", pC);

	  if( subsumes(pNext, pC) )
	    setPosSucc.insert(pNext);
	}
      else
	{
	  DECOMMENT("Checking Subsumed");
	  printExprNodeWComment("Next=", pNext->m_pName);
	  printExprNodeWComment("subsumed pC=", pC);

	  if( subsumed(pNext, pC) )
	    setPosSucc.insert(pNext);
	}
    }

  if( setPosSucc.size() == 0 )
    {
      DECOMMENT1("Search Done(top=%d)\n", bTopSearch);
      printExprNodeWComment("pX=", pX->m_pName);
      printExprNodeWComment("for pC=", pC);
      pResult->push_back(pX);
    }
  else
    {
      for(TaxonomyNodeSet::iterator i = setPosSucc.begin(); i != setPosSucc.end(); i++ )
	{
	  TaxonomyNode* pY = (TaxonomyNode*)*i;
	  if( pVisited->find(pY) == pVisited->end() )
	    search(bTopSearch, pC, pY, pVisited, pResult);
	}
    }
  END_DECOMMENT("TaxonomyBuilder::search");
}

void TaxonomyBuilder::doBottomSearch(ExprNode* pC, TaxonomyNodes* paSupers, TaxonomyNodes* paSubs)
{
  START_DECOMMENT2("TaxonomyBuilder::doBottomSearch");
  printExprNodeWComment("pC=", pC);

  TaxonomyNodeSet setSearchFrom;
  for(TaxonomyNodes::iterator i = paSupers->begin(); i != paSupers->end(); i++ )
    {
      TaxonomyNode* pSup = (TaxonomyNode*)*i;
      collectLeafs(pSup, &setSearchFrom);
    }

  if( setSearchFrom.size() == 0 )
    {
      paSubs->push_back(m_pTaxonomy->getBottom());
      DECOMMENT("Adding Bottom Node as a sub");
      END_DECOMMENT("TaxonomyBuilder::doBottomSearch");
      return ;
    }

  clearMarks();

  mark(m_pTaxonomy->getTop(), TaxonomyNode::MARKED_FALSE, NO_PROPAGATE);
  m_pTaxonomy->getBottom()->m_bMark = TaxonomyNode::MARKED_TRUE;
  markToldSubsumeds(pC, TaxonomyNode::MARKED_TRUE);
  
  for(TaxonomyNodes::iterator i = paSupers->begin(); i != paSupers->end(); i++ )
    {
      TaxonomyNode* pSup = (TaxonomyNode*)*i;
      mark(pSup, TaxonomyNode::MARKED_FALSE, NO_PROPAGATE);
    }

  TaxonomyNodeSet setVisited;
  TaxonomyNodes aSubs;
  for(TaxonomyNodeSet::iterator i = setSearchFrom.begin(); i != setSearchFrom.end(); i++ )
    {
      TaxonomyNode* pN = (TaxonomyNode*)*i;
      printExprNodeWComment("searchFrom=", pN->m_pName);
      if( subsumed(pN, pC) )
	search(FALSE, pC, pN, &setVisited, &aSubs);
    }

  if( aSubs.size() == 0 )
    paSubs->push_back(m_pTaxonomy->getBottom());
  else
    {
      for(TaxonomyNodes::iterator i = aSubs.begin(); i != aSubs.end(); i++ )
	paSubs->push_back((TaxonomyNode*)*i);
    }

  END_DECOMMENT("TaxonomyBuilder::doBottomSearch");
}

void TaxonomyBuilder::collectLeafs(TaxonomyNode* pNode, TaxonomyNodeSet* pLeafs)
{
  for(TaxonomyNodes::iterator i = pNode->m_aSubs.begin(); i != pNode->m_aSubs.end(); i++ )
    {
      TaxonomyNode* pSub = (TaxonomyNode*)*i;
      if( pSub->isLeaf() )
	pLeafs->insert(pSub);
      else
	collectLeafs(pSub, pLeafs);
    }
}

void TaxonomyBuilder::prepare()
{
  START_DECOMMENT2("TaxonomyBuilder::prepare");
  reset();
  computeToldInformation();
  createDefinitionOrder();
  computeConceptFlags();
  END_DECOMMENT("TaxonomyBuilder::prepare");
}

void TaxonomyBuilder::reset()
{
  START_DECOMMENT2("TaxonomyBuilder::reset");
  m_iCount = 0;
  m_pKB->prepare();
  Expressivity* pExpressivity = m_pKB->getExpressivity();
  m_bUseCD = (PARAMS_USE_CD_CLASSIFICATION() && 
	      m_pKB->m_pTBox->getUC()->size() == 0 &&
	      m_pKB->m_pTBox->unfold(EXPRNODE_TOP) == NULL &&
	      !pExpressivity->hasInverse() &&
	      !pExpressivity->hasNominal() && 
	      !pExpressivity->hasComplexSubRoles());
  
  m_pSetClasses = new ExprNodeSet;
  for(ExprNodeSet::iterator i = m_pKB->getClasses()->begin(); i != m_pKB->getClasses()->end(); i++ )
    m_pSetClasses->insert((ExprNode*)*i);

  m_mapUnionClasses.clear();
  m_setCyclicConcepts.clear();
  m_aMarkedNodes.clear();
  m_mapToldDisjoints.clear();

  m_pTaxonomy = new Taxonomy();
  m_pToldTaxonomy = new Taxonomy(m_pSetClasses, FALSE);
  END_DECOMMENT("TaxonomyBuilder::reset");
}

void TaxonomyBuilder::computeToldInformation()
{
  START_DECOMMENT2("TaxonomyBuilder::computeToldInformation");
  // compute told subsumers for each concept
  for(Expr2SetOfExprNodeSets::iterator i = m_pKB->m_pTBox->m_mapTBoxAxioms.begin(); i != m_pKB->m_pTBox->m_mapTBoxAxioms.end(); i++ )
    {
      ExprNode* pAxiom = (ExprNode*)i->first;
      ExprNode* pC1 = (ExprNode*)pAxiom->m_pArgs[0];
      ExprNode* pC2 = (ExprNode*)pAxiom->m_pArgs[1];

      bool bEquivalent = (pAxiom->m_iExpression==EXPR_EQCLASSES);
      ExprNodeSet* pExplanation = m_pKB->m_pTBox->getAxiomExplanation(pAxiom);
      bool bReverseArgs = (!isPrimitive(pC1) && isPrimitive(pC2));

      if( bEquivalent && bReverseArgs )
	addToldRelation(pC2, pC1, bEquivalent, pExplanation);
      else
	addToldRelation(pC1, pC2, bEquivalent, pExplanation);
    }

  // for( ATermAppl c : classes ) {
  // ATermAppl desc = (ATermAppl) kb.getTBox().getUnfoldingMap().get( c );
  //
  // if( desc == null )
  // continue;
  //			
  // addToldRelation( c, desc, false );
  // }

  // additional step for union classes. for example, if we have
  // C = or(A, B)
  // and both A and B subclass of D then we can conclude C is also
  // subclass of D
  for(ExprNode2ExprNodeListMap::iterator i = m_mapUnionClasses.begin(); i != m_mapUnionClasses.end(); i++ )
    {
      ExprNode* pC = (ExprNode*)i->first;
      ExprNodeList* pDisj = (ExprNodeList*)i->second;
      ExprNodes lca;
      m_pToldTaxonomy->computeLCA(pDisj, &lca);

      for(ExprNodes::iterator j = lca.begin(); j != lca.end(); j++ )
	{
	  ExprNode* pD = (ExprNode*)*j;
	  addToldSubsumer(pC, pD, NULL);
	}
    }
  
  m_mapUnionClasses.clear();
  m_pToldTaxonomy->assertValid();
  END_DECOMMENT("TaxonomyBuilder::computeToldInformation");
}

void TaxonomyBuilder::createDefinitionOrder()
{
  START_DECOMMENT2("TaxonomyBuilder::createDefinitionOrder");
  Taxonomy* pDefinitionOrderTaxonomy = new Taxonomy(m_pSetClasses, FALSE);
  for(ExprNodeSet::iterator i = m_pSetClasses->begin(); i != m_pSetClasses->end(); i++ )
    {
      ExprNode* pC = (ExprNode*)*i;
      printExprNodeWComment("C=", pC);
      Expr2ExprSetPairList* pDesc = m_pKB->m_pTBox->unfold(pC);
      if( pDesc == NULL )
	continue;
      
      for(Expr2ExprSetPairList::iterator j = pDesc->begin(); j != pDesc->end(); j++ )
	{
	  Expr2ExprSetPair* pPair = (Expr2ExprSetPair*)*j;
	  ExprNodes aList;
	  printExprNodeWComment("pDD=", pPair->first);
	  findPrimitives(pPair->first, &aList, TRUE, TRUE);
	  for(ExprNodes::iterator k = aList.begin(); k != aList.end(); k++ )
	    {
	      ExprNode* pD = (ExprNode*)*k;
	      printExprNodeWComment("pD=", pD);
	    }

	  for(ExprNodes::iterator k = aList.begin(); k != aList.end(); k++ )
	    {
	      ExprNode* pD = (ExprNode*)*k;
	      if( isEqual(pC, pD) == 0 )
		continue;

	      TaxonomyNode* pCNode = pDefinitionOrderTaxonomy->getNode(pC);
	      TaxonomyNode* pDNode = pDefinitionOrderTaxonomy->getNode(pD);

	      printExprNodeWComment("pDNode=", pDNode->m_pName);

	      if( pCNode == NULL )
		assertFALSE("InternalReasonerException: c + is not in the definition order");
	      else if( isEqualTaxonomyNode(pCNode, m_pToldTaxonomy->getTop()) == 0 )
		pDefinitionOrderTaxonomy->merge(pCNode, pDNode);
	      else
		{
		  pDNode->addSub(pCNode);
		  pDefinitionOrderTaxonomy->removeCycles(pCNode);
		}
	    }
	}
    }

  pDefinitionOrderTaxonomy->assertValid();
  m_aOrderedClasses.clear();
  pDefinitionOrderTaxonomy->topologicalSort(TRUE, &m_aOrderedClasses);
  m_pSetClasses->clear();
  START_DECOMMENT2("inner TaxonomyBuilder::createDefinitionOrder");
  DECOMMENT("OrderedClasses:");
  for(ExprNodes::iterator i = m_aOrderedClasses.begin(); i != m_aOrderedClasses.end(); i++) 
    {
      printExprNodeWComment("Class=", ((ExprNode*)*i));
      m_pSetClasses->insert((ExprNode*)*i);
    }
  END_DECOMMENT("TaxonomyBuilder::createDefinitionOrder");

  m_setCyclicConcepts.clear();
  for(ExprNode2TaxonomyNodeMap::iterator i = pDefinitionOrderTaxonomy->m_mNodes.begin(); i != pDefinitionOrderTaxonomy->m_mNodes.end(); i++)
    {
      TaxonomyNode* pNode = (TaxonomyNode*)i->second;
      if( pNode->m_setEquivalents.size() > 1 )
	m_setCyclicConcepts.insert(pNode->m_setEquivalents.begin(), pNode->m_setEquivalents.end());
    }

  END_DECOMMENT("TaxonomyBuilder::createDefinitionOrder");
}

void TaxonomyBuilder::computeConceptFlags()
{
  if( !m_bUseCD )
    return;

  // Use RBox domain axioms to mark some concepts as complex
  for(Expr2RoleMap::iterator i = m_pKB->m_pRBox->m_mRoles.begin(); i != m_pKB->m_pRBox->m_mRoles.end(); i++ )
    {
      ExprNode* pP = (ExprNode*)i->first;
      ExprNodeSet setDomains;
      m_pKB->getDomains(pP, &setDomains);
      for(ExprNodeSet::iterator j = setDomains.begin(); j != setDomains.begin(); j++ )
	m_mConceptFlags[(ExprNode*)*j] = FLAG_OTHER;
    }
  
  /*
   * Iterate over the post-absorption unfolded class descriptions to set
   * concept flags The iteration needs to be over classes to include
   * orphans
   */
  for(ExprNodes::iterator i = m_aOrderedClasses.begin(); i != m_aOrderedClasses.end(); i++ )
    {
      ExprNode* pC = (ExprNode*)*i;
      ExprNode* pNotC = createExprNode(EXPR_NOT, pC);
      Expr2ExprSetPairList* pDesc = m_pKB->m_pTBox->unfold(pC);

      if( m_pKB->m_pTBox->unfold(pNotC) || m_setCyclicConcepts.find(pC) != m_setCyclicConcepts.end() )
	{
	  m_mConceptFlags[pC] = FLAG_NONPRIMITIVE;
	  for(Expr2ExprSetPairList::iterator j = pDesc->begin(); j != pDesc->end(); j++ )
	    {
	      Expr2ExprSetPair* pPair = (Expr2ExprSetPair*)*j;
	      ExprNodes aList;
	      findPrimitives(pPair->first, &aList, TRUE, TRUE);
	      for(ExprNodes::iterator k = aList.begin(); k != aList.end(); k++ )
		{
		  ExprNode* pD = (ExprNode*)*k;
		  ExprNode2IntMap::iterator iFind = m_mConceptFlags.find(pD);
		  if( iFind == m_mConceptFlags.end() )
		    m_mConceptFlags[pD] = FLAG_PRIMITIVE;
		  else if( iFind->second == FLAG_COMPLETELY_DEFINED )
		    m_mConceptFlags[pD] = FLAG_PRIMITIVE;
		}
	    }
	  continue;
	}

      bool bFlagged = FALSE;
      ExprNodeSet setSupers;
      m_pToldTaxonomy->getFlattenedSubSupers(pC, TRUE, TRUE, &setSupers);
      for(ExprNodeSet::iterator k = setSupers.begin(); k != setSupers.end(); k++ )
	{
	  ExprNode* pSup = (ExprNode*)*k;
	  ExprNode2IntMap::iterator iFind = m_mConceptFlags.find(pSup);
	  int iSupFlag = iFind->second;
	  if( iSupFlag == FLAG_NONPRIMITIVE || iSupFlag == FLAG_NONPRIMITIVE_TA )
	    {
	      m_mConceptFlags[pC] = FLAG_NONPRIMITIVE_TA;
	      bFlagged = TRUE;
	      break;
	    }
	}

      if( bFlagged )
	continue;

      /*
       * The concept may have appeared in the definition of a
       * non-primitive or, it may already have an 'OTHER' flag.
       */
      if( m_mConceptFlags.find(pC) != m_mConceptFlags.end() )
	continue;

      if( isCDDesc(pDesc) )
	m_mConceptFlags[pC] = FLAG_COMPLETELY_DEFINED;
      else
	m_mConceptFlags[pC] = FLAG_PRIMITIVE;
    }
}

bool TaxonomyBuilder::isCDDesc(Expr2ExprSetPairList* pDesc)
{
  if( pDesc )
    {
      for(Expr2ExprSetPairList::iterator j = pDesc->begin(); j != pDesc->end(); j++ )
	{
	  Expr2ExprSetPair* pPair = (Expr2ExprSetPair*)*j;
	  if( !isCDDesc(pPair->first) )
	    return FALSE;
	}
    }
  return TRUE;
}

bool TaxonomyBuilder::isCDDesc(ExprNode* pDesc)
{
  if( pDesc == NULL )
    return TRUE;
  if( isPrimitive(pDesc) )
    return TRUE;
  if( pDesc->m_iExpression == EXPR_ALL )
    return TRUE;
  if( pDesc->m_iExpression == EXPR_AND )
    {
      // CHECK HERE : TaxonomyBuilder.c
      // Conjuction '&' not added in original code
      bool bAllCDConj = TRUE;
      for(int i = 0; i < ((ExprNodeList*)pDesc->m_pArgList)->m_iUsedSize; i++ )
	bAllCDConj &= isCDDesc(((ExprNodeList*)pDesc->m_pArgList)->m_pExprNodes[i]);
      return bAllCDConj;
    }
  if( pDesc->m_iExpression == EXPR_NOT )
    {
      if( isPrimitive((ExprNode*)pDesc->m_pArgs[0]) )
	return TRUE;
    }
  return FALSE;
}

void TaxonomyBuilder::clearMarks()
{
  for(TaxonomyNodes::iterator i = m_aMarkedNodes.begin(); i != m_aMarkedNodes.end(); i++ )
    {
      TaxonomyNode* pNode = (TaxonomyNode*)*i;
      pNode->m_bMark = TaxonomyNode::NOT_MARKED;
    }
  m_aMarkedNodes.clear();
}

TaxonomyNode* TaxonomyBuilder::checkSatisfiability(ExprNode* pC)
{
  START_DECOMMENT2("TaxonomyBuilder::checkSatisfiability");
  printExprNodeWComment("pC=", pC);

  bool bIsSatisfiable = m_pKB->m_pABox->isSatisfiable(pC, TRUE);
  if( !bIsSatisfiable )
    m_pTaxonomy->addEquivalentNode(pC, m_pTaxonomy->getBottom());

  DECOMMENT1("bIsSatisfiable=%d\n", bIsSatisfiable);

  if( PARAMS_USE_CACHING() )
    {
      ExprNode* pNotC = createExprNode(EXPR_NOT, pC);
      bIsSatisfiable = m_pKB->m_pABox->isSatisfiable(pNotC, TRUE);
      if( !bIsSatisfiable )
	m_pTaxonomy->addEquivalentNode(pC, m_pTaxonomy->getTop());
    }

  END_DECOMMENT("TaxonomyBuilder::checkSatisfiability");
  return m_pTaxonomy->getNode(pC);
}

bool TaxonomyBuilder::subCheckWithCache(TaxonomyNode* pNode, ExprNode* pC, bool bTopDown)
{
  START_DECOMMENT2("TaxonomyBuilder::subCheckWithCache");
  printExprNodeWComment("pNode=", pNode->m_pName);
  printExprNodeWComment("pC=", pC);

  int iCached = pNode->m_bMark;
  if( iCached != TaxonomyNode::NOT_MARKED )
    {
      DECOMMENT1("Return bCalcDMark=%d\n", (iCached==TaxonomyNode::MARKED_TRUE));
      END_DECOMMENT("TaxonomyBuilder::subCheckWithCache");
      return (iCached==TaxonomyNode::MARKED_TRUE);
    }

  // Search ancestors for marks to propogate
  int iSize = 0;
  TaxonomyNodes::iterator iStart, iEnd;
  if( !bTopDown )
    {
      iSize = pNode->m_aSubs.size();
      iStart = pNode->m_aSubs.begin();
      iEnd = pNode->m_aSubs.end();
    }
  else
    {
      iSize = pNode->m_aSupers.size();
      iStart = pNode->m_aSupers.begin();
      iEnd = pNode->m_aSupers.end();
    }

  if( iSize > 1 )
    {
      TaxonomyNode2TaxonomyNode mVisited, mToBeVisited;
      mVisited[pNode] = NULL;

      for(TaxonomyNodes::iterator i = iStart; i != iEnd; i++ )
	{
	  TaxonomyNode* pN = (TaxonomyNode*)*i;
	  mToBeVisited[pN] = pNode;
	}

      while(mToBeVisited.size()>0)
	{
	  TaxonomyNode* pRelative = (TaxonomyNode*)mToBeVisited.begin()->first;
	  TaxonomyNode* pReachedFrom = (TaxonomyNode*)mToBeVisited.find(pRelative)->second;

	  if( pRelative->m_bMark == TaxonomyNode::MARKED_FALSE )
	    {
	      TaxonomyNode* pN = pReachedFrom;
	      while(pN)
		{
		  mark(pN, TaxonomyNode::MARKED_FALSE, NO_PROPAGATE);
		  TaxonomyNode2TaxonomyNode::iterator iFind = mVisited.find(pN);
		  if( iFind != mVisited.end() )
		    pN = (TaxonomyNode*)iFind->second;
		  else
		    pN = NULL;
		}

	      DECOMMENT("Return bCalcDMark=0\n");
	      END_DECOMMENT("TaxonomyBuilder::subCheckWithCache");
	      return FALSE;
	    }
	  else if( pRelative->m_bMark == TaxonomyNode::NOT_MARKED )
	    {
	      if( !bTopDown )
		{
		  iStart = pRelative->m_aSubs.begin();
		  iEnd = pRelative->m_aSubs.end();
		}
	      else
		{
		  iStart = pRelative->m_aSupers.begin();
		  iEnd = pRelative->m_aSupers.end();
		}

	      for(TaxonomyNodes::iterator i = iStart; i != iEnd; i++ )
		{
		  TaxonomyNode* pN = (TaxonomyNode*)*i;
		  if( mVisited.find(pN) == mVisited.end() && mToBeVisited.find(pN) == mToBeVisited.end() )
		    mToBeVisited[pN] = pRelative;
		}
	    }
	  
	  mToBeVisited.erase(pRelative);
	  mVisited[pRelative] = pReachedFrom;
	}
    }

  bool bCalcDMark = bTopDown?subsumes(pNode->m_pName, pC):subsumes(pC, pNode->m_pName);
  mark(pNode, bCalcDMark?1:0, NO_PROPAGATE);
  DECOMMENT1("Return bCalcDMark=%d\n", bCalcDMark);
  END_DECOMMENT("TaxonomyBuilder::subCheckWithCache");
  return bCalcDMark;
}

bool TaxonomyBuilder::subsumes(TaxonomyNode* pNode, ExprNode* pC)
{
  return subCheckWithCache(pNode, pC, TRUE);
}

bool TaxonomyBuilder::subsumes(ExprNode* pSup, ExprNode* pSub)
{
  return m_pKB->m_pABox->isSubClassOf(pSub, pSup);
}

bool TaxonomyBuilder::subsumed(TaxonomyNode* pNode, ExprNode* pC)
{
  return subCheckWithCache(pNode, pC, FALSE);
}

void TaxonomyBuilder::addToldRelation(ExprNode* pC, ExprNode* pD, bool bEquivalent, ExprNodeSet* pExplanation)
{
  START_DECOMMENT2("TaxonomyBuilder::addToldRelation");
  printExprNodeWComment("C=", pC);
  printExprNodeWComment("D=", pD);
  
  if( !bEquivalent && (isEqual(pC, EXPRNODE_BOTTOM) == 0 || isEqual(pD, EXPRNODE_TOP) == 0 ) )
    {
      END_DECOMMENT("TaxonomyBuilder::addToldRelation");
      return;
    }

  if( !isPrimitive(pC) )
    {
      if( pC->m_iExpression == EXPR_OR )
	{
	  for(int i = 0; i < ((ExprNodeList*)pC->m_pArgList)->m_iUsedSize; i++ )
	    {
	      ExprNode* pE = (ExprNode*)((ExprNodeList*)pC->m_pArgList)->m_pExprNodes[i];
	      addToldRelation(pE, pD, FALSE, pExplanation);
	    }
	}
      else if( pC->m_iExpression == EXPR_NOT )
	{
	  if( isPrimitive(pD) )
	    {
	      ExprNode* pNegation = (ExprNode*)pC->m_pArgs[0];
	      addToldDisjoint(pD, pNegation);
	      addToldDisjoint(pNegation, pD);
	    }
	}
    }
  else if( isPrimitive(pD) )
    {
      if( strncmp(pD->m_cTerm, "bNode", 5) == 0 )
	{
	  END_DECOMMENT("TaxonomyBuilder::addToldRelation");
	  return;
	}

      if( !bEquivalent )
	addToldSubsumer(pC, pD, pExplanation);
      else
	addToldEquivalent(pC, pD);
    }
  else if( pD->m_iExpression == EXPR_AND )
    {
      for(int i = 0; i < ((ExprNodeList*)pD->m_pArgList)->m_iUsedSize; i++ )
	{
	  ExprNode* pE = (ExprNode*)((ExprNodeList*)pD->m_pArgList)->m_pExprNodes[i];
	  addToldRelation(pC, pE, FALSE, pExplanation);
	}
    }
  else if( pD->m_iExpression == EXPR_OR )
    {
      bool bAllPrimitive = TRUE;
      for(int i = 0; i < ((ExprNodeList*)pD->m_pArgList)->m_iUsedSize; i++ )
	{
	  ExprNode* pE = (ExprNode*)((ExprNodeList*)pD->m_pArgList)->m_pExprNodes[i];
	  if( isPrimitive(pE) )
	    {
	      if( bEquivalent )
		addToldSubsumer(pE, pC, NULL);
	    }
	  else
	    bAllPrimitive = FALSE;
	}

      if( bAllPrimitive )
	m_mapUnionClasses[pC] = (ExprNodeList*)pD->m_pArgList;
    }
  else if( isEqual(pD, EXPRNODE_BOTTOM) == 0 )
    {
      addToldEquivalent(pC, EXPRNODE_BOTTOM);
    }
  else if( pD->m_iExpression == EXPR_NOT )
    {
      // handle case sub(a, not(b)) which implies sub[a][b] is false
      ExprNode* pNegation = (ExprNode*)pD->m_pArgs[0];
      if( isPrimitive(pNegation) )
	{
	  addToldDisjoint(pC, pNegation);
	  addToldDisjoint(pNegation, pC);
	}
    }
  END_DECOMMENT("TaxonomyBuilder::addToldRelation");
}

void TaxonomyBuilder::addToldDisjoint(ExprNode* pC, ExprNode* pD)
{
  START_DECOMMENT2("TaxonomyBuilder::addToldDisjoint");
  printExprNodeWComment("C=", pC);
  printExprNodeWComment("D=", pD);
  
  ExprNodeSet* pDisjoints = NULL;
  ExprNode2ExprNodeSetMap::iterator i = m_mapToldDisjoints.find(pC);
  if( i == m_mapToldDisjoints.end() )
    {
      pDisjoints = new ExprNodeSet;
      m_mapToldDisjoints[pC] = pDisjoints;
    }
  else
    pDisjoints = (ExprNodeSet*)i->second;
  pDisjoints->insert(pD);
  END_DECOMMENT("TaxonomyBuilder::addToldDisjoint");
}

void TaxonomyBuilder::addToldEquivalent(ExprNode* pC, ExprNode* pD)
{
  START_DECOMMENT2("TaxonomyBuilder::addToldEquivalent");
  printExprNodeWComment("C=", pC);
  printExprNodeWComment("D=", pD);
  
  if( isEqual(pC, pD) == 0 )
    {
      END_DECOMMENT("TaxonomyBuilder::addToldEquivalent");
      return;
    }

  TaxonomyNode* pCNode = m_pToldTaxonomy->getNode(pC);
  TaxonomyNode* pDNode = m_pToldTaxonomy->getNode(pD);
  m_pToldTaxonomy->merge(pCNode, pDNode);
  END_DECOMMENT("TaxonomyBuilder::addToldEquivalent");
}

void TaxonomyBuilder::addToldSubsumer(ExprNode* pC, ExprNode* pD, ExprNodeSet* pExplanation)
{
  START_DECOMMENT2("TaxonomyBuilder::addToldSubsumer");
  printExprNodeWComment("C=", pC);
  printExprNodeWComment("D=", pD);
  
  TaxonomyNode* pCNode = m_pToldTaxonomy->getNode(pC);
  TaxonomyNode* pDNode = m_pToldTaxonomy->getNode(pD);

  if( pCNode == NULL || pDNode == NULL )
    assertFALSE("InternalReasonerException( c + is not in the definition order");

  if( isEqualTaxonomyNode(pCNode, pDNode) == 0 )
    {
      END_DECOMMENT("TaxonomyBuilder::addToldSubsumer");
      return;
    }

  if( isEqualTaxonomyNode(pCNode, m_pToldTaxonomy->getTop()) == 0 )
    m_pToldTaxonomy->merge(pCNode, pDNode);
  else
    {
      pDNode->addSub(pCNode);
      m_pToldTaxonomy->removeCycles(pCNode);
      if( pExplanation )
	pCNode->addSuperExplanation(pDNode, pExplanation);
    }
  END_DECOMMENT("TaxonomyBuilder::addToldSubsumer");
}

Taxonomy* TaxonomyBuilder::realize()
{
  if( PARAMS_REALIZE_INDIVIDUAL_AT_A_TIME() )
    return realizeByIndividuals();
  return realizeByConcepts();
}

Taxonomy* TaxonomyBuilder::realizeByIndividuals()
{
  for(ExprNodes::iterator i = m_pKB->m_pABox->m_aNodeList.begin(); i != m_pKB->m_pABox->m_aNodeList.end(); i++ )
    {
      ExprNode* pExprNode = (ExprNode*)*i;
      Node* pNode = (Node*)m_pKB->m_pABox->m_mNodes.find(pExprNode)->second;
      if( !pNode->isIndividual() ) continue;
      Individual* pIndividual = (Individual*)pNode;

      ExprNode2Int mMarked;
      ExprNodes aObviousTypes, aObviousNonTypes;
      m_pKB->m_pABox->getObviousTypes(pIndividual->m_pName, &aObviousTypes, &aObviousNonTypes);
      for(ExprNodes::iterator j = aObviousTypes.begin(); j != aObviousTypes.end(); j++ )
	{
	  ExprNode* pC = (ExprNode*)*j;
	  
	  // since nominals can be returned by getObviousTypes
	  // we need the following check
	  if( !m_pTaxonomy->contains(pC) )
	    continue;

	  ExprNodeSet setEquivalents;
	  m_pTaxonomy->getAllEquivalents(pC, &setEquivalents);
	  mark(&setEquivalents, &mMarked, TaxonomyNode::MARKED_TRUE);

	  ExprNodeSet setSupers;
	  m_pTaxonomy->getFlattenedSubSupers(pC, TRUE, TRUE, &setSupers);
	  mark(&setSupers, &mMarked, TaxonomyNode::MARKED_TRUE);

	  // FIXME: markToldDisjoints operates on a map key'd with
	  // TaxonomyNodes, not ATermAppls
	  // markToldDisjoints( c, false );
	}

      for(ExprNodes::iterator j = aObviousNonTypes.begin(); j != aObviousNonTypes.end(); j++ )
	{
	  ExprNode* pC = (ExprNode*)*j;
	  
	  ExprNodeSet setEquivalents;
	  m_pTaxonomy->getAllEquivalents(pC, &setEquivalents);
	  mark(&setEquivalents, &mMarked, TaxonomyNode::MARKED_FALSE);

	  ExprNodeSet setSupers;
	  m_pTaxonomy->getFlattenedSubSupers(pC, TRUE, FALSE, &setSupers);
	  mark(&setSupers, &mMarked, TaxonomyNode::MARKED_FALSE);
	}

      realizeByIndividual(pIndividual->m_pName, EXPRNODE_TOP, &mMarked);
    }
  return m_pTaxonomy;
}

bool TaxonomyBuilder::realizeByIndividual(ExprNode* pN, ExprNode* pC, ExprNode2Int* pMarked)
{
  bool bRealized = FALSE;

  if( isEqual(pC, EXPRNODE_BOTTOM) == 0 )
    return FALSE;

  bool bIsType;
  if( pMarked->find(pC) != pMarked->end() )
    {
      int iMark = (pMarked->find(pC))->second;
      bIsType = (iMark==1)?TRUE:FALSE;
    }
  else
    {
      bIsType = m_pKB->isType(pN, pC);
      (*pMarked)[pC] = bIsType?1:0;
    }

  if( bIsType )
    {
      TaxonomyNode* pNode = m_pTaxonomy->getNode(pC);
      for(TaxonomyNodes::iterator i = pNode->m_aSubs.begin(); i != pNode->m_aSubs.end(); i++ )
	{
	  TaxonomyNode* pSub = (TaxonomyNode*)*i;
	  ExprNode* pD = pSub->m_pName;
	  bRealized |= realizeByIndividual(pN, pD, pMarked);
	}
      
      // this concept is the most specific concept x belongs to
      // so add it here and return true
      if( !bRealized )
	{
	  if( pNode )
	    pNode->addInstance(pC);
	  bRealized = TRUE;
	}
    }

  return bRealized;
}

Taxonomy* TaxonomyBuilder::realizeByConcepts()
{
  ExprNodeSet setSubInstances;
  realizeByConcept(EXPRNODE_TOP, &(m_pKB->m_setIndividuals), &setSubInstances);
  return m_pTaxonomy;
}

void TaxonomyBuilder::realizeByConcept(ExprNode* pC, ExprNodeSet* pIndividuals, ExprNodeSet* pSubInstances)
{
  if( isEqual(pC, EXPRNODE_BOTTOM) == 0 )
    return;

  ExprNodeSet setMostSpecificInstances;
  m_pKB->retrieve(pC, pIndividuals, pSubInstances);
  for(ExprNodeSet::iterator i = pSubInstances->begin(); i != pSubInstances->end(); i++ )
    setMostSpecificInstances.insert((ExprNode*)*i);
  
  if( pSubInstances->size() > 0 )
    {
      TaxonomyNode* pNode = m_pTaxonomy->getNode(pC);
      for(TaxonomyNodes::iterator j = pNode->m_aSubs.begin(); j != pNode->m_aSubs.end(); j++ )
	{
	  TaxonomyNode* pSub = (TaxonomyNode*)*j;
	  ExprNode* pD = pSub->m_pName;

	  ExprNodeSet setSubInstances;
	  realizeByConcept(pD, pSubInstances, &setSubInstances);
	  removeAll(&setMostSpecificInstances, &setSubInstances);
	}

      if( setMostSpecificInstances.size() > 0 )
	pNode->setInstances(&setMostSpecificInstances);
    }
}
