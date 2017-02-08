#include "KRSSLoader.h"
#include "KnowledgeBase.h"
#include "Params.h"
#include "ReazienerUtils.h"

KnowledgeBase* g_pKB = NULL;
extern int g_iCommentIndent;
extern Params* g_pParams;

int main(int argc, char** argv)
{
  processParameters(argc, argv);

  if( PARAMS_GET_INPUTFILE() )
    {
      if( PARAMS_PRINT_DEBUGINFO_INHTML() )
	{
	  printf("<?xml version=\"1.0\" ?>\n");
	  printf("<document>");
	}
      g_pKB = new KnowledgeBase();
      KRSSLoader cLoader;
      cLoader.loadKRSSFile( PARAMS_GET_INPUTFILE(), g_pKB);

      g_pKB->prepare();
      if( g_pKB->isConsistent() )
	{
	  printf("KB is consistent\n");
	  g_pKB->printExpressivity();
	  g_pKB->classify();
	  g_pKB->realize();
	}
      else
	printf("KB is not consistent\n");


      if( g_pKB->isClassified() )
	{
	  if( PARAMS_PRINT_CLASSIFICATION_HIERARCHY() )
	    {
	      char cOutputFile[1024];
	      sprintf(cOutputFile, "%s.classfication.tree", g_pParams->g_pInputFile);
	      g_pKB->printClassTreeInFile(cOutputFile);
	    }
	  g_pKB->printClassTree();
	}

      if( PARAMS_PRINT_DEBUGINFO_INHTML() )
	printf("</document>");
    }
  return 0;
}
