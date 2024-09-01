#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>

struct state
{
	int curx, cury, mode, len;
	char ch;
	char *buf, *status, *filename;
};


struct termios term, nterm;

int
cortopos(int x, int y)
{
	return y*10+x;
}

void
sint()
{
	printf("\e[?25h");
	tcsetattr(STDIN_FILENO, TCSANOW, &term);
	printf("\n");
}

unsigned char
getvald()
{
	char ch = 0;
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

unsigned char
getvalx()
{
	char ch = 0;
	char b[3];
	int c = 0;
	int ret;
	while (1)
	{
		ch = getchar();
		if (ch == 13)
		{
			sscanf(b, "%x", &ret);	
			if (ret > 255)
				return 255;
			return ret;
		}
		else if ((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f'))
		{
			b[c] = ch;
			c++;
			b[c] = '\0';
			if (c == 2)
			{
				sscanf(b, "%x", &ret);	
				if (ret > 255)
					return 255;
				return ret;
			}
		}
	}
}

char
getvalc()
{
	char ch;
	while (1)
	{
		ch = getchar();
		printf("%d %c", ch, ch);
		if (ch >= 32 && ch <= 126)
			return ch;
	}
}

unsigned char
getval(int mode)
{
	switch (mode)
	{
		case 0:
			return getvald();
			break;
		case 1:
			return getvalx();
			break;
		case 2:
			return (char)getvalc();
			break;
		default:
			return 0;
			break;
	}
}

void
setup()
{
	atexit(sint);
	printf("\e[?25l");
	tcgetattr(STDIN_FILENO, &term);
	nterm = term;
	cfmakeraw(&nterm);
	tcsetattr(STDIN_FILENO, TCSANOW, &nterm);
}

void
printText(struct state* st)
{
	printf("\e[H\e[J");
	for (int i = 0; i < st->len; i++)
	{
		if (st->cury*10+st->curx == i)
			printf("\e[1;41m");
		else
			printf("\e[0m");

		if (st->mode == 1)
			printf("%X ", st->buf[i]);
		else if (st->mode == 0)
			printf("%d ", st->buf[i]);
		else if (st->mode == 2)
			printf("[%c] ", st->buf[i]);

		if (i%10 == 9)
			printf("\e[E");
	}
	printf("\e[E\e[0m(%d)[%d]{%s}|%dx%d|", st->ch, st->mode, st->status, st->curx, st->cury);
}

int
proccedKey(struct state* st)
{
	char *oldbuf;
	int pos;
	switch (st->ch)
	{
		case 3: // ^C
			return 0;
			break;
		case 68: // left
			if (st->curx > 0)
			{
				st->curx--;
			}
			else if (st->cury > 0)
			{
				st->curx = 9;
				st->cury--;
			}
			break;
		case 67: // right
			if (st->cury*10+st->curx == st->len-1)
				break;
			if (st->curx >= 9)
			{
				st->curx = 0;
				st->cury++;
			}
			else
			{
				st->curx++;
			}
			break;
		case 66: // down
			if (st->cury < st->len/10)
				st->cury++;
			break;
		case 65: // up
			if (st->cury > 0)
				st->cury--;
			break;
		case 97: // a
			st->status = "Append";
			printText(st);
			if (st->len == 0)
			{
				st->len++;
				st->buf = malloc(st->len);
				st->buf[0] = getval(st->mode);
				break;
			}
			st->len++;
			oldbuf = st->buf;
			st->buf = malloc(st->len);
			pos = cortopos(st->curx, st->cury);
			memcpy(st->buf, oldbuf, pos+1);
			memcpy(st->buf+pos+2, oldbuf+pos+1,
					st->len-(pos+2));
			free(oldbuf);
			st->buf[pos+1] = getval(st->mode);
			break;
		case 105: // i
			st->status="Insert";
			printText(st);
			if (st->len == 0)
			{
				st->len++;
				st->buf = malloc(st->len);
				st->buf[0] = getval(st->mode);
				break;
			}
			st->len++;
			oldbuf = st->buf;
			st->buf = malloc(st->len);
			pos = cortopos(st->curx, st->cury);
			memcpy(st->buf, oldbuf, pos);
			memcpy(st->buf+pos+1, oldbuf+pos,
					st->len-(pos+1));
			free(oldbuf);
			st->buf[pos] = getval(st->mode);
			break;
		case 114: // r
			st->status="Replace";
			printText(st);
			st->buf[cortopos(st->curx, st->cury)] = getval(st->mode);
			break;
		case 100: // d
			printf("\e[EDelete");
			if (st->len == 0)
				break;
			st->len--;
			if (st->len == 0)
			{
				free(st->buf);
				st->curx = 0;
				st->cury = 0;
				break;
			}
			oldbuf = st->buf;
			st->buf = malloc(st->len);
			pos = cortopos(st->curx, st->cury);
			memcpy(st->buf, oldbuf, pos);
			memcpy(st->buf+pos, oldbuf+pos+1,
					st->len-pos);
			free(oldbuf);
			if (pos>st->len-1)
			{
				if (st->curx == 0 && st->cury > 0)
				{
					st->cury--;
					st->curx = 9;
				}	
				else if(st->curx > 0)
					st->curx--;
			}
			break;
		case 119: // w
			st->status="Wrote";
			FILE* f = fopen(st->filename, "wb");
			if (!f)
			{
				perror("cannot open file");
			}
			fwrite(st->buf, 1 ,st->len, f);
			fclose(f);
			break;
		case 109: // m
			st->status="Mode has changed";
			st->mode++;
			if (st->mode == 3) st->mode = 0;
			break;
	}
	return 1;
}

int
main (int argc, char** argv)
{
	struct state st;
	st.status = "tiwula is a cutie pie";
	if (argc < 2)
	{
		puts("Usage: beditor <file> [d|x|c]\n\td - decimal mode\n\tx - hexadecimal\n\tc - char");
		return 1;
	}
	if (argc >= 3)
	{
		if (argv[2][0] == 'd')
			st.mode = 0;
		else if (argv[2][0] == 'x')
			st.mode = 1;
		else if (argv[2][0] == 'c')
			st.mode = 2;
	}
	st.filename = argv[1];
	setup();
	FILE *f = fopen(st.filename, "rb");
	if (f)
	{
		fseek(f, 0, SEEK_END);
		st.len = ftell(f);
		fseek(f, 0, SEEK_SET);
		st.buf = malloc(st.len);
		fread(st.buf, 1, st.len , f);
		fclose(f);
	}
	else
	{
		st.len = 0;
	}
	char loop = 1;
	while (loop)
	{
		printText(&st);
		st.ch = getchar();
		loop = proccedKey(&st);
	}
	sint();
	return 0;
}

