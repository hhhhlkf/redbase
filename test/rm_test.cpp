#include <gtest/gtest.h>
#include "../include/configloader.h"
#include "initLib.h"
#include "pf_manager.h"
#include "rm_manager.h"
#include <fstream>
#include "rm_filescan.h"
#include <ctime>
#include <string>
config *cfg = nullptr;
void loadConfig()
{
    configloader loader("./config.json");
    loader.loadConfig();
    cfg = loader.getConfig();
}

TEST(test, RM)
{
    PF_Manager PF;
    RM_Manager RM(PF);
    loadConfig();
    PF.SetBufferSize(cfg->getBufferSize(), LRU);
    initLib *lib = new initLib();
    ifstream ifs;
    ifs.open("../../src/test.txt", ios::in);
    if (ifs.fail())
    {
        cout << "error to open" << endl;
    }
    char buf[1024] = {0};
    RM_FileHandle rfh;
    RC rc;
    system("rm -f account.data");
    // cout << "************CreateFile begin!************" << endl;
    if ((rc = RM.CreateFile("account.data", cfg->getDiskSize())))
    {
        cout << "wrong rc is: " << rc << endl;
        // return 0;
    }
    // cout << "************CreateFile end!************" << endl;
    if ((rc = RM.OpenFile("account.data", rfh)))
    {
        cout << "wrong rc is: " << rc << endl;
        // return 0;
    };
    char *record = nullptr;
    int recordSize = 0;
    // cout << "************InsertRec begin!************" << endl;
    int count = 0;
    while (ifs.getline(buf, sizeof(buf)))
    {
        /*
        测试插入功能
        */
        record = lib->exdata(buf, recordSize);
        if ((rc = rfh.InsertRec(record, recordSize)))
        {
            cout << "wrong rc is: " << rc << endl;
        };
        /*
        测试删除功能
        */
        if (count > 500)
        {
            // srand((unsigned)time(NULL));
            int p = 0 + rand() % (1 + 1);
            int s = 0 + rand() % (50 + 1);
            RID rid = RID(p, s);
            if ((rc = rfh.DeleteRec(rid)))
            {
                cout << "wrong rc is: " << rc << endl;
            }
        }
        count++;
    }
    // cout << "************InsertRec end!************" << endl;

    /*
    测试打开功能
    */
    if ((rc = RM.CloseFile(rfh)))
    {
        cout << "wrong rc is: " << rc << endl;
        // return 0;
    }
    /*
    测试读取功能
    */
    if ((rc = RM.OpenFile("account.data", rfh)))
    {
        cout << "wrong rc is: " << rc << endl;
        // return 0;
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
    /*
    测试销毁功能
    */
    RM.DestroyFile("account.data");
    delete lib;
    ifs.close();
}