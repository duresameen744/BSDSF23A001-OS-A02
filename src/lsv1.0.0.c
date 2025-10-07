/*
 * ls-v1.5.0 -> Colorized Output (Feature 6)
 * Builds on v1.4.0 (alphabetical sort + -l and -x)
 */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <sys/ioctl.h>
#include <getopt.h>
#include <errno.h>
#include <limits.h>

/* --- ANSI color macros --- */
#define COLOR_RESET   "\033[0m"
#define COLOR_BLUE    "\033[0;34m"   /* directories */
#define COLOR_GREEN   "\033[0;32m"   /* executables */
#define COLOR_RED     "\033[0;31m"   /* archives */
#define COLOR_PINK    "\033[1;35m"   /* symbolic links */
#define COLOR_REVERSE "\033[7m"      /* special files */

/* --- Function declarations --- */
void do_ls(const char *path, bool long_listing, bool horizontal);
void simple_display(const char *path);
void handle_long_listing(const char *path);
void handle_horizontal_display(const char *path);
void print_permissions(mode_t mode);
void format_time(time_t mtime, char *time_str);
int compare_strings(const void *a, const void *b);
char **read_and_sort_filenames(const char *path, int *file_count);
int get_terminal_width(void);
void print_colorized(const char *dir, const char *name);

/* --- Get terminal width --- */
int get_terminal_width(void) {
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == -1 || w.ws_col <= 0)
        return 80;
    return w.ws_col;
}

/* --- Compare strings for qsort --- */
int compare_strings(const void *a, const void *b) {
    const char *str1 = *(const char **)a;
    const char *str2 = *(const char **)b;
    return strcmp(str1, str2);
}

/* --- Read directory entries and sort alphabetically --- */
char **read_and_sort_filenames(const char *path, int *file_count) {
    DIR *dir = opendir(path);
    if (!dir) return NULL;

    size_t capacity = 64;
    char **filenames = malloc(capacity * sizeof(char *));
    if (!filenames) {
        closedir(dir);
        return NULL;
    }

    int count = 0;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue; // skip hidden

        if (count >= (int)capacity) {
            capacity *= 2;
            char **tmp = realloc(filenames, capacity * sizeof(char *));
            if (!tmp) break;
            filenames = tmp;
        }
        filenames[count] = strdup(entry->d_name);
        if (!filenames[count]) break;
        count++;
    }
    closedir(dir);

    if (count > 0)
        qsort(filenames, count, sizeof(char *), compare_strings);

    *file_count = count;
    return filenames;
}

/* --- Print permissions like rwxrwxrwx --- */
void print_permissions(mode_t mode) {
    /* File type */
    if (S_ISDIR(mode))
        putchar('d');
    else if (S_ISLNK(mode))
        putchar('l');
    else
        putchar('-');

    /* Owner permissions */
    putchar((mode & S_IRUSR) ? 'r' : '-');
    putchar((mode & S_IWUSR) ? 'w' : '-');
    putchar((mode & S_IXUSR) ? 'x' : '-');

    /* Group permissions */
    putchar((mode & S_IRGRP) ? 'r' : '-');
    putchar((mode & S_IWGRP) ? 'w' : '-');
    putchar((mode & S_IXGRP) ? 'x' : '-');

    /* Others permissions */
    putchar((mode & S_IROTH) ? 'r' : '-');
    putchar((mode & S_IWOTH) ? 'w' : '-');
    putchar((mode & S_IXOTH) ? 'x' : '-');
}

/* --- Format time --- */
void format_time(time_t mtime, char *time_str) {
    struct tm *tm_info = localtime(&mtime);
    if (!tm_info) {
        strcpy(time_str, "??? ?? ??:??");
        return;
    }
    strftime(time_str, 13, "%b %e %H:%M", tm_info);
}

/* --- Print a filename with color based on file type --- */
void print_colorized(const char *dir, const char *name) {
    char full[PATH_MAX];
    snprintf(full, sizeof(full), "%s/%s", dir, name);

    struct stat st;
    if (lstat(full, &st) == -1) {
        printf("%s", name);
        return;
    }

    const char *color = COLOR_RESET;

    if (S_ISDIR(st.st_mode))
        color = COLOR_BLUE;
    else if (S_ISLNK(st.st_mode))
        color = COLOR_PINK;
    else if (S_ISCHR(st.st_mode) || S_ISBLK(st.st_mode) ||
             S_ISSOCK(st.st_mode) || S_ISFIFO(st.st_mode))
        color = COLOR_REVERSE;
    else if (st.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH))
        color = COLOR_GREEN;
    else if (strstr(name, ".tar") || strstr(name, ".gz") || strstr(name, ".zip"))
        color = COLOR_RED;

    printf("%s%s%s", color, name, COLOR_RESET);
}

/* --- Simple column display --- */
void simple_display(const char *path) {
    int count;
    char **files = read_and_sort_filenames(path, &count);
    if (!files || count == 0) {
        if (files) free(files);
        return;
    }

    int max_len = 0;
    for (int i = 0; i < count; i++) {
        int len = strlen(files[i]);
        if (len > max_len) max_len = len;
    }

    int width = get_terminal_width();
    int col_width = max_len + 2;
    int cols = width / col_width;
    if (cols < 1) cols = 1;
    int rows = (count + cols - 1) / cols;

    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            int idx = r + c * rows;
            if (idx < count) {
                print_colorized(path, files[idx]);
                int pad = col_width - strlen(files[idx]);
                for (int p = 0; p < pad; p++) putchar(' ');
            }
        }
        putchar('\n');
    }

    for (int i = 0; i < count; i++) free(files[i]);
    free(files);
}

/* --- Horizontal display (-x) --- */
void handle_horizontal_display(const char *path) {
    int count;
    char **files = read_and_sort_filenames(path, &count);
    if (!files || count == 0) {
        if (files) free(files);
        return;
    }

    int max_len = 0;
    for (int i = 0; i < count; i++) {
        int len = strlen(files[i]);
        if (len > max_len) max_len = len;
    }

    int width = get_terminal_width();
    int col_width = max_len + 2;
    int current = 0;

    for (int i = 0; i < count; i++) {
        int needed = col_width;
        if (current + needed > width && current > 0) {
            putchar('\n');
            current = 0;
        }

        print_colorized(path, files[i]);
        int pad = col_width - strlen(files[i]);
        for (int p = 0; p < pad; p++) putchar(' ');
        current += col_width;
    }

    putchar('\n');

    for (int i = 0; i < count; i++) free(files[i]);
    free(files);
}

/* --- Long listing (-l) --- */
void handle_long_listing(const char *path) {
    int count;
    char **files = read_and_sort_filenames(path, &count);
    if (!files || count == 0) {
        if (files) free(files);
        return;
    }

    struct file_info {
        char *name;
        struct stat st;
        char time_str[13];
        char *owner;
        char *group;
        char *link_target;
    } *info = malloc(count * sizeof(struct file_info));

    if (!info) {
        for (int i = 0; i < count; i++) free(files[i]);
        free(files);
        return;
    }

    size_t links_w = 1, owner_w = 1, group_w = 1, size_w = 1;

    for (int i = 0; i < count; i++) {
        info[i].name = files[i];
        info[i].link_target = NULL;

        char full[PATH_MAX];
        snprintf(full, sizeof(full), "%s/%s", path, files[i]);

        if (lstat(full, &info[i].st) == -1) continue;

        format_time(info[i].st.st_mtime, info[i].time_str);

        struct passwd *pw = getpwuid(info[i].st.st_uid);
        struct group *gr = getgrgid(info[i].st.st_gid);
        info[i].owner = strdup(pw ? pw->pw_name : "?");
        info[i].group = strdup(gr ? gr->gr_name : "?");

        if (S_ISLNK(info[i].st.st_mode)) {
            char target[PATH_MAX];
            ssize_t len = readlink(full, target, sizeof(target) - 1);
            if (len > 0) {
                target[len] = '\0';
                info[i].link_target = strdup(target);
            }
        }

        size_t len;
        len = snprintf(NULL, 0, "%lu", (unsigned long)info[i].st.st_nlink);
        if (len > links_w) links_w = len;

        len = strlen(info[i].owner);
        if (len > owner_w) owner_w = len;

        len = strlen(info[i].group);
        if (len > group_w) group_w = len;

        len = snprintf(NULL, 0, "%lld", (long long)info[i].st.st_size);
        if (len > size_w) size_w = len;
    }

    for (int i = 0; i < count; i++) {
        print_permissions(info[i].st.st_mode);
        printf(" %*lu %-*s %-*s %*lld %s ",
               (int)links_w, (unsigned long)info[i].st.st_nlink,
               (int)owner_w, info[i].owner,
               (int)group_w, info[i].group,
               (int)size_w, (long long)info[i].st.st_size,
               info[i].time_str);

        print_colorized(path, info[i].name);
        if (info[i].link_target) printf(" -> %s", info[i].link_target);
        putchar('\n');
    }

    for (int i = 0; i < count; i++) {
        free(info[i].owner);
        free(info[i].group);
        if (info[i].link_target) free(info[i].link_target);
    }
    free(info);

    for (int i = 0; i < count; i++) free(files[i]);
    free(files);
}

/* --- Master dispatcher --- */
void do_ls(const char *path, bool long_listing, bool horizontal) {
    if (long_listing)
        handle_long_listing(path);
    else if (horizontal)
        handle_horizontal_display(path);
    else
        simple_display(path);
}

/* --- main() --- */
int main(int argc, char *argv[]) {
    int opt;
    bool long_listing = false;
    bool horizontal = false;

    while ((opt = getopt(argc, argv, "lx")) != -1) {
        switch (opt) {
            case 'l': long_listing = true; break;
            case 'x': horizontal = true; break;
            default:
                fprintf(stderr, "Usage: %s [-l | -x] [directory]\n", argv[0]);
                return 1;
        }
    }

    const char *path = (optind < argc) ? argv[optind] : ".";
    do_ls(path, long_listing, horizontal);
    return 0;
}
