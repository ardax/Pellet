#include "DBUtils.h"
#include "Utils.h"
#include "TextUtils.h"

bool bStopWordsLoaded = FALSE;
map<string, bool> mapStopWords;

string prepQueryField(string sFieldValue, int iLimit)
{
	int j = 0, i = 0;
	int iLen = sFieldValue.length();
	char* ptxt = (char*)calloc(iLen*2, sizeof(char));

	for(i = 0; i < iLen; i++ )
	{
		if( sFieldValue[i] == '\'' && (i==0 || sFieldValue[i-1] != '\\') )
		{
			ptxt[j++] = '\\';
			ptxt[j++] = '\'';
		}
		else if( sFieldValue[i] == '"' && (i==0 || sFieldValue[i-1] != '\\') )
		{
			ptxt[j++] = '\\';
			ptxt[j++] = '"';
		}
		else
		{
			ptxt[j++] = sFieldValue[i];
		}

		if( iLimit != 0 && i == iLimit+3 )
			break;
	}

	if( i == iLimit+3 && iLimit != 0 )
	{
		ptxt[j++] = '.';
		ptxt[j++] = '.';
		ptxt[j++] = '.';
	}

	ptxt[j] = 0;
	string sTxt(ptxt);
	free(ptxt);
	return sTxt;
}

void prepQueryField_wChar(char* pFieldValue)
{
	int j = 0, i = 0;
	int iLen = strlen(pFieldValue);

	for(i = 0; i < iLen; i++ )
	{
		if( pFieldValue[i] == '\'' && (i==0 || pFieldValue[i-1] != '\\') )
		{
			pFieldValue[iLen+1] = '\0';
			for(int k = iLen-1; k >= i; k-- )
				pFieldValue[k+1] = pFieldValue[k];
			pFieldValue[i] = '\\';
			i++;
			iLen++;
		}
		else if( pFieldValue[i] == '"' && (i==0 || pFieldValue[i-1] != '\\') )
		{
			pFieldValue[iLen+1] = '\0';
			for(int k = iLen-1; k >= i; k-- )
				pFieldValue[k+1] = pFieldValue[k];
			pFieldValue[i] = '\\';
			i++;
			iLen++;
		}
	}
}

string fieldUpdated(string sQuery, string sFieldName, string sOldValue, string sNewValue, int iFieldSize)
{
	if( sOldValue != sNewValue )
	{
		string sReturn = sQuery;
		if( sQuery != "" )
			sReturn += ", ";

		//cout << sFieldName << " Compared\nOLD:" << sOldValue << "\nNEW:" << sNewValue << endl;

		sReturn += sFieldName + "='" + prepQueryField(sNewValue, iFieldSize) + "'";
		return sReturn;
	}
	return sQuery;
}

string fieldUpdatedInt(string sQuery, string sFieldName, int iOldValue, int iNewValue)
{
	if( iOldValue != iNewValue )
	{
		string sReturn = sQuery;
		if( sQuery != "" )
			sReturn += ", ";
		sReturn += sFieldName + "='" + num2str(iNewValue) + "'";
		return sReturn;
	}
	return sQuery;
}

void loadStopWords()
{
#if 0
	bStopWordsLoaded = TRUE;

	FILE* fp;
	if( (fp = fopen("/home/ardax/Projects/Feedzie/Scripts_v2/stopword.en.big", "r")) )
	{
		char line[1024];
		while( fgets(line, 1024, fp) )
		{
			int iLen = strlen(line);
			for(int i = 0; i < iLen; i++ )
			{
				if( line[i] == '\n' || line[i] == '\r' )
				{
					line[i] = '\0';
					break;
				}
			}

			string sWord(line);
			mapStopWords[sWord] = TRUE;
		}
		fclose(fp);
	}
#endif
}

bool isStopWord(string sWord)
{
	map<string, bool>::iterator iter;
	iter = mapStopWords.find(sWord);
	if( iter != mapStopWords.end() )
		return TRUE;
	return FALSE;
}

string prepSearchIndex(string txt, bool bUseJustWhiteSpace)
{
	string sListing = "";
	map<string, bool> mWords;

	if( !bStopWordsLoaded ) loadStopWords();

	char* pc = NULL;
	if( bUseJustWhiteSpace )
		pc = strtok((char*)txt.c_str(), " \t\n\r");
	else
		pc = strtok((char*)txt.c_str(), " \t\n\r,.()[]|@#$%^&*=-*+'\"{}/?><|:;’~`!/\\");


	while(pc)
	{
		if( strlen(pc) > 2 )
		{
			string sWord(pc);

			for(int i = 0; i < sWord.length(); i++ )
			{
				if( sWord[i] >= 'A' && sWord[i] <= 'Z' )
					sWord[i] = tolower(sWord[i]);
			}

			if( isStopWord(sWord) == FALSE && !isallnum(pc) )
			{
				if( strlen(pc) == 3 )
					sWord += "_";
				mWords[sWord] = TRUE;
			}
		}

		if( bUseJustWhiteSpace )
			pc = strtok(NULL, " \t\n\r");
		else
			pc = strtok(NULL, " \t\n\r,.()[]|@#$%^&*=-*+'\"{}?><|;:’~`!/\\");
	}
	map<string, bool>::iterator iter;
	for (iter = mWords.begin(); iter != mWords.end(); iter++)
	{
		string sWord = iter->first;
		sListing += sWord + " ";
	}

	return sListing;
}
