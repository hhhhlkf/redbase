#if !defined(PF_FILEHANDLE_H)
#define PF_FILEHANDLE_H
#include "pf.h"
#include "pf_pagehandle.h"
#include "pf_internal.h"
#include "pf_buffermgr.h"
class PF_FileHandle
{
    friend class PF_Manager;

private:
    /* data */
    PF_BufferMgr *pBufferMgr; // pointer to buffer manager
    PF_FileHdr hdr;           // file header
    int bFileOpen;            // file open flag
    int bHdrChanged;          // dirty flag for file hdr
    int unixfd;
    int IsValidPageNum(PageNum pageNum) const; // OS file descriptor
public:
    PF_FileHdr* getHdr();
    RC setHdr(const PF_FileHdr& header);
    RC FlushPages() const;
    PF_FileHandle(const PF_FileHandle &fileHandle);
    PF_FileHandle &operator=(const PF_FileHandle &fileHandle);
    PF_FileHandle(/* args */);
    ~PF_FileHandle();
    RC GetFirstPage(PF_PageHandle &pageHandle) const;
    RC GetNextPage(PageNum current, PF_PageHandle &pageHandle) const;
    RC GetThisPage(PageNum pageNum, PF_PageHandle &pageHandle) const;
    RC GetLastPage(PF_PageHandle &pageHandle) const;
    RC GetPrevPage(PageNum current, PF_PageHandle &pageHandle) const;
    RC AllocatePage(PF_PageHandle &pageHandle); // Allocate a new page
    RC DisposePage(PageNum pageNum);            // Dispose of a page
    RC MarkDirty(PageNum pageNum) const;        // Mark page as dirty
    RC UnpinPage(PageNum pageNum) const;        // Unpin the page

    // Force a page or pages to disk (but do not remove from the buffer pool)
    RC ForcePages(PageNum pageNum = ALL_PAGES) const;
};

#endif // PF_FILEHANDLE_H
