
#include "initLib.h"
#include "configloader.h"
#include "pf_manager.h"
#include "rm_manager.h"
#include <fstream>

PF_Manager PF;
RM_Manager RM(PF);
int main(int argc, char const *argv[])
{
    configloader loader("../config.json");
    initLib *lib = new initLib();
    ifstream ifs;
    ifs.open("../src/test.txt", ios::in);
    if (ifs.fail())
    {
        cout << "error to open" << endl;
        return 0;
    }
    char buf[1024] = {0};
    RM_FileHandle rfh;
    RC rc;
    if ((rc = RM.CreateFile("account.data", 24)))
    {
        cout << "rc" << rc << endl;
        return 0;
    }
    if ((rc = RM.OpenFile("account.data", rfh)))
    {
        cout << "rc" << rc << endl;
        return 0;
    };
    char *record = nullptr;
    int recordSize = 0;
    while (ifs.getline(buf, sizeof(buf)))
    {
        // cout << buf << endl;
        record = lib->exdata(buf, recordSize);
        if ((rc = rfh.InsertRec(record, recordSize)))
        {
            cout << "rc" << rc << endl;
        };
    }
    RM.CloseFile(rfh);
    RM.DestroyFile("account.data");
    ifs.close();
    return 0;
}
