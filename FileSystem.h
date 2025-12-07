#ifndef FILE_SYSTEM_H
#define FILE_SYSTEM_H

#include "DiskManager.h"
#include <vector>
#include <iostream>
#include <string>
#include <cstring>
#include <algorithm> 
#include <cstdlib> 
#include <windows.h> 

struct FileBuffer {
    Inode node;
    std::string content;
};

class FileSystem {
private:
    DiskManager* disk;
    Superblock sb;
    std::vector<bool> blockBitmap;

    void updateVisualizer() {
        std::ofstream vizFile("disk_map.txt");
        if(vizFile.is_open()) {
            for(bool bit : blockBitmap) {
                vizFile << (bit ? "1" : "0");
            }
            vizFile.close();
        }
    }

    void logStart(std::string op, std::string name, std::string content = "") {
        JournalEntry entry;
        entry.is_active = true;
        strcpy(entry.operation, op.c_str());
        strncpy(entry.filename, name.c_str(), 31);
        
        char buffer[BLOCK_SIZE] = {0};
        memcpy(buffer, &entry, sizeof(JournalEntry));
        disk->writeBlock(2, buffer);

        if (!content.empty()) {
            char dataBuffer[BLOCK_SIZE] = {0};
            size_t copySize = (content.size() < BLOCK_SIZE) ? content.size() : BLOCK_SIZE;
            memcpy(dataBuffer, content.c_str(), copySize);
            disk->writeBlock(3, dataBuffer); 
        }
    }

    void logEnd() {
        char buffer[BLOCK_SIZE] = {0}; 
        disk->writeBlock(2, buffer);
    }

    void saveInode(Inode& newNode) {
        char buffer[BLOCK_SIZE];
        disk->readBlock(1, buffer); 
        Inode* inodes = reinterpret_cast<Inode*>(buffer);
        int maxInodes = BLOCK_SIZE / sizeof(Inode);

        for (int i = 0; i < maxInodes; i++) {
            if (!inodes[i].is_used) {
                inodes[i] = newNode; 
                disk->writeBlock(1, buffer); 
                return;
            }
        }
    }

    int findInodeIndex(std::string name) {
        char buffer[BLOCK_SIZE];
        disk->readBlock(1, buffer);
        Inode* inodes = reinterpret_cast<Inode*>(buffer);
        int maxInodes = BLOCK_SIZE / sizeof(Inode);

        for (int i = 0; i < maxInodes; i++) {
            if (inodes[i].is_used && strcmp(inodes[i].filename, name.c_str()) == 0) {
                return i;
            }
        }
        return -1; 
    }

public:
    // --- CONSTRUCTOR UPDATED TO FIX UI BUG ---
    FileSystem(DiskManager* d) : disk(d) {
        blockBitmap.resize(TOTAL_BLOCKS, false);
        blockBitmap[0]=true; blockBitmap[1]=true; blockBitmap[2]=true; blockBitmap[3]=true;
        
        // This resets the UI to Green immediately when the program starts!
        updateVisualizer(); 
    }
    // -----------------------------------------

    void format() {
        disk->formatDisk(); 
        std::fill(blockBitmap.begin(), blockBitmap.end(), false);
        blockBitmap[0]=true; blockBitmap[1]=true; blockBitmap[2]=true; blockBitmap[3]=true;
        updateVisualizer(); 
    }

    bool mount() {
        if (!disk->mountDisk()) return false;
        char buffer[BLOCK_SIZE];
        
        disk->readBlock(0, buffer);
        memcpy(&sb, buffer, sizeof(Superblock));
        
        disk->readBlock(1, buffer);
        Inode* inodes = reinterpret_cast<Inode*>(buffer);
        
        std::fill(blockBitmap.begin(), blockBitmap.end(), false);
        blockBitmap[0]=true; blockBitmap[1]=true; blockBitmap[2]=true; blockBitmap[3]=true;
        
        int maxInodes = BLOCK_SIZE / sizeof(Inode);
        for(int i=0; i<maxInodes; i++) {
            if (inodes[i].is_used) {
                int fileBlocks = (inodes[i].file_size + BLOCK_SIZE - 1) / BLOCK_SIZE;
                for(int j=0; j<fileBlocks; j++) {
                    blockBitmap[inodes[i].blocks[j]] = true;
                }
            }
        }

        disk->readBlock(2, buffer);
        JournalEntry* entry = reinterpret_cast<JournalEntry*>(buffer);
        
        if (entry->is_active) {
            std::cout << "\n!!! SYSTEM CRASH DETECTED !!!" << std::endl;
            if (strcmp(entry->operation, "CREATE") == 0) {
                std::cout << "Attempting Data Recovery from Shadow Block (3)..." << std::endl;
                char backupBuffer[BLOCK_SIZE];
                disk->readBlock(3, backupBuffer);
                std::string recoveredContent(backupBuffer);
                std::string recName = "recovered_" + std::string(entry->filename);
                createFile(recName, recoveredContent, false); 
                std::cout << "SUCCESS! Data saved to '" << recName << "'" << std::endl;
            }
            else {
                int idx = findInodeIndex(entry->filename);
                if (idx != -1) deleteFile(entry->filename);
            }
            logEnd();
        }
        std::cout << "FileSystem Mounted. Free Blocks: " << sb.free_blocks_count << std::endl;
        updateVisualizer();
        return true;
    }

    bool createFile(std::string filename, std::string content, bool simulateCrash = false) {
        logStart("CREATE", filename, content);

        int requiredBlocks = (content.size() + BLOCK_SIZE - 1) / BLOCK_SIZE;
        std::vector<int> freeBlockIndices;
        
        for (int i = 4; i < TOTAL_BLOCKS; i++) { 
            if (!blockBitmap[i]) {
                freeBlockIndices.push_back(i);
                blockBitmap[i] = true;
                if (freeBlockIndices.size() == requiredBlocks) break;
            }
        }

        if (freeBlockIndices.size() < requiredBlocks) {
            std::cout << "Error: Not enough space!" << std::endl;
            logEnd(); 
            return false;
        }

        for (int i = 0; i < requiredBlocks; i++) {
            if (i == 0) { 
                std::cout << "WRITING TO DISK... (You have 2 seconds to close the window!)" << std::endl;
                Sleep(2000); 
            }
            if (simulateCrash && i == requiredBlocks / 2) {
                updateVisualizer(); 
                exit(1); 
            }

            char buffer[BLOCK_SIZE] = {0};
            std::string chunk = content.substr(i * BLOCK_SIZE, BLOCK_SIZE);
            memcpy(buffer, chunk.c_str(), chunk.size());
            disk->writeBlock(freeBlockIndices[i], buffer);
        }

        Inode newNode;
        newNode.id = 1; 
        strncpy(newNode.filename, filename.c_str(), 31);
        newNode.filename[31] = '\0';
        newNode.file_size = content.size();
        newNode.is_used = true;
        for(int i=0; i<requiredBlocks; i++) newNode.blocks[i] = freeBlockIndices[i];
        saveInode(newNode);
        
        sb.free_blocks_count -= requiredBlocks;
        logEnd();
        std::cout << "File Saved!" << std::endl;
        updateVisualizer();
        return true;
    }

    void readFile(std::string filename) {
        int index = findInodeIndex(filename);
        if (index == -1) {
            std::cout << "Error: File not found!" << std::endl;
            return;
        }
        char buffer[BLOCK_SIZE];
        disk->readBlock(1, buffer);
        Inode* inodes = reinterpret_cast<Inode*>(buffer);
        Inode fileNode = inodes[index];

        std::cout << "\n--- Reading File: " << fileNode.filename << " ---" << std::endl;
        int blocksToRead = (fileNode.file_size + BLOCK_SIZE - 1) / BLOCK_SIZE;
        std::cout << "Content: ";
        for(int i = 0; i < blocksToRead; i++) {
            char dataBuffer[BLOCK_SIZE];
            disk->readBlock(fileNode.blocks[i], dataBuffer);
            int charsToPrint = (i == blocksToRead - 1) ? (fileNode.file_size % BLOCK_SIZE) : BLOCK_SIZE;
            if (charsToPrint == 0) charsToPrint = BLOCK_SIZE;
            for(int j=0; j<charsToPrint; j++) std::cout << dataBuffer[j];
        }
        std::cout << "\n-----------------------" << std::endl;
    }

    void deleteFile(std::string filename) {
        logStart("DELETE", filename); 
        int index = findInodeIndex(filename);
        if (index == -1) {
            std::cout << "Error: File not found!" << std::endl;
            logEnd();
            return;
        }
        char buffer[BLOCK_SIZE];
        disk->readBlock(1, buffer);
        Inode* inodes = reinterpret_cast<Inode*>(buffer);
        int fileBlocks = (inodes[index].file_size + BLOCK_SIZE - 1) / BLOCK_SIZE;
        for(int i=0; i<fileBlocks; i++) {
            blockBitmap[inodes[index].blocks[i]] = false;
        }
        sb.free_blocks_count += fileBlocks;
        inodes[index].is_used = false;
        disk->writeBlock(1, buffer);
        logEnd(); 
        std::cout << "File Deleted!" << std::endl;
        updateVisualizer();
    }

    void optimize() {
        std::cout << "Starting Optimization..." << std::endl;
        char buffer[BLOCK_SIZE];
        disk->readBlock(1, buffer); 
        Inode* inodes = reinterpret_cast<Inode*>(buffer);
        int maxInodes = BLOCK_SIZE / sizeof(Inode);
        std::vector<FileBuffer> backups;

        for(int i=0; i<maxInodes; i++) {
            if(inodes[i].is_used) {
                FileBuffer fb;
                fb.node = inodes[i]; 
                int blocks = (inodes[i].file_size + BLOCK_SIZE - 1) / BLOCK_SIZE;
                for(int b=0; b<blocks; b++) {
                    char dataChunk[BLOCK_SIZE];
                    disk->readBlock(inodes[i].blocks[b], dataChunk);
                    int size = (b == blocks-1) ? (inodes[i].file_size % BLOCK_SIZE) : BLOCK_SIZE;
                    if(size == 0) size = BLOCK_SIZE;
                    fb.content.append(dataChunk, size);
                }
                backups.push_back(fb);
            }
        }
        std::fill(blockBitmap.begin(), blockBitmap.end(), false);
        blockBitmap[0]=true; blockBitmap[1]=true; blockBitmap[2]=true; blockBitmap[3]=true;
        
        char cleanBuffer[BLOCK_SIZE] = {0};
        disk->writeBlock(1, cleanBuffer); 
        sb.free_blocks_count = TOTAL_BLOCKS - 4; 
        for(auto& fb : backups) {
            createFile(fb.node.filename, fb.content, false);
        }
        updateVisualizer();
    }

    void inspectBlock(int blockNum) {
        if (blockNum < 0 || blockNum >= TOTAL_BLOCKS) return;
        std::cout << "--- BLOCK " << blockNum << " DETAILS ---" << std::endl;
        
        if (blockNum < 4) {
            std::cout << "Type: SYSTEM (Reserved)" << std::endl;
            std::cout << "Status: USED" << std::endl;
        } 
        else if (!blockBitmap[blockNum]) {
            std::cout << "Status: FREE" << std::endl;
        } 
        else {
            std::cout << "Status: USED" << std::endl;
            char inodeBuffer[BLOCK_SIZE];
            disk->readBlock(1, inodeBuffer);
            Inode* inodes = reinterpret_cast<Inode*>(inodeBuffer);
            bool found = false;
            for(int i=0; i<INODE_COUNT; i++) {
                if (inodes[i].is_used) {
                    int fileBlocks = (inodes[i].file_size + BLOCK_SIZE - 1) / BLOCK_SIZE;
                    for(int j=0; j<fileBlocks; j++) {
                        if (inodes[i].blocks[j] == blockNum) {
                            std::cout << "Owner File: " << inodes[i].filename << std::endl;
                            found = true;
                            break;
                        }
                    }
                }
                if(found) break;
            }
            char dataBuffer[BLOCK_SIZE];
            disk->readBlock(blockNum, dataBuffer);
            std::string content(dataBuffer, BLOCK_SIZE);
            content.erase(std::find(content.begin(), content.end(), '\0'), content.end());
            if(!content.empty()) std::cout << "Raw Data: " << content << std::endl;
        }
        std::cout << "-----------------------------------" << std::endl;
    }
};

#endif