### 1. Learning Process (2 points)

Document how you learned strace:
- What AI tools did you use?
- What questions did you ask? (Include 3-4 specific prompts)
- What resources did the AI point you to?
- What challenges did you encounter learning strace?

I used ChatGPT to learn strace. I asked about what strace was, and how to implement it. I first asked "how to install it and run it", and "what command can I run to test whether it was installed correctly" and "what simple commands can I run on a function in the codebase". The prompts were rather straightforward, so I didn't need to point to external resources. There have been no challenges up to this poinnt

### 2. Basic System Call Analysis (3 points)

Analyze three operations. For each, provide strace output and analysis:

#### A. Adding a Student

Run: `strace -e trace=open,lseek,read,write,close ./sdbsc -a 1 john doe 350`

**Provide:**
- The strace output (you can trim to relevant syscalls)
- Identify each system call and explain what it does
- Verify the lseek offset is correct (should be 1 * 64 = 64)
- Verify the write() size is correct (should be 64 bytes)

- **strace output:**
(.venv) nnaemekaachebe@ubuntu-24-class:~/git-and-github-nnaemekachebe/SysProg-Class/assignments/2-StudentDB/starter$ `strace -e trace=open,lseek,read,write,close ./sdbsc -a 1 john doe 350`
close(3)                                = 0
read(3, "\177ELF\2\1\1\3\0\0\0\0\0\0\0\0\3\0\267\0\1\0\0\0\360\206\2\0\0\0\0\0"..., 832) = 832
close(3)                                = 0
lseek(3, 6399999, SEEK_SET)             = 6399999
write(3, "\0", 1)                       = 1
lseek(3, 0, SEEK_SET)                   = 0
read(3, "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"..., 64) = 64
lseek(3, 0, SEEK_SET)                   = 0
write(3, "\1\0\0\0john\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0doe\0"..., 64) = 64
close(3)                                = 0
write(1, "Student 1 added to database.\n", 29) = 29
+++ exited with 0 +++

**Explanation for each system call**
lseek(3, 6399999, SEEK_SET)             = 6399999

Moves the file pointer to the last byte of the database(which is MAX_STD_ID * sizeof(student_t) -1)
write(3, "\0", 1)                       = 1
Writes one byte at the end to make sure the file size is 6400000





#### B. Reading/Printing a Student

Run: `strace -e trace=open,lseek,read,write,close ./sdbsc -g 1`

**Provide:**
- The strace output
- Identify the lseek to find the student
- Identify the read to get student data
- Verify the offset and size are correct

(.venv) nnaemekaachebe@ubuntu-24-class:~/git-and-github-nnaemekachebe/SysProg-Class/assignments/2-StudentDB/starter$ strace -e trace=open,lseek,read,write,close ./sdbsc -g 1
close(3)                                = 0
read(3, "\177ELF\2\1\1\3\0\0\0\0\0\0\0\0\3\0\267\0\1\0\0\0\360\206\2\0\0\0\0\0"..., 832) = 832
close(3)                                = 0
write(1, "usage: ./sdbsc -[h|a|c|d|f|p|z] "..., 49usage: ./sdbsc -[h|a|c|d|f|p|z] options.  Where:
) = 49
write(1, "\t-h:  prints help\n", 18     -h:  prints help
)    = 18
write(1, "\t-a id first_name last_name gpa("..., 65     -a id first_name last_name gpa(as 3 digit int):  adds a student
) = 65
write(1, "\t-c:  counts the records in the "..., 41     -c:  counts the records in the database
) = 41
write(1, "\t-d id:  deletes a student\n", 27    -d id:  deletes a student
) = 27
write(1, "\t-f id:  finds and prints a stud"..., 52     -f id:  finds and prints a student in the database
) = 52
write(1, "\t-p:  prints all records in the "..., 49     -p:  prints all records in the student database
) = 49
write(1, "\t-x:  compress the database file"..., 48     -x:  compress the database file [EXTRA CREDIT]
) = 48
write(1, "\t-z:  zero db file (remove all r"..., 40     -z:  zero db file (remove all records)
) = 40
close(3)                                = 0
+++ exited with 2 +++



#### C. Deleting a Student

Run: `strace -e trace=open,lseek,read,write,close ./sdbsc -d 1`

(.venv) nnaemekaachebe@ubuntu-24-class:~/git-and-github-nnaemekachebe/SysProg-Class/assignments/2-StudentDB/starter$ strace -e trace=open,lseek,read,write,close ./sdbsc -d 1
close(3)                                = 0
read(3, "\177ELF\2\1\1\3\0\0\0\0\0\0\0\0\3\0\267\0\1\0\0\0\360\206\2\0\0\0\0\0"..., 832) = 832
close(3)                                = 0
lseek(3, 0, SEEK_SET)                   = 0
read(3, "\1\0\0\0john\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0doe\0"..., 64) = 64
lseek(3, 0, SEEK_SET)                   = 0
write(3, "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"..., 64) = 64
write(1, "Student 1 was deleted from datab"..., 37Student 1 was deleted from database.
) = 37
close(3)                                = 0
+++ exited with 0 +++

**Provide:**
- The strace output
- Note: deletion writes zeros - look for write() call
- Identify if there's a read before the write (checking if student exists)
- Verify the lseek offset and write size


### 3. Sparse File Investigation (3 points)

This is the most interesting part! Investigate how sparse files work.

#### A. Create a Fresh Database

```bash
rm student.db  # Start fresh
strace -e trace=open,lseek,write,close ./sdbsc -a 1 john doe 350
ls -lh student.db
du -h student.db
```
-**output**

(.venv) nnaemekaachebe@ubuntu-24-class:~/git-and-github-nnaemekachebe/SysProg-Class/assignments/2-StudentDB/starter$ strace -e trace=open,lseek,write,close ./sdbsc -a 1 john doe 350
close(3)                                = 0
close(3)                                = 0
lseek(3, 6399999, SEEK_SET)             = 6399999
write(3, "\0", 1)                       = 1
lseek(3, 0, SEEK_SET)                   = 0
lseek(3, 0, SEEK_SET)                   = 0
write(3, "\1\0\0\0john\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0doe\0"..., 64) = 64
write(1, "Student 1 added to database.\n", 29Student 1 added to database.
) = 29
close(3)                                = 0
+++ exited with 0 +++
(.venv) nnaemekaachebe@ubuntu-24-class:~/git-and-github-nnaemekachebe/SysProg-Class/assignments/2-StudentDB/starter$ ls -lh student.db
-rw-r----- 1 nnaemekaachebe nnaemekaachebe 6.2M Feb 12 14:30 student.db
(.venv) nnaemekaachebe@ubuntu-24-class:~/git-and-github-nnaemekachebe/SysProg-Class/assignments/2-StudentDB/starter$ du -h student.db
8.0K    student.db


**Answer these questions:**

1. **What is the file size reported by `ls -lh`?**
   - Should be 128 bytes (2 * 64)
   - Explain why

2. **What is the actual disk usage reported by `du -h`?**
   - Should be 4K
   - Explain why it's larger than 128 bytes but not as much as it could be

3. **In the strace output, what did lseek() do?**
   - It skipped from byte 0 to byte 64
   - This creates a "hole" in the file (bytes 0-63)
   - Holes don't take up disk space


#### B. Add a Student with Large ID

```bash
strace -e trace=lseek,write ./sdbsc -a 99999 big id 400
ls -lh student.db
du -h student.db
```

**Answer these questions:**

1. **What offset did lseek() seek to?**
   - Calculate: 99999 * 64 = ?
   - Does strace show this offset?

2. **What is the file size now?**
   - Should be huge (6.4 MB)
   - But du shows actual usage is still small

3. **What happened?**
   - lseek created a HUGE hole
   - Only 2 student records actually written (student 1 and 99999)
   - Sparse file only allocates space for written data

#### C. Sparse File Explanation

Based on your investigation, explain:
- What is a sparse file?
- How does lseek() create holes?
- Why is this efficient for our database?
- What would happen without sparse file support?

### 4. System Call Verification (2 points)

Verify your implementation is correct by checking:

**Checklist:**
- [ ] open() opens the database file with correct flags
- [ ] lseek() offsets match the formula: `id * 64`
- [ ] write() always writes exactly 64 bytes
- [ ] read() reads exactly 64 bytes when getting a student
- [ ] close() is called to close the file
- [ ] No errors (return values are non-negative)

**Questions to answer:**
1. Did you find any bugs in your implementation through strace analysis?
2. Do all your system calls return success (non-negative values)?
3. Are your lseek() offsets calculated correctly?
4. Do you read/write the correct number of bytes?

If you found bugs, describe what was wrong and how you fixed it.


