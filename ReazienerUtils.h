#ifndef _REAZIENER_UTILS_
#define _REAZIENER_UTILS_

#include <cassert>
#include "Params.h"

#define _DEBUG_OUTPUT_

#ifdef _DEBUG_OUTPUT_

#define START_DECOMMENT2(function)		\
  if( PARAMS_PRINT_DEBUGINFO_INHTML() )			\
    {							\
      g_iCommentIndent++;				\
      for(int i = 0; i < g_iCommentIndent; i++ )	\
	printf("   ");					\
      printf("<function name=\"%s\">\n", function);	\
    }

#define START_DECOMMENT(function, comment)	\
  if( PARAMS_PRINT_DEBUGINFO_INHTML() )				\
    {							\
      g_iCommentIndent++;						\
      for(int i = 0; i < g_iCommentIndent; i++ )			\
	printf("   ");							\
      printf("<function name=\"%s\" comment=\"%s\">\n", function, comment); \
    }

#define END_DECOMMENT(function)			\
  if( PARAMS_PRINT_DEBUGINFO_INHTML() )				\
    {							\
      for(int i = 0; i < g_iCommentIndent; i++ )	\
	printf("   ");					\
      printf("</function>\n", function);		\
      g_iCommentIndent--;				\
    }

#define DECOMMENT(comment)			\
  if( PARAMS_PRINT_DEBUGINFO_INHTML() )				\
    {							\
      for(int i = 0; i < g_iCommentIndent; i++ )	\
	printf("   ");					\
      printf("<comment>%s</comment>\n", comment);	\
    }

#define DECOMMENT1(expr, arg1)			\
  if( PARAMS_PRINT_DEBUGINFO_INHTML() )				\
    {							\
      for(int i = 0; i < g_iCommentIndent; i++ )	\
	printf("   ");					\
      printf("<comment>");				\
      printf(expr, arg1);				\
      printf("</comment>\n");				\
    }

#define DECOMMENT2(expr, arg1, arg2)	\
  if( PARAMS_PRINT_DEBUGINFO_INHTML() )		\
    {							\
      for(int i = 0; i < g_iCommentIndent; i++ )	\
	printf("   ");					\
      printf("<comment>");				\
      printf(expr, arg1, arg2);				\
      printf("</comment>\n");				\
    }

#else

#define START_DECOMMENT2(function) ;
#define START_DECOMMENT(function, comment) ;
#define END_DECOMMENT(function) ;
#define DECOMMENT(comment) ;
#define DECOMMENT1(expr, arg1) ;
#define DECOMMENT2(expr, arg1, arg2) ;

#endif

void assertFALSE(char* pMsg);

#endif
