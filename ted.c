#include "tui.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <locale.h>

#define BUFINC 128

int logi10(int a) {
	for (int i = -1;; ++i) {
		a /= 10;
		if (a == 0)
			return i;
	}
}

int visual_str_len(wchar_t * str, unsigned max_len) {
	int l = 0;
	
	for (int i = 0; str[i] && i < max_len; ++i) {
		l += 1 + (str[i] == '\t') * (TABSIZ - 1);
	}

	return l;
}

typedef struct {
	wchar_t * content;
	unsigned nwchar_t;
} Line;

struct File {
	Line * line;
	unsigned nline;
} file;

void move_cursor_to_line(unsigned y);
void move_cursor_to_column(unsigned x);
int get_current_line_length();
int get_num_lines();
void init_file();
void draw_file_to_window(Window * win, Window * numwin);
void update_window_sizes();
void update_cursor_position_scroll();
void correct_cursor_column();
void correct_cursor_line();
void correct_cursor();
void set_multiplicator(wchar_t);
void insert_wchar_at_position();
void check_line_length(Line * line, unsigned needed);
void check_number_of_lines(unsigned needed);
void insert_line_at_position(unsigned index, const wchar_t * content, unsigned size);
void remove_wchar_at_position(Line * line, unsigned index);
void remove_line_at_position(unsigned index);

void move_cursor_up();
void move_cursor_down();
void move_cursor_left();
void move_cursor_right();
void go_bottom();
void goto_call();
void write();
void quit();
void set_insert();
void append();
void insert_at_start();
void append_at_end();
void delete_character();
void delete();

void normal();
void insert();

enum MODES {
	NORMAL,
	INSERT
};

const wchar_t * mode_desc[2] = {
	L"Normal",
	L"Insert"
};

void (*mode[3])(void) = {
	[NORMAL] = normal,
	[INSERT] = insert,
};

void (*key_cmd[256])(void) = {
	['j'] = move_cursor_down,
	['k'] = move_cursor_up,
	['h'] = move_cursor_left,
	['l'] = move_cursor_right,
	['G'] = go_bottom,
	['g'] = goto_call,
	['i'] = set_insert,
	['a'] = append,
	['I'] = insert_at_start,
	['A'] = append_at_end,
	['d'] = delete,
	['x'] = delete_character,
	['w'] = write,
	['q'] = quit
};

struct Cursor {
	int line;
	int column;
	int x;
	int y;
} cursor;

Window main_window;
Window num_window;
Window status_bar;

int running = 1;
unsigned scroll = 0;
unsigned multiplicator = 1;
unsigned mulset = 0;
unsigned active_mode = 0;
char * path;

int get_num_lines() {
	int i = 0;
	for (i = 0; file.line[i].content; ++i);

	return i;
}

void quit() {
	running = 0;
}

void init_file() {
	FILE * f = fopen(path, "r");


	file.line = malloc(sizeof(Line) * BUFINC);
	file.nline = BUFINC;

	if (f) {
		wchar_t buf[BUFSIZ];

		int i = 0;

		while (fgetws(buf, BUFSIZ, f) != NULL) {
			buf[wcslen(buf) - 1] = L'\0';

			if (i == file.nline - 1) {
				file.nline += BUFINC;
				file.line = realloc(file.line, sizeof(Line) * file.nline);
			}

			file.line[i].nwchar_t = wcslen(buf) + 1;
			file.line[i].content = malloc(file.line[i].nwchar_t * sizeof(wchar_t));
			wcsncpy(file.line[i].content, buf, file.line[i].nwchar_t);
			++i;
		}

		fclose(f);
	}


	if (get_num_lines() == 0) {
		file.line[0].nwchar_t = BUFINC;
		file.line[0].content = malloc(file.line[0].nwchar_t * sizeof(wchar_t));
	}
}

void write() {
	FILE * f = fopen(path, "w");

	if (!f) {
		perror("ted");
		quit();
	}

	for (int i = 0; i < get_num_lines(); ++i) {
		fputws(file.line[i].content, f);
		fputwc('\n', f);
	}

	fclose(f);
}

void draw_file_to_window(Window * win, Window * numwin) {
	int y = -scroll;
	for (int i = 0; i < file.nline && file.line[i].content && y < (int)main_window.height; ++i) {
		printfxy_to_window(win, 0, y, L"%ls¬", file.line[i].content);
		printfxy_to_window(numwin, 0, y + 1 + win->has_borders, cursor.line == i ? L"> %d" : L"%d", i + 1);
		y += visual_str_len(file.line[i].content, file.line[i].nwchar_t) / win->width + 1;
	}
}

void move_cursor_up() {
	cursor.line -= multiplicator;
	correct_cursor();
}

void move_cursor_down() {
	cursor.line += multiplicator;
	correct_cursor();
}

void move_cursor_left() {
	cursor.column -= multiplicator;
	correct_cursor_column();
}

void move_cursor_right() {
	cursor.column += multiplicator;
	correct_cursor_column();
}

void go_bottom() {
	cursor.line = get_num_lines() - 1;
	cursor.column = 0;
}

void goto_call() {
	wchar_t c = getch();

	switch(c) {
		case 'g': cursor.line = multiplicator < get_num_lines() ? multiplicator - 1 : get_num_lines() - 1;
							cursor.column = 0;
							break;
	}
}

void delete() {
	wchar_t c = getch();

	switch(c) {
		case 'd': remove_line_at_position(cursor.line);
	}

	correct_cursor();
}

void delete_character() {
	remove_wchar_at_position(&file.line[cursor.line], cursor.column);
	correct_cursor_column();
}

void update_window_sizes() {

	num_window.x = 0;
	num_window.y = 0;
	num_window.width = logi10(get_num_lines()) + 5;
	num_window.height = get_term_height();
	num_window.has_borders = 0;

	main_window.width = get_term_width() - num_window.width;
	main_window.height = num_window.height - 1;
	main_window.x = 1 + num_window.width;
	main_window.y = 1;
	main_window.has_borders = 0;

	status_bar.y = main_window.height + 1;
	status_bar.height = 1;
	status_bar.width = main_window.width + num_window.width;
	status_bar.x = 1;
	status_bar.has_borders = 0;
}

void update_cursor_position_and_scroll() {
	cursor.x = main_window.has_borders + main_window.x;
	cursor.y = main_window.has_borders + main_window.y;

	cursor.x += visual_str_len(file.line[cursor.line].content, cursor.column) % main_window.width;

	for (int i = 0; i < cursor.line; ++i) {
		cursor.y += visual_str_len(file.line[i].content, file.line[i].nwchar_t) / main_window.width + 1;
	}

	cursor.y += cursor.column / main_window.width;

	if (cursor.y - (int)scroll > (int)main_window.height) {
		scroll = cursor.y - main_window.height;
	} else if (cursor.y - (int)scroll < 1) {
		scroll = cursor.y - 1;
	}

	cursor.y -= scroll;
}

void set_multiplicator(wchar_t c) {
	if (mulset) {
		if (multiplicator * 10 + c - 48 < 1000)
			multiplicator = multiplicator * 10 + c - 48;
	} else {
		multiplicator = c - 48;
	}
}

void correct_cursor_column() {
	Line * line = &file.line[cursor.line];

	if (cursor.column + 1 > wcslen(line->content)) {
		if (wcslen(line->content) == 0) {
			cursor.column = 0;
		} else {
			cursor.column = wcslen(line->content) - 1;
		}
	} else if (cursor.column < 0) {
		cursor.column = 0;
	}
}

void correct_cursor_line() {
	if (cursor.line >= get_num_lines()) {
		cursor.line = get_num_lines() - 1;
	} else if (cursor.line < 0) {
		cursor.line = 0;
	}
}

void correct_cursor() {
	correct_cursor_line();
	correct_cursor_column();
}

void set_insert() {
	active_mode = INSERT;
}

void append() {
	active_mode = INSERT;
	if (wcslen(file.line[cursor.line].content) > 0)
		++cursor.column;
}

void append_at_end() {
	active_mode = INSERT;
	cursor.column = wcslen(file.line[cursor.line].content);
}

void insert_at_start() {
	active_mode = INSERT;
	cursor.column = 0;
}

void normal() {
	wchar_t c = getch();

	if (*key_cmd[c]) {
		(*key_cmd[c])();
		mulset = 0;
		multiplicator = 1;
	} else if(c > 47 && c < 58) {
		set_multiplicator(c);
		mulset = 1;
	}
}

void insert() {
	wchar_t c = getch();
	Line * line = &file.line[cursor.line];

	if (c == 27) {
		active_mode = NORMAL;
	} else if (c == 127) {
		if (cursor.column > 0) {
			remove_wchar_at_position(line, --cursor.column);
		} else if (cursor.line > 0) {
			unsigned n = wcslen(file.line[cursor.line - 1].content);
			check_line_length(&file.line[cursor.line - 1], file.line[cursor.line - 1].nwchar_t);

			wcsncat(file.line[cursor.line - 1].content, file.line[cursor.line].content, file.line[cursor.line].nwchar_t);
			remove_line_at_position(cursor.line--);
			cursor.column = n;
		}
	} else if (c == '\n') {
		check_number_of_lines(get_num_lines() + 1);
		insert_line_at_position(cursor.line, line->content + cursor.column, line->nwchar_t);
		file.line[cursor.line].content[cursor.column] = 0;
		++cursor.line;
		cursor.column = 0;
	} else {
		insert_wchar_at_position(line, cursor.column, c);
		++cursor.column;
	}
}

void remove_wchar_at_position(Line * line, unsigned index) {
	for (int i = index; i < wcslen(line->content) + 1; ++i)
		line->content[i] = line->content[i + 1];
}

void remove_line_at_position(unsigned index) {
	for (int i = index; i < get_num_lines() + 1; ++i)
		file.line[i] = file.line[i + 1];
}

void insert_line_at_position(unsigned index, const wchar_t * content, unsigned size) {
	for (int i = get_num_lines(); i > index; --i) {
		file.line[i] = file.line[i - 1];
	}
	file.line[index + 1].content = malloc(sizeof(wchar_t) * size);
	check_line_length(&file.line[index + 1], size);
	wmemset(file.line[index + 1].content, 0, file.line[index + 1].nwchar_t);
	wcsncpy(file.line[index + 1].content, content, size);
}

void insert_wchar_at_position(Line * line, unsigned index, wchar_t c) {
	check_line_length(line, wcslen(line->content));

	unsigned s = wcslen(line->content) + 1 - index;
	wchar_t * end = malloc(sizeof(wchar_t) * s);
	wcsncpy(end, line->content + index, s);
	line->content[index] = c;
	line->content[index + 1] = 0;
	wcsncat(line->content, end, s);
}

void check_line_length(Line * line, unsigned needed) {
	if (wcslen(line->content) + 1 == line->nwchar_t) {
		line->nwchar_t += BUFINC;
		line->content = realloc(line->content, sizeof(wchar_t) * line->nwchar_t);
	}
}

void check_number_of_lines(unsigned needed) {
	if (needed > file.nline) {
		file.nline += BUFINC;
		file.line = realloc(file.line, sizeof(Line) * file.nline);
	}
}

int main(int argc, char ** argv) {
	setlocale(LC_ALL, "");

	if (argc != 2) {
		fwprintf(stderr, L"%s [file]\n", argv[0]);
		return -1;
	}

	path = argv[1];
	init_file();

	inittui();
	showecho(0);

	cursor.line = 0;
	cursor.column = 0;

	while (running) {
		update_window_sizes();

		update_cursor_position_and_scroll();
		draw_window(&main_window);
		draw_window(&status_bar);
		draw_window(&num_window);
		draw_file_to_window(&main_window, &num_window);
		printfxy_to_window(&status_bar, 0, 0, L"-- %ls -- ", mode_desc[active_mode]);

		if (mulset) {
			printfxy_to_window(&status_bar, status_bar.width - logi10(multiplicator) - 2, 0, L"%d", multiplicator);
		}

		set_cursor_position(cursor.x, cursor.y);

		mode[active_mode]();
	}

	showcursor(1);

	endtui();
	return 0;
}
