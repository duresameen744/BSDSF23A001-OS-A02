/*
 * lsv1.6.0 - Recursive Listing (-R) with colorized output
 * Builds on earlier features: -l, -x, alphabetical sort, color output
 */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <sys/ioctl.h>
#include <getopt.h>
#include <limits.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

/* ANSI colors */
#define COLOR_RESET   "\033[0m"
#define COLOR_BLUE    "\033[0;34m"   /* directories */
#define COLOR_GREEN   "\033[0;32m"   /* executables */
#define COLOR_RED     "\033[0;31m"   /* archives */
#define COLOR_PINK    "\033[1;35m"   /* symbolic links */
#define COLOR_REVERSE "\033[7m"      /* special files */

enum DisplayMode { DEFAULT_MODE, LONG_MODE, HORIZONTAL_MODE };

/* Prototypes */
void do_ls(const char *dir, enum DisplayMode mode, int recursive);
char **read_dir_filenames(const char *dir, int *count, int *maxlen);
int compare_names(const void *a, const void *b);
int get_terminal_width(void);
void print_colorized(const char *dir, const char *filename);
void display_columns_default(char **names, int n, int maxlen, const char *dir);
void display_horizontal(char **names, int n, int maxlen, const char *dir);
void display_long(const char *path, const char *filename);

/* --- get terminal width (fallback 80) --- */
int get_terminal_width(void)
{
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == -1 || w.ws_col <= 0)
        return 80;
    return w.ws_col;
}

/* --- alphabetical compare for qsort --- */
int compare_names(const void *a, const void *b) {
    const char *n1 = *(const char **)a;
    const char *n2 = *(const char **)b;
    return strcmp(n1, n2);
}

/* --- colorized printing, quiet on stat failure --- */
void print_colorized(const char *dir, const char *filename) {
    char full[PATH_MAX];
    if (snprintf(full, sizeof(full), "%s/%s", dir, filename) >= (int)sizeof(full)) {
        /* too long, print plain */
        printf("%s", filename);
        return;
    }

    struct stat st;
    if (lstat(full, &st) < 0) {
        /* cannot stat, print plain (do not spam perror) */
        printf("%s", filename);
        return;
    }

    const char *color = COLOR_RESET;

    if (S_ISDIR(st.st_mode)) color = COLOR_BLUE;
    else if (S_ISLNK(st.st_mode)) color = COLOR_PINK;
    else if (S_ISCHR(st.st_mode) || S_ISBLK(st.st_mode) ||
             S_ISSOCK(st.st_mode) || S_ISFIFO(st.st_mode)) color = COLOR_REVERSE;
    else if (st.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)) color = COLOR_GREEN;
    else {
        /* archive detection by suffix */
        const char *ext = strrchr(filename, '.');
        if (ext && (strcmp(ext, ".tar") == 0 || strcmp(ext, ".gz") == 0 ||
                    strcmp(ext, ".zip") == 0 || strcmp(ext, ".bz2") == 0 ||
                    strcmp(ext, ".xz") == 0 || strcmp(ext, ".tgz") == 0))
            color = COLOR_RED;
    }

    printf("%s%s%s", color, filename, COLOR_RESET);
}

/* --- read dir entries into allocated array, return count and maxlen --- */
char **read_dir_filenames(const char *dir, int *count, int *maxlen) {
    DIR *dp = opendir(dir);
    if (!dp) return NULL;

    struct dirent *entry;
    size_t cap = 64;
    char **names = malloc(cap * sizeof(char *));
    if (!names) { closedir(dp); return NULL; }

    int n = 0;
    int maxl = 0;
    errno = 0;
    while ((entry = readdir(dp)) != NULL) {
        if (entry->d_name[0] == '.') continue;  /* skip hidden */
        int len = (int)strlen(entry->d_name);
        if (len > maxl) maxl = len;

        if (n >= (int)cap) {
            cap *= 2;
            char **tmp = realloc(names, cap * sizeof(char *));
            if (!tmp) break;
            names = tmp;
        }
        names[n] = strdup(entry->d_name);
        if (!names[n]) break;
        n++;
    }

    if (errno != 0) {
        for (int i = 0; i < n; ++i) free(names[i]);
        free(names);
        closedir(dp);
        return NULL;
    }

    closedir(dp);

    if (n > 0) qsort(names, n, sizeof(char *), compare_names);

    *count = n;
    *maxlen = maxl;
    return names;
}

/* --- long listing (single file) --- */
void display_long(const char *path, const char *filename) {
    char full[PATH_MAX];
    if (snprintf(full, sizeof(full), "%s/%s", path, filename) >= (int)sizeof(full)) return;

    struct stat st;
    if (lstat(full, &st) < 0) {
        /* skip printing if cannot stat */
        return;
    }

    /* File type */
    if (S_ISDIR(st.st_mode)) putchar('d');
    else if (S_ISLNK(st.st_mode)) putchar('l');
    else if (S_ISCHR(st.st_mode)) putchar('c');
    else if (S_ISBLK(st.st_mode)) putchar('b');
    else if (S_ISFIFO(st.st_mode)) putchar('p');
    else if (S_ISSOCK(st.st_mode)) putchar('s');
    else putchar('-');

    /* Permissions (owner/group/other), handle special bits if needed */
    putchar((st.st_mode & S_IRUSR) ? 'r' : '-');
    putchar((st.st_mode & S_IWUSR) ? 'w' : '-');
    putchar((st.st_mode & S_IXUSR) ? 'x' : '-');
    putchar((st.st_mode & S_IRGRP) ? 'r' : '-');
    putchar((st.st_mode & S_IWGRP) ? 'w' : '-');
    putchar((st.st_mode & S_IXGRP) ? 'x' : '-');
    putchar((st.st_mode & S_IROTH) ? 'r' : '-');
    putchar((st.st_mode & S_IWOTH) ? 'w' : '-');
    putchar((st.st_mode & S_IXOTH) ? 'x' : '-');

    /* link count, owner, group, size */
    struct passwd *pw = getpwuid(st.st_uid);
    struct group *gr = getgrgid(st.st_gid);
    char timebuf[64];
    struct tm *tm_info = localtime(&st.st_mtime);
    if (tm_info) strftime(timebuf, sizeof(timebuf), "%b %e %H:%M", tm_info);
    else strncpy(timebuf, "??? ?? ??:??", sizeof(timebuf));

    printf(" %2lu %-8s %-8s %8lld %s ",
           (unsigned long)st.st_nlink,
           pw ? pw->pw_name : "-",
           gr ? gr->gr_name : "-",
           (long long)st.st_size,
           timebuf);

    /* name (colorized) */
    print_colorized(path, filename);

    /* symlink target */
    if (S_ISLNK(st.st_mode)) {
        char target[PATH_MAX];
        ssize_t len = readlink(full, target, sizeof(target) - 1);
        if (len > 0) {
            target[len] = '\0';
            printf(" -> %s", target);
        }
    }

    putchar('\n');
}

/* --- default column display (down then across) --- */
void display_columns_default(char **names, int n, int maxlen, const char *dir) {
    if (n <= 0) return;
    int width = get_terminal_width();
    int spacing = 2;
    int col_width = maxlen + spacing;
    if (col_width < 1) col_width = 1;
    int cols = width / col_width;
    if (cols < 1) cols = 1;
    if (cols > n) cols = n;
    int rows = (n + cols - 1) / cols;

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            int i = c * rows + r;
            if (i >= n) continue;
            print_colorized(dir, names[i]);
            int pad = col_width - (int)strlen(names[i]);
            for (int p = 0; p < pad; ++p) putchar(' ');
        }
        putchar('\n');
    }
}

/* --- horizontal (-x) left-to-right across rows --- */
void display_horizontal(char **names, int n, int maxlen, const char *dir) {
    if (n <= 0) { putchar('\n'); return; }
    int width = get_terminal_width();
    int spacing = 2;
    int col_width = maxlen + spacing;
    if (col_width < 1) col_width = 1;
    if (col_width > width) col_width = width;

    int cur = 0;
    for (int i = 0; i < n; ++i) {
        if (cur > 0 && cur + col_width > width) {
            putchar('\n');
            cur = 0;
        }
        print_colorized(dir, names[i]);
        int pad = col_width - (int)strlen(names[i]);
        for (int p = 0; p < pad; ++p) putchar(' ');
        cur += col_width;
    }
    putchar('\n');
}

/* --- recursive core --- */
void do_ls(const char *dir, enum DisplayMode mode, int recursive) {
    int n = 0, maxlen = 0;
    char **names = read_dir_filenames(dir, &n, &maxlen);
    if (!names) return;

    /* Print directory header only when recursive (so each directory in recursion shows header).
       Top-level multiple-directory headers are handled in main (only when not recursive). */
    if (recursive) {
        printf("%s:\n", dir);
    }

    /* display */
    if (mode == LONG_MODE) {
        for (int i = 0; i < n; ++i) display_long(dir, names[i]);
    } else if (mode == HORIZONTAL_MODE) {
        display_horizontal(names, n, maxlen, dir);
    } else {
        display_columns_default(names, n, maxlen, dir);
    }

    /* recurse into directories */
    if (recursive) {
        for (int i = 0; i < n; ++i) {
            /* build path */
            char path[PATH_MAX];
            if (snprintf(path, sizeof(path), "%s/%s", dir, names[i]) >= (int)sizeof(path)) continue;

            struct stat st;
            if (lstat(path, &st) == 0) {
                if (S_ISDIR(st.st_mode) && strcmp(names[i], ".") != 0 && strcmp(names[i], "..") != 0) {
                    putchar('\n');  /* separate directories */
                    do_ls(path, mode, recursive);
                }
            }
        }
    }

    /* cleanup */
    for (int i = 0; i < n; ++i) free(names[i]);
    free(names);
}

/* --- main --- */
int main(int argc, char *argv[]) {
    int opt;
    enum DisplayMode mode = DEFAULT_MODE;
    int recursive_flag = 0;

    while ((opt = getopt(argc, argv, "lxR")) != -1) {
        switch (opt) {
            case 'l': mode = LONG_MODE; break;
            case 'x': mode = HORIZONTAL_MODE; break;
            case 'R': recursive_flag = 1; break;
            default:
                fprintf(stderr, "Usage: %s [-l | -x | -R] [directory...]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    /* if no dir args, use "." */
    if (optind == argc) {
        do_ls(".", mode, recursive_flag);
    } else {
        int multi = (argc - optind > 1);
        for (int i = optind; i < argc; ++i) {
            /* print header for multiple directories only when NOT using recursive mode
               (do_ls prints headers for recursive traversal itself) */
            if (multi && !recursive_flag) {
                printf("%s:\n", argv[i]);
            }
            do_ls(argv[i], mode, recursive_flag);
            if (i < argc - 1) putchar('\n');
        }
    }
    return 0;
}
