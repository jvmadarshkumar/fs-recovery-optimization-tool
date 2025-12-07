#ifndef DISKMANAGER_H
#define DISKMANAGER_H

#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include "fs_config.h"

using namespace std;

class DiskManager {
private:
    fstream diskFile;
    string diskName;

public:
    DiskManager(string name) : diskName(name) {}

    void formatDisk() {
        diskFile.open(diskName, ios::out | ios::binary | ios::trunc);
        if (!diskFile.is_open()) {
            cerr << "Error: Could not create disk file!" << endl;
            return;
        }
        char buffer[BLOCK_SIZE] = {0};
        for (int i = 0; i < TOTAL_BLOCKS; i++) {
            diskFile.write(buffer, BLOCK_SIZE);
        }
        Superblock sb;
        sb.magic_number = MAGIC_NUMBER;
        sb.total_blocks = TOTAL_BLOCKS;
        sb.inode_count = INODE_COUNT;
        sb.free_blocks_count = TOTAL_BLOCKS - 1; 
        
        diskFile.seekp(0);
        diskFile.write(reinterpret_cast<char*>(&sb), sizeof(Superblock));
        diskFile.close();
    }

    bool mountDisk() {
        diskFile.open(diskName, ios::in | ios::out | ios::binary);
        return diskFile.is_open();
    }

    void writeBlock(int blockNum, char* data) {
        if (!diskFile.is_open()) return;
        diskFile.seekp(blockNum * BLOCK_SIZE);
        diskFile.write(data, BLOCK_SIZE);
    }

    void readBlock(int blockNum, char* buffer) {
        if (!diskFile.is_open()) return;
        diskFile.seekg(blockNum * BLOCK_SIZE);
        diskFile.read(buffer, BLOCK_SIZE);
    }

    ~DiskManager() {
        if (diskFile.is_open()) diskFile.close();
    }
};

#endif