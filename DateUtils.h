#ifndef _DATEUTILS_
#define _DATEUTILS_

#include "Utils.h"

#define SEC2MIN 60L
#define SEC2HOUR (60L*SEC2MIN)
#define SEC2DAY (24L*SEC2HOUR)

typedef struct tm TM;

/*
 *  days per month plus days/year
 */
static	int	dmsize[] =
{
	365, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};
static	int	ldmsize[] =
{
	366, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};


long tm2sec(struct tm *tm);
long getTodayInSeconds(bool bLocal = FALSE);
string tm2str(TM* pTM, bool bDateTime = TRUE);
int getMonth(string sMonth);
void getTimeZoneInfo(char *sTimeZoneStr, int *TZHour, int *TZMin, bool *bNegative);
long convertToDateTime(const char* pDate, TM* pTM);
int* yrsize(int y);
string getTodayDateTime();
void getTodayDate(char* pDate);
void getNowInRFCFormat(char* pBuffer, int iBufferSize, bool bLocal = TRUE);
unsigned long int tvdiff2ms(struct timeval *a, struct timeval *b);
void convertLong2DateTime(long lDate, char* pDate);
void convertLong2Date(long lDate, char* pDate);
void getNow(char* pBuffer, int iBufferSize, bool bLocal = TRUE);
bool startsAfter(tm* pTM1, tm* pTM2);
bool isSame(tm* pTM1, tm* pTM2);

#endif
