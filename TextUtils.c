#include "TextUtils.h"

#include <fstream>
#include <sys/dir.h>
#include <sys/types.h>
#include <sys/param.h>
#include <time.h>
#include <syslog.h>

static int str_alloc_count = 0;
static size_t str_alloc_size = 0;

int stricmp (const char *s, const char *t)
{
	int d = 0;
	do {
		d = toupper(*s) - toupper(*t);
	} while (*s++ && *t++ && !d);
	return (d);
}

int strincmp (const char *s, const char *t, int n)
{
	int k = 0;
	int d = 0;
	do {
		d = toupper(*s) - toupper(*t);
		k++;
		if( k == n ) return (d);
	} while (*s++ && *t++ && !d);
	return (d);
}

char *pValidateTxt = NULL;
int iValidateTxtLength = 0;

string validateStr(string txt)
{
	int iLen = txt.length();
	if( iLen == 0 ) return "";

	if( iValidateTxtLength < iLen+1 )
	{
		if( iValidateTxtLength == 0 )
			pValidateTxt = (char*)calloc(iLen+1, sizeof(char));
		else
			pValidateTxt = (char*)realloc(pValidateTxt, sizeof(char)*(iLen+1));
		iValidateTxtLength = iLen+1;
	}

	int j = 0;
	int p = 0;
	bool bCDATASeen = FALSE, bEmptyStart = TRUE;
	for(int i = 0 ; i < iLen; i++ )
	{
		if( bEmptyStart && txt[i] != ' ' && txt[i] != '\t' )
			bEmptyStart = FALSE;

		if( i+8 < iLen &&
				txt[i] == '<' &&  txt[i+1] == '!' &&
				txt[i+2] == '[' &&  txt[i+3] == 'C' &&
				txt[i+4] == 'D' &&  txt[i+5] == 'A' &&
				txt[i+6] == 'T' &&  txt[i+7] == 'A' && txt[i+8] == '[')
		{
			p = 9;
			bCDATASeen = TRUE;
		}
		else if(  bCDATASeen && i+2 < iLen &&
				txt[i] == ']' && txt[i+1] == ']' && txt[i+2] == '>' )
		{
			p = 3;
		}

		if( p == 0 )
		{
			if( !bEmptyStart && txt[i] != '\r' )
				pValidateTxt[j++] = txt[i];
		}
		else
		{
			p -= 1;
			if( j == 0 ) bEmptyStart = TRUE;
		}
	}

	if( pValidateTxt[j-1] == ' ' || pValidateTxt[j-1] == '\t' || pValidateTxt[j-1] == '\n' || pValidateTxt[j-1] == '\r' || (int)pValidateTxt[j-1] < 0 )
	{
		// check for suffix empty chars
		int k = 0;
		for(k = j-1; k >= 0; k-- )
		{
			if( pValidateTxt[k] == ' ' || pValidateTxt[j-1] == '\t' || pValidateTxt[j-1] == '\n' || pValidateTxt[j-1] == '\r' || (int)pValidateTxt[j-1] < 0 )
				pValidateTxt[k] = '\0';
			else
				break;
		}
	}
	else
		pValidateTxt[j] = '\0';

	string stripped(pValidateTxt);
	return stripped;
}

char* ptxt = NULL;
int iPTxtLength = 0;

string StripHTMLCodes(string txt)
{
	// check whether there is any tag
	if( txt.find("<") == -1 && txt.find("&gt;") == -1 && txt.find("&lt;") == -1 ) return txt;

	int j = 0;
	int iLen = txt.length();
	if( iPTxtLength < iLen )
	{
		if( iPTxtLength == 0 )
			ptxt = (char*)calloc(iLen, sizeof(char));
		else
			ptxt = (char*)realloc(ptxt, sizeof(char)*iLen);
		iPTxtLength = iLen;
	}

	if( txt.find("&gt;") != -1 || txt.find("&lt;") != -1 )
	{
		// first convert these to < and >
		for(int i = 0 ; i < iLen; i++ )
		{
			if( i+3 < iLen && txt[i] == '&' && txt[i+1] == 'g' && txt[i+2] == 't' && txt[i+3] == ';' )
			{
				ptxt[j++] = '>';
				i += 3;
			}
			else if( i+3 < iLen && txt[i] == '&' && txt[i+1] == 'l' && txt[i+2] == 't' && txt[i+3] == ';' )
			{
				ptxt[j++] = '<';
				i += 3;
			}
			else
				ptxt[j++] = txt[i];
		}

		ptxt[j] = 0;
		txt = string(ptxt);
		iLen = txt.length();
		for(int i = 0; i < iLen; i++)
			ptxt[i] = 0;
	}

	j = 0;
	bool bExclude = FALSE;
	for(int i = 0 ; i < iLen; i++ )
	{
		if( txt[i] == '<' )
		{
			if( i+3 < iLen && txt[i+1] == '!' && txt[i+2] == '-' && txt[i+3] == '-' )
			{
				int iJump = -1;
				for(int k = i+4 ; k < iLen; k++ )
				{
					if( k+2 < iLen && txt[k] == '-' && txt[k+1] == '-' && txt[k+2] == '>' )
					{
						iJump = k+2;
						break;
					}
				}
				if( iJump != -1 )
				{
					i = iJump;
					continue;
				}
			}

			int iTagNameStarts = -1, iTagNameEnds = -1;
			int iTagEnds = -1;
			// tag starts?, find its matching end
			for(int k = i+1 ; k < iLen; k++ )
			{
				if( txt[k] != ' ' && txt[k] != '\t' && txt[k] != '/' && iTagNameStarts == -1 )
				{
					if( iTagNameEnds == -1 )
						iTagNameStarts = k;
				}

				if( txt[k] == '>' )
				{
					if( iTagNameEnds == -1 ) iTagNameEnds = k;
					iTagEnds = k;

					if( iTagNameStarts == -1 )
					{
						// wierd; empty < >; skip this
						i = k;
					}

					break;
				}
				else if( txt[k] == ' ' || txt[k] == '\t' || txt[k] == '\r' )
				{
					if( iTagNameEnds == -1 ) iTagNameEnds = k;
				}
			}

			if( iTagNameStarts != -1 && iTagNameEnds != -1  && iTagEnds != -1 )
			{
				// get the tag name
				char cTagName[10000];
				for(int t = iTagNameStarts ; t <iTagNameEnds; t++ )
					cTagName[t-iTagNameStarts] = txt[t];

				if( cTagName[iTagNameEnds-iTagNameStarts-1] == '/' )
					cTagName[iTagNameEnds-iTagNameStarts-1] = 0;
				else
					cTagName[iTagNameEnds-iTagNameStarts] = 0;

				int ii = strlen(cTagName);
				if( ii == 1 )
				{
					if( stricmp(cTagName, "a") == 0 || stricmp(cTagName, "i") == 0 || stricmp(cTagName, "u") == 0 ||
							stricmp(cTagName, "b") == 0 || stricmp(cTagName, "b") == 0 || stricmp(cTagName, "s") == 0 ||
							stricmp(cTagName, "p") == 0 || stricmp(cTagName, "t") == 0 || stricmp(cTagName, "q") == 0 )
					{
						i = iTagEnds;
						continue;
					}
					else
					{
						cout << "Invalid Tag(1)? : " << cTagName << endl;
					}
				}
				else if( ii == 2 )
				{
					if( stricmp(cTagName, "br") == 0 || stricmp(cTagName, "em") == 0 ||
							stricmp(cTagName, "ol") == 0 || stricmp(cTagName, "li") == 0 ||
							stricmp(cTagName, "ul") == 0 || stricmp(cTagName, "hr") == 0 ||
							stricmp(cTagName, "td") == 0 || stricmp(cTagName, "tr") == 0 ||
							stricmp(cTagName, "dt") == 0 || stricmp(cTagName, "h1") == 0 ||
							stricmp(cTagName, "h2") == 0 || stricmp(cTagName, "h3") == 0 ||
							stricmp(cTagName, "h4") == 0 || stricmp(cTagName, "h5") == 0 ||
							stricmp(cTagName, "h6") == 0 || stricmp(cTagName, "tt") == 0 ||
							stricmp(cTagName, "dl") == 0 || stricmp(cTagName, "dd") == 0 ||
							stricmp(cTagName, "tl") == 0 || stricmp(cTagName, "th") == 0 )
					{
						i = iTagEnds;
						continue;
					}
					else
					{
						cout << "Invalid Tag(2)? : " << cTagName << endl;
					}
				}
				else if( ii == 3 )
				{
					if( stricmp(cTagName, "img") == 0 || stricmp(cTagName, "div") == 0 ||
							stricmp(cTagName, "pre") == 0 || stricmp(cTagName, "div") == 0 ||
							stricmp(cTagName, "big") == 0 || stricmp(cTagName, "col") == 0 ||
							stricmp(cTagName, "map") == 0 || stricmp(cTagName, "wbr") == 0 ||
							stricmp(cTagName, "o:p") == 0 || stricmp(cTagName, "sup") == 0 ||
							stricmp(cTagName, "sub") == 0 || stricmp(cTagName, "sup") == 0 )
					{
						i = iTagEnds;
						continue;
					}
					else
					{
						cout << "Invalid Tag(3)? : " << cTagName << endl;
					}
				}
				else if( ii == 4 )
				{
					if( stricmp(cTagName, "html") == 0 || stricmp(cTagName, "body") == 0 ||
							stricmp(cTagName, "span") == 0 || stricmp(cTagName, "font") == 0 ||
							stricmp(cTagName, "nobr") == 0 || stricmp(cTagName, "cite") == 0 ||
							stricmp(cTagName, "form") == 0 || stricmp(cTagName, "?xml") == 0 ||
							stricmp(cTagName, "code") == 0 || stricmp(cTagName, "area") == 0 ||
							stricmp(cTagName, "head") == 0 || stricmp(cTagName, "link") == 0 ||
							stricmp(cTagName, "bold") == 0 || stricmp(cTagName, "meta") == 0 )
					{
						i = iTagEnds;
						continue;
					}
					else
					{
						cout << "Invalid Tag(4)? : " << cTagName << endl;
					}
				}
				else
				{
					if( stricmp(cTagName, "script") == 0 || stricmp(cTagName, "blockquote") == 0 ||
							stricmp(cTagName, "object") == 0 || stricmp(cTagName, "embed") == 0 ||
							stricmp(cTagName, "param") == 0 || stricmp(cTagName, "blink") == 0 ||
							stricmp(cTagName, "strong") == 0 || stricmp(cTagName, "table") == 0 ||
							stricmp(cTagName, "tbody") == 0 || stricmp(cTagName, "strong") == 0 ||
							stricmp(cTagName, "center") == 0 || stricmp(cTagName, "style") == 0 ||
							stricmp(cTagName, "strike") == 0 || stricmp(cTagName, "nolayer") == 0 ||
							stricmp(cTagName, "quote") == 0 || stricmp(cTagName, "iframe") == 0 ||
							stricmp(cTagName, "noscript") == 0 || stricmp(cTagName, "small") == 0 ||
							stricmp(cTagName, "content") == 0 || stricmp(cTagName, "option") == 0 ||
							stricmp(cTagName, "input") == 0 || stricmp(cTagName, "select") == 0 ||
							stricmp(cTagName, "textarea") == 0 || stricmp(cTagName, "title") == 0 ||
							stricmp(cTagName, "marquee") == 0 || stricmp(cTagName, "caption") == 0 ||
							stricmp(cTagName, "insert") == 0 || stricmp(cTagName, "footnote") == 0 ||
							stricmp(cTagName, "htmlcode") == 0 || stricmp(cTagName, "blockqoute") == 0 ||
							stricmp(cTagName, "bgsound") == 0 || stricmp(cTagName, "legend") == 0 ||
							stricmp(cTagName, "fieldset") == 0 || stricmp(cTagName, "thread") == 0 )
					{
						i = iTagEnds;
						continue;
					}
					else
					{
						cout << "Invalid Tag? : " << cTagName << endl;
					}
				}
			}
		}
		ptxt[j++] = txt[i];
	}
	ptxt[j] = '\0';
	string stripped(ptxt);
	return stripped;
}

string normalizeHTMLStr(string txt)
{
	if( txt.find("&amp;") != -1 )
		txt = replaceStr(txt, "&amp;", "&");

	// check whether there is any tag
	if( txt.find("&#60;") != -1 )
	{
		if( txt.find("&#60;img") != -1 ||
				txt.find("&#60;a") != -1 ||
				txt.find("&#60;p") != -1 ||
				txt.find("&#60;i") != -1 ||
				txt.find("&#60;u") != -1 ||
				txt.find("&#60;h") != -1 ||
				txt.find("&#60;b") != -1 ||
				txt.find("&#60;strong") != -1 ||
				txt.find("&#60;IMG") != -1 ||
				txt.find("&#60;STONG") != -1 ||
				txt.find("&#60;A") != -1 ||
				txt.find("&#60;P") != -1 ||
				txt.find("&#60;I") != -1 ||
				txt.find("&#60;U") != -1 ||
				txt.find("&#60;H") != -1 ||
				txt.find("&#60;B") != -1 )
			return replaceStr(txt, "&#60;", "<");
	}

	if( txt.find("&lt;") != -1 )
	{
		if( txt.find("&lt;img") != -1 ||
				txt.find("&lt;a") != -1 ||
				txt.find("&lt;p") != -1 ||
				txt.find("&lt;i") != -1 ||
				txt.find("&lt;u") != -1 ||
				txt.find("&lt;h") != -1 ||
				txt.find("&lt;b") != -1 ||
				txt.find("&lt;strong") != -1 ||
				txt.find("&lt;IMG") != -1 ||
				txt.find("&lt;STRONG") != -1 ||
				txt.find("&lt;A") != -1 ||
				txt.find("&lt;P") != -1 ||
				txt.find("&lt;I") != -1 ||
				txt.find("&lt;U") != -1 ||
				txt.find("&lt;H") != -1 ||
				txt.find("&lt;B") != -1 )
		{
			string str = replaceStr(txt, "&lt;", "<");
			txt = replaceStr(str, "&gt;", ">");
			return txt;
		}
	}
	return txt;
}

bool isallalpha(const char* pc)
{
	int len = strlen(pc);
	for(int i = 0; i < len; i++ )
	{
		if( !((pc[i] >= 'a' && pc[i] <= 'z') || (pc[i] >= 'A' && pc[i] <= 'Z')) )
			return FALSE;
	}
	return TRUE;
}

bool isallnum(const char* pc)
{
	int len = strlen(pc);
	for(int i = 0; i < len; i++ )
	{
		if( !(pc[i] >= '0' && pc[i] <= '9') )
			return FALSE;
	}
	return TRUE;
}

bool isalpha(const char c)
{
	if( (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') )
		return TRUE;
	return FALSE;
}

bool isalphanum(const char c)
{
	if( (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') )
		return TRUE;
	return FALSE;
}

bool isnum(const char c)
{
	if( c >= '0' && c <= '9' )
		return TRUE;
	return FALSE;
}

bool iswhite(const char c)
{
	if( c == ' ' || c == '\t' || c == '\r' || c == '\n' )
		return TRUE;
	return FALSE;
}

string num2str(int i)
{
	char cNum[100];
	sprintf(cNum, "%d", i);
	return string(cNum);
}

string float2str(float f)
{
	char cNum[100];
	sprintf(cNum, "%f", f);
	return string(cNum);
}

string long2str(long l)
{
	char cNum[100];
	sprintf(cNum, "%ld", l);
	return string(cNum);
}

string replaceStr(string txt, string s, string sReplacement, bool bReplaceAll)
{
	int j = 0;
	int is = s.length();
	int t = txt.length();

	bool bFirstMatch = false;
	char* ptxt = (char*)calloc(txt.length()*6+1, sizeof(char));
	for(int i = 0; i < t; i++ ){
		bool bMatch = TRUE;
		for(int k = 0; k < is; k++ ){
			if( i+k >= t || txt[i+k] != s[k] ){
				bMatch = FALSE;
				break;
			}
		}

		if( bMatch && (bFirstMatch == false || bReplaceAll) ){
			for(int k = 0; k < sReplacement.length() ; k++ )
				ptxt[j++] = sReplacement[k];
			i += is-1;
			bFirstMatch = true;
		}
		else{
			ptxt[j++] = txt[i];
		}
	}

	ptxt[j] = 0;
	string sTxt(ptxt);
	free(ptxt);
	return sTxt;
}

void moveStr(char* pTarget, const char* pSource, int iLength, bool bKeepAll)
{
	int i;
	for(i = 0; i < iLength; i++ )
	{
		if( bKeepAll ||  (pSource[i] != '\n' && pSource[i] != '\r') )
			pTarget[i] = pSource[i];
		else
			pTarget[i] = ' ';
	}

	pTarget[i] = '\0';
}

void deEmptyStr(char* pstr, bool bDe)
{
	if( bDe )
	{
		if( pstr[0] == '\0' )
		{
			pstr[0] = '#';
			pstr[1] = '\0';
		}
	}
	else
	{
		if( pstr[0] == '#' && pstr[1] == '\0' )
			pstr[0] = '\0';
	}
}
void reallocStr( char** strP, size_t* maxsizeP, size_t size )
{
	if ( *maxsizeP == 0 ){
		*maxsizeP = MAX( 200, size + 100 );
		*strP = NEW( char, *maxsizeP + 1 );
		*strP[0] = '\0';
		++str_alloc_count;
		str_alloc_size += *maxsizeP;
	}
	else if ( size > *maxsizeP ){
		str_alloc_size -= *maxsizeP;
		*maxsizeP = MAX( *maxsizeP * 2, size * 5 / 4 );
		*strP = RENEW( *strP, char, *maxsizeP + 1 );
		str_alloc_size += *maxsizeP;
	}
	else
		return;

	if ( *strP == 0 ){
		syslog(LOG_ERR, "out of memory reallocating a string to %d bytes", *maxsizeP );
		exit( 1 );
	}
}

void decodeStr(char* buf, bool bLink)
{
	const char *r = buf;
	while( *r )
		switch( *r )
		{
		case '%':
			/* Is this really a 2-digit hex string? */
			if( isxdigit(r[1]) && isxdigit(r[2]) )
			{
				char hex[3];
				int i;
				hex[0] = *++r;
				hex[1] = *++r;
				hex[2] = 0;
				++r;
				sscanf( hex, "%x", &i );
				*buf++ = i;
				break;
			}
			/* no break, follow through */
		case '+':
		{
			if( bLink )
			{
				*buf++ = *r++;
			}
			else
			{
				*r++;
				*buf++ = ' ';
			}
			break;
		}
		default:
			*buf++ = *r++;
			/* no break */
		}
	*buf = 0;
}

void trimStr(char* pStr)
{
	char cTrimmed[1000];
	int iLen = strlen(pStr), j = 0, i = 0;
	bool bLeftTrimmed = FALSE;
	int iLastSpaceAt = -1;

	for(i = iLen-1 ;i>=0; i-- )
	{
		if( pStr[i] == ' ' || pStr[i] == '\n' || pStr[i] == '\r' || pStr[i] == '\t' )
			iLastSpaceAt = i;
		else
			break;
	}

	if( iLastSpaceAt == -1 )
	{
		iLastSpaceAt = iLen;
		//return;
	}
	for(i = 0 ;i < iLen && i < iLastSpaceAt; i++ )
	{
		if( bLeftTrimmed == FALSE && pStr[i] != ' ' )
		{
			bLeftTrimmed = TRUE;
			cTrimmed[j++] = pStr[i];
			cTrimmed[j] = '\0';
		}
		else if( bLeftTrimmed )
		{
			cTrimmed[j++] = pStr[i];
			cTrimmed[j] = '\0';
		}
	}
	if( strlen(cTrimmed) < iLen )
	{
		for(i = 0; i < strlen(cTrimmed); i++ )
			pStr[i] = cTrimmed[i];
		pStr[i] = '\0';
	}
}

void appendText(const char* pSourceTxt, char* pTargetTxt, bool bAppendToEnd, bool bAppendSpaceBetween)
{
	int iSourceLen = strlen(pSourceTxt);
	int iTargetLen = strlen(pTargetTxt);

	if( bAppendToEnd )
	{
		if( bAppendSpaceBetween && iTargetLen > 0 )
			pTargetTxt[iTargetLen++] = ' ';
		for(int i = 0; i < iSourceLen; i++ )
			pTargetTxt[iTargetLen+i] = pSourceTxt[i];
	}
	else
	{
		int j = bAppendSpaceBetween?1:0;
		for(int i = iTargetLen-1; i >= 0 ; i-- )
			pTargetTxt[i+iSourceLen+j] = pTargetTxt[i];

		if( bAppendSpaceBetween && pTargetTxt[0] != '\0' )
		{
			pTargetTxt[iSourceLen] = ' ';
			iTargetLen++;
		}
		for(int i = 0; i < iSourceLen; i++ )
			pTargetTxt[i] = pSourceTxt[i];
	}
	pTargetTxt[iTargetLen+iSourceLen] = '\0';
}

void lowerStr(char* s)
{
	int iLen = strlen(s);
	for(int i = 0; i < iLen; i++ ){
		if( s[i] >= 'A' && s[i] <= 'Z' )
			s[i] = tolower(s[i]);
	}
}

void upperStr(char* s)
{
	int iLen = strlen(s);
	for(int i = 0; i < iLen; i++ ){
		if( s[i] >= 'a' && s[i] <= 'z' )
			s[i] = toupper(s[i]);
	}
}

int hasString(const char* pTxt, const char* pStr, bool bCaseSensitive)
{
	int iLen = strlen(pTxt);
	int iLen2 = strlen(pStr);

	for(int i = 0 ; i < iLen; i++ ){
		if( (bCaseSensitive && pTxt[i] == pStr[0]) || (bCaseSensitive==false && tolower(pTxt[i]) == tolower(pStr[0])) ){
			int j = 0;
			for(j = 0; j < iLen2; j++ ){
				if( (bCaseSensitive && pTxt[i+j] != pStr[j]) || (bCaseSensitive==false && tolower(pTxt[i+j]) != tolower(pStr[j])) )
					break;
			}
			if( j == iLen2 )
				return i;
		}
	}
	return -1;
}

int posChar(const char* pTxt, char c, bool bReverse)
{
	int iLen = strlen(pTxt);
	if( bReverse ){
		for(int i = iLen-1 ; i >= 0; i-- )
			if( pTxt[i] == c )
				return i;
	}
	else{
		for(int i =0 ; i < iLen; i++ )
			if( pTxt[i] == c )
				return i;
	}
	return -1;
}

bool endsWith(const char* pWord, const char* pSubString)
{
	int iLen = strlen(pWord);
	int iSubLen = strlen(pSubString);
	if( iLen > iSubLen )
	{
		int j = 0;
		for(int i = iLen-1; i >= 0; i-- )
		{
			if( pWord[i] == pSubString[iSubLen-(j+1)] )
			{
				if( iSubLen == j+1 )
					return TRUE;
			}
			else
				return FALSE;
			j++;
			if( j == iSubLen )
				return FALSE;
		}
	}
	return FALSE;
}

bool startsWith(const char* pWord, const char* pSubString, bool bCaseSensitive)
{
	return (hasString(pWord, pSubString, bCaseSensitive) == 0);
}

bool isNumber(const char* str)
{
	int iLen = strlen(str);
	for(int i = 0; i < iLen; i++)
	{
		if( i == 0 && str[0] == '-' && iLen > 1 )
			continue;

		if( !(str[i] >= '0' && str[i] <= '9') )
		{
			if( str[i] != ',' && str[i] != '.' )
				return FALSE;
		}
	}

	if( iLen == 0 || (iLen == 1 && (str[0] == ',' || str[0] == '.')) )
		return FALSE;

	return TRUE;
}

void replaceChar(char* pstr, char c, char c_replacement)
{
	int iLen = strlen(pstr);
	for(int i = 0; i < iLen; i++ )
	{
		if( pstr[i] == c )
			pstr[i] = c_replacement;
	}
}

bool isEmpty(char* pStr)
{
	if( pStr )
		return (pStr[0]=='\0');
	return TRUE;
}

/*********************************************/


int Minimum (int a, int b, int c)
{
	int mi = a;
	if( b < mi )
		mi = b;
	if( c < mi )
		mi = c;
	return mi;
}

int* GetCellPointer (int *pOrigin, int col, int row, int nCols)
{
	return pOrigin + col + (row * (nCols + 1));
}

int GetAt (int *pOrigin, int col, int row, int nCols)
{
	int *pCell;
	pCell = GetCellPointer (pOrigin, col, row, nCols);
	return *pCell;
}

void PutAt (int *pOrigin, int col, int row, int nCols, int x)
{
	int *pCell;
	pCell = GetCellPointer (pOrigin, col, row, nCols);
	*pCell = x;
}

int g_iLevenshteinMatrix[50000]; // 45x45 words?
int getLevenshteinDistance(char const *s, char const *t)
{
	int *d; // pointer to matrix
	int i; // iterates through s
	int j; // iterates through t
	char s_i; // ith character of s
	char t_j; // jth character of t
	int cost; // cost
	int result; // result
	int cell; // contents of target cell
	int above; // contents of cell immediately above
	int left; // contents of cell immediately to left
	int diag; // contents of cell immediately above and to left

	// Step 1

	int n = strlen (s);
	int m = strlen (t);
	if( n == 0 )
		return m;
	if( m == 0 )
		return n;

	int sz = (n+1) * (m+1) * sizeof (int);

	// Step 2
	for (i = 0; i <= n; i++)
		PutAt (g_iLevenshteinMatrix, i, 0, n, i);

	for (j = 0; j <= m; j++)
		PutAt (g_iLevenshteinMatrix, 0, j, n, j);

	// Step 3
	for (i = 1; i <= n; i++)
	{
		s_i = s[i-1];

		// Step 4
		for (j = 1; j <= m; j++)
		{
			t_j = t[j-1];

			// Step 5
			if (s_i == t_j)
				cost = 0;
			else
				cost = 1;

			// Step 6

			above = GetAt (g_iLevenshteinMatrix,i-1,j, n);
			left = GetAt (g_iLevenshteinMatrix,i, j-1, n);
			diag = GetAt (g_iLevenshteinMatrix, i-1,j-1, n);
			cell = Minimum (above + 1, left + 1, diag + cost);
			PutAt (g_iLevenshteinMatrix, i, j, n, cell);
		}
	}

	// Step 7
	result = GetAt (g_iLevenshteinMatrix, n, m, n);
	//printf("s=%s t=%s dist=%d\n", s, t, result);
	return result;
}

int tokenizeStr(const char* pStr, char pArray[100][100], char cSep)
{
	int iIndex = 0, j = 0;
	if( pStr[0] == '\0' )
		return 0;

	int len = strlen(pStr);
	for(int i = 0; i < len && iIndex < 99; i++ ){
		if( pStr[i] == cSep || j == 99 ){
			if( j > 0 ){
				pArray[iIndex][j] = '\0';
				iIndex++;
			}
			j = 0;
		}
		else
			pArray[iIndex][j++] = pStr[i];
	}

	if( j > 0 ){
		pArray[iIndex][j] = '\0';
		iIndex++;
	}

	return iIndex;

#if 0
	int iIndex = 0, j = 0;
	if( pStr[0] == '\0' )
		return 0;

	for(int i = 0; i < 100; i++ )
		pArray[i][0] = '\0';

	for(int i = 0; i < strlen(pStr) && iIndex < 99; i++ ){
		if( pStr[i] == cSep || j == 99 ){
			if( j > 0 )
				iIndex++;
			j = 0;
		}
		else{
			pArray[iIndex][j++] = pStr[i];
			pArray[iIndex][j] = '\0';
		}}
	if( pStr[strlen(pStr)-1] == cSep )
		return iIndex;
	return iIndex+1;
#endif
}

void left(const char* pStr, int iPos, char* pLeftStr)
{
	int j = 0;
	int iLen = strlen(pStr);
	for(int i = iPos; i < iLen; i++ )
		pLeftStr[j++] = pStr[i];
	pLeftStr[j] = '\0';
}

void shift(char* pStr, int iShiftCount/*=1*/, int iStrLen/*=0*/)
{
	if( iStrLen == 0 )
		iStrLen = strlen(pStr);
	for(int i = 0; i < iStrLen-iShiftCount; i++ )
		pStr[i] = pStr[i+iShiftCount];
	pStr[iStrLen-iShiftCount] = '\0';
}

bool isAllLowerCase(char* pStr)
{
	for(int i = 0; i < strlen(pStr); i++ ){
		if( pStr[i] != tolower(pStr[i]) )
			return FALSE;
	}
	return TRUE;
}

bool isAllUpperCase(char* pStr)
{
	for(int i = 0; i < strlen(pStr); i++ ){
		if( pStr[i] != toupper(pStr[i]) )
			return FALSE;
	}
	return TRUE;
}

bool isQuoted(const char* pStr)
{
	if( pStr[0] == '\"' && pStr[strlen(pStr)-1] == '\"' )
		return TRUE;
	return FALSE;
}

void unQuote(const char* pStr, char* pUnQuotedStr)
{
	for(int i = 1; i < strlen(pStr)-1; i++ ){
		pUnQuotedStr[i-1] = pStr[i];
		pUnQuotedStr[i] = '\0';
	}
}

int findChar(const char* pStr, char c, int iStart)
{
	for(int i = iStart; i < strlen(pStr); i++ ){
		if( pStr[i] == c )
			return i;
	}
	return -1;
}

//#define FIX_21

bool replaceNumberWith(const char* pIn, char* pOut, char* pReplacement)
{
	int sLen = strlen(pIn);
	pOut[0] = '\0';
	int k = 0;
	bool bNumber = false;
	bool bAtLeastOne = false;
	for(int i = 0; i < sLen; i++){
		if( pIn[i] >= '0' && pIn[i] <= '9' ){
			bNumber = true;
		}
		else{
			if( bNumber ){
				sprintf(pOut, "%s%s", pOut, pReplacement);
				bNumber = false;
				bAtLeastOne = true;
				k = strlen(pOut);
			}
			pOut[k++] = pIn[i];
			pOut[k] = '\0';
		}}

	if(  bNumber ){
		sprintf(pOut, "%s%s", pOut, pReplacement);
		bAtLeastOne = true;
	}

#ifdef FIX_21
	char cPartialNumber[1024];
	sprintf(cPartialNumber, "%s/%s", pReplacement, pReplacement);
	if( strcmp(pOut, cPartialNumber) == 0 )
		strcpy(pOut, pReplacement);
	else{
		sprintf(cPartialNumber, "%s:%s", pReplacement, pReplacement);
		if( strcmp(pOut, cPartialNumber) == 0 ){
			strcpy(pOut, pReplacement);
		}
		else{
			sprintf(cPartialNumber, "%s-%s", pReplacement, pReplacement);
			if( strcmp(pOut, cPartialNumber) == 0 ){
				strcpy(pOut, pReplacement);
			}
			else{
				sprintf(cPartialNumber, "%s-for-%s", pReplacement, pReplacement);
				if( strcmp(pOut, cPartialNumber) == 0 ){
					strcpy(pOut, pReplacement);
				}
				else{
					sprintf(cPartialNumber, "%s-to-%s", pReplacement, pReplacement);
					if( strcmp(pOut, cPartialNumber) == 0 ){
						strcpy(pOut, pReplacement);
					}
					else{
						sprintf(cPartialNumber, "%s\\/%s", pReplacement, pReplacement);
						if( strcmp(pOut, cPartialNumber) == 0 ){
							strcpy(pOut, pReplacement);
						}
					}
				}
			}
		}
	}
#endif
	return bAtLeastOne;
}

bool searchInText(const char* pText, const char* pQuery)
{
	int l = strlen(pQuery);
	bool bMatchTillEnd = false;
	if( pQuery[l-1] == '$' ){
		bMatchTillEnd = true;
		l--;
	}
	int iStart = 0;
	bool bStartAtBegining = false;
	if( pQuery[0] == '^' ){
		bStartAtBegining = true;
		iStart = 1;
		l--;
	}

	int len = strlen(pText);
	for(int i = 0; i <= len-l; i++){
		if( tolower(pText[i]) == tolower(pQuery[iStart]) && (l == 1 || tolower(pText[i+1]) == tolower(pQuery[iStart+1])) ){
			bool bMatch = true;
			for(int k = i+2; k < i+l; k++){
				if( tolower(pText[k]) != tolower(pQuery[iStart+k-i]) ){
					bMatch = false;
					break;
				}
			}
			if( bMatch ){
				if( bMatchTillEnd ){
					if( i+l == len )
						return true;
					return false;
				}
				return true;
			}
		}
		if( bStartAtBegining )
			return false;
	}
	return false;
}

int searchPosOfText(const char* pText, const char* pQuery)
{
	int l = strlen(pQuery);
	bool bMatchTillEnd = false;
	if( pQuery[l-1] == '$' ){
		bMatchTillEnd = true;
		l--;
	}
	int iStart = 0;
	bool bStartAtBegining = false;
	if( pQuery[0] == '^' ){
		bStartAtBegining = true;
		iStart = 1;
		l--;
	}

	int len = strlen(pText);
	for(int i = 0; i <= len-l; i++){
		if( tolower(pText[i]) == tolower(pQuery[iStart]) && (l == 1 || tolower(pText[i+1]) == tolower(pQuery[iStart+1])) ){
			bool bMatch = true;
			for(int k = i+2; k < i+l; k++){
				if( tolower(pText[k]) != tolower(pQuery[iStart+k-i]) ){
					bMatch = false;
					break;
				}
			}
			if( bMatch ){
				if( bMatchTillEnd ){
					if( i+l == len )
						return i;
					return -1;
				}
				return i;
			}
		}
		if( bStartAtBegining )
			return -1;
	}
	return -1;
}

string encodeNewLine(string line)
{
	char cLine[10000]; int k = 0;
	for(int i = 0; i < strlen(line.c_str()); i++){
		if( line[i] == '\n' ){
			cLine[k++] = '_';
			cLine[k++] = '_';
			cLine[k++] = 'n';
			cLine[k++] = 'l';
			cLine[k++] = '_';
			cLine[k++] = '_';
		}
		else
			cLine[k++] = line[i];
	}
	cLine[k] = '\0';
	return (string)cLine;
}

string decodeNewLine(string line)
{
	char cLine[10000]; int k = 0;
	int len = strlen(line.c_str());
	for(int i = 0; i < len; i++){
		if( i+5 < len && line[i] == '_' && line[i+1] == '_' && line[i+2] == 'n' && line[i+3] == 'l' && line[i+4] == '_' && line[i+5] == '_' ){
			cLine[k++] = '\n';
			i += 5;
		}
		else
			cLine[k++] = line[i];
	}
	cLine[k] = '\0';
	return (string)cLine;
}

void orderText(char pText[100][100], int iCounter)
{
	for (int i = 1; i < iCounter; i++) {
		for (int j = 1; j < iCounter; j++) {
			if (strcmp(pText[j - 1], pText[j]) > 0) {
				char tmp[100];
				strcpy(tmp, pText[j-1]);
				strcpy(pText[j-1], pText[j]);
				strcpy(pText[j], tmp);
			}}}
}
