#include <bits/stdc++.h>
using namespace std;



static const int BLOCK_SIZE = 512;
static const int TOTAL_BLOCKS = 200;

struct Extent {
    int start;
    int len;
};

struct FileMeta {
    string name;
    int size;
    vector<Extent> extents;
};

class Disk {
public:
    vector<bool> bitmap;
    vector<string> blocks;

    Disk() {
        bitmap.assign(TOTAL_BLOCKS, false);
        blocks.assign(TOTAL_BLOCKS, string(BLOCK_SIZE, '\0'));
    }

    bool isFree(int i) { return bitmap[i] == false; }

    int allocContiguous(int needed) {
        int run = 0, start = 0;
        for (int i = 0; i < TOTAL_BLOCKS; i++) {
            if (!bitmap[i]) {
                if (run == 0) start = i;
                run++;
            } else run = 0;

            if (run == needed) {
                for (int j = start; j < start + needed; j++)
                    bitmap[j] = true;
                return start;
            }
        }
        return -1;
    }

    void freeExtent(Extent e) {
        for (int i = e.start; i < e.start + e.len; i++)
            bitmap[i] = false;
    }

    void writeBlock(int idx, string data) {
        blocks[idx] = data.substr(0, BLOCK_SIZE);
    }

    string readBlock(int idx) {
        return blocks[idx];
    }
};

class FileSystem {
public:
    Disk disk;
    map<string, FileMeta> files;

    void createFile(string name, int size) {
        if (files.count(name)) {
            cout << "File already exists.\n";
            return;
        }

        int needed = (size + BLOCK_SIZE - 1) / BLOCK_SIZE;

        int start = disk.allocContiguous(needed);
        if (start == -1) {
            cout << "Not enough contiguous space.\n";
            return;
        }

        FileMeta f;
        f.name = name;
        f.size = size;
        f.extents.push_back({start, needed});
        files[name] = f;

        cout << "Created file '" << name << "' blocks [" << start << ":" << needed << "]\n";
    }

    void deleteFile(string name) {
        if (!files.count(name)) {
            cout << "File not found.\n";
            return;
        }

        for (auto e : files[name].extents)
            disk.freeExtent(e);

        files.erase(name);
        cout << "File deleted.\n";
    }

    void writeFile(string name, string text) {
        if (!files.count(name)) {
            cout << "File not found.\n";
            return;
        }

        FileMeta &f = files[name];
        int maxBlocks = 0;
        for (auto e : f.extents) maxBlocks += e.len;

        string padded = text;
        padded.resize(maxBlocks * BLOCK_SIZE, '\0');

        int blockIndex = 0;
        for (auto e : f.extents) {
            for (int i = 0; i < e.len; i++) {
                string chunk = padded.substr(blockIndex * BLOCK_SIZE, BLOCK_SIZE);
                disk.writeBlock(e.start + i, chunk);
                blockIndex++;
            }
        }

        f.size = text.size();
        cout << "Data written.\n";
    }

    void readFile(string name) {
        if (!files.count(name)) {
            cout << "File not found.\n";
            return;
        }

        FileMeta &f = files[name];
        string out;
        int readBytes = 0;

        for (auto e : f.extents) {
            for (int i = 0; i < e.len; i++) {
                string blk = disk.readBlock(e.start + i);
                int need = min(BLOCK_SIZE, f.size - readBytes);
                if (need <= 0) break;
                out += blk.substr(0, need);
                readBytes += need;
            }
        }

        cout << "----- FILE CONTENT START -----\n";
        cout << out << "\n";
        cout << "----- FILE CONTENT END -----\n";
    }

    void listFiles() {
        cout << "Files (" << files.size() << "):\n";
        for (auto &p : files) {
            cout << p.first << " size=" << p.second.size
                 << " extents=" << p.second.extents.size() << "\n";
        }
    }

    void fragmentationReport() {
        cout << "Fragmentation Report:\n";
        for (auto &p : files) {
            cout << p.first << " -> extents: " << p.second.extents.size() << "\n";
        }
    }

    void simulateCrash() {
        ofstream j("journal.log", ios::trunc);
        j << "CORRUPTED_JOURNAL_ENTRY";
        j.close();
        cout << "Simulated crash (journal truncated).\n";
    }

    void recover() {
        cout << "Recovery simulated (demo version).\n";
    }
};

void help() {
    cout << "Commands:\n";
    cout << " create <name> <size>\n";
    cout << " delete <name>\n";
    cout << " write <name> <text>\n";
    cout << " read <name>\n";
    cout << " ls\n";
    cout << " frag\n";
    cout << " crash\n";
    cout << " recover\n";
    cout << " exit\n";
}

int main() {
    FileSystem fs;
    cout << "File System Recovery & Optimization Tool\n";
    help();

    while (true) {
        cout << "> ";
        string cmd;
        cin >> cmd;

        if (cmd == "create") {
            string n; int s;
            cin >> n >> s;
            fs.createFile(n, s);
        }
        else if (cmd == "delete") {
            string n;
            cin >> n;
            fs.deleteFile(n);
        }
        else if (cmd == "write") {
            string n;
            string text;
            cin >> n;
            getline(cin, text);
            if (text.size() > 0 && text[0] == ' ') text = text.substr(1);
            fs.writeFile(n, text);
        }
        else if (cmd == "read") {
            string n;
            cin >> n;
            fs.readFile(n);
        }
        else if (cmd == "ls") fs.listFiles();
        else if (cmd == "frag") fs.fragmentationReport();
        else if (cmd == "crash") fs.simulateCrash();
        else if (cmd == "recover") fs.recover();
        else if (cmd == "exit") break;
        else help();
    }

    return 0;
}
