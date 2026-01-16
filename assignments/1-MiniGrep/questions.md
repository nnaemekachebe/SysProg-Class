# Questions

Answer the following questions about your minigrep implementation:

## 1. Pointer Arithmetic in Pattern Matching

In your `str_match()` function, you need to check if the pattern appears anywhere in the line, not just at the beginning. Explain your approach for checking all possible positions in the line. How do you use pointer arithmetic to advance through the line?

> At a very high level I essentially had two pointers to check  the line, a line seeker and a pattern store. Using a loop to increment the pointer (using the '++' notation), I was able to traverse the line, and use the dereferencing key (*) to see the contents of the pointer. If at any point the first character that was pointed to in the line matched the first character in the pattern store, that sets up a loop 'rabbit hole' where if the characters continue to match, the loop stays, and eventually if all the characters match(using the length of the pattern whith another counter to check this), then we exit with a value of 1. If not, we go through the entire loop and exit with a value of 0

## 2. Case-Insensitive Comparison

When implementing case-insensitive matching (the `-i` flag), you need to compare characters without worrying about case. Explain how you handle the case where the pattern is "error" but the line contains "ERROR" or "Error". What functions did you use and why?

> I used the 'tolower()' function with the derefereced characters stored in the pointers(using additional variables as stores, which could've been done more efficiently in hindsight, but ended up being more readable). Of course, the 'tolower()' function was used depending on whether the str_match is case_insensitive in the first place(0 or 1).

## 3. Memory Management

Your program allocates a line buffer using `malloc()`. Explain what would happen if you forgot to call `free()` before your program exits. Would this cause a problem for:
   - A program that runs once and exits?
   - A program that runs in a loop processing thousands of files?

> If it only runs once, that wouldn't too much of a problem, because the memory would be reclaimed by the OS.

If its a program that runs in a loop processing thousands of files, then the memory usage will grow with time, which may lead to slowing down or lack of space to allocate more memory

## 4. Buffer Size Choice

The starter code defines `LINE_BUFFER_SZ` as 256 bytes. What happens if a line in the input file is longer than 256 characters? How does `fgets()` handle this situation? (You may need to look up the documentation for `fgets()` to answer this.)

> The behavior in this case would be undefined. Based on the manual: https://www.man7.org/linux/man-pages/man3/fgets.3.html, fgets() is  stored after the last character in the buffer, and the rest of the uncontinued line is read from that point on.

## 5. Return Codes

The program uses different exit codes (0, 1, 2, 3, 4) for different situations. Why is it useful for command-line utilities to return different codes instead of always returning 0 or 1? Give a practical example of how you might use these return codes in a shell script.

> This would be good practice to know why the code is exited, and would be useful to apply to error handling and unit tests. For example, depending on the exit code returned(0 for no matches found, 1 for matches found), it can be used in a simple log script to echo in the shell.
