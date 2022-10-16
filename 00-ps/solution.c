#include "solution.h"
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define SIZE 100

void FreeDoubleArray(char** arr, const size_t len) {
	for (size_t count = 0; count < len; ++count) {
		if (arr[count] != NULL)
			free(arr[count]);
	}
	free(arr);
}

bool IsPID(const char* name) {
	int count = 0;
    while(name[count] != '\0') {
        if(!isdigit(name[count])) {
            return false;
        }
		count++;
    }
    return true;
}

char* GetNextPID(DIR* dirp) {
    char* name = NULL;
    struct dirent* dir_info;
    while((dir_info = readdir(dirp))) {
	if (!errno) {
		report_error("/proc/", errno);
	}
        if(IsPID(dir_info->d_name)) {
            name = dir_info->d_name;
            break;
        }
    }
    return name;
}

size_t ParseDataFromFile(const char* path, char*** arr) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        report_error(path, errno);
        exit(0);
    }
    char buf[1];
    ssize_t n;
    size_t str_pos = 0;
    size_t elem_pos = 0;
    size_t cur_arr_len = SIZE;
    size_t cur_str_len = SIZE;
    char** tmp_arr = (char** )malloc(SIZE * sizeof(char* ));
    for (size_t count = 0; count < SIZE; ++count) {
		tmp_arr[count] = (char* )malloc(SIZE * sizeof(char));
		memset(tmp_arr[count], '\0', SIZE * sizeof(char));
    }
    while ((n = read(fd, buf, 1)) != 0 ) {
        if (n < 0) {
            report_error(path, errno);
            exit(0);
        }
        if (buf[0] == '\0') {
            elem_pos++;
            if (elem_pos == cur_arr_len) {
                char** new_tmp_arr = (char** )realloc(tmp_arr, 2 * cur_arr_len * sizeof(char* ));
				if (new_tmp_arr == NULL) {
					report_error(path, errno);
					exit(0);
				}
				tmp_arr = new_tmp_arr;
				for (size_t count = cur_arr_len; count < 2 * cur_arr_len; ++count) {
					tmp_arr[count] = (char* )malloc(SIZE * sizeof(char));
					memset(tmp_arr[count], '\0', SIZE * sizeof(char));
				}
				cur_arr_len *= 2;
            }
            str_pos = 0;
	    	cur_str_len = SIZE;
        }
        else {
			if (str_pos == cur_str_len) {
				char* new_str = (char* )realloc(tmp_arr[elem_pos], 2 * cur_str_len * sizeof(char));
				if (new_str == NULL) {
					report_error(path, errno);
					exit(0);
				}
				tmp_arr[elem_pos] = new_str;
				memset(&tmp_arr[elem_pos][str_pos], '\0', cur_str_len * sizeof(char));
				cur_str_len *= 2;
			}
        	tmp_arr[elem_pos][str_pos] = buf[0];
			str_pos++;
        }
    }
    if (elem_pos == 0) {
        *arr = (char** )malloc(sizeof(char*));
        (*arr)[0] = NULL;
	FreeDoubleArray(tmp_arr, cur_arr_len);
        return 1;
    }
    *arr = (char** )malloc((elem_pos + 1) * sizeof(char*));
    (*arr)[elem_pos] = NULL;
    for (size_t count = 0; count < elem_pos; ++count) {
        (*arr)[count] = (char* )malloc((strlen(tmp_arr[count]) + 1) * sizeof(char));
        memcpy((*arr)[count], tmp_arr[count], strlen(tmp_arr[count]) + 1);
    }
	FreeDoubleArray(tmp_arr, cur_arr_len);
    close(fd);
    return elem_pos + 1;
}

void GetPathToExe(const char* pid_str, char** exe_path) {
    char symlink_path[PATH_MAX] = { '\0' };
    char tmp_exe[PATH_MAX] = { '\0' };
    sprintf(symlink_path, "/proc/%s/exe", pid_str);
    ssize_t nbytes = readlink(symlink_path, tmp_exe, PATH_MAX);
    if (nbytes == -1)
        report_error(symlink_path, 2);
    size_t len = strlen(tmp_exe) + 1;
    *exe_path = (char* )malloc(len * sizeof(char));
    memcpy(*exe_path, tmp_exe, len);
}

size_t GetArgv(const char* pid_str, char*** argv) {
	char cmdline_path[PATH_MAX] = { '\0' };
	sprintf(cmdline_path, "/proc/%s/cmdline", pid_str);
	return ParseDataFromFile(cmdline_path, argv);
}

size_t GetEnvp(const char* pid_str, char*** envp) {
	char environ_path[PATH_MAX] = { '\0' };
	sprintf(environ_path, "/proc/%s/environ", pid_str);
	return ParseDataFromFile(environ_path, envp);
}

void ps(void) {
    char* pid_str;
    char* exe;
    char** argv;
    char** envp;
    DIR* dp = opendir("/proc");
    if (NULL == dp) {
        report_error("/proc", errno);
        exit(0);
    }
    while ((pid_str = GetNextPID(dp))) {
        GetPathToExe(pid_str, &exe);
        size_t argv_len = GetArgv(pid_str, &argv);
        size_t envp_len = GetEnvp(pid_str, &envp);
        report_process((pid_t)(atoi(pid_str)), exe, argv, envp);
        free(exe);
        FreeDoubleArray(argv, argv_len);
        FreeDoubleArray(envp, envp_len);
    }
    closedir(dp);
}
