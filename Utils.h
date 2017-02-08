#ifndef _UTILS_
#define _UTILS_

#include <sys/dir.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <map>
#include <list>

using namespace std;

typedef int BOOL;
#ifndef TRUE
#define TRUE  1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#define NEW(t,n) ((t*) malloc( sizeof(t) * (n) ))
#define RENEW(o,t,n) ((t*) realloc( (void*) o, sizeof(t) * (n) ))


typedef list<string> strings;
typedef map<string, int> StringMap;
typedef map<int, int> IntMap;
typedef map<int, float> Int2FloatMap;
typedef list<float> floats;
typedef list<int> IntList;

typedef struct _InputFile_
{
	void 	reset();
	bool 	openFile();
	void	closeFile();
	bool 	getNextLine(char* pBuffer, int iBufferSize, bool bRemoveNewLine = true);
	int		getLineCount();

	char	m_cInputFile[1000];
	long	m_lFileSize;
	char	m_cFileMD5[100];
	FILE*	m_pFile;

	_InputFile_*	m_pNext;
} InputFile;

typedef struct _MongoDB_
{
	void	reset(){
		m_cAddress[0] = m_cDBName[0] = m_cUserName[0] = m_cPassword[0] = '\0';
		m_iPort = -1;
		m_iType = 0;
	}
	string getInJSON(){
		string json = "";
		json += "{";
		json += "\"db_name\": \""+(string)m_cDBName+"\", ";
		char cNum[100]; sprintf(cNum, "%d", m_iPort);
		json += "\"port\": \""+(string)cNum+"\", ";
		if( m_cUserName[0] != '\0' ){
			json += "\"username\": \""+(string)m_cUserName+"\", ";
			json += "\"password\": \""+(string)m_cPassword+"\", ";
		}
		json += "\"address\": \""+(string)m_cAddress+"\"";
		json += "}";
		return json;
	}

	char	m_cAddress[1000];
	int		m_iPort;
	char	m_cDBName[100];
	char	m_cUserName[100];
	char	m_cPassword[100];
	int		m_iType;
} MongoDB;


int parseDuration(const char* pDuration);
string filterPipe(string txt);
unsigned long hash(const char *str, bool bCaseSensitive);
bool isWebAddress(char* pc);
void getMaxIntsInInt2FloatMap(Int2FloatMap* pSource, Int2FloatMap* pTarget, int iMax);
int file_select(const struct direct *entry);
void readFile(char* pFileName, char* pContent);
float math_abs(float f1, float f2);
int math_abs(int i1, int i2);
//double log2(double n);
void shuffleArray(int* arr, int size);
double getTimeInMilliseconds();
bool isThereFolder(const char* pFolder);
string getSpaceInStr(long iBytes);

#endif
