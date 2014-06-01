#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <windows.h>

#include "synchro.h"

print_t g_print = NULL;

void set_print(print_t print) {
	g_print = print;
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
		DEBUG("%s does not exist\n", file);
	} else {
		DEBUG("%s exists\n", file);
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
	DEBUG("Copying: %s => %s\n", srcpath, destpath);
	char buf[buffer_size];

	FILE* source = fopen(srcpath, "rb");
	FILE* dest = fopen(destpath, "wb");

	int counter = 0;

	size_t size = 0;
	while ((size = fread(buf, 1, buffer_size, source))) {
		fwrite(buf, 1, size, dest);
		counter += size;
	}

	fclose(source);
	fclose(dest);
}

void copy_dir(const char* src, const char* dest) {
	if (!is_dir(dest)) {
		mkdir(dest);
	}
	DIR *d;
	d = opendir(src);
	if (!d) {
		DEBUG("Source does not exist: %s\n", src);
		return;
	}

	struct dirent *dir;
	while ((dir = readdir(d)) != NULL) {
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
			cp(src_filepath, dest_filepath, 1<<16);
		}
	}

	closedir(d);
}

void sync_dir(const char* src, const char* dst) {
	synchro_log("Starting sync dir: %s => %s\n", src, dst);

	DIR *d;
	d = opendir(src);
	if (!d) {
		DEBUG("Source does not exist: %s\n", src);
		return;
	}

	if (!is_dir(dst)) {
		copy_dir(src, dst);
		return;
	}

	struct dirent *dir;
	while ((dir = readdir(d)) != NULL) {
		char* fname = dir->d_name;
		if (strcmp(fname, ".") == 0 || strcmp(fname, "..") == 0) {
			continue;
		}

		char src_filepath[PATH_SIZE];
		snprintf(src_filepath, PATH_SIZE, "%s/%s", src, fname);
		char dest_filepath[PATH_SIZE];
		snprintf(dest_filepath, PATH_SIZE, "%s/%s", dst, fname);

		if (is_dir(src_filepath)) {
			sync_dir(src_filepath, dest_filepath);
		} else {
			if (!exists(dest_filepath) || is_more_recent(src_filepath, dest_filepath)) {
				cp(src_filepath, dest_filepath, 1<<16);
			}
		}
	}

	closedir(d);
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

	DEBUG("Is src more recent (%s)? %s\n", src, c);
	return src_statbuf.st_mtime > dst_statbuf.st_mtime;
}