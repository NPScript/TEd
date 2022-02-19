#include <stdio.h>
#include <stdarg.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <termios.h>
#include <malloc.h>
#include <stdlib.h>

#include "tui.h"
#include "highlight.h"

struct termios term;

unsigned get_term_width() {
	struct winsize w;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

	return w.ws_col;
}

unsigned get_term_height() {
	struct winsize w;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

	return w.ws_row;
}

unsigned get_cursor_x_position() {
	wprintf(L"\033[6n");
	wchar_t buf[32];
	unsigned n;
	unsigned m;
	unsigned x;

	for (wchar_t * c = buf; c < buf + 32; ++c) {
		*c = getch();

		if (*c == 'R') {
			m = c - buf;
			break;
		} else if (*c == ';') {
			n = c - buf;
		}
	}

	x = 0;

	for (int i = m - 1; i > n; --i) {
		int p = 1;

		for (int j = 0; j < m - 1 - i; ++j)
			p *= 10;

		x += ((int)buf[i] - 48) * p;
	}

	return x;
}

unsigned get_cursor_y_position() {
	wprintf(L"\033[6n");
	wchar_t buf[32];
	unsigned n;
	unsigned y;
	unsigned was_echo = is_echo();

	showecho(0);

	for (wchar_t * c = buf; c < buf + 32; ++c) {
		*c = getch();

		if (*c == 'R') {
			break;
		} else if (*c == ';') {
			n = c - buf;
		}
	}

	showecho(was_echo);

	y = 0;

	for (int i = n - 1; i > 1; --i) {
		int p = 1;

		for (int j = 0; j < n - 1 - i; ++j)
			p *= 10;

		y += ((int)buf[i] - 48) * p;
	}

	return y;
}

void inittui() {
	wprintf(L"\033[?1049h");
	tcgetattr(fileno(stdin), &term);
}

void endtui() {
	wprintf(L"\033[?1049l");
}

wchar_t getch() {
	wchar_t c;
	term.c_lflag &= ~ICANON;

	tcsetattr(fileno(stdin), TCSANOW, &term);
	c = getwc(stdin);

	term.c_lflag |= ICANON;
	tcsetattr(fileno(stdin), TCSANOW, &term);
	return c;
}

void showecho(int echo) {
	if (!echo)
		term.c_lflag &= ~(ECHO | ECHOE | ECHOK | ECHONL);
	else
		term.c_lflag |= ECHO | ECHOE | ECHOK | ECHONL;
	tcsetattr(fileno(stdin), TCSANOW, &term);
}

int is_echo() {
	return term.c_iflag & ECHO;
}

void showcursor(int show) {
	wprintf(L"\033[?25%c", show ? 'h' : 'l');
}

void printfxy(unsigned x, unsigned y, const wchar_t * fmt, ...) {
	va_list list;

	wprintf(L"\033[%d;%dH", y, x);

	va_start(list, fmt);
	vwprintf(fmt, list);
	va_end(list);
}

void draw_window(Window * win) {
	for (int i = 0; i < win->height; ++i)
		printfxy(win->x, win->y + i, L"%*s", win->width, "");

	if (win->has_borders) {
		printfxy(win->x, win->y, L"%lc", TLCORNER);
		for (int i = 1; i < win->width - 1; ++i)
			printfxy(win->x + i, win->y, L"%lc", HLINE);
		printfxy(win->x + win->width - 1, win->y, L"%lc", TRCORNER);

		printfxy(win->x, win->y + win->height - 1, L"%lc", BLCORNER);
		for (int i = 1; i < win->width - 1; ++i)
			printfxy(win->x + i, win->y + win->height - 1, L"%lc", HLINE);
		printfxy(win->x + win->width - 1, win->y + win->height - 1, L"%lc", BRCORNER);

		for (int i = 1; i < win->height - 1; ++i)
			printfxy(win->x, win->y + i, L"%lc", VLINE);

		for (int i = 1; i < win->height - 1; ++i)
			printfxy(win->x + win->width - 1, win->y + i, L"%lc", VLINE);
	}

	fflush(stdout);
}

void printfxy_to_window(Window * win, int x, int y, const wchar_t * fmt, ...) {
	va_list list;
	wchar_t * buf = malloc(sizeof(wchar_t) * BUFSIZ);
	int cx;
	int cy;
	int tabstop = 0;

	va_start(list, fmt);
	vswprintf(buf, BUFSIZ, fmt, list);
	va_end(list);

	cx = x + win->x + win->has_borders;
	cy = y + win->y + win->has_borders;

	if (cy >= (int)(win->y + win->height - win->has_borders))
		return;

	wprintf(L"\033[%d;%dH", cy, cx);

	for (wchar_t * c = buf; *c;) {
		if (*c == L'\033') {
			int running = 1;

			while (running) {
				if ((*c < L'0' || *c > L'9') && *c != L';') {
					running = 0;
				}

				putwc(*c, stdout);
				++c;
			}
			continue;
		}

		if (*c == L'\n') {
			wprintf(RETURN_SIGN);
		}

		if (*c == L'\t') {
			tabstop = TABSIZ;
			wprintf(TAB_SIGN);
			*c = L' ';
		}

		if (*c == L'\n' || cx == win->x + win->width - win->has_borders) {
			if (++cy >= (int)(win->y + win->height - win->has_borders))
				break;
			cx = win->x + win->has_borders;
			wprintf(L"\033[%d;%dH", cy, cx);
		}

		if (*c != L'\n') {
			if (cy >= (int)(win->y + win->has_borders) && tabstop < TABSIZ - 1)
				putwc(*c, stdout);
			++cx;
		}

		if (tabstop)
			--tabstop;
		else
			++c;
	}

	free(buf);
}

void set_cursor_position(int x, int y) {
	wprintf(L"\033[%d;%dH", y, x);
}

void set_cursor_position_in_window(Window * win, int x, int y) {
	wprintf(L"\033[%d;%dH", y + win->y + win->has_borders, x + win->x + win->has_borders);
}
