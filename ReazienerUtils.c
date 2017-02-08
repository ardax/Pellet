#include "ReazienerUtils.h"
#include "Utils.h"

//int g_iCommentIndent = 0;

void assertFALSE(char* pMsg)
{
  printf("ERROR: %s\n", pMsg);
  assert(FALSE);
}
