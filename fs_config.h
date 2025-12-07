#ifndef FS_CONFIG_H
#define FS_CONFIG_H

#include <cstdint>

const int BLOCK_SIZE = 4096;       
const int DISK_SIZE = 10 * 1024 * 1024; 
const int TOTAL_BLOCKS = DISK_SIZE / BLOCK_SIZE;
const int INODE_COUNT = 100;       
const int MAGIC_NUMBER = 0xDEADBEEF; 

struct Superblock {
    int magic_number;      
    int total_blocks;
    int inode_count;
    int free_blocks_count;
    char padding[BLOCK_SIZE - (4 * sizeof(int))]; 
};

struct Inode {
    int id;
    bool is_used;
    char filename[32];
    int file_size;
    int blocks[12]; 
};

struct JournalEntry {
    bool is_active;        
    char operation[10];    
    char filename[32];     
};

#endif