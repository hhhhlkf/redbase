#if !defined(PF_ALGOMGR_H)
#define PF_ALGOMGR_H
#include "pf_buffermgr.h"
struct PF_BufPageDesc;
class PF_AlgoMgr
{
public:
    bool flag;
    virtual RC LinkHead(int slot, PF_BufPageDesc *&bufTable) = 0;
    virtual RC InsertFree(int slot, PF_BufPageDesc *&bufTable) = 0;
    virtual RC SelectSlot(int &slot, PF_BufPageDesc *&bufTable) = 0;
    PF_AlgoMgr();
    ~PF_AlgoMgr();
};
class PF_Lru;
class PF_Clock : public PF_AlgoMgr
{
private:
    int currentP; // 当前指针指向的
    int bufferSize;

public:
    PF_Clock(int size);
    ~PF_Clock();
    RC LinkHead(int slot, PF_BufPageDesc *&bufTable);
    RC InsertFree(int slot, PF_BufPageDesc *&bufTable);
    RC SelectSlot(int &slot, PF_BufPageDesc *&bufTable);
};

class PF_Lru : public PF_AlgoMgr
{
private:
    int *first;
    int *last;
    int *free;

public:
    PF_Lru(int &first, int &last, int &free);
    ~PF_Lru();
    RC LinkHead(int slot, PF_BufPageDesc *&bufTable);
    RC InsertFree(int slot, PF_BufPageDesc *&bufTable);
    RC SelectSlot(int &slot, PF_BufPageDesc *&bufTable);
    RC Unlink(int slot, PF_BufPageDesc *&bufTable);
};

#endif // PF_ALGOMGR_H
