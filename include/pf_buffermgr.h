#if !defined(PF_BUFFERMGR_H)
#define PF_BUFFERMGR_H
#include "redbase.h"
#include "pf.h"
#include "pf_internal.h"
#include "pf_algomgr.h"
#include <iostream>
const int INVALID_SLOT = -1;
struct PF_BufPageDesc
{
    char *pData;     // page contents
    int next;        // next in the linked list of buffer pages
    int prev;        // prev in the linked list of buffer pages
    int bDirty;      // TRUE if page is dirty
    int flag;        // pin count
    PageNum pageNum; // page number for this page
    int fd;          // OS file descriptor of this page
    int pinCount;    // pin count
};

enum ALGORITHM
{
    LRU,
    CLOCK
};
class PF_AlgoMgr;
class PF_BufferMgr
{
private:
    /* data */
public:
    PF_BufferMgr(int numPages, ALGORITHM algor);
    ~PF_BufferMgr();
    RC getPage(int fd, PageNum pageNum, char **ppBuffer,
               int bMultiplePins = TRUE);
    RC AllocatePage(int fd, PageNum pageNum, char **ppbuffer);
    RC MarkDirty(int fd, PageNum pageNum);
    RC UnpinPage(int fd, PageNum pageNum);
    RC FlushPages(int fd);
    RC ForcePages(int fd, PageNum PageNum);
    RC ClearBuffer();
    RC AllocateBlock(char *&buffer);
    // Dispose of a memory chunk managed by the buffer manager.
    RC DisposeBlock(char *buffer);

private:
    RC ReadPage(int fd, PageNum pageNum, char *dest);
    RC InsertFree(int slot);
    RC LinkHead(int slot); // Insert slot at head of used
    RC Unlink(int slot);   // Unlink slot

    // Write a page
    RC WritePage(int fd, PageNum pageNum, char *source);
    RC InternalAlloc(int &slot); // Get a slot to use

    // Init the page desc entry
    RC InitPageDesc(int fd, PageNum pageNum, int slot);
    PF_BufPageDesc *bufTable;
    int numPages;
    int pageSize;
    int first;
    int last;
    int free;
    PF_AlgoMgr *algorithm;
};

#endif // PF_BUFFERMGR_H
