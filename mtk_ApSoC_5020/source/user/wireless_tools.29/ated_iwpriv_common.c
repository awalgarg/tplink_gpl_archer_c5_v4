#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "ated_iwpriv_common.h"

/* 
 * brief: split string into some parts
 * IN str: input str, it will be MODIFIED!
 * OUT strArr: array storing split
 * IN strArrLen:    array len
 * IN deliminator:     deliminator to split the string
 * return: number of split results.
 */
int SplitString(char *str, char *strArr[], int strArrLen, const char *deliminator)
{
    char *spilt;
    char *res;
    res = strtok_r(str, deliminator, &spilt);
    int index = 0;
    while ((res != NULL) && (index < strArrLen))
    {
        strArr[index] =  res;
        res = strtok_r(NULL, deliminator, &spilt);
        
        index ++;
    }
    
    return index;
}

int getCmdResults(const char* cmd, const char* target, char* resultbuf, int resultLen)
{
	char fileLineBuf[MAX_LINE_LEN] = {0};
	FILE* resStream = NULL;
	
	int ret = 0;
	
	memset(resultbuf, 0, resultLen);
	
	resStream = popen(cmd, "r");
	if (resStream)
	{
		while (fgets(fileLineBuf, sizeof(fileLineBuf), resStream))
		{
			if (target)
			{
				if (strstr(fileLineBuf, target))
				{
					strlcpy(resultbuf, fileLineBuf, resultLen);
					break;
				}
			}
			else
			{
				strlcpy(resultbuf+strlen(resultbuf), fileLineBuf, resultLen - strlen(resultbuf));
			}
		}
		pclose(resStream);
	}
	else
	{
		ret = -1;
	}
	return ret;
}
