#include "stat.h"

void alph_cnt(char *buf, int *res)
{
	for (int i = 0; buf[i]; i++)
	{
		if ('a' <= buf[i] && buf[i] <= 'z')
			res[buf[i]-'a']++;

		else if ('A' <= buf[i] && buf[i] <= 'Z')
			res[buf[i]-'A']++;
	}
}
