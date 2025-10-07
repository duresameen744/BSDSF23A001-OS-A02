**REPORT.md**

Q: What is the crucial difference between stat() and lstat()? When to use lstat()?

A: stat(path, &st) follows symbolic links — if path is a symlink, stat() returns metadata of the file that the link points to. lstat(path, &st) does not follow symlinks — it returns metadata about the link itself (type is S_IFLNK, permissions are the link's). For ls -l we should use lstat() because ls -l shows symbolic links as links (type l) and then typically prints the link target using readlink(). Using stat() would hide the fact that the file is a symlink by giving the target's type.

Q: Explain how st_mode stores type and permissions and how to extract them using bitwise operators and macros.

A: st_mode is an integer bitfield that encodes both the file type and permission bits. The file type bits are masked by S_IFMT (or inspected using helper macros like S_ISDIR(st_mode), S_ISREG(st_mode), S_ISLNK(st_mode)). Permission bits are tested using masks such as S_IRUSR (owner read), S_IWUSR (owner write), S_IXUSR (owner execute), and corresponding group/other bits (S_IRGRP, S_IWGRP, S_IXGRP, S_IROTH, S_IWOTH, S_IXOTH). Example C test:

if (st.st_mode & S_IRUSR) putchar('r'); else putchar('-');
if (st.st_mode & S_IWUSR) putchar('w'); else putchar('-');
if (st.st_mode & S_IXUSR) putchar('x'); else putchar('-');


Special bits are S_ISUID, S_ISGID, and S_ISVTX (sticky). They modify the displayed execute characters to s/S or t/T depending on whether the execute bit is set. For file type you can do:

switch (st.st_mode & S_IFMT) {
    case S_IFDIR: puts("directory"); break;
    case S_IFREG: puts("regular"); break;
    case S_IFLNK: puts("symlink"); break;
    // ...
}


or prefer the macros S_ISDIR(st.st_mode), S_ISREG(...), S_ISLNK(...) which are clearer and portable.

7) Extra testing & debugging tips

If make fails, ensure Makefile SRC path is exactly src/lsv1.0.0.c.

If you see strange column alignment, list many files or check owner/group name lengths — the code computes maximum widths dynamically.

To show hidden files (not part of this feature) you would add -a in parsing; that is saved for later features.

If readlink() returns -1 for symlinks, check permissions; it normally works.

----FEATURE 3------------

Why is one loop not enough?
Because in “down then across,” items are not sequential in memory. You must jump indices like row + n*rows. A single loop would only print straight down, not across columns.

Purpose of ioctl()
It retrieves terminal width dynamically.
If you use a fixed width (like 80), your layout will break when terminal is resized.

-------FEATURE 4-----------
Q: Explain the general logic for printing items in a left-to-right (“horizontal”) columnar format. Why is a simple single loop insufficient?

A: Left-to-right printing requires deciding when to break the current output row and start a new one. A single simple loop that prints each filename sequentially without measuring the remaining line width can’t decide where a line break should occur to prevent overflow. The correct algorithm computes the maximum column width (maxlen + spacing) and the terminal width, then iterates filenames, tracking the current line width. Before printing a name, the algorithm checks whether current_line_width + col_width > terminal_width — if true, it emits a newline and resets the counter. This ensures names are placed left-to-right in rows and wrapped when necessary. The single loop is still used, but it must maintain state (cur — width used so far) and consult terminal width each time, so it’s not a naive one-liner printing names only.

Q: What is the purpose of ioctl() here? What are the limitations of using a fixed width fallback (80 columns)?

A: ioctl() with TIOCGWINSZ returns the terminal window size (ws_col) so the program can pack columns to exactly the user’s terminal width — this is what allows the output to adapt when the user resizes the terminal. If we used a fixed fallback (e.g., 80) only, the layout would be suboptimal on wider or narrower terminals: on wide terminals we’d underutilize space (too few columns), and on narrow terminals we’d overflow or wrap incorrectly. Using ioctl() provides accurate, responsive formatting. The fallback is only used when ioctl() fails (e.g., output is piped, or not a controlling terminal) to keep behaviour safe in non-interactive environments.

----------FEATURE 5---------------------

Q1:

Why is it necessary to read all directory entries into memory before you can sort them?

Because sorting requires random access to all filenames at once. You can’t sort while reading one-by-one because you must compare filenames pairwise. Storing them first in an array allows qsort() to rearrange them efficiently.
Drawback: For directories with millions of files, it can consume huge memory and slow performance, potentially causing memory exhaustion.

Q2:

Explain the purpose and signature of the comparison function required by qsort().

int compare(const void *a, const void *b)
This function tells qsort() how to order two elements. Since qsort works with generic data (void *), you must cast them back to the correct type (e.g., char **) and return:

Negative → if a < b

Zero → if a == b

Positive → if a > b

The const void * ensures qsort() does not modify the data through these pointers.

------------------------FEATURE 6-----------------------

Q — How do ANSI escape codes work? Show the code sequence for printing text in green.
ANSI escape codes are special sequences beginning with the ESC character (ASCII 27), written \033 (or \x1b) followed by [ and parameters and a letter. To set text green and then reset, use:

printf("\033[0;32m");   // set foreground green
printf("myfile\n");
printf("\033[0m");      // reset colors


\033[0;32m means: reset attributes (0) then set foreground color 32 (green). \033[0m resets attributes to default.

Q — Which st_mode bits indicate executability?
To determine if a file is executable by owner, group, or others, check:

Owner execute bit: st_mode & S_IXUSR

Group execute bit: st_mode & S_IXGRP

Others execute bit: st_mode & S_IXOTH

---------------------------FEATURE 7------------------------------

Q1: What is a “base case” in recursion? In your recursive ls, what is the base case?
A base case is a condition where the recursive function stops calling itself and returns a result directly. Without a base case recursion would never end. In the recursive do_ls, the base case is implicitly reached when do_ls is called on a directory that contains no (non-hidden) subdirectories to recurse into. Each recursive call lists files and only calls do_ls on entries that are directories; when there are no such entries the function returns and recursion unwinds. A second protective base condition is that lstat() must succeed and the entry must be a directory and not . or .. — otherwise recursion does not continue.

Q2: Why construct a full path parent_dir/subdir before recursing?
You must construct the full path so lstat() and further opendir() operate on the correct file system location. If you simply call do_ls("subdir") while the current working directory is still parent_dir, it might work if you haven't changed CWD — but it becomes incorrect when the caller uses absolute paths or when recursion is deeper (relative paths will break). Using full paths (absolute or relative with parent prefix) guarantees the recursive call operates on the intended directory regardless of the current working directory or caller context. Also helps avoid name collisions when multiple directories with the same name exist in different parents.
