#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <windows.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#include "synchro.h"

char* mode_map[] = {
	"preview", "real"
};

print_t g_print = NULL;
progress_value_t g_progress_value = NULL;
int g_abort = 0;
int g_total_estimated_step = 0;
int g_current_step = 0;
int g_mode = REAL_MODE;
clock_t g_last_call = 0;
clock_t g_min_delay = 300;
int g_log_level = 0;

void set_print(print_t print) {
	g_print = print;
}

void set_progress_value(progress_value_t progress_value) {
	g_progress_value = progress_value;
}

void set_progress_min_delay(int min_delay) {
	g_min_delay = (clock_t) min_delay;
}

void set_abort() {
	g_abort = 1;
}

void reset_abort() {
	g_abort = 0;
}

void set_mode(int mode) {
	g_mode = mode;
	if (mode == PREVIEW_MODE) {
		g_total_estimated_step = 0;
		g_current_step = 0;
	}
}

int get_total_step() {
	return g_total_estimated_step;
}

int is_aborted() {
	return g_abort;
}

void synchro_log(const char* format, ...) {
	char buf[BUFFER_SIZE];
	va_list params;
	va_start(params, format);
	vsnprintf(buf, BUFFER_SIZE, format, params);
	va_end(params);
	g_print(buf);
}

int exists(const char* file) {
	struct stat statbuf;
	if (stat(file, &statbuf) == -1) {
		DEBUG_LOG("%s does not exist\n", file);
	} else {
		DEBUG_LOG("%s exists\n", file);
	}
	return stat(file, &statbuf) != -1;
}

int is_dir(const char* file) {
	struct stat statbuf;
	if(stat(file, &statbuf) == -1) {
		return 0;
	}
	return S_ISDIR(statbuf.st_mode);
}

void cp(const char* srcpath, const char* destpath, int buffer_size) {
	INFO_LOG("Copying: %s => %s\n", srcpath, destpath);
	char buf[buffer_size];

	FILE* source = fopen(srcpath, "rb");
	FILE* dest = fopen(destpath, "wb");

	int counter = 0;

	size_t size = 0;
	while ((size = fread(buf, 1, buffer_size, source))) {
		if (g_abort) {
			break;
		}
		fwrite(buf, 1, size, dest);
		counter += size;
	}

	fclose(source);
	fclose(dest);

	if (g_abort) {
		unlink(destpath);
	}
}

int copy_dir(const char* src, const char* dest) {
	int result = 0;

	DEBUG_LOG("Starting copy dir in mode %s: %s => %s\n", mode_map[g_mode], src, dest);
	if (!is_dir(dest)) {
		REAL_MODE_EXEC(mkdir(dest));
	}

	DIR *d;
	d = opendir(src);
	if (!d) {
		ERROR_LOG("Cannot open directory for copy %s. Error(%d): %s\n", src, errno, strerror(errno));
		result = -1;
		goto cleanup;
	}

	struct dirent *dir;
	while ((dir = readdir(d)) != NULL) {
		if (g_abort) {
			break;
		}
		char* fname = dir->d_name;
		if (strcmp(fname, ".") == 0 || strcmp(fname, "..") == 0) {
			continue;
		}

		char src_filepath[PATH_SIZE];
		snprintf(src_filepath, PATH_SIZE, "%s/%s", src, fname);
		char dest_filepath[PATH_SIZE];
		snprintf(dest_filepath, PATH_SIZE, "%s/%s", dest, fname);

		if (is_dir(src_filepath)) {
			copy_dir(src_filepath, dest_filepath);
		} else {
			REAL_MODE_EXEC(cp(src_filepath, dest_filepath, 1<<16));
		}
	}

cleanup:
	if (d) {
		closedir(d);
	}

	return result;
}

int sync_dir(const char* src, const char* dst, int level) {
	DEBUG_LOG("Starting sync dir in mode %s: %s => %s\n", mode_map[g_mode], src, dst);
	if (level == 0) {
		REAL_MODE_EXEC();
	}
	int result = 0;

	DIR *d = NULL;
	d = opendir(src);
	if (!d) {
		ERROR_LOG("Cannot open directory for sync %s. Error(%d): %s\n", src, errno, strerror(errno));
		result = -1;
		goto cleanup;
	}

	if (!is_dir(dst)) {
		result = copy_dir(src, dst);
		goto cleanup;
	}

	struct dirent *dir;
	while ((dir = readdir(d)) != NULL) {
		if (g_abort) {
			break;
		}
		char* fname = dir->d_name;
		if (strcmp(fname, ".") == 0 || strcmp(fname, "..") == 0) {
			continue;
		}

		char src_filepath[PATH_SIZE];
		snprintf(src_filepath, PATH_SIZE, "%s/%s", src, fname);
		char dest_filepath[PATH_SIZE];
		snprintf(dest_filepath, PATH_SIZE, "%s/%s", dst, fname);

		if (is_dir(src_filepath)) {
			sync_dir(src_filepath, dest_filepath, level + 1);
		} else {
			if (!exists(dest_filepath) || is_more_recent(src_filepath, dest_filepath)) {
				REAL_MODE_EXEC(cp(src_filepath, dest_filepath, 1<<16));
			}
		}
	}

cleanup:
	if (d) {
		closedir(d);
	}

	if (g_abort) {
		result = 1;
	}
	return result;
}

int is_more_recent(const char* src, const char* dst) {
	struct stat src_statbuf;
	stat(src, &src_statbuf);
//	if(stat(src, &src_statbuf) == -1) {
//		return 0;
//	}
	struct stat dst_statbuf;
	stat(dst, &dst_statbuf);
//	if(stat(dst, &dst_statbuf) == -1) {
//		return 0;
//	}

#ifdef DEBUG_MODE
	char *c = "no";
	if (src_statbuf.st_mtime > dst_statbuf.st_mtime) {
		c = "yes";
	}
#endif

	DEBUG_LOG("Is src more recent (%s)? %s\n", src, c);
	return src_statbuf.st_mtime > dst_statbuf.st_mtime;
}

void inform_progress() {
	if (clock() - g_last_call < g_min_delay) {
		return;
	}
	g_last_call = clock();
	g_progress_value(g_current_step);
}