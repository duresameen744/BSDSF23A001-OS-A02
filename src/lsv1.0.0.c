/*
 * Programming Assignment 02: lsv1.3.0
 * Feature 4 – Horizontal Display (-x)
 * Builds upon Feature 3 (Column Display)
 */

#define _POSIX_C_SOURCE 200809L   // Enables strdup() and POSIX APIs

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <getopt.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <sys/types.h>
#include <sys/ioctl.h>

extern int errno;

/* --- Function prototypes --- */
void do_ls(const char *dir, int long_listing, int horizontal);
void mode_to_string(mode_t mode, char *str);
void format_time(time_t mtime, char *time_str);
void display_columns(char **files, int count, int terminal_width);
void display_horizontal(char **files, int count, int terminal_width);

/* --- Convert mode to permission string (e.g., -rw-r--r--) --- */
void mode_to_string(mode_t mode, char *str)
{
    str[0] = (S_ISDIR(mode)) ? 'd' : (S_ISLNK(mode)) ? 'l' : '-';
    str[1] = (mode & S_IRUSR) ? 'r' : '-';
    str[2] = (mode & S_IWUSR) ? 'w' : '-';
    str[3] = (mode & S_IXUSR) ? 'x' : '-';
    str[4] = (mode & S_IRGRP) ? 'r' : '-';
    str[5] = (mode & S_IWGRP) ? 'w' : '-';
    str[6] = (mode & S_IXGRP) ? 'x' : '-';
    str[7] = (mode & S_IROTH) ? 'r' : '-';
    str[8] = (mode & S_IWOTH) ? 'w' : '-';
    str[9] = (mode & S_IXOTH) ? 'x' : '-';
    str[10] = '\0';
}

/* --- Format modification time ("Mon DD HH:MM") --- */
void format_time(time_t mtime, char *time_str)
{
    char *raw_time = ctime(&mtime);
    strncpy(time_str, raw_time + 4, 12); // e.g., "Sep 25 13:22"
    time_str[12] = '\0';
}

/* --- Column Display (Down Then Across) --- */
void display_columns(char **files, int count, int terminal_width)
{
    if (count == 0)
        return;

    int max_len = 0;
    for (int i = 0; i < count; i++) {
        int len = strlen(files[i]);
        if (len > max_len)
            max_len = len;
    }

    int col_width = max_len + 2; // Add spacing
    int num_cols = terminal_width / col_width;
    if (num_cols < 1) num_cols = 1;
    int num_rows = (count + num_cols - 1) / num_cols;

    for (int row = 0; row < num_rows; row++) {
        for (int col = 0; col < num_cols; col++) {
            int index = row + col * num_rows;
            if (index < count) {
                printf("%-*s", col_width, files[index]);
            }
        }
        printf("\n");
    }
}

/* --- Horizontal Display (-x): Left-to-right, wrapping as needed --- */
void display_horizontal(char **files, int count, int terminal_width)
{
    if (count == 0)
        return;

    int max_len = 0;
    for (int i = 0; i < count; i++) {
        int name_len = strlen(files[i]);
        if (name_len > max_len)
            max_len = name_len;
    }

    int spacing = 2;
    int col_width = max_len + spacing;
    if (col_width > terminal_width)
        col_width = terminal_width;

    int cur_width = 0;
    for (int i = 0; i < count; i++) {
        if (cur_width + col_width > terminal_width) {
            printf("\n");
            cur_width = 0;
        }
        printf("%-*s", col_width, files[i]);
        cur_width += col_width;
    }
    printf("\n");
}

/* --- Core LS Function (Handles -l, -x, default) --- */
void do_ls(const char *dir, int long_listing, int horizontal)
{
    struct dirent *entry;
    DIR *dp = opendir(dir);
    int count = 0;

    if (dp == NULL) {
        fprintf(stderr, "Cannot open directory: %s\n", dir);
        return;
    }

    // First pass: count and handle long listing
    errno = 0;
    while ((entry = readdir(dp)) != NULL) {
        if (entry->d_name[0] == '.')
            continue;
        count++;

        if (long_listing) {
            char full_path[1024];
            snprintf(full_path, sizeof(full_path), "%s/%s", dir, entry->d_name);

            struct stat file_stat;
            if (stat(full_path, &file_stat) == -1) {
                perror("stat");
                continue;
            }

            char permissions[11];
            mode_to_string(file_stat.st_mode, permissions);
            struct passwd *pw = getpwuid(file_stat.st_uid);
            struct group *gr = getgrgid(file_stat.st_gid);
            char time_str[13];
            format_time(file_stat.st_mtime, time_str);

            printf("%s %2lu %-8s %-8s %8ld %s %s\n",
                   permissions,
                   file_stat.st_nlink,
                   pw ? pw->pw_name : "unknown",
                   gr ? gr->gr_name : "unknown",
                   file_stat.st_size,
                   time_str,
                   entry->d_name);
        }
    }

    // Column or horizontal display
    if (!long_listing) {
        struct winsize w;
        int terminal_width = 80; // default fallback

        if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) != -1)
            terminal_width = w.ws_col;

        char **files = malloc(count * sizeof(char *));
        if (!files) {
            perror("malloc");
            closedir(dp);
            return;
        }

        rewinddir(dp);
        int index = 0;
        errno = 0;
        while ((entry = readdir(dp)) != NULL) {
            if (entry->d_name[0] == '.')
                continue;
            files[index] = strdup(entry->d_name);
            index++;
        }

        // Choose layout
        if (horizontal)
            display_horizontal(files, count, terminal_width);
        else
            display_columns(files, count, terminal_width);

        for (int i = 0; i < count; i++)
            free(files[i]);
        free(files);
    }

    if (errno != 0)
        perror("readdir failed");

    closedir(dp);
}

/* --- Main Function --- */
int main(int argc, char const *argv[])
{
    int long_listing = 0;
    int horizontal = 0;
    int opt;

    while ((opt = getopt(argc, (char *const *)argv, "lx")) != -1) {
        switch (opt) {
            case 'l':
                long_listing = 1;
                break;
            case 'x':
                horizontal = 1;
                break;
            default:
                fprintf(stderr, "Usage: %s [-l | -x] [directory...]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    int non_opt_args = argc - optind;

    if (non_opt_args == 0) {
        do_ls(".", long_listing, horizontal);
    } else {
        for (int i = optind; i < argc; i++) {
            printf("Directory listing of %s:\n", argv[i]);
            do_ls(argv[i], long_listing, horizontal);
            puts("");
        }
    }

    return 0;
}
