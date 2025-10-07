/*
<<<<<<< HEAD
 * ls-v1.5.0 -> Colorized Output (Feature 6)
 * Builds on v1.4.0 (alphabetical sort + -l and -x)
 */

#define _POSIX_C_SOURCE 200809L

=======
 * Programming Assignment 02: lsv1.4.0
 * Feature 5 – Alphabetical Sort
 * Complete implementation with alphabetical sorting for all display modes
 * FIXED: No compilation warnings
 */

#define _POSIX_C_SOURCE 200809L
>>>>>>> feature-alphabetical-sort-v1.4.0
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
<<<<<<< HEAD
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
=======

// Function declarations
>>>>>>> feature-alphabetical-sort-v1.4.0
void do_ls(const char *path, bool long_listing, bool horizontal);
void simple_display(const char *path);
void handle_long_listing(const char *path);
void handle_horizontal_display(const char *path);
void print_permissions(mode_t mode);
void format_time(time_t mtime, char *time_str);
int compare_strings(const void *a, const void *b);
char **read_and_sort_filenames(const char *path, int *file_count);
int get_terminal_width(void);
<<<<<<< HEAD
void print_colorized(const char *dir, const char *name);

/* --- Get terminal width --- */
int get_terminal_width(void) {
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == -1 || w.ws_col <= 0)
        return 80;
    return w.ws_col;
}

/* --- Compare strings for qsort --- */
=======

/* --- Get terminal width for column display --- */
int get_terminal_width(void) {
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == -1) {
        return 80;  // Fallback width
    }
    return w.ws_col;
}

/* --- qsort comparison function for alphabetical sorting --- */
>>>>>>> feature-alphabetical-sort-v1.4.0
int compare_strings(const void *a, const void *b) {
    const char *str1 = *(const char **)a;
    const char *str2 = *(const char **)b;
    return strcmp(str1, str2);
}

<<<<<<< HEAD
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
=======
/* --- Read directory and sort filenames alphabetically --- */
char **read_and_sort_filenames(const char *path, int *file_count) {
    DIR *dir = opendir(path);
    if (!dir) {
        perror("opendir");
        return NULL;
    }

    // Initial allocation
    size_t capacity = 64;
    char **filenames = malloc(capacity * sizeof(char *));
    if (!filenames) {
        perror("malloc");
        closedir(dir);
        return NULL;
    }

    int count = 0;
    struct dirent *entry;

    // Read all non-hidden filenames
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') {
            continue; // Skip hidden files
        }

        // Resize array if needed - FIXED: cast to int for comparison
        if (count >= (int)capacity) {
            capacity *= 2;
            char **new_filenames = realloc(filenames, capacity * sizeof(char *));
            if (!new_filenames) {
                perror("realloc");
                break;
            }
            filenames = new_filenames;
        }

        // Store filename
        filenames[count] = strdup(entry->d_name);
        if (!filenames[count]) {
            perror("strdup");
            break;
        }
        count++;
    }

    closedir(dir);

    // Sort the filenames alphabetically
    if (count > 0) {
        qsort(filenames, count, sizeof(char *), compare_strings);
    }

    *file_count = count;
    return filenames;
}

/* --- Convert st_mode into rwxrwxrwx string --- */
void print_permissions(mode_t mode) {
    printf((S_ISDIR(mode)) ? "d" : "-");
    printf((mode & S_IRUSR) ? "r" : "-");
    printf((mode & S_IWUSR) ? "w" : "-");
    printf((mode & S_IXUSR) ? "x" : "-");
    printf((mode & S_IRGRP) ? "r" : "-");
    printf((mode & S_IWGRP) ? "w" : "-");
    printf((mode & S_IXGRP) ? "x" : "-");
    printf((mode & S_IROTH) ? "r" : "-");
    printf((mode & S_IWOTH) ? "w" : "-");
    printf((mode & S_IXOTH) ? "x" : "-");
}

/* --- Format time like ls -l (Mon DD HH:MM) --- */
void format_time(time_t mtime, char *time_str) {
    struct tm *tm_info = localtime(&mtime);
    if (!tm_info) {
        strcpy(time_str, "??? ?? ??:??");
        return;
    }
    strftime(time_str, 13, "%b %e %H:%M", tm_info);
}

/* --- Default column display (down then across) --- */
void simple_display(const char *path) {
    int file_count;
    char **filenames = read_and_sort_filenames(path, &file_count);
    if (!filenames || file_count == 0) {
        if (filenames) {
            for (int i = 0; i < file_count; i++) free(filenames[i]);
            free(filenames);
        }
        return;
    }

    // Find maximum filename length
    int max_len = 0;
    for (int i = 0; i < file_count; i++) {
        int len = strlen(filenames[i]);
        if (len > max_len) max_len = len;
    }

    // Calculate column layout
    int terminal_width = get_terminal_width();
    int col_width = max_len + 2;  // 2 spaces between columns
    int num_cols = terminal_width / col_width;
    if (num_cols < 1) num_cols = 1;
    int num_rows = (file_count + num_cols - 1) / num_cols;  // Ceiling division

    // Print in "down then across" order
    for (int row = 0; row < num_rows; row++) {
        for (int col = 0; col < num_cols; col++) {
            int index = row + col * num_rows;
            if (index < file_count) {
                printf("%-*s", col_width, filenames[index]);
>>>>>>> feature-alphabetical-sort-v1.4.0
            }
        }
        putchar('\n');
    }

<<<<<<< HEAD
    for (int i = 0; i < count; i++) free(files[i]);
    free(files);
}

/* --- Horizontal display (-x) --- */
void handle_horizontal_display(const char *path) {
    int count;
    char **files = read_and_sort_filenames(path, &count);
    if (!files || count == 0) {
        if (files) free(files);
=======
    // Cleanup
    for (int i = 0; i < file_count; i++) {
        free(filenames[i]);
    }
    free(filenames);
}

/* --- Horizontal display (-x option) --- */
void handle_horizontal_display(const char *path) {
    int file_count;
    char **filenames = read_and_sort_filenames(path, &file_count);
    if (!filenames || file_count == 0) {
        if (filenames) {
            for (int i = 0; i < file_count; i++) free(filenames[i]);
            free(filenames);
        }
>>>>>>> feature-alphabetical-sort-v1.4.0
        return;
    }

    // Find maximum filename length
    int max_len = 0;
<<<<<<< HEAD
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

=======
    for (int i = 0; i < file_count; i++) {
        int len = strlen(filenames[i]);
        if (len > max_len) max_len = len;
    }

    // Calculate display parameters
    int terminal_width = get_terminal_width();
    int col_width = max_len + 2;  // 2 spaces between items
    int current_width = 0;

    // Print in horizontal (row-major) order
    for (int i = 0; i < file_count; i++) {
        int needed_width = strlen(filenames[i]) + 2;
        
        // Wrap to next line if needed
        if (current_width + needed_width > terminal_width && current_width > 0) {
            printf("\n");
            current_width = 0;
        }
        
        printf("%-*s", col_width, filenames[i]);
        current_width += col_width;
    }
    
    if (file_count > 0) {
        printf("\n");
    }

    // Cleanup
    for (int i = 0; i < file_count; i++) {
        free(filenames[i]);
    }
    free(filenames);
}

/* --- Long listing format (-l option) --- */
void handle_long_listing(const char *path) {
    int file_count;
    char **filenames = read_and_sort_filenames(path, &file_count);
    if (!filenames || file_count == 0) {
        if (filenames) {
            for (int i = 0; i < file_count; i++) free(filenames[i]);
            free(filenames);
        }
        return;
    }

    // First pass: collect all file info and calculate column widths
>>>>>>> feature-alphabetical-sort-v1.4.0
    struct file_info {
        char *name;
        struct stat st;
        char time_str[13];
        char *owner;
        char *group;
<<<<<<< HEAD
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
=======
    } *file_info = malloc(file_count * sizeof(struct file_info));

    if (!file_info) {
        perror("malloc");
        for (int i = 0; i < file_count; i++) free(filenames[i]);
        free(filenames);
        return;
    }

    size_t links_width = 1, owner_width = 1, group_width = 1, size_width = 1;

    for (int i = 0; i < file_count; i++) {
        file_info[i].name = filenames[i];
        
        char fullPath[1024];
        snprintf(fullPath, sizeof(fullPath), "%s/%s", path, filenames[i]);

        if (lstat(fullPath, &file_info[i].st) == -1) {
            perror("lstat");
            // Initialize with defaults to avoid crashes
            file_info[i].owner = strdup("?");
            file_info[i].group = strdup("?");
            strcpy(file_info[i].time_str, "??? ?? ??:??");
            continue;
        }

        // Format time
        format_time(file_info[i].st.st_mtime, file_info[i].time_str);

        // Get owner and group names
        struct passwd *pw = getpwuid(file_info[i].st.st_uid);
        struct group *gr = getgrgid(file_info[i].st.st_gid);
        
        if (pw) {
            file_info[i].owner = strdup(pw->pw_name);
        } else {
            char buf[32];
            snprintf(buf, sizeof(buf), "%u", (unsigned)file_info[i].st.st_uid);
            file_info[i].owner = strdup(buf);
        }
        
        if (gr) {
            file_info[i].group = strdup(gr->gr_name);
        } else {
            char buf[32];
            snprintf(buf, sizeof(buf), "%u", (unsigned)file_info[i].st.st_gid);
            file_info[i].group = strdup(buf);
        }

        // Calculate column widths
        // Links column
        unsigned long links = file_info[i].st.st_nlink;
        size_t link_digits = 1;
        while (links >= 10) { links /= 10; link_digits++; }
        if (link_digits > links_width) links_width = link_digits;

        // Owner column
        size_t owner_len = strlen(file_info[i].owner);
        if (owner_len > owner_width) owner_width = owner_len;

        // Group column
        size_t group_len = strlen(file_info[i].group);
        if (group_len > group_width) group_width = group_len;

        // Size column
        off_t size = file_info[i].st.st_size;
        size_t size_digits = 1;
        while (size >= 10) { size /= 10; size_digits++; }
        if (size_digits > size_width) size_width = size_digits;
    }

    // Second pass: display all files with proper formatting
    for (int i = 0; i < file_count; i++) {
        char fullPath[1024];
        snprintf(fullPath, sizeof(fullPath), "%s/%s", path, file_info[i].name);

        if (lstat(fullPath, &file_info[i].st) == -1) {
            continue; // Skip if we can't stat
        }

        // Print permissions
        print_permissions(file_info[i].st.st_mode);
        printf(" ");

        // Print other fields with proper alignment
        printf("%*lu %-*s %-*s %*ld %s %s\n",
               (int)links_width, (unsigned long)file_info[i].st.st_nlink,
               (int)owner_width, file_info[i].owner,
               (int)group_width, file_info[i].group,
               (int)size_width, (long)file_info[i].st.st_size,
               file_info[i].time_str,
               file_info[i].name);
    }

    // Cleanup
    for (int i = 0; i < file_count; i++) {
        free(file_info[i].owner);
        free(file_info[i].group);
    }
    free(file_info);
    
    for (int i = 0; i < file_count; i++) {
        free(filenames[i]);
    }
    free(filenames);
}

/* --- Main LS function that handles all modes --- */
void do_ls(const char *path, bool long_listing, bool horizontal) {
    if (long_listing) {
        handle_long_listing(path);
    } else if (horizontal) {
        handle_horizontal_display(path);
    } else {
        simple_display(path);
    }
}

/* --- Main function --- */
int main(int argc, char *argv[]) {
    int opt;
    bool long_listing = false;
    bool horizontal = false;

    // Parse command-line options
    while ((opt = getopt(argc, argv, "lx")) != -1) {
        switch (opt) {
            case 'l':
                long_listing = true;
                break;
            case 'x':
                horizontal = true;
                break;
>>>>>>> feature-alphabetical-sort-v1.4.0
            default:
                fprintf(stderr, "Usage: %s [-l | -x] [directory]\n", argv[0]);
                return 1;
        }
    }
<<<<<<< HEAD
=======

    // Determine directory to list
    const char *path = (optind < argc) ? argv[optind] : ".";

    do_ls(path, long_listing, horizontal);
>>>>>>> feature-alphabetical-sort-v1.4.0

    const char *path = (optind < argc) ? argv[optind] : ".";
    do_ls(path, long_listing, horizontal);
    return 0;
}
