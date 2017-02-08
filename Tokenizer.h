#ifndef _ZIE_TOKENIZER_
#define _ZIE_TOKENIZER_

#include "Utils.h"
#include "TextUtils.h"

void breakIntoSentences(char* pSentence, list<char*>* paSentences);

char* tokenizeSentence(const char* txt, bool bKeepTags = FALSE);
void readTokenizationRules(char* pDotRules, char* pHypenRules, char* pInseperableRules);
bool isInDotRules(char* p);
bool isInHypenRules(char* p);
bool rulesApply(char* txt);
bool isInseperable(char* p);

#endif
