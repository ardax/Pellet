#ifndef _DBUTILS_
#define _DBUTILS_

#include <iostream>
#include <string>
#include <map>
#include <list>
using namespace std;

string prepQueryField(string sFieldValue, int iLimit);
void prepQueryField_wChar(char* pFieldValue);

string fieldUpdated(string sQuery, string sFieldName, string sOldValue, string sNewValue, int iFieldSize);
string fieldUpdatedInt(string sQuery, string sFieldName, int iOldValue, int iNewValue);

string prepSearchIndex(string txt, bool bUseJustWhiteSpace);

#endif
