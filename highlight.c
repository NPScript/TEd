#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <wchar.h>

// highlight tui.h -O ansi

wchar_t * get_highlight_buffer(const wchar_t * buf) {
	int fd1[2];
	int fd2[2];

	pid_t p;

	if (pipe(fd1) == -1) {
		fprintf(stderr, "Pipe Failed");
		return NULL;
	}
	if (pipe(fd2) == -1) {
		fprintf(stderr, "Pipe Failed");
		return NULL;
	}

	p = fork();

	if (p < 0) {
		fprintf(stderr, "fork Failed");
		return NULL;
	}

	else if (p > 0) {
		int bs = 1024;
		int ins = wcslen(buf) * 2;
		char * ret = malloc(bs);
		char * in = malloc(ins);
		wchar_t * out;

		sprintf(in, "%ls", buf);

		close(fd1[0]);

		write(fd1[1], in, ins);
		close(fd1[1]);

		wait(NULL);

		close(fd2[1]);

		while (read(fd2[0], ret + bs - 1024, 1024)) {
			bs *= 2;
			ret = realloc(ret, bs);
		}

		out = malloc(sizeof(wchar_t) * bs);
		swprintf(out, bs, L"%s", ret);
		close(fd2[0]);

		free(ret);

		return out;
	}

	else {
		close(fd1[1]);
		close(fd2[0]);

		dup2(fd1[0], STDIN_FILENO);
		dup2(fd2[1], STDOUT_FILENO);

		execl("/bin/highlight", "highlight", "-O", "truecolor", "--syntax", "c", NULL);

		close(fd1[0]);
		close(fd2[1]);

		exit(0);
	}
}
