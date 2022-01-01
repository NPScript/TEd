#ifndef TUI_H
#define TUI_H

#include <stdio.h>
#include <wchar.h>

#define TABSIZ 2
#define VLINE    L'│'
#define HLINE    L'─'
#define TLCORNER L'┌'
#define BLCORNER L'└'
#define TRCORNER L'┐'
#define BRCORNER L'┘'
#define LTEE     L'├'
#define RTEE     L'┤'
#define BTEE     L'┴'
#define TTEE     L'┬'
#define PLUS     L'─'
#define RGB(t, r, g, b) "\033[38;2;" #r ";" #g ";" #b "m" t "\033[0m"
#define SET_COLOR(r, g, b) wprintf(L"\033[38;2;" #r ";" #g ";" #b "m")
#define RESET_COLOR() wprintf(L"\033[0m")

typedef struct {
	unsigned width;
	unsigned height;
	unsigned x;
	unsigned y;
	unsigned has_borders;
} Window;

unsigned get_term_width();
unsigned get_term_height();

unsigned get_cursor_x_position();
unsigned get_cursor_y_position();

void inittui();
void endtui();

wchar_t getch();

void printfxy(unsigned x, unsigned y, const wchar_t * fmt, ...);

void printfxy_to_window(Window * win, int x, int y, const wchar_t * fmt, ...);
void draw_window(Window * win);

int is_echo();
void showecho(int echo);
void showcursor(int show);

void set_cursor_position(int x, int y);
void set_cursor_position_in_window(Window * win, int x, int y);

#endif
