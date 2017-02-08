


#ifndef _TEXTUTILS_
#define _TEXTUTILS_

#include "Utils.h"
#include <list>
#include <iostream>
#include <stdlib.h>
#include <string.h>
using namespace std;

typedef struct _Word_
{
  char m_pTitle[100];
  char m_pTag[100];
  void* m_pNext;
  void* m_pPrev;
} Word;

int stricmp (const char *s, const char *t);
int strincmp (const char *s, const char *t, int n);

void lowerStr(char* s);
void upperStr(char* s);
bool isAllLowerCase(char* pStr);
bool isAllUpperCase(char* pStr);

string validateStr(string txt);
string StripHTMLCodes(string txt);
string normalizeHTMLStr(string txt);

bool isallalpha(const char* pc);
bool isallnum(const char* pc);
bool isalpha(const char c);
bool isalphanum(const char c);
bool isnum(const char c);
bool iswhite(const char c);
bool isEmpty(char* pStr);

bool isQuoted(const char* pStr);
void unQuote(const char* pStr, char* pUnQuotedStr);
int findChar(const char* pStr, char c, int iStart = 0);

void shift(char* pStr, int iShiftCount = 1, int iStrLen = 0);

string num2str(int i);
string float2str(float f);
string long2str(long l);

void moveStr(char* pTarget, const char* pSource, int iLength, bool bKeepAll = FALSE);
string replaceStr(string txt, string s, string sReplacement, bool bReplaceAll = true);
void deEmptyStr(char* pstr, bool bDe);
void reallocStr( char** strP, size_t* maxsizeP, size_t size );
void decodeStr(char* buf, bool bLink);
void trimStr(char* str);
void appendText(const char* pSourceTxt, char* pTargetTxt, bool bAppendToEnd = TRUE, bool bAppendSpace = FALSE);

int posChar(const char* pTxt, char c, bool bReverse = FALSE);
int hasString(const char* pTxt, const char* pStr, bool bCaseSensitive = true);
bool endsWith(const char* pWord, const char* pSubString);
bool startsWith(const char* pWord, const char* pSubString, bool bCaseSensitive = true);
bool isNumber(const char* str);
void replaceChar(char* pstr, char c, char c_replacement);
bool replaceNumberWith(const char* pIn, char* pOut, char* pReplacement);

int tokenizeStr(const char* pStr, char pArray[100][100], char cSep);

void left(const char* pStr, int iPos, char* pLeftStr);

bool searchInText(const char* pText, const char* pQuery);
int searchPosOfText(const char* pText, const char* pQuery);

/*************/
int getLevenshteinDistance(char const *s, char const *t);

int Minimum (int a, int b, int c);
int *GetCellPointer (int *pOrigin, int col, int row, int nCols);
int GetAt (int *pOrigin, int col, int row, int nCols);
void PutAt (int *pOrigin, int col, int row, int nCols, int x);
/*************/

void orderText(char pText[100][100], int iCounter);

string encodeNewLine(string line);
string decodeNewLine(string line);

#endif
