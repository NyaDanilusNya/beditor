#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>


int curx=0,cury=0;

struct termios term, nterm;

int
cortopos(int x, int y)
{
	return cury*10+curx;
}

void
sint()
{
	printf("\e[?25h");
	tcsetattr(STDIN_FILENO, TCSANOW, &term);
}

char
getvald()
{
	char ch;
	char b[4];
	int c = 0;
	int ret;
	while (1)
	{
		ch = getchar();
		if (ch == 13)
		{
			sscanf(b, "%d", &ret);	
			if (ret > 255)
				return 255;
			return ret;
		}
		else if (ch >= '0' && ch <= '9')
		{
			b[c] = ch;
			c++;
			b[c] = '\0';
			if (c == 3)
			{
				sscanf(b, "%d", &ret);	
				if (ret > 255)
					return 255;
				return ret;
			}
		}
	}
}

int
main (int argc, char** argv)
{
	int mode = 0;
	char* status = "tiwula is a cutie pie";
	if (argc < 2)
		return 1;
	if (argc >= 3)
	{
		if (argv[2][0] == 'x')
			mode = 1;
		else if (argv[2][0] == 'd')
			mode = 0;
	}
	FILE *f = fopen(argv[1], "rb");
	if (!f)
	{
		perror("cannot open file");
	}
	atexit(sint);
	printf("\e[?25l");
	tcgetattr(STDIN_FILENO, &term);
	nterm = term;
	cfmakeraw(&nterm);
	
	tcsetattr(STDIN_FILENO, TCSANOW, &nterm);
	fseek(f, 0, SEEK_END);
	int len = ftell(f);
	fseek(f, 0, SEEK_SET);
	char* buf = malloc(len);
	fread(buf, 1, len , f);
	fclose(f);
	char ch;
	char loop = 1;
	char *oldbuf;
	int pos;
	while (loop)
	{
		printf("\e[H\e[J");
		for (int i = 0; i < len; i++)
		{
			if (cury*10+curx == i)
				printf("\e[1;41m");
			else
				printf("\e[0m");
			
			if (mode == 1)
				printf("%X ", buf[i]);
			else if (mode == 0)
				printf("%d ", buf[i]);
			else if (mode == 2)
				printf("[%c] ", buf[i]);

			if (i%10 == 9)
				printf("\e[E");
		}
		printf("\e[E\e[0m(%d)[%d]{%s}|%dx%d|", ch, mode, status, curx, cury);
		ch = getchar();
		switch (ch)
		{
			case 3: // ^C
				loop = 0;
				break;
			case 68:
				if (curx > 0)
					curx--;
				break;
			case 67:
				if (cury*10+curx == len-1)
					break;
				if (curx >= 9)
				{
					curx = 0;
					cury++;
				}
				else
				{
					curx++;
				}
				break;
			case 66:
				if (cury < len/10)
					cury++;
				break;
			case 65:
				if (cury > 0)
					cury--;
				break;
			case 97:
				status = "Append";
				if (len == 0)
				{
					len++;
					buf = malloc(len);
					buf[0] = getvald();
					break;
				}
				len++;
				oldbuf = buf;
				buf = malloc(len);
				pos = cortopos(curx, cury);
				memcpy(buf, oldbuf, pos+1);
				memcpy(buf+pos+2, oldbuf+pos+1,
						len-(pos+2));
				free(oldbuf);
				buf[pos+1] = getvald();
				break;
			case 105:
				status="Insert";
				if (len == 0)
				{
					len++;
					buf = malloc(len);
					buf[0] = getvald();
					break;
				}
				len++;
				oldbuf = buf;
				buf = malloc(len);
				pos = cortopos(curx, cury);
				memcpy(buf, oldbuf, pos);
				memcpy(buf+pos+1, oldbuf+pos,
						len-(pos+1));
				free(oldbuf);
				buf[pos] = getvald();
				break;
			case 114:
				status="Replace";
				buf[cortopos(curx, cury)] = getvald();
				break;
			case 100:
				printf("\e[ERemove");
				if (len == 0)
					break;
				len--;
				oldbuf = buf;
				buf = malloc(len);
				pos = cortopos(curx, cury);
				memcpy(buf, oldbuf, pos);
				memcpy(buf+pos, oldbuf+pos+1,
					 len-pos);
				free(oldbuf);
				if (pos>len-1)
				{
					if (curx == 0 && cury > 0)
					{
						cury--;
						curx = 9;
					}	
					else if(curx > 0)
						curx--;
				}
				break;
			case 119:
				status="Wrote";
				f = fopen(argv[1], "wb");
				if (!f)
				{
					perror("cannot open file");
				}
				fwrite(buf, 1 , len, f);
				fclose(f);
				break;
			case 109:
				status="Mode has changed";
				mode++;
				if (mode == 3) mode = 0;
				break;
		}
	}
	sint();
	return 0;
}

