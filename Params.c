#include "Utils.h"
#include "TextUtils.h"
#include "Params.h"

Params* g_pParams = NULL;

void processParameters(int argc, char** argv)
{
  g_pParams = (Params*)calloc(1, sizeof(Params));

  for(int i = 1; i < argc; i++ )
    {
      if( strncmp(argv[i], "file=", 5) == 0 )
	{
	  char* cInputFile = (argv[i] + 5);
	  moveStr(g_pParams->g_pInputFile, cInputFile, strlen(cInputFile));
	}
      else if( strncmp(argv[i], "debug", 5) == 0 )
	{
	  g_pParams->g_bPrintDebugInfoInHTML = TRUE;
	}
      else if( strncmp(argv[i], "printclasses", 12) == 0 )
	{
	  g_pParams->g_bPrintClassificationHierarchy = TRUE;
	}
    }
  g_pParams->g_bUseAbsorption = TRUE;
  g_pParams->g_bUseRoleAbsorption = TRUE;
  g_pParams->g_bUseNominalAbsorption = TRUE;
  g_pParams->g_bUseHaveValueAbsorption = TRUE;

  g_pParams->g_bUseCaching = TRUE;
  g_pParams->g_bUseAdvancedCaching = TRUE;
  g_pParams->g_bUsePseudoModel = TRUE;
  //g_pParams->g_bPrintHTML = TRUE;
  //g_pParams->g_bUseCompletionStrategy = TRUE;
  g_pParams->g_bValidateABox = TRUE;

  g_pParams->g_bUseCDClassification = TRUE;
}

bool PARAMS_USE_INCREMENTAL_CONSISTENCE() { return g_pParams->g_bUseIncrementalConsistency; }
bool PARAMS_USE_INCREMENTAL_DELETION() { return g_pParams->g_bUseIncrementalDeletion; }
bool PARAMS_USE_NOMINAL_ABSORPTION() { return g_pParams->g_bUseNominalAbsorption; }
bool PARAMS_USE_PSEUDO_NOMINALS() { return g_pParams->g_bUsePseudoNominals; }
bool PARAMS_USE_TRACING() { return g_pParams->g_bUseTracing; }
bool PARAMS_USE_CACHING() { return g_pParams->g_bUseCaching; }
bool PARAMS_USE_ADVANCED_CACHING() { return g_pParams->g_bUseAdvancedCaching; }
bool PARAMS_USE_COMPLETION_QUEUE() { return g_pParams->g_bUseCompletionQueue; }
bool PARAMS_KEEP_ABOX_ASSERTIONS() { return g_pParams->g_bKeepABoxAssertions; }
bool PARAMS_IGNORE_INVERSES() { return g_pParams->g_bIgnoreInverses; }
bool PARAMS_USE_ABSORPTION() { return g_pParams->g_bUseAbsorption; }
bool PARAMS_USE_ROLE_ABSORPTION() { return g_pParams->g_bUseRoleAbsorption; }
bool PARAMS_USE_HASVALUE_ABSORPTION() { return g_pParams->g_bUseHaveValueAbsorption; }
bool PARAMS_USE_PSEUDO_MODEL() { return g_pParams->g_bUsePseudoModel; }
bool PARAMS_USE_CD_CLASSIFICATION() { return g_pParams->g_bUseCDClassification; }
bool PARAMS_SATURATE_TABLEAU() { return g_pParams->g_bSaturateTableau; }
bool PARAMS_COPY_ON_WRITE() { return g_pParams->g_bCopyOnWrite; }
bool PARAMS_USE_COMPLETION_STRATEGY() { return g_pParams->g_bUseCompletionStrategy; }
bool PARAMS_USE_FULL_DATATYPE_REASONING() { return g_pParams->g_bUseFullDataTypeReasoning; }
bool PARAMS_USE_SMART_RESTORE() { return g_pParams->g_bUseSmartRestore; }
bool PARAMS_CHECK_NOMINAL_EDGES() { return g_pParams->g_bCheckNominalEdges; }
bool PARAMS_REALIZE_INDIVIDUAL_AT_A_TIME() { return g_pParams->g_bRealizeIndividualAtATime; }
bool PARAMS_USE_BINARY_INSTANCE_RETRIEVAL() { return g_pParams->g_bUseBinaryInstanceRetrieval; }
bool PARAMS_MAINTAIN_COMPLETION_QUEUE() { return g_pParams->g_bMaintainCompletionQueue; }
bool PARAMS_USE_UNIQUE_NAME_ASSUMPTION() { return g_pParams->g_bUseUniqueNameAssumption; }
bool PARAMS_VALIDATE_ABOX() { return g_pParams->g_bValidateABox; }
bool PARAMS_USE_DISJUNCTION_SORTING() { return g_pParams->g_bUseDisjunctionSorting; }
bool PARAMS_NO_SORTING() { return g_pParams->g_bNoSorting; }
bool PARAMS_USE_SEMANTIC_BRANCHING() { return g_pParams->g_bUseSemanticBranching; }
bool PARAMS_USE_DEPTHFIRST_SEARCH() { return g_pParams->g_bUseDepthFirstSearch; }
bool PARAMS_PRINT_DEBUGINFO_INHTML() { return g_pParams->g_bPrintDebugInfoInHTML; }
bool PARAMS_PRINT_CLASSIFICATION_HIERARCHY(){ return g_pParams->g_bPrintClassificationHierarchy; }


char* PARAMS_GET_INPUTFILE() { return g_pParams->g_pInputFile; }
