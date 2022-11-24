
#include "initLib.h"
#include "configloader.h"
#include "pf_manager.h"
#include "rm_manager.h"
#include <fstream>
#include "rm_filescan.h"
#include "time.h"
#include <string>
PF_Manager PF;
RM_Manager RM(PF);
int main(int argc, char const *argv[])
{
    configloader loader("../config.json");
    loader.loadConfig();
    config *cfg = loader.getConfig();
    PF.SetBufferSize(cfg->getBufferSize(), LRU);
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
    // cout << "************CreateFile begin!************" << endl;
    if ((rc = RM.CreateFile("account.data", cfg->getDiskSize())))
    {
        cout << "rc" << rc << endl;
        return 0;
    }
    // cout << "************CreateFile end!************" << endl;
    if ((rc = RM.OpenFile("account.data", rfh)))
    {
        cout << "rc" << rc << endl;
        return 0;
    };
    char *record = nullptr;
    int recordSize = 0;
    // cout << "************InsertRec begin!************" << endl;
    int count = 0;
    while (ifs.getline(buf, sizeof(buf)))
    {
        // cout << buf << endl;
        record = lib->exdata(buf, recordSize);
        if ((rc = rfh.InsertRec(record, recordSize)))
        {
            cout << "rc" << rc << endl;
        };
        if (count > 500)
        {
            // srand((unsigned)time(NULL));
            int p = 0 + rand() % (1 + 1);
            int s = 0 + rand() % (50 + 1);
            RID rid = RID(p, s);
            if ((rc = rfh.DeleteRec(rid)))
            {
                cout << "rc: " << rc << endl;
            }
        }
        count++;
    }
    // cout << "************InsertRec end!************" << endl;

    /*
        测试删除功能
    */
    // for (int i = 1; i < 100; i++)
    // {
    //     int p = 0 + rand() % (9 + 1);
    //     int s = 0 + rand() % (100 + 1);
    //     RID rid = RID(p, s);
    //     // cout << "p:" << p << " s:" << s << endl;
    //     if ((rc = rfh.DeleteRec(rid)))
    //     {

    //         cout << "rc: " << rc << endl;
    //     }
    // }
    // cout << "p:" << 0 << " s:" << 0 << endl;
    // if ((rc = rfh.DeleteRec(RID(0, 0))))
    // {

    //     cout << "rc: " << rc << endl;
    // }

    /*
        测试读取功能
    */
    if ((rc = RM.CloseFile(rfh)))
    {
        cout << "rc" << rc << endl;
        return 0;
    }
    if ((rc = RM.OpenFile("account.data", rfh)))
    {
        cout << "rc" << rc << endl;
        return 0;
    };
    RM_FileScan rfc;
    rfc.OpenScan(rfh, ALL, -1, -1, NO_OP, nullptr);
    // cout << "************GetNextRec begin!************" << endl;
    for (int i = 0; i < 1000; i++)
    {
        RM_Record rec;
        // 获取下一条数据
        if ((rc = rfc.GetNextRec(rec)))
        {
            cout << rc << endl;
        }
        char *pData = nullptr;
        int length = -1;
        if ((rc = rec.GetData(pData, length)))
        {
            // cout << rc << endl;
        }
        else
        {
            string outStr = lib->rexdata(pData, length);
            cout << outStr << endl;
        }
        // 解析数据库所存数据
    }
    // cout << "************GetNextRec end!************" << endl;
    RM.DestroyFile("account.data");
    delete lib;
    ifs.close();
    return 0;
}
