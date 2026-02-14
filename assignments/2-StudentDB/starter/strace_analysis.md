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
lseek(3, 64, SEEK_SET)                  = 64
read(3, "", 64)                         = 0
lseek(3, 64, SEEK_SET)                  = 64
write(3, "\1\0\0\0john\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0doe\0"..., 64) = 64
close(3)                                = 0
write(1, "Student 1 added to database.\n", 29) = 29
+++ exited with 0 +++

**Explanation for each system call**
According to ChatGPT, the commands
'
close(3)                                = 0
read(3, "\177ELF\2\1\1\3\0\0\0\0\0\0\0\0\3\0\267\0\1\0\0\0\360\206\2\0\0\0\0\0"..., 832) = 832
close(3)
'

are from the ELF(Executable and Linkable Format), which is the OS reading an exectuable/shared library file, so these are not a part of the main function.


lseek(3, 6399999, SEEK_SET)             = 6399999

Moves the file pointer to the last byte of the database(which is MAX_STD_ID * sizeof(student_t) -1)
write(3, "\0", 1)                       = 1
Writes one byte at the end to make sure the file size is 6400000

lseek(3, 0, SEEK_SET)                   = 0
Go to the start of the file

read(3, "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"..., 64) = 64

Read 64 bytes and see whats in the slot 1

lseek(3, 0, SEEK_SET) = 0
Goes back to the beginninng of the slot so that the unpoming write overwrites it

write(3, "\1\0\0\0john\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0doe\0"..., 64) = 64

Writes the 64-byte student_t record for ID 1

close(3)                                = 0
Closes the database

write(1, "Student 1 added to database.\n", 29) = 29
Writes the success message to stdout (fd 1)

+++ exited with 0 +++
Program returned with exit code 0

Based on these analyses, the sixes of lseek and write have been verified to be correct


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

**Analysis**

close(3)                                = 0
read(3, "\177ELF\2\1\1\3\0\0\0\0\0\0\0\0\3\0\267\0\1\0\0\0\360\206\2\0\0\0\0\0"..., 832) = 832
close(3) 

Same as the previous part, this is the startup code for the ELF files

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

This is writing to stdout the correct usage of this function, essentially stating that the commands are being run the wrong way. -g is the wrong flag

close(3)                                = 0
+++ exited with 2 +++

This closes the file
And exits with code two, which correlates to EXIT_FAIL_ARGS

Now, running this with the right code would look like this:

(.venv) nnaemekaachebe@ubuntu-24-class:~/git-and-github-nnaemekachebe/SysProg-Class/assignments/2-StudentDB/starter$ strace -e trace=open,lseek,read,write,close ./sdbsc -f 1
close(3)                                = 0
read(3, "\177ELF\2\1\1\3\0\0\0\0\0\0\0\0\3\0\267\0\1\0\0\0\360\206\2\0\0\0\0\0"..., 832) = 832
close(3)                                = 0
lseek(3, 0, SEEK_SET)                   = 0
read(3, "\1\0\0\0john\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0doe\0"..., 64) = 64
write(1, "ID     FIRST_NAME               "..., 69ID     FIRST_NAME               LAST_NAME                        GPA
) = 69
write(1, "1      john                     "..., 701      john                     doe                              3.45
) = 70
close(3)                                = 0
+++ exited with 0 +++

close(3)                                = 0
read(3, "\177ELF\2\1\1\3\0\0\0\0\0\0\0\0\3\0\267\0\1\0\0\0\360\206\2\0\0\0\0\0"..., 832) = 832
close(3)

This is the startup with the ELF files

lseek(3, 0, SEEK_SET)                   = 0
read(3, "\1\0\0\0john\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0doe\0"..., 64) = 64

3 is the file of interest here, which is the student database

lseek goes to the beginning of the file
The read command reads the data in that slot up to 64 bits

fd points to stdout

write(1, "ID     FIRST_NAME               "..., 69ID     FIRST_NAME               LAST_NAME                        GPA
) = 69
write(1, "1      john                     "..., 701      john                     doe                              3.45
) = 70

This is the output that is displayed in stdout(or the terminal)

close(3)                                = 0
+++ exited with 0 +++

the file (the database) is closed and exited normally with no errors

Normal/expected behavior is verified







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

**Output Analysis**

- deletion writes zeroes: write(3, "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"..., 64) = 64

- there is a read before the write: 

read(3, "\1\0\0\0john\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0doe\0"..., 64) = 64
lseek(3, 0, SEEK_SET)                   = 0
write(3, "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"..., 64) = 64


- lseek(3, 0, SEEK_SET)                   = 0. This is the accurate offset for lseek, and from the output above the write size is verified.

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

SysProg-Class/assignments/2-StudentDB/starter$ strace -e trace=open,lseek,write,close ./sdbsc -a 1 john doe 350
close(3)                                = 0
close(3)                                = 0
lseek(3, 64, SEEK_SET)                  = 64
lseek(3, 64, SEEK_SET)                  = 64
write(3, "\1\0\0\0john\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0doe\0"..., 64) = 64
write(1, "Student 1 added to database.\n", 29Student 1 added to database.
) = 29
close(3)                                = 0
+++ exited with 0 +++
(.venv) nnaemekaachebe@ubuntu-24-class:~/git-and-github-nnaemekachebe/SysProg-Class/assignments/2-StudentDB/starter$ ls -lh student.db
-rw-r----- 1 nnaemekaachebe nnaemekaachebe 128 Feb 13 19:09 student.db
(.venv) nnaemekaachebe@ubuntu-24-class:~/git-and-github-nnaemekachebe/SysProg-Class/assignments/2-StudentDB/starter$ du -h student.db
4.0K    student.db


**Answer these questions:**

1. **What is the file size reported by `ls -lh`?**
   - Should be 128 bytes (2 * 64)
   - Explain why

   The first ID is not used, and the second has the information of the added student

2. **What is the actual disk usage reported by `du -h`?**
   - Should be 4K
   - Explain why it's larger than 128 bytes but not as much as it could be

   According to GPT, Linux assigns allocates storage by blocks, and the smallest block size is 4KB. But because sparse data exists, most of the 4KB is not consumed by actual disk block storage space


3. **In the strace output, what did lseek() do?**
   - It skipped from byte 0 to byte 64
   - This creates a "hole" in the file (bytes 0-63)
   - Holes don't take up disk space

   All 3 of the above. rec_offfest() skipps bytes 0-63, meaning a hole was mad,e and holes don't take up true disk space




#### B. Add a Student with Large ID

```bash
strace -e trace=lseek,write ./sdbsc -a 99999 big id 400
ls -lh student.db
du -h student.db
```

**output**

(.venv) nnaemekaachebe@ubuntu-24-class:~/git-and-github-nnaemekachebe/SysProg-Class/assignments/2-StudentDB/starter$ strace -e trace=lseek,write ./sdbsc -a 99999 big id 400
ls -lh student.db
du -h student.db
lseek(3, 6399936, SEEK_SET)             = 6399936
lseek(3, 6399936, SEEK_SET)             = 6399936
write(3, "\237\206\1\0big\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0id\0\0"..., 64) = 64
write(1, "Student 99999 added to database."..., 33Student 99999 added to database.
) = 33
+++ exited with 0 +++
-rw-r----- 1 nnaemekaachebe nnaemekaachebe 6.2M Feb 13 19:20 student.db
8.0K    student.db


**Answer these questions:**

1. **What offset did lseek() seek to?**
   - Calculate: 99999 * 64 = 6399936
   - Does strace show this offset?

   Yes it does!
   lseek(3, 6399936, SEEK_SET)             = 6399936
   lseek(3, 6399936, SEEK_SET)             = 6399936


2. **What is the file size now?**
   - Should be huge (6.4 MB)
   - But du shows actual usage is still small

   Yes, this is because only 2 student blocks werre actually written into




3. **What happened?**
   - lseek created a HUGE hole
   - Only 2 student records actually written (student 1 and 99999)
   - Sparse file only allocates space for written data

#### C. Sparse File Explanation

Based on your investigation, explain:
- What is a sparse file?
This is a file where large regions only contains zeroes, but the OS does not physically store those zero bytes on disk
- How does lseek() create holes?
By jumping across different bytes without wrritiing into them
- Why is this efficient for our database?
Fast random access, storage efficiency
- What would happen without sparse file support?
'Wasted' space on the systems, and for larger databases the issue wuld only exacerbate

### 4. System Call Verification (2 points)

Verify your implementation is correct by checking:

**Checklist:**
- [ check] open() opens the database file with correct flags
- [ check] lseek() offsets match the formula: `id * 64`
- [ check] write() always writes exactly 64 bytes
- [ check] read() reads exactly 64 bytes when getting a student
- [ ] close() is called to close the file
- [ ] No errors (return values are non-negative)

**Questions to answer:**
1. Did you find any bugs in your implementation through strace analysis? Yes, it made me go back and review my code a couple of times, I almost wish I started with strace midweay rather than leaving it till the very end
2. Do all your system calls return success (non-negative values)?
Apart from one functin(which I believe had the wrong flag), and when run with the right flag gave the correct output, all the other fucntions returned successes
3. Are your lseek() offsets calculated correctly?
Yes
4. Do you read/write the correct number of bytes?
Yes

If you found bugs, describe what was wrong and how you fixed it.

I was allocating memory wrongly for the add_Student, where I would forcefully use the MAX)STD_ID to fill out the database(and write 0s in the last slot) to pass the tests.  When I used the strace analysis and read through the instructions again, I discovered that was the improper implementation. I fixed the offset at the beginning, so that the ID=0 would always be skipped and implemented it across add_student, del_student and the compress functions
