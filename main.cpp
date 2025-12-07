#include <iostream>
#include <string>
#include <limits>
#include "DiskManager.h"
#include "FileSystem.h" 

using namespace std;

int main() {
    string diskName = "vdisk.bin";
    DiskManager dm(diskName);
    FileSystem fs(&dm); 
    
    int choice;
    while(true) {
        cout << "\n--- File Recovery System ---" << endl;
        cout << "1. Format Disk" << endl;
        cout << "2. Mount Disk" << endl;
        cout << "3. Create File" << endl;
        cout << "4. Read File" << endl;
        cout << "5. Delete File" << endl;
        cout << "6. Optimize Disk" << endl;
        cout << "7. Create File (Crash Sim)" << endl;
        cout << "8. Inspect Block" << endl;
        cout << "9. Exit" << endl;
        cout << "Enter choice: ";
        
        if (!(cin >> choice)) {
            cout << "Invalid input!" << endl;
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            continue;
        }
        cin.ignore(numeric_limits<streamsize>::max(), '\n');

        if (choice == 1) fs.format(); 
        else if (choice == 2) fs.mount();
        else if (choice == 3) {
            string name, content;
            cout << "Enter Filename: "; getline(cin, name); 
            cout << "Enter Content: "; getline(cin, content);
            fs.createFile(name, content);
        }
        else if (choice == 4) {
            string name;
            cout << "Enter Filename to Read: "; getline(cin, name);
            fs.readFile(name);
        }
        else if (choice == 5) {
            string name;
            cout << "Enter Filename to Delete: "; getline(cin, name);
            fs.deleteFile(name);
        }
        else if (choice == 6) fs.optimize();
        else if (choice == 7) { 
            string name, content;
            cout << "Enter Filename: "; getline(cin, name); 
            cout << "Enter Content: "; getline(cin, content);
            fs.createFile(name, content, true);
        }
        else if (choice == 8) {
            int blk;
            cout << "Enter Block Number: "; cin >> blk;
            fs.inspectBlock(blk);
        }
        else if (choice == 9) break;
    }
    return 0;
}