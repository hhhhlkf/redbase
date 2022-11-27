#include <gtest/gtest.h>
#include "../include/configloader.h"
#include "ix_manager.h"
#include "ix_indexhandle.h"
config *cfg = nullptr;
void loadConfig()
{
    configloader loader("./config.json");
    loader.loadConfig();
    cfg = loader.getConfig();
}

TEST(test_insert, IX)
{

    loadConfig();
    PF_Manager pf;
    pf.SetBufferSize(cfg->getBufferSize(), LRU);
    IX_Manager ix(pf);
    RC rc;
    system("rm -f account.data.0");
    if (rc = ix.CreateIndex("account.data", 0, INT, 4, cfg->getDiskSize()))
    {
        cout << "rc:" << rc << endl;
    }
    IX_IndexHandle ixh;
    if (rc = ix.OpenIndex("account.data", 0, ixh))
    {
        cout << "rc:" << rc << endl;
    }
    for (int i = 0; i < 9; i++)
    {
        RID r(i, i);
        RC rc;
        rc = ixh.InsertEntry(&i, r);
        ASSERT_EQ(rc, 0);
        RID s;
        rc = ixh.Search(&i, s);
        ASSERT_EQ(rc, 0);
        ASSERT_EQ(r, s);
    }
    if (rc = ix.CloseIndex(ixh))
    {
        cout << "rc:" << rc << endl;
    }
}

TEST(test_del, IX)
{
    loadConfig();
    PF_Manager pf;
    pf.SetBufferSize(cfg->getBufferSize(), LRU);
    IX_Manager ix(pf);
    RC rc;
    IX_IndexHandle ixh;
    if (rc = ix.OpenIndex("account.data", 0, ixh))
    {
        cout << "rc:" << rc << endl;
    }
    for (int i = 0; i < 9; i++)
    {
        RID r(-1, -1);
        RC rc;
        rc = ixh.DeleteEntry(&i, r);
        ASSERT_EQ(rc, 0);
        RID s;
        rc = ixh.Search(&i, s);
        ASSERT_EQ(rc, 0) << "can not find right index";
    }
    ix.CloseIndex(ixh);
}
