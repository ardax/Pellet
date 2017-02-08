#include "TaxonomyPrinter.h"
#include "Taxonomy.h"
#include "Params.h"


extern ExprNode* EXPRNODE_TOP;
extern ExprNode* EXPRNODE_BOTTOM;


TreeTaxonomyPrinter::TreeTaxonomyPrinter()
{
  for(int i = 0; i < INTENDSIZE-1; i++ )
    m_cIntend[i] = ' ';
  m_cIntend[INTENDSIZE-1] = '\0';
}

void TreeTaxonomyPrinter::print(Taxonomy* pTaxonomy)
{
  m_pTaxonomy = pTaxonomy;
  
  printf("<comment>\n");
  printTree();
  printf("</comment>\n");
}

void TreeTaxonomyPrinter::printInFile(Taxonomy* pTaxonomy, char* pFileName)
{
  m_pTaxonomy = pTaxonomy;
  
  FILE* pFILE;
  if( (pFILE = fopen(pFileName, "w")) )
    {
      ExprNodeSet setPrintedNodes;

      for(ExprNode2TaxonomyNodeMap::iterator i = m_pTaxonomy->m_mNodes.begin(); i != m_pTaxonomy->m_mNodes.end(); i++ )
	{
	  TaxonomyNode* pNode = (TaxonomyNode*)i->second;
	  if( setPrintedNodes.find(pNode->m_pName) != setPrintedNodes.end() )
	    continue;
	  setPrintedNodes.insert(pNode->m_pName);

	  fprintf(pFILE, "(");
	  
	  if( pNode->m_setEquivalents.size() > 1 )
	    {
	      fprintf(pFILE, "(");
	      for(ExprNodeSet::iterator e = pNode->m_setEquivalents.begin(); e != pNode->m_setEquivalents.end(); e++ )
		{
		  ExprNode* pE = (ExprNode*)*e;
		  setPrintedNodes.insert(pE);
		  if( ::isEqual(pE, EXPRNODE_TOP) == 0 )
		    fprintf(pFILE, "TOP ");
		  else if( ::isEqual(pE, EXPRNODE_BOTTOM) == 0 )
		    fprintf(pFILE, "BOTTOM ");
		  else
		    fprintf(pFILE, "%s ", pE->m_cTerm);
		}
	      fprintf(pFILE, ") ");
	    }
	  else if( ::isEqual(pNode->m_pName, EXPRNODE_TOP) == 0 )
	    printNode(pFILE, EXPRNODE_TOP);
	  else if( ::isEqual(pNode->m_pName, EXPRNODE_BOTTOM) == 0 )
	    printNode(pFILE, EXPRNODE_BOTTOM);
	  else
	    fprintf(pFILE, "%s ", pNode->m_pName->m_cTerm);

	  // print supers
	  if( pNode->m_aSupers.size() > 0 )
	    {
	      fprintf(pFILE, "(");
	      for(TaxonomyNodes::iterator j = pNode->m_aSupers.begin(); j != pNode->m_aSupers.end(); j++ )
		{
		  TaxonomyNode* pSuper = (TaxonomyNode*)*j;
		  if( ::isEqual(pSuper->m_pName, EXPRNODE_TOP) == 0 )
		    printNode(pFILE, EXPRNODE_TOP);
		  else
		    fprintf(pFILE, "%s ", pSuper->m_pName->m_cTerm);
		}
	      fprintf(pFILE, ") ");
	    }
	  else
	    fprintf(pFILE, "NIL ");

	  // print subs
	  if( pNode->m_aSubs.size() > 0 )
	    {
	      fprintf(pFILE, "(");
	      for(TaxonomyNodes::iterator j = pNode->m_aSubs.begin(); j != pNode->m_aSubs.end(); j++ )
		{
		  TaxonomyNode* pSub = (TaxonomyNode*)*j;
		  if( ::isEqual(pSub->m_pName, EXPRNODE_BOTTOM) == 0 )
		    printNode(pFILE, EXPRNODE_BOTTOM);
		  else
		    fprintf(pFILE, "%s ", pSub->m_pName->m_cTerm);
		}
	      fprintf(pFILE, ") ");
	    }
	  else
	    fprintf(pFILE, "NIL ");
	  fprintf(pFILE, ")\n");
	}
      
      fclose(pFILE);
    }
}

void TreeTaxonomyPrinter::printNode(FILE* pFILE, ExprNode* pNode)
{
  ExprNode2TaxonomyNodeMap::iterator iFind = m_pTaxonomy->m_mNodes.find(pNode);
  if( iFind != m_pTaxonomy->m_mNodes.end() )
    {
      TaxonomyNode* pTNode = (TaxonomyNode*)iFind->second;
      if( pTNode->m_setEquivalents.size() > 1 )
	{
	  fprintf(pFILE, "(");
	  for(ExprNodeSet::iterator e = pTNode->m_setEquivalents.begin(); e != pTNode->m_setEquivalents.end(); e++ )
	    {
	      ExprNode* pE = (ExprNode*)*e;
	      if( ::isEqual(pE, EXPRNODE_TOP) == 0 )
		fprintf(pFILE, "TOP ");
	      else if( ::isEqual(pE, EXPRNODE_BOTTOM) == 0 )
		fprintf(pFILE, "BOTTOM ");
	      else
		fprintf(pFILE, "%s ", pE->m_cTerm);
	    }
	  fprintf(pFILE, ") ");
	}
      else if( ::isEqual(pNode, EXPRNODE_TOP) == 0 )
	fprintf(pFILE, "TOP ");
      else if( ::isEqual(pNode, EXPRNODE_BOTTOM) == 0 )
	fprintf(pFILE, "BOTTOM ");
      else
	fprintf(pFILE, "%s ", pTNode->m_pName->m_cTerm);
    }
}

void TreeTaxonomyPrinter::printTree()
{
  ExprNodeSet setTop;
  setTop.insert(EXPRNODE_TOP);
  m_pTaxonomy->getEquivalents(EXPRNODE_TOP, &setTop);
  printHTML("<ul>");
  printTree(&setTop, " ");
  printHTML("</ul>");

  ExprNodeSet setBottom;
  setBottom.insert(EXPRNODE_BOTTOM);
  m_pTaxonomy->getEquivalents(EXPRNODE_BOTTOM, &setBottom);
  
  if( setBottom.size() > 1 )
    {
      printHTML("<ul>");
      printNode(&setBottom, " ");
      printHTML("</ul>");
    }
}

void TreeTaxonomyPrinter::printTree(ExprNodeSet* pSet, string sIndent)
{
  if( pSet->find(EXPRNODE_BOTTOM) != pSet->end() )
    return;
  
  printNode(pSet, sIndent);
  printHTML("<ul>");
  ExprNode* pC = (ExprNode*)(*pSet->begin());
  SetOfExprNodeSet setSubSets;
  m_pTaxonomy->getSubSupers(pC, TRUE, FALSE, &setSubSets);
  for(SetOfExprNodeSet::iterator i = setSubSets.begin(); i != setSubSets.end(); i++ )
    {
      ExprNodeSet* pEqs = (ExprNodeSet*)*i;
      if( pEqs->find(pC) != pEqs->end() )
	continue;
      printTree(pEqs, sIndent+"   ");
    }
  printHTML("</ul>");
}

void TreeTaxonomyPrinter::printNode(ExprNodeSet* pSet, string sIndent)
{
  if( PARAMS_PRINT_DEBUGINFO_INHTML() )
    printHTML("<li>");
  else
    printf("%s", sIndent.c_str());
  
  int iIndex = 0;
  ExprNode* pC = (ExprNode*)(*pSet->begin());
  for(ExprNodeSet::iterator i = pSet->begin(); i != pSet->end(); i++ )
    {
      if( iIndex > 0 ) printf(" = ");
      printURI((ExprNode*)*i);
      iIndex++;
    }

  ExprNodeSet setInstances;
  m_pTaxonomy->getInstances(pC, &setInstances, TRUE);
  if( setInstances.size() > 0 )
    {
      printf(" - (");
      bool bPrinted = FALSE;
      int iAnonCount = 0;
      for(ExprNodeSet::iterator i = setInstances.begin(); i != setInstances.end(); i++ )
	{
	  ExprNode* pX = (ExprNode*)*i;
	  if( strncmp(pX->m_cTerm, "bNode", 5) == 0 )
	    iAnonCount++;
	  else
	    {
	      if( bPrinted )
		printf(" , ");
	      bPrinted = TRUE;
	      printURI(pX);
	    }
	}

      if( iAnonCount > 0 )
	{
	  if( bPrinted ) 
	    printf(", ");
	  printf("%d Anonymous Individual", iAnonCount);
	  if( iAnonCount > 1 ) printf("s");
	}
      printf(")");
    }

  if( PARAMS_PRINT_DEBUGINFO_INHTML() )
    printHTML("</li>");
  else
    printf("\n");
}

void TreeTaxonomyPrinter::printURI(ExprNode* pC)
{
  if( isEqual(pC, EXPRNODE_TOP) == 0 )
    printf("owl:Thing");
  else if( isEqual(pC, EXPRNODE_BOTTOM) == 0 )
    printf("owl:Nothing");
  else if( pC->m_iArity == 0 )
    printf("%s", pC->m_cTerm);
}

void TreeTaxonomyPrinter::printHTML(char* pHTMLStr)
{
  if( PARAMS_PRINT_DEBUGINFO_INHTML() )
    printf("%s", pHTMLStr);
}
