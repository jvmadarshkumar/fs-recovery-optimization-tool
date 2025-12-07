\# 📂 File System Recovery \& Optimization Tool



!\[Language](https://img.shields.io/badge/Language-C%2B%2B-blue)

!\[Frontend](https://img.shields.io/badge/Frontend-Python-yellow)

!\[Status](https://img.shields.io/badge/Status-Completed-green)



A robust, user-space simulation of a File System designed to demonstrate advanced Operating System concepts including \*\*Fault Tolerance\*\*, \*\*Defragmentation\*\*, and \*\*Low-Level Memory Management\*\*.



This project implements a \*\*Hybrid Architecture\*\*:

\* \*\*Backend (C++):\*\* Handles raw binary manipulation of the virtual disk.

\* \*\*Frontend (Python):\*\* Visualizes the memory blocks allocation in real-time.



---



\## 🚀 Key Features



\### 1. 🛡️ Crash Recovery (Journaling)

Implements \*\*Write-Ahead Logging (WAL)\*\* with \*\*Shadow Paging\*\*.

\* \*\*Scenario:\*\* If the system loses power (process killed) during a file write.

\* \*\*Recovery:\*\* On reboot, the system detects the unfinished Journal entry and automatically restores the data from a reserved "Shadow Block" (Block 3).



\### 2. ⚡ Optimization (Defragmentation)

Solves the problem of \*\*External Fragmentation\*\*.

\* \*\*Algorithm:\*\* Reads all valid files into a RAM buffer, wipes the disk, and rewrites files sequentially to eliminate gaps (holes) between data blocks.



\### 3. 🔍 Block Forensics

Includes a low-level inspection tool.

\* Allows the administrator to query any specific \*\*Block Number\*\* to reveal its status (Free/Used), Owner File, and Raw Content.



---



\## 🛠️ Architecture



The system treats a 10MB binary file (`vdisk.bin`) as a raw block device.



\*\*Disk Layout:\*\*

`\[ Block 0 ]` \*\*Superblock:\*\* Global File System Metadata.

`\[ Block 1 ]` \*\*Inode Table:\*\* File Records (Filename -> Block Pointers).

`\[ Block 2 ]` \*\*Journal:\*\* Crash Recovery Logs.

`\[ Block 3 ]` \*\*Shadow Block:\*\* Data Backup Area.

`\[ Block 4+ ]` \*\*Data Area:\*\* User File Content.



---



\## 💻 How to Run



\### Prerequisites

\* \*\*C++ Compiler:\*\* G++ (MinGW or Linux GCC).

\* \*\*Python:\*\* Version 3.x (with `tkinter` installed).



\### Step 1: Compile the Backend

```bash

g++ main.cpp -o fs\_tool.exe

