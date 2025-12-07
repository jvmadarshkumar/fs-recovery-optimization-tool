// FINAL VERSION WITH CRASH 3 (METADATA LOSS)
// File System Recovery & Optimization Tool
// Supports Crash 1, Crash 2, Crash 3 + Recovery System

#include <bits/stdc++.h>
using namespace std;

static const int TOTAL_BLOCKS = 200;
static const int BLOCK_SIZE = 512;

struct Extent {
    int start;
    int len;
};

struct FileMeta {
    int id;
    string name;
    int size;
    vector<Extent> extents;
    time_t ctime;
    time_t mtime;
};

class Disk {
public:
    vector<bool> bitmap;
    vector<string> blocks;

    Disk(int n = TOTAL_BLOCKS) {
        bitmap.assign(n, false);
        blocks.assign(n, string(BLOCK_SIZE, '\0'));
    }

    int free_blocks() const {
        int cnt = 0;
        for (bool b : bitmap)
            if (!b) cnt++;
        return cnt;
    }

    int alloc_contig(int need) {
        int run = 0, start = 0;
        for (int i = 0; i < TOTAL_BLOCKS; i++) {
            if (!bitmap[i]) {
                if (run == 0) start = i;
                run++;
            } else run = 0;
            if (run == need) {
                for (int j = start; j < start + need; j++)
                    bitmap[j] = true;
                return start;
            }
        }
        return -1;
    }

    vector<int> alloc_any(int need) {
        vector<int> out;
        for (int i = 0; i < TOTAL_BLOCKS && (int)out.size() < need; i++) {
            if (!bitmap[i]) {
                bitmap[i] = true;
                out.push_back(i);
            }
        }
        if ((int)out.size() < need) {
            for (int x : out) bitmap[x] = false;
            return {};
        }
        return out;
    }

    void free_extent(Extent e) {
        for (int i = e.start; i < e.start + e.len; i++)
            bitmap[i] = false;
    }

    void write_block(int idx, const string &d) {
        if (idx < 0 || idx >= TOTAL_BLOCKS) return;
        blocks[idx] = d.substr(0, BLOCK_SIZE);
    }

    string read_block(int idx) {
        if (idx < 0 || idx >= TOTAL_BLOCKS) return "";
        return blocks[idx];
    }
};

class FileSystem {
public:
    Disk disk;
    map<string, FileMeta> files;
    int nextID = 1;

    string JOURNAL = "journal.log";
    string CHECKPOINT = "meta.chkpt";

    FileSystem() {
        load_checkpoint();
    }

    void log(const string &s) {
        ofstream o(JOURNAL, ios::app);
        o << s << "\n";
    }

    void checkpoint() {
        ofstream o(CHECKPOINT);
        for (auto &p : files) {
            auto &m = p.second;
            o << m.name << " " << m.size << " " << m.extents.size();
            for (auto &e : m.extents)
                o << " " << e.start << ":" << e.len;
            o << "\n";
        }
        cout << "Checkpoint saved.\n";
    }

    void load_checkpoint() {
        ifstream in(CHECKPOINT);
        if (!in.good()) return;

        files.clear();
        string name;
        while (in >> name) {
            int size, ec;
            in >> size >> ec;

            FileMeta m;
            m.id = nextID++;
            m.name = name;
            m.size = size;

            for (int i = 0; i < ec; i++) {
                string s; in >> s;
                int pos = s.find(':');
                int st = stoi(s.substr(0, pos));
                int ln = stoi(s.substr(pos + 1));
                m.extents.push_back({st, ln});

                for (int b = st; b < st + ln; b++)
                    disk.bitmap[b] = true;
            }
            files[name] = m;
        }
        cout << "Checkpoint loaded.\n";
    }

    // ---------- File Operations ----------
    void create_file(string name, int size) {
        int need = (size + BLOCK_SIZE - 1) / BLOCK_SIZE;

        log("BEGIN CREATE " + name);

        int st = disk.alloc_contig(need);
        vector<Extent> ext;

        if (st != -1) {
            ext.push_back({st, need});
            log("ALLOC CONTIG " + to_string(st) + " " + to_string(need));
        } else {
            auto b = disk.alloc_any(need);
            if (b.empty()) {
                cout << "Not enough space.\n";
                log("ABORT CREATE");
                return;
            }
            sort(b.begin(), b.end());
            int s = b[0], l = 1;
            for (int i = 1; i < b.size(); i++) {
                if (b[i] == b[i - 1] + 1) l++;
                else { ext.push_back({s, l}); s = b[i]; l = 1; }
            }
            ext.push_back({s, l});
            log("ALLOC NONCONTIG");
        }

        FileMeta fm;
        fm.id = nextID++;
        fm.name = name;
        fm.size = size;
        fm.extents = ext;
        fm.ctime = fm.mtime = time(nullptr);

        files[name] = fm;
        log("COMMIT CREATE " + name);

        cout << "Created file '" << name << "'\n";
    }

    void write_file(string n, string text) {
        if (!files.count(n)) { cout << "File not found.\n"; return; }

        auto &m = files[n];
        int need = (text.size() + BLOCK_SIZE - 1) / BLOCK_SIZE;

        int have = 0;
        for (auto &e : m.extents) have += e.len;

        if (need > have) {
            auto b = disk.alloc_any(need - have);
            if (b.empty()) { cout << "Not enough space.\n"; return; }

            sort(b.begin(), b.end());
            int s = b[0], l = 1;
            for (int i = 1; i < b.size(); i++) {
                if (b[i] == b[i-1] + 1) l++;
                else { m.extents.push_back({s, l}); s = b[i]; l = 1; }
            }
            m.extents.push_back({s, l});
        }

        string padded = text;
        padded.resize(need * BLOCK_SIZE, '\0');

        int idx = 0;
        for (auto &e : m.extents) {
            for (int i = 0; i < e.len && idx < need; i++) {
                disk.write_block(e.start + i, padded.substr(idx * BLOCK_SIZE, BLOCK_SIZE));
                idx++;
            }
        }

        m.size = text.size();
        m.mtime = time(nullptr);
        cout << "Data written.\n";
    }

    void read_file(string n) {
        if (!files.count(n)) { cout << "File not found.\n"; return; }

        auto &m = files[n];
        string out;
        out.reserve(m.size);

        int r = 0;
        for (auto &e : m.extents) {
            for (int i = 0; i < e.len; i++) {
                int need = min(BLOCK_SIZE, m.size - r);
                if (need <= 0) break;
                out += disk.read_block(e.start + i).substr(0, need);
                r += need;
            }
        }

        cout << out << "\n";
    }

    // ------------------- CRASHES ---------------------

    void crash_1() {
        ofstream o(JOURNAL, ios::trunc);
        o << "CORRUPT_JOURNAL";
        cout << "Crash 1: Journal truncated.\n";
    }

    void crash_2() {
        for (int i = 0; i < 10; i++)
            disk.bitmap[i] = !disk.bitmap[i];
        cout << "Crash 2: Bitmap corrupted.\n";
    }

    void crash_3() {
        if (files.empty()) { cout << "No metadata to corrupt.\n"; return; }
        cout << "Crash 3: All metadata removed.\n";
        files.clear();
    }

    // ------------------- RECOVERY ---------------------

    void recover() {
        cout << "Running recovery...\n";

        // Reload checkpoint
        load_checkpoint();

        cout << "Recovery complete.\n";
    }

    void list_files() {
        cout << "Files = " << files.size()
             << " | Free blocks = " << disk.free_blocks() << "\n";
        for (auto &p : files)
            cout << p.first << " size=" << p.second.size 
                 << " extents=" << p.second.extents.size() << "\n";
    }
};

void help() {
    cout << "Commands:\n";
    cout << " create <name> <size>\n";
    cout << " write <name> <text>\n";
    cout << " read <name>\n";
    cout << " ls\n";
    cout << " crash1 | crash2 | crash3\n";
    cout << " recover\n";
    cout << " chkpt\n";
    cout << " exit\n";
}

int main() {
    FileSystem fs;
    help();

    while (true) {
        cout << "> ";
        string cmd;
        cin >> cmd;

        if (cmd == "create") {
            string n; int s; cin >> n >> s;
            fs.create_file(n, s);
        }
        else if (cmd == "write") {
            string n; string t;
            cin >> n;
            getline(cin, t);
            if (t.size() && t[0] == ' ') t = t.substr(1);
            fs.write_file(n, t);
        }
        else if (cmd == "read") {
            string n; cin >> n;
            fs.read_file(n);
        }
        else if (cmd == "ls") fs.list_files();
        else if (cmd == "crash1") fs.crash_1();
        else if (cmd == "crash2") fs.crash_2();
        else if (cmd == "crash3") fs.crash_3();
        else if (cmd == "recover") fs.recover();
        else if (cmd == "chkpt") fs.checkpoint();
        else if (cmd == "exit") break;
        else help();
    }
}
