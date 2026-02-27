1. In this assignment I suggested you use `fgets()` to get user input in the main while loop. Why is `fgets()` a good choice for this application?

    > **Answer**: `fgets()` is a good choice for this application for several reasons:
    > - **Buffer size safety**: `fgets()` requires a maximum number of characters to read, preventing buffer overflows compared to `gets()` (which is deprecated)
    > - **Handles newlines**: `fgets()` includes the newline character in the output (if the line fits within the buffer), which helps us detect complete lines
    > - **EOF handling**: `fgets()` returns NULL on EOF, allowing us to gracefully handle when the user presses Ctrl+D
    > - **Error detection**: Returns NULL on both EOF and error, allowing proper error handling
    > - Compared to alternatives like `scanf()`, `fgets()` doesn't skip whitespace and is more predictable for reading entire lines

2. You needed to use `malloc()` to allocate memory for `cmd_buff` in `dsh_cli.c`. Can you explain why you needed to do that, instead of allocating a fixed-size array?

    > **Answer**: We use `malloc()` for dynamic memory allocation instead of fixed-size arrays for several reasons:
    > - **Flexibility**: Dynamic allocation allows the buffer to grow as needed, accommodating varying input sizes
    > - **Memory efficiency**: We only allocate what we need, rather than reserving maximum possible space for every command
    > - **Avoiding stack overflow**: Large fixed-size arrays on the stack could cause stack overflow issues
    > - **Proper memory management**: When combined with `free()`, we have explicit control over memory lifecycle
    > - **The struct design**: The `cmd_buff_t` structure has a pointer `_cmd_buffer` that we allocate dynamically, which is the standard C pattern for flexible string storage

3. In `dshlib.c`, the function `build_cmd_list()` must trim leading and trailing spaces from each command before storing it. Why is this necessary? If we didn't trim spaces, what kind of issues might arise when executing commands in our shell?

    > **Answer**: Trimming leading and trailing spaces is necessary for several reasons:
    > - **Correct tokenization**: If we don't trim, spaces at the beginning or end could create empty tokens or malformed commands
    > - **Command recognition**: Commands like "  ls  " should be recognized as "ls", not as commands with leading spaces
    > - **Argument parsing**: Arguments should not have leading/trailing spaces that could affect the command execution
    > - **User experience**: Users often add spaces accidentally, and trimming makes the shell more forgiving
    > - **Preventing bugs**: Without trimming, we might pass " ls" (with leading space) to execvp, which would fail because it looks for a command named " ls" rather than "ls"

4. For this question you need to do some research on STDIN, STDOUT, and STDERR in Linux. We've learned this week that shells are "robust brokers of input and output". Google _"linux shell stdin stdout stderr explained"_ to get started.

- One topic you should have found information on is "redirection". Please provide at least 3 redirection examples that we should implement in our custom shell, and explain what challenges we might have implementing them.

    > 1. **`command > file`** (output redirection): Redirects STDOUT to a file, overwriting it. Challenge: Need to open file with O_WRONLY|O_CREAT|O_TRUNC flags, use dup2() to replace STDOUT file descriptor.
    > 
    > 2. **`command < file`** (input redirection): Redirects file content to STDIN. Challenge: Need to open file, use dup2() to replace STDIN, handle file not found errors.
    > 
    > 3. **`command >> file`** (append redirection): Like > but appends to file instead of overwriting. Challenge: Need O_WRONLY|O_CREAT|O_APPEND flags instead of O_TRUNC.
    > 
    > Additional examples: `2>` (STDERR redirect), `&>` (both STDOUT and STDERR), `<>` (read and write).
    > 
    > The main challenge is using `dup2()` correctly to redirect file descriptors before calling execvp(), and properly managing these file descriptors to avoid leaks.

- You should have also learned about "pipes". Redirection and piping both involve controlling input and output in the shell, but they serve different purposes. Explain the key differences between redirection and piping.

    > **Answer**: **Key differences between redirection and piping:**
    > 
    > | Aspect | Redirection | Piping |
    > |--------|-------------|--------|
    > | **Purpose**: Redirection connects commands to files, piping connects the output of one command to input of another |
    > | **Direction**: Redirection is one-way to/from files, piping is one-way between two processes |
    > | **Syntax**: Redirection:`<`, `>`, `>>`, piping: `|` 
    > | **Implementation**: Redirection: Uses file I/O (open, dup2) | Piping: Uses pipe() system call |
    > | **Data flow**: Redirection: File ↔ Process | Piping: Process ↔ Process |
    > 
    > **Redirection** changes where a process reads input or writes output (from/to a file).
    > **Piping** creates a communication channel between two processes, where the stdout of one becomes stdin of another.

- STDERR is often used for error messages, while STDOUT is for regular output. Why is it important to keep these separate in a shell?

    > - **Error isolation**: Error messages shouldn't pollute regular output, making it easier to parse/pipe only the actual data
    > - **User experience**: Users can see errors even when redirecting output (e.g., `ls > files.txt` still shows errors on screen)
    > - **Scripting**: Programs can check exit codes and separate error handling from normal output
    > - **Debugging**: Developers can easily identify what went wrong vs normal output
    > - **Filtering**: Users can handle errors differently (e.g., redirect errors: `2>/dev/null`)

- How should our custom shell handle errors from commands that fail? Consider cases where a command outputs both STDOUT and STDERR. Should we provide a way to merge them, and if so, how?

    > **Error handling approach:**
    > - Check the return value from child processes using `waitpid()` and `WEXITSTATUS`
    > - Store the exit code and potentially expose it via a mechanism like `$?` variable or an `rc` command
    > 
    > **Handling STDOUT and STDERR together:**
    > - Yes, we should provide ways to merge them:
    >   - `command > file 2>&1` - Redirect STDOUT to file, then STDERR to where STDOUT goes
    >   - `command &> file` - Shorthand for above (bash)
    >   - `command 2>&1 | another` - Merge both streams before piping
    > 
    > **Implementation:** Use `dup2()` to first redirect STDOUT, then duplicate the STDOUT fd onto STDERR (after STDOUT points to the file), effectively merging them.
