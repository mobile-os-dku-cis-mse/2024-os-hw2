#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
	FILE *f;
	char *line;
	size_t n;
	ssize_t len;
	int res[26] = {0};

	if (argc < 2)
	{
		puts("usage: ./a.out <filename>");
		exit(1);
	}

	f = fopen(argv[1], "r");
	if (!f)
	{
		perror("fopen");
		exit(1);
	}

	while (getdelim(&line, &n, '\n', f) != -1)
	{
		for (int i = 0; line[i]; i++)
		{
			if ('a' <= line[i] && line[i] <= 'z')
				res[line[i]-'a']++;

			if ('A' <= line[i] && line[i] <= 'Z')
				res[line[i]-'A']++;
		}
	}

	fclose(f);

	puts("char | occurrence");
	for (int i = 0; i < 26; i++)
		printf("%c(%c) | %d\n", 'A'+i, 'a'+i, res[i]);

	return 0;
}
