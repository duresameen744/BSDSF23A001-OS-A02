/*
 * Programming Assignment 02: lsv1.4.0
 * Feature 5 – Alphabetical Sort
 * Complete implementation with alphabetical sorting for all display modes
 * FIXED: No compilation warnings
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

// Function declarations
void do_ls(const char *path, bool long_listing, bool horizontal);
void simple_display(const char *path);
void handle_long_listing(const char *path);
void handle_horizontal_display(const char *path);
void print_permissions(mode_t mode);
void format_time(time_t mtime, char *time_str);
int compare_strings(const void *a, const void *b);
char **read_and_sort_filenames(const char *path, int *file_count);
int get_terminal_width(void);

/* --- Get terminal width for column display --- */
int get_terminal_width(void) {
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == -1) {
        return 80;  // Fallback width
    }
    return w.ws_col;
}

/* --- qsort comparison function for alphabetical sorting --- */
int compare_strings(const void *a, const void *b) {
    const char *str1 = *(const char **)a;
    const char *str2 = *(const char **)b;
    return strcmp(str1, str2);
}

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
            }
        }
        printf("\n");
    }

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
        return;
    }

    // Find maximum filename length
    int max_len = 0;
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
    struct file_info {
        char *name;
        struct stat st;
        char time_str[13];
        char *owner;
        char *group;
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
            default:
                fprintf(stderr, "Usage: %s [-l | -x] [directory]\n", argv[0]);
                return 1;
        }
    }

    // Determine directory to list
    const char *path = (optind < argc) ? argv[optind] : ".";

    do_ls(path, long_listing, horizontal);

    return 0;
}
