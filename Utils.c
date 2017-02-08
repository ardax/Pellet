#include "Utils.h"
#include "TextUtils.h"
#include <sys/stat.h>

bool g_bAbort = false;
bool g_bShutdown = false;
bool g_bTestMode = false;

FILE* g_pCOMMENTOUT = NULL;

int g_iCommentIndent = 0;
int parseDuration(const char* pDuration)
{
	if( isallnum(pDuration) )
		return atoi(pDuration);

	char* tmp = strdup(pDuration);
	char* pc = strtok(tmp, ":");
	int iFieldIndex = 0, iField1 = -1, iField2 = -1, iField3 = -1;
	while(pc)
	{
		if( iFieldIndex == 0 )
			iField1 = atoi(pc);
		else if( iFieldIndex == 1 )
			iField2 = atoi(pc);
		else if( iFieldIndex == 2 )
			iField3 = atoi(pc);

		iFieldIndex += 1;
		pc = strtok(NULL, ":");
	}

	free(tmp);
	if( iField1 != -1 && iField2 != -1 && iField3 != -1 )
		return 60*60*iField1 + 60*iField2 + iField3;
	else if( iField1 != -1 && iField2 != -1 && iField3 == -1 )
		return 60*iField1 + iField2;
	return 0;
}

string getSpaceInStr(long iBytes)
{
	char cStr[1000];

	if( iBytes > 1024*1024*1024 ){
		// in giga bytes
		float fGBs = (float)iBytes/((float)(1024*1024*1024));
		sprintf(cStr, "%.1f GBs", fGBs);
	}
	else if( iBytes > 1024*1024 ){
		// in mega bytes
		float fMBs = (float)iBytes/((float)(1024*1024));
		sprintf(cStr, "%.1f MBs", fMBs);
	}
	else if( iBytes > 1024 ){
		// in kilo bytes
		float fKBs = (float)iBytes/((float)(1024));
		sprintf(cStr, "%.1f KBs", fKBs);
	}
	else{
		sprintf(cStr, "%ld Bs", iBytes);
	}

	return (string)cStr;
}

bool isThereFolder(const char* pFolder)
{
	struct stat s;
	int err = stat(pFolder, &s);
	if( err == -1 ){
		return false;
	}
	else if( S_ISDIR(s.st_mode) )
		return true;

	return false;
}

string filterPipe(string txt)
{
	if( txt == "" ) return "#";
	else if( txt.find("|") == -1 )
		return txt;
	return replaceStr(txt, "|", "&pipe;");
}

unsigned long hash(const char *str, bool bCaseSensitive)
{
	unsigned long hash = 5381;
	int c;

	if( bCaseSensitive )
	{
		while (c = *str++)
		{
			hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
		}
	}
	else
	{
		while (c = tolower(*str++))
		{
			hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
		}
	}

	return hash;
}

bool isWebAddress(char* pc)
{
	int iLen = strlen(pc);
	if( iLen > 7 && pc[0] == 'h' )
	{
		if( strstr(pc, "http://") == pc || strstr(pc, "ftp://") == pc || strstr(pc, "www.") == pc )
		{
			return TRUE;
		}
	}
	return FALSE;
}

void getMaxIntsInInt2FloatMap(Int2FloatMap* pSource, Int2FloatMap* pTarget, int iMax)
{
	if( iMax == 1 )
	{
		int iMaxKey = 0;
		float fMaxValue = 0.0;

		Int2FloatMap::iterator iter;
		for(iter = pSource->begin(); iter != pSource->end(); iter++ )
		{
			int iKey = iter->first;
			float fValue = iter->second;

			if( fMaxValue < fValue )
			{
				fMaxValue = fValue;
				iMaxKey = iKey;
			}
		}

		if( iMaxKey != 0 )
			(*pTarget)[iMaxKey] = fMaxValue;
	}
	else
	{
		int iMaxKeys[100];
		float fMaxValues[100];
		for(int i =  0; i < 100; i++ )
		{
			iMaxKeys[i] = 0;
			fMaxValues[i] = 0;
		}

		Int2FloatMap::iterator iter;
		for(iter = pSource->begin(); iter != pSource->end(); iter++ )
		{
			int iKey = iter->first;
			float fValue = iter->second;

			for(int i = 0; i < iMax; i++ )
			{
				if( fMaxValues[i] < fValue )
				{
					if( iMaxKeys[i] > 0 )
					{
						for(int j = iMax-1; j > i; j-- )
						{
							fMaxValues[j] = fMaxValues[j-1];
							iMaxKeys[j] = iMaxKeys[j-1];
						}
					}
					fMaxValues[i] = fValue;
					iMaxKeys[i] = iKey;
					break;
				}
			}
		}

		for(int i = 0 ; i < iMax; i++ )
		{
			if( iMaxKeys[i] != 0 )
				(*pTarget)[iMaxKeys[i]] = fMaxValues[i];
		}
	}
}

int file_select(const struct direct *entry) 
{
	if ((strcmp(entry->d_name, ".") == 0) || (strcmp(entry->d_name, "..") == 0))
		return (FALSE);
	else
		return (TRUE);
}

void readFile(char* pFileName, char* pContent)
{
	FILE* pFile;
	// Load sentences
	if( (pFile = fopen(pFileName, "r")) ){
		char line[20000];
		while( fgets(line, 20000, pFile) )
		{
			if( line[strlen(line)-1] == '\n' )
				line[strlen(line)-1] = '\0';
			appendText(line, pContent, TRUE, TRUE);
		}
		fclose(pFile);
	}
}

float math_abs(float f1, float f2)
{
	if( f1 > f2 )
		return f1-f2;
	return f2-f1;
}

int math_abs(int i1, int i2)
{
	if( i1 > i2 )
		return i1-i2;
	return i2-i1;
}

/*************************************************************************/
/*************************************************************************/
/*************************************************************************/

void InputFile::reset()
{
	m_cInputFile[0] = '\0';
	m_cFileMD5[0] = '\0';
	m_lFileSize = 0;
	m_pFile = NULL;
	m_pNext = NULL;
}

bool InputFile::openFile()
{
	if( m_cInputFile[0] != '\0' ){
		m_pFile = fopen(m_cInputFile, "r");
		if( m_pFile ){
			//fprintf(stderr, "fileno(%s)=%d\n", m_cInputFile, fileno(m_pFile));
			return true;
		}
	}
	return false;
}

void InputFile::closeFile()
{
	if( m_pFile )
		fclose(m_pFile);
	m_pFile = NULL;
}

bool InputFile::getNextLine(char* pBuffer, int iBufferSize, bool bRemoveNewLine)
{
	if( fgets(pBuffer, iBufferSize, m_pFile) ){
		if( bRemoveNewLine && pBuffer[strlen(pBuffer)-1] == '\n' )
			pBuffer[strlen(pBuffer)-1] = '\0';
		return true;
	}
	return false;
}

int InputFile::getLineCount()
{
	int count = 0;
	FILE* pFile = fopen(m_cInputFile, "rb");
	if( pFile ){
		char cBuffer[1000];
		while( fgets(cBuffer, 1000, pFile) )
			count++;
		fclose(pFile);
	}
	return count;
}

double getTimeInMilliseconds()
{
	struct timeval  tv;
	gettimeofday(&tv, NULL);
	return (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ;
}

/*
double log2(double n)
{
    // log(n)/log(2) is log2.
	return ::log(n)/::log(2);
}
 */

void shuffleArray(int* arr, int size)
{
	srand(time(NULL));
	int n = size;
	while (n > 1){
		int k = rand()%n;
		n--;
		int temp = arr[n];
		arr[n] = arr[k];
		arr[k] = temp;
	}
}
