#ifndef _REAZIERNER_PARAMS_
#define _REAZIERNER_PARAMS_

/* PARAMETER VARIABLES */
typedef struct _PARAMS_
{
  bool g_bUseIncrementalConsistency;
  bool g_bUseIncrementalDeletion;
  bool g_bUseNominalAbsorption;
  bool g_bUsePseudoNominals;
  bool g_bUseTracing;
  bool g_bUseCaching;
  bool g_bUseAdvancedCaching;
  bool g_bUseCompletionQueue;
  bool g_bUseCompletionStrategy;
  bool g_bKeepABoxAssertions;
  bool g_bIgnoreInverses;
  bool g_bUseAbsorption;
  bool g_bUseRoleAbsorption;
  bool g_bUseHaveValueAbsorption;
  bool g_bUsePseudoModel;
  bool g_bUseCDClassification;
  bool g_bUseSmartRestore;
  bool g_bSaturateTableau;
  bool g_bCopyOnWrite;
  bool g_bUseFullDataTypeReasoning;
  bool g_bCheckNominalEdges;
  bool g_bRealizeIndividualAtATime;
  bool g_bUseBinaryInstanceRetrieval;
  bool g_bMaintainCompletionQueue;
  bool g_bUseUniqueNameAssumption;
  bool g_bValidateABox;
  bool g_bUseDisjunctionSorting;
  bool g_bNoSorting;
  bool g_bUseSemanticBranching;
  bool g_bUseDepthFirstSearch;
  bool g_bPrintDebugInfoInHTML;
  bool g_bPrintClassificationHierarchy;
  char g_pInputFile[1024];
} Params;

void processParameters(int argc, char** argv);

bool PARAMS_USE_INCREMENTAL_CONSISTENCE();
bool PARAMS_USE_INCREMENTAL_DELETION();
bool PARAMS_USE_NOMINAL_ABSORPTION();
bool PARAMS_USE_PSEUDO_NOMINALS();
bool PARAMS_USE_TRACING();
bool PARAMS_USE_CACHING();
bool PARAMS_USE_ADVANCED_CACHING();
bool PARAMS_USE_COMPLETION_QUEUE();
bool PARAMS_USE_COMPLETION_STRATEGY();
bool PARAMS_KEEP_ABOX_ASSERTIONS();
bool PARAMS_IGNORE_INVERSES();
bool PARAMS_USE_ABSORPTION();
bool PARAMS_USE_ROLE_ABSORPTION();
bool PARAMS_USE_HASVALUE_ABSORPTION();
bool PARAMS_USE_PSEUDO_MODEL();
bool PARAMS_USE_SMART_RESTORE();
bool PARAMS_USE_CD_CLASSIFICATION();
bool PARAMS_SATURATE_TABLEAU();
bool PARAMS_COPY_ON_WRITE();
bool PARAMS_USE_FULL_DATATYPE_REASONING();
bool PARAMS_CHECK_NOMINAL_EDGES();
bool PARAMS_REALIZE_INDIVIDUAL_AT_A_TIME();
bool PARAMS_USE_BINARY_INSTANCE_RETRIEVAL();
bool PARAMS_MAINTAIN_COMPLETION_QUEUE();
bool PARAMS_USE_UNIQUE_NAME_ASSUMPTION();
bool PARAMS_VALIDATE_ABOX();
bool PARAMS_USE_DISJUNCTION_SORTING();
bool PARAMS_NO_SORTING();
bool PARAMS_USE_SEMANTIC_BRANCHING();
bool PARAMS_USE_DEPTHFIRST_SEARCH();
bool PARAMS_PRINT_DEBUGINFO_INHTML();
bool PARAMS_PRINT_CLASSIFICATION_HIERARCHY();

char* PARAMS_GET_INPUTFILE();

#endif
