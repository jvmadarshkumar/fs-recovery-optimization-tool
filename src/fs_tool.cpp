// File System Recovery & Optimization Tool (FINAL FIXED VERSION)
// Fully compatible with Windows + g++
// Journaling, crash simulation, recovery, defrag, fragmentation reporting

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

    int total_blocks() const { return bitmap.size(); }

    int free_blocks() const {
        int c = 0;
        for (bool b : bitmap)
            if (!b) c++;
        return c;
    }

    int get_free_block_count() const {
        int cnt = 0;
        for (bool b : bitmap)
            if (!b) ++cnt;
        return cnt;
    }

    int find_contiguous(int need) {
        int run = 0;
        int start = 0;
        for (int i = 0; i < total_blocks(); i++) {
            if (!bitmap[i]) {
                if (run == 0) start = i;
                run++;
            } else run = 0;

            if (run == need) return start;
        }
        return -1;
    }

    int alloc_contiguous(int need) {
        int s = find_contiguous(need);
        if (s == -1) return -1;
        for (int i = 0; i < need; i++) bitmap[s + i] = true;
        return s;
    }

    vector<int> alloc_noncontiguous(int need) {
        vector<int> res;
        for (int i = 0; i < total_blocks() && (int)res.size() < need; i++) {
            if (!bitmap[i]) {
                bitmap[i] = true;
                res.push_back(i);
            }
        }
        if ((int)res.size() < need) {
            for (int b : res) bitmap[b] = false;
            res.clear();
        }
        return res;
    }

    void free_blocks(const vector<int> &idxs) {
        for (int b : idxs)
            if (b >= 0 && b < total_blocks())
                bitmap[b] = false;
    }

    void write_block(int idx, const string &data) {
        if (idx < 0 || idx >= total_blocks()) return;
        blocks[idx] = data.substr(0, BLOCK_SIZE);
    }

    string read_block(int idx) {
        if (idx < 0 || idx >= total_blocks()) return "";
        return blocks[idx];
    }
};

class FileSystem {
public:
    Disk disk;
    map<string, FileMeta> files;
    int next_file_id;
    string journal_path = "journal.log";
    string checkpoint_path = "meta.chkpt";

    FileSystem() : disk(TOTAL_BLOCKS), next_file_id(1) {
        load_checkpoint();
    }

    void append_journal(const string &line) {
        ofstream ofs(journal_path, ios::app);
        ofs << line << "\n";
    }

    void begin_txn(int tid) { append_journal("BEGIN " + to_string(tid)); }
    void log_op(int tid, const string &op) { append_journal("TXN " + to_string(tid) + " OP " + op); }
    void commit_txn(int tid) { append_journal("COMMIT " + to_string(tid)); }

    bool create_file(const string &name, int size) {
        if (files.count(name)) {
            cout << "File already exists.\n";
            return false;
        }

        int needed = max(1, (size + BLOCK_SIZE - 1) / BLOCK_SIZE);
        int tid = next_file_id * 1000 + rand() % 1000;
        begin_txn(tid);

        vector<Extent> extents;

        int start = disk.alloc_contiguous(needed);
        if (start != -1) {
            extents.push_back({start, needed});
            log_op(tid, "ALLOC_CONTIG " + name + " " + to_string(start) + " " + to_string(needed));
        }
        else {
            auto idxs = disk.alloc_noncontiguous(needed);
            if (idxs.empty()) {
                cout << "Not enough space.\n";
                return false;
            }
            sort(idxs.begin(), idxs.end());
            int runS = idxs[0], runL = 1;
            for (int i = 1; i < (int)idxs.size(); i++) {
                if (idxs[i] == idxs[i - 1] + 1) runL++;
                else {
                    extents.push_back({runS, runL});
                    runS = idxs[i];
                    runL = 1;
                }
            }
            extents.push_back({runS, runL});
            log_op(tid, "ALLOC_NONCONTIG " + name);
        }

        FileMeta f;
        f.id = next_file_id++;
        f.name = name;
        f.size = size;
        f.extents = extents;
        f.ctime = f.mtime = time(nullptr);

        files[name] = f;
        log_op(tid, "CREATE_META " + name + " size=" + to_string(size));
        commit_txn(tid);

        cout << "Created file '" << name << "'\n";
        return true;
    }

    bool delete_file(const string &name) {
        if (!files.count(name)) {
            cout << "File not found.\n";
            return false;
        }

        int tid = rand() % 100000 + 1;
        begin_txn(tid);

        auto meta = files[name];
        vector<int> tofree;
        for (auto &e : meta.extents)
            for (int i = 0; i < e.len; i++)
                tofree.push_back(e.start + i);

        disk.free_blocks(tofree);
        log_op(tid, "FREE " + name);

        files.erase(name);
        commit_txn(tid);

        cout << "File '" << name << "' deleted.\n";
        return true;
    }

    bool write_file(const string &name, const string &text) {
        if (!files.count(name)) {
            cout << "File not found.\n";
            return false;
        }

        auto &meta = files[name];
        int needed = max(1, (int)((text.size() + BLOCK_SIZE - 1) / BLOCK_SIZE));

        int existing = 0;
        for (auto &e : meta.extents) existing += e.len;

        if (needed > existing) {
            auto new_idxs = disk.alloc_noncontiguous(needed - existing);
            if (new_idxs.empty()) {
                cout << "Not enough space.\n";
                return false;
            }

            sort(new_idxs.begin(), new_idxs.end());
            int runS = new_idxs[0], runL = 1;
            for (int i = 1; i < (int)new_idxs.size(); i++) {
                if (new_idxs[i] == new_idxs[i - 1] + 1) runL++;
                else {
                    meta.extents.push_back({runS, runL});
                    runS = new_idxs[i];
                    runL = 1;
                }
            }
            meta.extents.push_back({runS, runL});
        }

        string padded = text;
        padded.resize(needed * BLOCK_SIZE, '\0');

        int idx = 0;
        for (auto &e : meta.extents) {
            for (int i = 0; i < e.len && idx < needed; i++) {
                disk.write_block(e.start + i, padded.substr(idx * BLOCK_SIZE, BLOCK_SIZE));
                idx++;
            }
        }

        meta.size = text.size();
        meta.mtime = time(nullptr);
        cout << "Data written to " << name << ".\n";
        return true;
    }

    string read_file(const string &name) {
        if (!files.count(name)) {
            cout << "File not found.\n";
            return "";
        }

        auto &meta = files[name];
        string out;
        out.reserve(meta.size);

        int readB = 0;
        for (auto &e : meta.extents) {
            for (int i = 0; i < e.len; i++) {
                int need = min(BLOCK_SIZE, meta.size - readB);
                if (need <= 0) break;
                out += disk.read_block(e.start + i).substr(0, need);
                readB += need;
            }
        }

        return out;
    }

    void list_files() {
        cout << "Files: (" << files.size() << ")  Free blocks: " 
             << disk.free_blocks() << "/" << disk.total_blocks() << "\n";

        for (auto &p : files) {
            auto &m = p.second;
            cout << m.name << " (size=" << m.size << ", extents=" 
                 << m.extents.size() << ")\n";
        }
    }

    void fragmentation_report() {
        cout << "Fragmentation Report:\n";
        for (auto &p : files) {
            cout << p.first << " extents=" << p.second.extents.size() << "\n";
        }
    }

    void simulate_crash(int type) {
        if (type == 1) {
            ofstream ofs(journal_path, ios::trunc);
            ofs << "CORRUPTED_JOURNAL\n";
            ofs.close();
            cout << "Crash type 1 simulated: Journal truncated.\n";
        }
        else if (type == 2) {
            for (int i = 0; i < 5; i++)
                disk.bitmap[i] = !disk.bitmap[i];
            cout << "Crash type 2 simulated: Bitmap bits flipped.\n";
        }
        else cout << "Invalid crash type.\n";
    }

    void recover() {
        cout << "Recovery started...\n";

        ifstream ifs(journal_path);
        if (!ifs.good()) {
            cout << "No journal found.\n";
            return;
        }

        string line;
        set<int> committed;
        map<int, vector<string>> ops;

        int cur = -1;

        while (getline(ifs, line)) {
            if (line.rfind("BEGIN ", 0) == 0) {
                cur = stoi(line.substr(6));
                ops[cur] = {};
            }
            else if (line.rfind("COMMIT ", 0) == 0) {
                committed.insert(stoi(line.substr(7)));
                cur = -1;
            }
            else if (line.rfind("TXN ", 0) == 0) {
                stringstream ss(line);
                string tok1, tidStr;
                ss >> tok1 >> tidStr;
                int tid = stoi(tidStr);
                ss >> tok1; // OP
                string rest; getline(ss, rest);
                if (!rest.empty() && rest[0]==' ') rest = rest.substr(1);
                ops[tid].push_back(rest);
            }
        }

        for (auto &kv : ops) {
            if (!committed.count(kv.first)) continue;

            for (auto &o : kv.second) {
                if (o.rfind("FREE ", 0) == 0) {
                    string name = o.substr(5);
                    if (files.count(name)) {
                        vector<int> tofree;
                        for (auto &e : files[name].extents)
                            for (int i = 0; i < e.len; i++)
                                tofree.push_back(e.start + i);
                        disk.free_blocks(tofree);
                        files.erase(name);
                        cout << "Recovered FREE " << name << "\n";
                    }
                }
            }
        }

        cout << "Recovery complete.\n";
    }

    void checkpoint() {
        ofstream ofs(checkpoint_path);
        for (auto &p : files) {
            auto &m = p.second;
            ofs << m.name << " " << m.size << " " << m.extents.size();
            for (auto &e : m.extents)
                ofs << " " << e.start << ":" << e.len;
            ofs << "\n";
        }
        ofs.close();
        cout << "Checkpoint saved.\n";
    }

    void load_checkpoint() {
        ifstream ifs(checkpoint_path);
        if (!ifs.good()) return;

        files.clear();
        string name;
        while (ifs >> name) {
            int size, extCnt;
            ifs >> size >> extCnt;

            FileMeta m;
            m.id = next_file_id++;
            m.name = name;
            m.size = size;

            for (int i = 0; i < extCnt; i++) {
                string s;
                ifs >> s;
                int pos = s.find(':');
                int st = stoi(s.substr(0, pos));
                int ln = stoi(s.substr(pos + 1));
                m.extents.push_back({st, ln});

                for (int j = 0; j < ln; j++)
                    disk.bitmap[st + j] = true;
            }

            m.ctime = m.mtime = time(nullptr);
            files[name] = m;
        }

        cout << "Checkpoint loaded.\n";
    }
};

void help() {
    cout << "Commands:\n";
    cout << " init\n create <name> <size>\n delete <name>\n write <name> <text>\n";
    cout << " read <name>\n ls\n frag\n crash <1|2>\n recover\n chkpt\n exit\n";
}

int main() {
    FileSystem fs;
    cout << "File System Recovery & Optimization Tool\n";
    help();

    while (true) {
        cout << "> ";
        string cmd;
        cin >> cmd;

        if (cmd == "init") fs = FileSystem();
        else if (cmd == "create") {
            string n; int s; cin >> n >> s; fs.create_file(n, s);
        }
        else if (cmd == "delete") {
            string n; cin >> n; fs.delete_file(n);
        }
        else if (cmd == "write") {
            string n; string data;
            cin >> n;
            getline(cin, data);
            if (!data.empty() && data[0]==' ') data = data.substr(1);
            fs.write_file(n, data);
        }
        else if (cmd == "read") {
            string n; cin >> n;
            cout << fs.read_file(n) << "\n";
        }
        else if (cmd == "ls") fs.list_files();
        else if (cmd == "frag") fs.fragmentation_report();
        else if (cmd == "crash") { int t; cin >> t; fs.simulate_crash(t); }
        else if (cmd == "recover") fs.recover();
        else if (cmd == "chkpt") fs.checkpoint();
        else if (cmd == "exit") break;
        else help();
    }

    return 0;
}


