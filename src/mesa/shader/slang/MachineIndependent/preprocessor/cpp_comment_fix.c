/* converts c++ to c comments */
/* usage: ./cpp_comment_fix source */

#include <stdio.h>

int main (int argc, char *argv[])
{
	FILE *f;
	int c;
	char *buf = NULL;
	int size = 0, i = 0;
	
	f = fopen (argv[1], "r");
	while ((c = fgetc (f)) != EOF)
	{
		buf = (void *) realloc (buf, size + 1);
		buf[size] = c;
		size++;
	}
	fclose (f);
	
	f = fopen (argv[1], "w");

	while (i < size)
	{
		if (buf[i] == '/')
		{
			if (buf[i+1] == '/')
			{
				fprintf (f, "/*");
				i+=2;
				while (buf[i] != '\n' && buf[i] != '\r' && i < size)
					fprintf (f, "%c", buf[i++]);
				fprintf (f, " */\n");
				if (i < size && buf[i] == '\n')
					i++;
				else if (i < size && buf[i] == '\r')
					i+=2;
			}
			else
			{
				fprintf (f, "/");
				i++;

				if (buf[i] == '*')
				{
					fprintf (f, "*");
					i++;

					for (;;)
					{
						if (buf[i] == '*' && buf[i+1] == '/')
						{
							fprintf (f, "*/");
							i+=2;
							break;
						}
						else
						{
							fprintf (f, "%c", buf[i]);
							i++;
						}
					}
				}
			}
		}
		else
		{
			fprintf (f, "%c", buf[i]);
			i++;
		}
	}
	fclose (f);
	return 0;
}

