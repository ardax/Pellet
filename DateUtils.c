#include "DateUtils.h"
#include "Utils.h"
#include "TextUtils.h"

#include <stdio.h>

int* yrsize(int y)
{
	if((y%4) == 0 && ((y%100) != 0 || (y%400) == 0))
		return ldmsize;
	else
		return dmsize;
}

long tm2sec(struct tm *tm)
{
	long secs;
	int i, yday, year, *d2m;

	secs = 0;

	if( tm->tm_year > 2100 || tm->tm_mon > 11 || tm->tm_mon < 0 || tm->tm_mday > 31 )
		return 0;

	if( tm->tm_year < 1900 )
		year = tm->tm_year + 1900;
	else
		year = tm->tm_year;
	/*
	 *  seconds per year
	 */
	for(i = 1970; i < year; i++){
		d2m = yrsize(i);
		secs += d2m[0] * SEC2DAY;
	}

	/*
	 *  if mday is set, use mon and mday to compute yday
	 */
	if(tm->tm_mday){
		yday = 0;
		d2m = yrsize(year);
		for(i=0; i<tm->tm_mon; i++)
			yday += d2m[i+1];
		yday += tm->tm_mday-1;
	}else{
		yday = tm->tm_yday;
	}
	secs += yday * SEC2DAY;

	/*
	 * hours, minutes, seconds
	 */
	secs += tm->tm_hour * SEC2HOUR;
	secs += tm->tm_min * SEC2MIN;
	secs += tm->tm_sec;

	/*
	 * Only handles zones mentioned in /env/timezone,
	 * but things get too ambiguous otherwise.
  if(strcmp(tm->zone, timezone.stname) == 0)
    secs -= timezone.stdiff;
  else if(strcmp(tm->zone, timezone.dlname) == 0)
    secs -= timezone.dldiff;
	 */

	if(secs < 0)
		secs = 0;
	return secs;
}

long convertToDateTime(const char* pDate, TM* pTM)
{
	if( strlen(pDate) > 0 )
	{
		int iTimeZoneDifferenceHour = 0;
		int iTimeZoneDifferenceMin = 0;
		bool bNegativeDifference = TRUE;

		int iDay, iMonth, iYear, iHour, iMin = 0, iSec = 0;
		string sMonth;
		char month[2048];
		char sDummy[2048];

		int iTry = -1, rc;
		int iMethodCount = 24;
		do
		{
			if( iTry == -1 )
			{
				rc = sscanf(pDate, "%d-%d-%d %d:%d:%d", &iYear, &iMonth, &iDay, &iHour, &iMin, &iSec);
				if( rc == 6 && iYear > 1900 && iMonth > 0 )
					break;
			}
			else if( iTry == 0 )
			{
				int rc = sscanf(pDate, "%s %d %s %d %d:%d:%d %s", sDummy, &iDay, &month, &iYear, &iHour, &iMin, &iSec, sDummy);
				sMonth = string(month);
				if( rc == 8 )
				{
					int iH, iM;
					sscanf(sDummy, "%d:%d", &iH, &iM);
					iMonth = getMonth(sMonth);
					if( iYear < 100 )
						iYear += 2000;

					/* Make sure that day-month match*/
					if( iMonth > 12 && iMonth < 32 && iDay < 13 && iDay > 0 )
					{
						int iTmp = iMonth;
						iMonth = iDay;
						iDay = iTmp;
					}

					//cout << "iDay=" << iDay << " Month=" << month << " iYear=" << iYear << " iHour=" << iHour << endl;
					getTimeZoneInfo(sDummy, &iTimeZoneDifferenceHour, &iTimeZoneDifferenceMin, &bNegativeDifference);
					if( iYear > 1900 && iMonth > 0 )
						break;
				}
			}
			else if( iTry == 1 )
			{
				rc = sscanf(pDate, "%s %d %s %d %d:%d %s", sDummy, &iDay, &month, &iYear, &iHour, &iMin, sDummy);

				int iH, iM;
				sscanf(sDummy, "%d:%d", &iH, &iM);
				iMonth = getMonth(sMonth);

				if( iYear < 100 )
				{
					iYear += 2000;
				}

				/* Make sure that day-month match*/
				if( iMonth > 12 && iMonth < 32 && iDay < 13 && iDay > 0 )
				{
					int iTmp = iMonth;
					iMonth = iDay;
					iDay = iTmp;
				}
				getTimeZoneInfo(sDummy, &iTimeZoneDifferenceHour, &iTimeZoneDifferenceMin, &bNegativeDifference);
				if( rc == 7 && iYear > 1900 && iMonth > 0 )
					break;
			}
			else if( iTry == 2 )
			{
				rc = sscanf(pDate, "%d-%d-%dT%d:%d:%d%s", &iYear, &iMonth, &iDay, &iHour, &iMin, &iSec, sDummy);
				if( rc == 7 && iYear > 1900 && iMonth > 0 )
					break;
			}
			else if( iTry == 3 )
			{
				int iDum1, iDum2;
				rc = sscanf(pDate, "%d-%d-%dT%d:%d:%d-%d:%d", &iYear, &iMonth, &iDay, &iHour, &iMin, &iSec, &iDum1, &iDum2);

				// iDum1 : Hour, iDum2 : Min
				iTimeZoneDifferenceHour = iDum1;
				iTimeZoneDifferenceMin = iDum2;
				bNegativeDifference = TRUE;

				if( rc == 8 && iYear > 1900 && iMonth > 0 )
					break;
			}
			else if( iTry == 4 )
			{
				rc = sscanf(pDate, "%d %s %d %d:%d:%d %s", &iDay, &month, &iYear, &iHour, &iMin, &iSec, sDummy);

				sMonth = string(month);
				iMonth = getMonth(sMonth);
				getTimeZoneInfo(sDummy, &iTimeZoneDifferenceHour, &iTimeZoneDifferenceMin, &bNegativeDifference);

				if( rc == 7 && iYear > 1900 && iMonth > 0 )
					break;
			}
			else if( iTry == 5 )
			{
				rc = sscanf(pDate, "%d/%d/%d @%d:%d", &iMonth, &iDay, &iYear, &iHour, &iMin);
				iSec = 0;
				if( iYear < 100 )
					iYear += 2000;
				if( rc == 5 && iYear > 1900 && iMonth > 0 )
					break;
			}
			else if( iTry == 6 )
			{
				rc = sscanf(pDate, "%d/%d/%d %d:%d %s", &iMonth, &iDay, &iYear, &iHour, &iMin, sDummy);
				getTimeZoneInfo(sDummy, &iTimeZoneDifferenceHour, &iTimeZoneDifferenceMin, &bNegativeDifference);
				if( rc == 6 && iYear > 1900 && iMonth > 0 )
					break;
			}
			else if( iTry == 7 )
			{
				rc = sscanf(pDate, "%d-%d-%dT%d:%d%s", &iYear, &iMonth, &iDay, &iHour, &iMin, sDummy);
				getTimeZoneInfo(sDummy, &iTimeZoneDifferenceHour, &iTimeZoneDifferenceMin, &bNegativeDifference);
				if( rc == 6 && iYear > 1900 && iMonth > 0 )
					break;
			}
			else if( iTry == 8 )
			{
				rc = sscanf(pDate, "%s %s %d, %d @ %d:%d", sDummy, month, &iDay, &iYear, &iHour, &iMin);
				sMonth = string(month);
				iMonth = getMonth(sMonth);
				if( rc == 6 && iYear > 1900 && iMonth > 0 )
					break;
			}
			else if( iTry == 9 )
			{
				rc = sscanf(pDate, "%d %s %d %d:%d %s", &iDay, month, &iYear, &iHour, &iMin, sDummy);

				sMonth = string(month);
				iMonth = getMonth(sMonth);

				if( iYear < 100 )
					iYear += 2000;
				getTimeZoneInfo(sDummy, &iTimeZoneDifferenceHour, &iTimeZoneDifferenceMin, &bNegativeDifference);
				if( rc == 6 && iYear > 1900 && iMonth > 0 )
					break;
			}
			else if( iTry == 10 )
			{
				rc = sscanf(pDate, "%s %s %d %d %d:%d:%d %s", sDummy, month, &iDay, &iYear, &iHour, &iMin, &iSec, sDummy);

				int iH, iM;
				sscanf(sDummy, "%d:%d", &iH, &iM);
				iMonth = getMonth(sMonth);
				if( iYear < 100 )
					iYear += 2000;

				/* Make sure that day-month match*/
				if( iMonth > 12 && iMonth < 32 && iDay < 13 && iDay > 0 )
				{
					int iTmp = iMonth;
					iMonth = iDay;
					iDay = iTmp;
				}

				getTimeZoneInfo(sDummy, &iTimeZoneDifferenceHour, &iTimeZoneDifferenceMin, &bNegativeDifference);
				if( rc == 8 && iYear > 1900 && iMonth > 0 )
					break;
			}
			else if( iTry == 11 )
			{
				rc = sscanf(pDate, "%s %s %d %d:%d %s %d", sDummy, month, &iDay, &iHour, &iMin, sDummy, &iYear);
				getTimeZoneInfo(sDummy, &iTimeZoneDifferenceHour, &iTimeZoneDifferenceMin, &bNegativeDifference);
				sMonth = string(month);
				if( rc == 7 && iYear > 1900 && iMonth > 0 )
					break;
			}
			else if( iTry == 12 )
			{
				rc = sscanf(pDate, "%s %s %d %d:%d:%d %s %d", sDummy, month, &iDay, &iHour, &iMin, &iSec, sDummy, &iYear);
				sMonth = string(month);
				getTimeZoneInfo(sDummy, &iTimeZoneDifferenceHour, &iTimeZoneDifferenceMin, &bNegativeDifference);

				if( rc == 8 && iYear > 1900 && iMonth > 0 )
					break;
			}
			else if( iTry == 13 )
			{
				rc = sscanf(pDate, "%d-%d-%d %d:%d:%d %s", &iYear, &iMonth, &iDay, &iHour, &iMin, &iSec, sDummy);
				getTimeZoneInfo(sDummy, &iTimeZoneDifferenceHour, &iTimeZoneDifferenceMin, &bNegativeDifference);

				if( rc == 7 && iYear > 1900 && iMonth > 0 )
					break;
			}
			else if( iTry == 14 )
			{
				rc = sscanf(pDate, "%d-%d-%d %d:%d:%d.0", &iYear, &iMonth, &iDay, &iHour, &iMin, &iSec);
				getTimeZoneInfo("GMT", &iTimeZoneDifferenceHour, &iTimeZoneDifferenceMin, &bNegativeDifference);

				if( rc == 6 && iYear > 1900 && iMonth > 0 )
					break;
			}
			else if( iTry == 15 )
			{
				rc = sscanf(pDate, "%d %s %d", &iDay, month, &iYear);

				sMonth = string(month);
				iMonth = getMonth(sMonth);

				if( rc == 3 && iYear > 1900 && iDay > 0 && iMonth > 0 )
					break;
			}
			else if( iTry == 16 )
			{
				rc = sscanf(pDate, "%d-%d-%d", &iYear, &iMonth, &iDay);
				iHour = 0;
				iMin = 0;
				iSec = 0;
				if( rc == 3 && iYear > 1900 && iMonth > 0 && iDay > 0 )
					break;
			}
			else if( iTry == 17 )
			{
				rc = sscanf(pDate, "%s %d %d", month, &iDay, &iYear);

				sMonth = string(month);
				iMonth = getMonth(sMonth);

				if( rc == 3 && iYear > 1900 && iDay > 0 && iMonth > 0 )
					break;
			}
			else if( iTry == 18 )
			{
				rc = sscanf(pDate, "%d/%d/%d", &iDay, &iMonth, &iYear);

				/* Make sure that day-month match*/
				if( iMonth > 12 && iMonth < 32 && iDay < 13 && iDay > 0 )
				{
					int iTmp = iMonth;
					iMonth = iDay;
					iDay = iTmp;
				}

				if( rc == 3 && iYear > 1900 && iDay > 0 && iMonth > 0 )
					break;
			}
			else if( iTry == 19 )
			{
				rc = sscanf(pDate, "%d-%d-%d", &iDay, &iMonth, &iYear);
				iHour = 0;
				iMin = 0;
				iSec = 0;
				if( rc == 3 && iYear > 1900 && iMonth > 0 && iDay > 0 )
					break;
			}
			else if( iTry == 20 )
			{
				rc = sscanf(pDate, "%s %s %d, %d", sDummy, month, &iDay, &iYear);

				sMonth = string(month);
				iMonth = getMonth(sMonth);

				iHour = 0;
				iMin = 0;
				iSec = 0;
				if( rc == 4 && iYear > 1900 && iMonth > 0 && iDay > 0 )
					break;
			}
			else if( iTry == 21 )
			{
				rc = sscanf(pDate, "%s %d, %d %d:%d %s", month, &iDay, &iYear, &iHour, &iMin, sDummy);
				iSec = 0;

				if( stricmp(sDummy, "PM") == 0 )
				{
					if( iHour < 12 )
						iHour += 12;
				}

				if( rc == 6 && iYear > 1900 && iMonth > 0 )
					break;
			}
			else if( iTry == 22 )
			{
				rc = sscanf(pDate, "%s %d %s %d", sDummy, &iDay, month, &iYear);

				sMonth = string(month);
				iMonth = getMonth(sMonth);

				iHour = 0;
				iMin = 0;
				iSec = 0;
				if( rc == 4 && iYear > 1900 && iMonth > 0 && iDay > 0 )
					break;
			}
			else if( iTry == 23 )
			{
				char sDummy2[2048];
				rc = sscanf(pDate, "%s %d, %d %d:%d %s %s", month, &iDay, &iYear, &iHour, &iMin, sDummy2, sDummy);
				iSec = 0;

				sMonth = string(month);
				iMonth = getMonth(sMonth);

				if( stricmp(sDummy2, "PM") == 0 )
				{
					if( iHour < 12 )
						iHour += 12;
				}

				getTimeZoneInfo(sDummy, &iTimeZoneDifferenceHour, &iTimeZoneDifferenceMin, &bNegativeDifference);
				if( rc == 7 && iYear > 1900 && iMonth > 0 )
					break;
			}

			iTry += 1;
		}
		while(iTry < iMethodCount);

		if( iYear < 100 && iYear > 9 )
			iYear += 1900;
		else if( iYear < 100 && iYear < 10 )
			iYear += 2000;

		/* Make sure that day-month match*/
		if( iMonth > 12 && iMonth < 32 && iDay < 13 && iDay > 0 )
		{
			int iTmp = iMonth;
			iMonth = iDay;
			iDay = iTmp;
		}

		if( iMonth > 0 && iMonth < 13 && iDay > 0 && iDay < 32 && iYear > 1900 &&
				iHour > -1 && iHour < 24 && iMin > -1 && iMin < 60 && iSec > -1 && iSec < 60)
		{
			TM tm;
			if( pTM == NULL )
				pTM = &tm;

			pTM->tm_year = iYear;
			pTM->tm_mon = iMonth-1;
			pTM->tm_mday = iDay;
			pTM->tm_hour = iHour;
			pTM->tm_min = iMin;
			pTM->tm_sec = iSec;

			if( iTimeZoneDifferenceHour > 0 || iTimeZoneDifferenceMin > 0 )
			{
				//printf("Zone Difference(%s) : %d hours, %d minutes (%d) (%d)\n", pDate, iTimeZoneDifferenceHour, iTimeZoneDifferenceMin, bNegativeDifference, iTry);
				long lSeconds = tm2sec(pTM);
				if( bNegativeDifference )
					lSeconds += (iTimeZoneDifferenceHour*60*60+iTimeZoneDifferenceMin*60);
				else
					lSeconds -= (iTimeZoneDifferenceHour*60*60+iTimeZoneDifferenceMin*60);

				return lSeconds;
			}
			return tm2sec(pTM);
		}

		//printf("Failed to process [%s]\n", pDate);
	}

	return 0;
}

void getTimeZoneInfo(char *sTimeZoneStr, int *TZHour, int *TZMin, bool *bNegative)
{
	if( !sTimeZoneStr || strlen(sTimeZoneStr) == 3 )
	{
		if( !sTimeZoneStr || stricmp(sTimeZoneStr, "GMT") == 0 )
		{
			*TZHour = 0;
			*bNegative = FALSE;
			return;
		}
		else if( stricmp(sTimeZoneStr, "CET") == 0 )
		{
			*TZHour = 1;
			*bNegative = FALSE;
			return;
		}
		else if( stricmp(sTimeZoneStr, "AST") == 0 || stricmp(sTimeZoneStr, "EDT") == 0 )
		{
			*TZHour = 4;
			*bNegative = TRUE;
			return;
		}
		else if( stricmp(sTimeZoneStr, "EST") == 0 || stricmp(sTimeZoneStr, "CDT") == 0 )
		{
			*TZHour = 5;
			*bNegative = TRUE;
			return;
		}
		else if( stricmp(sTimeZoneStr, "CST") == 0 || stricmp(sTimeZoneStr, "MDT") == 0 )
		{
			*TZHour = 6;
			*bNegative = TRUE;
			return;
		}
		else if( stricmp(sTimeZoneStr, "MST") == 0 || stricmp(sTimeZoneStr, "PDT") == 0  )
		{
			*TZHour = 7;
			*bNegative = TRUE;
			return;
		}
		else if( stricmp(sTimeZoneStr, "PST") == 0 || stricmp(sTimeZoneStr, "AKDT") == 0 )
		{
			*TZHour = 8;
			*bNegative = TRUE;
			return;
		}
		else if( stricmp(sTimeZoneStr, "EET") == 0 )
		{
			*TZHour = 2;
			*bNegative = FALSE;
			return;
		}
		else if( stricmp(sTimeZoneStr, "MSK") == 0 )
		{
			*TZHour = 3;
			*bNegative = FALSE;
			return;
		}
		else if( stricmp(sTimeZoneStr, "IST") == 0 )
		{
			*TZHour = 5;
			*TZMin = 30;
			*bNegative = FALSE;
			return;
		}
		else if( stricmp(sTimeZoneStr, "AWST") == 0 )
		{
			*TZHour = 8;
			*bNegative = FALSE;
			return;
		}
		else if( stricmp(sTimeZoneStr, "JST") == 0 )
		{
			*TZHour = 9;
			*bNegative = FALSE;
			return;
		}
		else if( stricmp(sTimeZoneStr, "ACST") == 0 )
		{
			*TZHour = 9;
			*TZMin = 30;
			*bNegative = FALSE;
			return;
		}
		else if( stricmp(sTimeZoneStr, "AEST") == 0 )
		{
			*TZHour = 10;
			*bNegative = FALSE;
			return;
		}
		else if( stricmp(sTimeZoneStr, "AKST") == 0 )
		{
			*TZHour = 9;
			*bNegative = TRUE;
			return;
		}
		else if( stricmp(sTimeZoneStr, "GST") == 0 )
		{
			*TZHour = 10;
			*bNegative = FALSE;
			return;
		}
		else if( stricmp(sTimeZoneStr, "WAT") == 0 )
		{
			*TZHour = 1;
			*bNegative = TRUE;
			return;
		}
		else if( stricmp(sTimeZoneStr, "YST") == 0 )
		{
			*TZHour = 8;
			*bNegative = TRUE;
			return;
		}
		else if( stricmp(sTimeZoneStr, "ADT") == 0 )
		{
			*TZHour = 3;
			*bNegative = TRUE;
			return;
		}
		else if( stricmp(sTimeZoneStr, "NST") == 0 )
		{
			*TZHour = 3;
			*TZMin = 30;
			*bNegative = TRUE;
			return;
		}
		else if( stricmp(sTimeZoneStr, "HST") == 0 || stricmp(sTimeZoneStr, "CAT") == 0)
		{
			*TZHour = 10;
			*bNegative = TRUE;
			return;
		}
	}

	if( stricmp(sTimeZoneStr, "Z") == 0 )
	{
		// Dont do anything
		return;
	}
	else if( stricmp(sTimeZoneStr, "AT") == 0 )
	{
		*TZHour = 9;
		*bNegative = TRUE;
		return;
	}
	else
	{
		int iH, iM;
		int rc = sscanf(sTimeZoneStr, "-%d:%d", &iH, &iM);

		if( rc == 2 )
		{
			*TZHour = iH;
			*TZMin = iM;
			*bNegative = TRUE;
		}
		else
		{
			rc = sscanf(sTimeZoneStr, "GMT+%d", &iH);

			if( rc == 1 )
			{
				*TZHour = iH;
				*bNegative = FALSE;
			}
			else
			{
				rc = sscanf(sTimeZoneStr, "GMT-%d", &iH);

				if( rc == 1 )
				{
					*TZHour = iH;
					*bNegative = TRUE;
				}
				else
				{
					rc = sscanf(sTimeZoneStr, "+%d:%d", &iH, &iM);

					if( rc == 2 )
					{
						*TZHour = iH;
						*TZMin = iM;
						*bNegative = FALSE;
					}
					else
					{
						rc = sscanf(sTimeZoneStr, "%d:%d", &iH, &iM);
						if( rc == 2 )
						{
							*TZHour = iH;
							*TZMin = iM;
							*bNegative = FALSE;
						}
						else
						{
							rc = sscanf(sTimeZoneStr, "+%d", &iH);
							if( rc == 1 )
							{
								*TZHour = iH;
								*TZMin = 0;
								*bNegative = FALSE;
							}
							else
							{
								/* Check this +0900 */
								if( strlen(sTimeZoneStr) == 5 && (sTimeZoneStr[0] == '+' || sTimeZoneStr[0] == '-') )
								{
									*bNegative = (sTimeZoneStr[0] == '-');
									sTimeZoneStr++;

									int iHour1, iHour2, iMin1, iMin2;
									iHour1 = sTimeZoneStr[0]-48;
									iHour2 = sTimeZoneStr[1]-48;
									iMin1 = sTimeZoneStr[2]-48;
									iMin2 = sTimeZoneStr[3]-48;
									//rc = sscanf(sTimeZoneStr, "%d%d%d%d", &iHour1, &iHour2, &iMin1, &iMin2);

									if( iHour1+iHour2 > 0 || iMin1+iMin2 > 0 )
									{
										*TZHour = iHour1*10+iHour2;
										*TZMin = iMin1*10+iMin2;
									}
								}
							}
						}
					}
				}
			}
		}
	}

}

int getMonth(string sMonth)
{
	int iMonth = -1;

	if( sMonth.length() < 3 )
		return -1;

	sMonth[0] = tolower(sMonth[0]);
	sMonth[1] = tolower(sMonth[1]);
	sMonth[2] = tolower(sMonth[2]);

	if( sMonth.find("jan") == 0 )
		iMonth = 1;
	else if( sMonth.find("feb") == 0 )
		iMonth = 2;
	else if( sMonth.find("mar") == 0 )
		iMonth = 3;
	else if( sMonth.find("apr") == 0 )
		iMonth = 4;
	else if( sMonth.find("may") == 0 )
		iMonth = 5;
	else if( sMonth.find("jun") == 0 )
		iMonth = 6;
	else if( sMonth.find("jul") == 0 )
		iMonth = 7;
	else if( sMonth.find("aug") == 0 )
		iMonth = 8;
	else if( sMonth.find("sep") == 0 )
		iMonth = 9;
	else if( sMonth.find("oct") == 0 )
		iMonth = 10;
	else if( sMonth.find("nov") == 0 )
		iMonth = 11;
	else if( sMonth.find("dec") == 0 )
		iMonth = 12;
	return iMonth;
}

string tm2str(TM* pTM, bool bDateTime)
{
	if( pTM->tm_year > 0 )
	{
		char cFormatedDate[100];

		if( pTM->tm_year > 1970 )
		{
			if( bDateTime )
				sprintf(cFormatedDate, "%d-%d-%d %d:%d:%d", pTM->tm_year, (pTM->tm_mon+1), pTM->tm_mday, pTM->tm_hour, pTM->tm_min, pTM->tm_sec);
			else
				sprintf(cFormatedDate, "%d-%d-%d", pTM->tm_year, (pTM->tm_mon+1), pTM->tm_mday);
		}
		else
		{
			if( bDateTime )
				sprintf(cFormatedDate, "%d-%d-%d %d:%d:%d", (1970+pTM->tm_year), (pTM->tm_mon+1), pTM->tm_mday, pTM->tm_hour, pTM->tm_min, pTM->tm_sec);
			else
				sprintf(cFormatedDate, "%d-%d-%d", (1970+pTM->tm_year), (pTM->tm_mon+1), pTM->tm_mday);
		}
		return string(cFormatedDate);
	}
	return "";
}

string getTodayDateTime()
{
	time_t rawtime;
	tm* ptm;

	char cDate[1024];

	time(&rawtime);
	ptm = gmtime(&rawtime);

	sprintf(cDate, "%d-%d-%d %d:%d:%d", (1900+ptm->tm_year), (1+ptm->tm_mon), ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
	return string(cDate);
}

void getTodayDate(char* pDate)
{
	time_t rawtime;
	tm* ptm;
	time(&rawtime);
	ptm = gmtime(&rawtime);
	sprintf(pDate, "%d-%d-%d", (1900+ptm->tm_year), (1+ptm->tm_mon), ptm->tm_mday);
}

long getTodayInSeconds(bool bLocal)
{
	time_t rawtime;
	time(&rawtime);
	if( bLocal )
		return tm2sec(localtime(&rawtime));
	return tm2sec(gmtime(&rawtime));
}

void getNowInRFCFormat(char* pBuffer, int iBufferSize, bool bLocal)
{
	time_t t;
	struct tm *tmp;

	t = time(NULL);
	if( bLocal )
		tmp = localtime(&t);
	else
		tmp = gmtime(&t);

	if (strftime(pBuffer, iBufferSize, "%a, %d %b %Y %H:%M:%S %Z\0", tmp) == 0)
	{
		// error occured; zero p
		pBuffer[0] = '\0';
	}
}

unsigned long int tvdiff2ms(struct timeval *a, struct timeval *b)
{
	return ((a->tv_sec-b->tv_sec)*1000) + ((a->tv_usec-b->tv_usec)/1000);
}

void convertLong2DateTime(long lDate, char* pDate)
{
	if( lDate == 0 )
	{
		pDate[0] = '\0';
		return;
	}

	time_t time = (time_t)lDate;
	struct tm* t = gmtime(&time);
	if( t )
	{
		sprintf(pDate, "%d-%02d-%02d %02d:%02d:%02d",
				(1900+t->tm_year),
				(1+t->tm_mon),
				t->tm_mday,
				t->tm_hour,
				t->tm_min,
				t->tm_sec);
	}
	else
		sprintf(pDate, "0000-00-00 00:00:00");
}

void convertLong2Date(long lDate, char* pDate)
{
	if( lDate == 0 )
	{
		pDate[0] = '\0';
		return;
	}

	time_t time = (time_t)lDate;
	struct tm* t = gmtime(&time);
	if( t )
	{
		sprintf(pDate, "%d-%02d-%02d",
				(1900+t->tm_year),
				(1+t->tm_mon),
				t->tm_mday);
	}
	else
		sprintf(pDate, "0000-00-00");
}

void getNow(char* pBuffer, int iBufferSize, bool bLocal)
{
	time_t rawtime;
	tm* ptm;

	time(&rawtime);
	if( bLocal )
		ptm = localtime(&rawtime);
	else
		ptm = gmtime(&rawtime);

	sprintf(pBuffer, "%d-%d-%d %d:%d:%d\0", (1900+ptm->tm_year), (1+ptm->tm_mon), ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
}

bool startsAfter(tm* pTM1, tm* pTM2)
{
	if( pTM1->tm_year > pTM2->tm_year ) return TRUE;
	else if( pTM1->tm_year < pTM2->tm_year ) return FALSE;

	if( pTM1->tm_mon > pTM2->tm_mon ) return TRUE;
	else if( pTM1->tm_mon < pTM2->tm_mon ) return FALSE;

	if( pTM1->tm_mday > pTM2->tm_mday ) return TRUE;
	else if( pTM1->tm_mday < pTM2->tm_mday ) return FALSE;

	if( pTM1->tm_hour > pTM2->tm_hour ) return TRUE;
	else if( pTM1->tm_hour < pTM2->tm_hour ) return FALSE;

	if( pTM1->tm_min > pTM2->tm_min ) return TRUE;
	else if( pTM1->tm_min < pTM2->tm_min ) return FALSE;

	if( pTM1->tm_sec > pTM2->tm_sec ) return TRUE;
	else if( pTM1->tm_sec < pTM2->tm_sec ) return FALSE;

	return FALSE;
}

bool isSame(tm* pTM1, tm* pTM2)
{
	if( pTM1->tm_year == pTM2->tm_year &&
			pTM1->tm_mon == pTM2->tm_mon &&
			pTM1->tm_mday == pTM2->tm_mday &&
			pTM1->tm_hour == pTM2->tm_hour &&
			pTM1->tm_min == pTM2->tm_min &&
			pTM1->tm_sec == pTM2->tm_sec )
		return TRUE;
	return FALSE;
}


