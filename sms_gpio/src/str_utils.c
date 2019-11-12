#ifndef _STR_UTILS_C_
#define _STR_UTILS_C_

#include <ctype.h>
#include <stdio.h>

void lowerStr(char *str)
{
	for (int i = 0; str[i]; i++)
	{
		if (str[i] >= 'A' && str[i] <= 'Z')
		{
			str[i] -= 'A' - 'a';
		}
	}
}

void clearStr(char *str)
{
	for (int i = 0; str[i]; i++)
	{
		if (str[i] == '\r' || str[i] == '\n' || (str[i] >= ' ' && str[i] <= '~'))
		{
			continue;
		}

		str[i] = '?';
	}
}

void removeChar(char *s, int c){ 
  //in:abcd123def,'d'
  //out:abc123ef
    int j, n = strlen(s); 
    for (int i=j=0; i<n; i++) 
       if (s[i] != c) 
          s[j++] = s[i]; 
      
    s[j] = '\0'; 
}
#endif