#ifndef _TAXONOMY_PRINTER_
#define _TAXONOMY_PRINTER_

#include "ExpressionNode.h"

#define INTENDSIZE 4

class Taxonomy;

class TaxonomyPrinter
{
 public:
  TaxonomyPrinter() {}

  virtual void print(Taxonomy* pTaxonomy) = 0;
  virtual void printInFile(Taxonomy* pTaxonomy, char* pFileName) = 0;

};

class TreeTaxonomyPrinter : public TaxonomyPrinter
{
 public:
  TreeTaxonomyPrinter();

  void print(Taxonomy* pTaxonomy);
  void printInFile(Taxonomy* pTaxonomy, char* pFileName);

  void printTree();
  void printTree(ExprNodeSet* pSet, string sIndent);
  void printNode(ExprNodeSet* pSet, string sIndent);
  void printURI(ExprNode* pC);
  void printHTML(char* pHTMLStr);
  void printNode(FILE* pFile, ExprNode* pNode);

  char m_cIntend[INTENDSIZE];

  Taxonomy* m_pTaxonomy;
};

#endif
