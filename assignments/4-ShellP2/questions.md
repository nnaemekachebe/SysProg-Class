1. Can you think of why we use `fork/execvp` instead of just calling `execvp` directly? What value do you think the `fork` provides?

    > **Answer**: We use fork/exec instead of calling execvp directly because:
    > - fork() creates a child process that is a copy of the parent
    > - This allows the shell (parent) to continue running after launching a command
    > - The parent can wait for the child to complete using waitpid()
    > - If we called execvp() directly in the shell process, the shell itself would be replaced by the command and would no longer be able to accept new commands
    > - fork() provides the ability to run multiple commands while keeping the shell alive
    > - It also allows the shell to capture exit codes and handle errors from child processes

2. What happens if the fork() system call fails? How does your implementation handle this scenario?

    > **Answer**: If fork() fails, it returns -1 to the parent process and no child is created. Common failure reasons include:
    > - Out of memory (cannot allocate process structures)
    > - Maximum process limit reached (ulimit -u)
    > - Permission denied (rare for fork)
    > 
    > In my implementation, I check if pid < 0 after calling fork(), and if so:
    > - I print an error message using perror("fork")
    > - Return ERR_EXEC_CMD to indicate execution failure

3. How does execvp() find the command to execute? What system environment variable plays a role in this process?

    > **Answer**: execvp() uses the PATH environment variable to find commands:
    > - PATH contains a colon-separated list of directories to search
    > - execvp() iterates through each directory in PATH
    > - For each directory, it tries to execute the command
    > - If the command is found, it executes it; if not, it tries the next directory
    > - If all directories fail, execvp() returns -1 with errno=ENOENT
    > - Example PATH: "/usr/local/bin:/usr/bin:/bin"

4. What is the purpose of calling wait() in the parent process after forking? What would happen if we didn't call it?

    > **Answer**: The purpose of calling wait() (or waitpid()) in the parent process:
    > - Prevents the child from becoming a "zombie" process
    > - Allows the parent to retrieve the child's exit status
    > - Ensures proper cleanup of process resources
    > - Synchronizes parent and child (parent waits for child to complete)
    > 
    > If we didn't call wait():
    > - Child processes would become zombies (still in process table but not running)
    > - Over time, this would exhaust the process table
    > - The parent wouldn't know when children finished
    > - Could lead to resource leaks and system instability

5. In the referenced demo code we used WEXITSTATUS(). What information does this provide, and why is it important?

    > **Answer**: WEXITSTATUS() provides the exit code of a terminated child process:
    > - It extracts the 8-bit exit status from the status word returned by waitpid()
    > - Only valid if WIFEXITED(status) returns true (child exited normally)
    > - Important because it allows the shell to:
    >   - Know if a command succeeded (exit code 0) or failed (non-zero)
    >   - Implement error handling
    >   - Support the rc command (stores last return code)
    >   - Chain commands based on success/failure (not implemented in Part 2)

6. Describe how your implementation of build_cmd_buff() handles quoted arguments. Why is this necessary?

    > **Answer**: My implementation handles quoted arguments as follows:
    > - When encountering a '"' (double quote), it skips the quote and finds the closing quote
    > - Everything between quotes (including spaces) becomes a single argument
    > - Same for single quotes '\'' 
    > - Quotes are removed from the final arguments
    > 
    > This is necessary because:
    > - Filenames and arguments can contain spaces (e.g., "My Documents")
    > - Users need to pass arguments with spaces to commands
    > - Without quote handling, "echo hello world" would be parsed as 3 arguments instead of 2

7. What changes did you make to your parsing logic compared to the previous assignment? Were there any unexpected challenges in refactoring your old code?

    > **Answer**: For Part 2, I:
    > - Copied the build_cmd_buff() function from Part 1 (no changes needed)
    > - The function was already designed for single command parsing
    > - Part 2 only uses cmd_buff_t (not command_list_t with pipes)
    > - Removed the pipe splitting logic that was in Part 1's build_cmd_list()
    > 
    > Challenges:
    > - No significant refactoring needed - the code reused cleanly
    > - Made sure to NULL-terminate argv[] (required for execvp)
    > - Had to ensure clear_cmd_buff() was called between commands

8. For this question, you need to do some research on Linux signals. You can use [this google search](https://www.google.com/search?q=Linux+signals+overview+site%3Aman7.org+OR+site%3Alinux.die.net+OR+site%3Atldp.org&oq=Linux+signals+overview+site%3Aman7.org+OR+site%3Alinux.die.net+OR+site%3Atldp.org&gs_lcrp=EgZjaHJvbWUyBggAEEUYOdIBBzc2MGowajeoAgCwAgA&sourceid=chrome&ie=UTF-8) to get started.

- What is the purpose of signals in a Linux system, and how do they differ from other forms of interprocess communication (IPC)?

    > **Answer**: Signals are a limited form of interprocess communication in Linux:
    > - Purpose: Notify processes of asynchronous events (e.g., keyboard interrupts, process termination)
    > - They're software interrupts delivered to a process
    > - Unlike other IPC (pipes, message queues, sockets):
    >   - Signals are asynchronous - no persistent connection needed
    >   - Carry minimal information (just the signal number)
    >   - Cannot transfer data payloads like pipes can
    >   - Used for simple notifications, not data transfer
    >   - Each signal has a default action (terminate, stop, ignore)

- Find and describe three commonly used signals (e.g., SIGKILL, SIGTERM, SIGINT). What are their typical use cases?

    > **Answer**: Three commonly used signals:
    > 
    > 1. **SIGINT (Signal 2)**: 
    >    - Send to a process when user presses Ctrl+C
    >    - Default action: Terminate the process
    >    - Use case: Interrupt a running program gracefully (programs can catch and handle it)
    > 
    > 2. **SIGTERM (Signal 15)**: 
    >    - Request a process to terminate gracefully
    >    - Default action: Terminate the process
    >    - Use case: Proper shutdown requests (e.g., systemd stopping a service)
    >    - Programs can catch this signal to cleanup before exiting
    > 
    > 3. **SIGKILL (Signal 9)**: 
    >    - Forcefully terminate a process immediately
    >    - Default action: Terminate (cannot be caught or ignored)
    >    - Use case: Kill a hung or unresponsive program that won't respond to SIGTERM

- What happens when a process receives SIGSTOP? Can it be caught or ignored like SIGINT? Why or why not?

    > **Answer**: SIGSTOP is a job control signal:
    > - It pauses (stops) the process execution
    > - The process is suspended until it receives SIGCONT
    > - Unlike SIGINT, SIGSTOP CANNOT be caught, blocked, or ignored
    > - This is by design - it's a fundamental operating system mechanism for job control
    > - The kernel must be able to stop processes for scheduler to work properly
    > - Use case: Shell job control (Ctrl+Z sends SIGSTOP to background a process)

