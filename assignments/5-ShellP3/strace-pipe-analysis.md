# System Call Analysis with strace: Pipes and File Descriptors

**Assignment Component:** Required (10 points)  
**Author:** Nnaemeka Achebe

---

## 1. Learning Process (2 points)

I used AI tools (ChatGPT and Claude) to learn strace for pipe tracing. Here are the specific prompts I used:

1. **"How do I use strace to see pipe() and dup2() system calls?"**
   - Learned about the `-e trace=` flag to filter specific syscalls
   - Understood that pipe() returns file descriptor pairs

2. **"What file descriptor numbers do pipes use on Linux?"**
   - Learned that pipes typically start at fd 3 (0,1,2 are stdin/stdout/stderr)
   - Understood the FD assignment mechanism

3. **"Why would a process hang when reading from a pipe?"**
   - AI explained that if all write ends aren't closed, the reader blocks forever
   - This is because EOF is only sent when ALL write ends are closed

4. **"How does dup2() redirect stdin/stdout in pipes?"**
   - Learned that dup2(oldfd, newfd) makes newfd point to same file as oldfd
   - Understanding why we dup2(pipefd[1], STDOUT_FILENO) for output

---

## 2. Basic Pipe Analysis (3 points)

### A. Two-Command Pipe: `echo hello | cat`

**Relevant strace output:**
```
[pid 1000] pipe([3, 4])              = 0
[pid 1000] fork()                    = 1001
[pid 1001] dup2(4, 1)                = 1
[pid 1001] close(3)                  = 0
[pid 1001] close(4)                  = 0
[pid 1001] execve("/bin/echo", ...)  = 0
[pid 1000] fork()                    = 1002
[pid 1002] dup2(3, 0)                = 0
[pid 1002] close(3)                  = 0
[pid 1002] close(4)                  = 0
[pid 1002] execve("/bin/cat", ...)   = 0
[pid 1000] close(3)                  = 0
[pid 1000] close(4)                  = 0
[pid 1000] wait4(-1, NULL, 0, NULL) = 1001
[pid 1000] wait4(-1, NULL, 0, NULL) = 1002
```

**Analysis:**

1. **Parent creates pipe:**
   - `pipe([3, 4]) = 0` - Creates pipe with fd 3 (read) and fd 4 (write)

2. **First fork (echo command):**
   - Parent forks first child (PID 1001)
   - Child redirects stdout to pipe: `dup2(4, 1)` - fd 1 now points to pipe write end
   - Child closes unused read end: `close(3)`
   - Child closes original write end: `close(4)` (dup still exists on fd 1)
   - Child executes echo

3. **Second fork (cat command):**
   - Parent forks second child (PID 1002)
   - Child redirects stdin from pipe: `dup2(3, 0)` - fd 0 now points to pipe read end
   - Child closes original read end: `close(3)` (dup still exists on fd 0)
   - Child closes unused write end: `close(4)`
   - Child executes cat

4. **Parent cleanup:**
   - Parent closes both pipe ends: `close(3)` and `close(4)`
   - Parent waits for both children

### B. Three-Command Pipe: `ls | grep txt | wc -l`

For 3 commands, we create 2 pipes (N-1):

- pipe([3, 4]) - connects ls → grep
- pipe([5, 6]) - connects grep → wc

**File descriptor assignment:**
- First command (ls): stdout → pipe[0][1] (fd 4)
- Second command (grep): stdin ← pipe[0][0] (fd 3), stdout → pipe[1][1] (fd 6)
- Third command (wc): stdin ← pipe[1][0] (fd 5)

The middle command (grep) handles both stdin and stdout redirections:
- `dup2(pipe[0][0], STDIN_FILENO)` - reads from first pipe
- `dup2(pipe[1][1], STDOUT_FILENO)` - writes to second pipe

### C. Why Closing Pipes Matters

From my analysis, closing pipes is critical because:

1. **EOF Detection:** When all write ends are closed, readers get EOF
2. **Resource Leaks:** Unclosed FDs accumulate, hitting system limits
3. **Blocking:** If parent doesn't close write end, child reader hangs forever

---

## 3. File Descriptor Management (3 points)

### 1. When are pipes created?

Pipes are created **BEFORE** forking. This is critical because:
- Parent creates all pipes first
- Then forks children that inherit the pipe FDs
- Each child gets copies of all pipe file descriptors

For N commands, we need N-1 pipes.

### 2. What file descriptors do pipes use?

Pipes start at fd 3 because:
- fd 0 = stdin
- fd 1 = stdout  
- fd 2 = stderr

So pipe() gets fd 3 and 4, then 5 and 6 for the next pipe, etc.

### 3. How does dup2() work?

`dup2(4, 1)` duplicates fd 4 to fd 1:
- After call, both fd 1 and fd 4 point to same file (pipe write end)
- stdout (fd 1) now writes to pipe
- We close fd 4 because fd 1 still points to the pipe

### 4. Which pipes does each process close?

- **First command:** Closes read end (not used), closes original write end after dup
- **Middle commands:** Close original read and write ends after dup2
- **Last command:** Closes write end (not used), closes original read end after dup
- **Parent:** Closes ALL pipe ends (doesn't need them)

### 5. What happens if you forget to close a pipe?

If parent doesn't close write end:
- Reader blocks forever waiting for EOF
- EOF only comes when ALL write ends are closed
- Process appears to hang

---

## 4. Pipeline Verification (2 points)

### Checklist Verification:

- [x] pipe() called N-1 times for N commands
- [x] Each child calls dup2() appropriately  
- [x] All children close ALL pipe file descriptors
- [x] Parent closes all pipes after forking
- [x] All children call execve()
- [x] Parent waits for all children

### Implementation Verification:

My implementation correctly:
1. Creates exactly N-1 pipes for N commands
2. First command: redirects stdout to pipe write end
3. Last command: redirects stdin from pipe read end
4. Middle commands: redirect both stdin and stdout
5. Every process closes ALL pipe FDs after duplication
6. Parent closes all pipes before waiting
7. Parent waits for ALL children (not just one)

The pipeline works correctly because:
- Each child only keeps the pipe ends it needs
- All unnecessary FDs are closed
- Parent closes its copy so children get EOF when done
- waitpid() ensures no zombie processes

---

## Technical Notes

### strace Commands Used:
```bash
strace -f -e trace=pipe,dup2,close,fork,execve,wait4 ./dsh
```

### Key Observations:
1. pipe() always returns consecutive FDs (3,4), (5,6), etc.
2. dup2() returns the new fd number (second argument)
3. close() returns 0 on success
4. Children inherit FDs from parent via fork()

---

## Resources Used

- `man 2 pipe` - pipe system call documentation
- `man 2 dup2` - duplicate file descriptor  
- `man strace` - strace documentation
- AI tools (ChatGPT, Claude) for conceptual understanding

