#include "pf_filehandle.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
PF_FileHandle::PF_FileHandle()
{
    bFileOpen = FALSE;
    pBufferMgr = NULL;
}
PF_FileHandle::PF_FileHandle(const PF_FileHandle &fileHandle)
{
    this->pBufferMgr = fileHandle.pBufferMgr;
    this->hdr = fileHandle.hdr;
    this->bFileOpen = fileHandle.bFileOpen;
    this->bHdrChanged = fileHandle.bHdrChanged;
    this->unixfd = fileHandle.unixfd;
}
PF_FileHandle &PF_FileHandle::operator=(const PF_FileHandle &fileHandle)
{
    // Test for self-assignment
    if (this != &fileHandle)
    {
        // Just copy the members since there is no memory allocation involved
        this->pBufferMgr = fileHandle.pBufferMgr;
        this->hdr = fileHandle.hdr;
        this->bFileOpen = fileHandle.bFileOpen;
        this->bHdrChanged = fileHandle.bHdrChanged;
        this->unixfd = fileHandle.unixfd;
    }

    // Return a reference to this
    return (*this);
}
RC PF_FileHandle::GetFirstPage(PF_PageHandle &pageHandle) const
{
    return (GetNextPage((PageNum)-1, pageHandle));
}
RC PF_FileHandle::GetLastPage(PF_PageHandle &pageHandle) const
{
    return (GetPrevPage((PageNum)hdr.numPages, pageHandle));
}

PF_FileHandle::~PF_FileHandle()
{
}
RC PF_FileHandle::GetNextPage(PageNum current, PF_PageHandle &pageHandle) const
{
    int rc; // return code

    // File must be open
    if (!bFileOpen)
        return (PF_CLOSEDFILE);

    // Validate page number (note that -1 is acceptable here)
    if (current != -1 && !IsValidPageNum(current))
        return (PF_INVALIDPAGE);

    // Scan the file until a valid used page is found
    for (current++; current < hdr.numPages; current++)
    {

        // If this is a valid (used) page, we're done
        if (!(rc = GetThisPage(current, pageHandle)))
            return (0);

        // If unexpected error, return it
        if (rc != PF_INVALIDPAGE)
            return (rc);
    }

    // No valid (used) page found
    return (PF_EOF);
}
RC PF_FileHandle::GetPrevPage(PageNum current, PF_PageHandle &pageHandle) const
{
    int rc;
    if (!bFileOpen)
        return (PF_CLOSEDFILE);
    if (current != hdr.numPages && !IsValidPageNum(current))
        return (PF_INVALIDPAGE);
    for (current--; current >= 0; current--)
    {
        if (!(rc = GetThisPage(current, pageHandle)))
            return (0);
        if (rc != PF_INVALIDPAGE)
            return (rc);
    }
    return (PF_EOF);
}
RC PF_FileHandle::GetThisPage(PageNum pageNum, PF_PageHandle &pageHandle) const
{
    int rc;         // return code
    char *pPageBuf; // address of page in buffer pool

    // File must be open
    if (!bFileOpen)
        return (PF_CLOSEDFILE);

    // Validate page number
    if (!IsValidPageNum(pageNum))
        return (PF_INVALIDPAGE);

    // Get this page from the buffer manager
    if ((rc = pBufferMgr->getPage(unixfd, pageNum, &pPageBuf)))
        return (rc);

    // If the page is valid, then set pageHandle to this page and return ok
    if (((PF_PageHdr *)pPageBuf)->nextFree == PF_PAGE_USED)
    {

        pageHandle.pageNum = pageNum;
        pageHandle.pPageData = pPageBuf;
        return (0);
    }
    // cout << "pageNum " << pageNum << " come here" << endl;
    // If the page is *not* a valid one, then unpin the page
    //减去刚才getPage时加上的pinCount
    if ((rc = UnpinPage(pageNum)))
        return (rc);
    // cout << "here" << endl;
    return (PF_INVALIDPAGE);
}
RC PF_FileHandle::AllocatePage(PF_PageHandle &pageHandle)
{
    int rc;         // return code
    int pageNum;    // new-page number
    char *pPageBuf; // address of page in buffer poolf
    // File must be open
    if (!bFileOpen)
        return (PF_CLOSEDFILE);
    // If the free list isn't empty...
    if (hdr.firstFree != PF_PAGE_LIST_END)
    {
        pageNum = hdr.firstFree;
        //
        // Get the first free page into the buffer
        if ((rc = pBufferMgr->getPage(unixfd,
                                      pageNum,
                                      &pPageBuf)))
            return (rc);
        // Set the first free page to the next page on the free list
        cout << "the number of used blocks is " << hdr.firstFree << endl;
        hdr.firstFree = ((PF_PageHdr *)pPageBuf)->nextFree;

        // cout << "hdr.firstFree:" << hdr.firstFree << endl;
    }
    else
    {
        //先申请一块内存，等到该内存进行强制写回时，会自动扩充文件大小
        // The free list is empty...
        // cout << "I have came here!!!!!!!!!!!!!!" << endl;
        pageNum = hdr.numPages;

        // Allocate a new page in the file
        //硬存，直接覆盖
        if ((rc = pBufferMgr->AllocatePage(unixfd,
                                           pageNum,
                                           &pPageBuf)))
            return (rc);
        // Increment the number of pages for this file
        hdr.numPages++;
    }
    // Mark the header as changed
    bHdrChanged = TRUE;
    // Mark this page as used
    ((PF_PageHdr *)pPageBuf)->nextFree = PF_PAGE_USED;
    // Zero out the page data
    memset(pPageBuf + sizeof(PF_PageHdr), 0, PF_PAGE_SIZE);
    // Mark the page dirty because we changed the next pointer
    if ((rc = MarkDirty(pageNum)))
        return (rc);
    PF_PageHdr *header = (PF_PageHdr *)pPageBuf;
    // cout << pageNum << " : " << header->freeCnt << " " << header->nextFree << endl;
    // Set the pageHandle local variables
    pageHandle.pageNum = pageNum;
    pageHandle.pPageData = pPageBuf;
    // Return ok
    return (0);
}

RC PF_FileHandle::DisposePage(PageNum pageNum)
{
    int rc;
    char *pPageBuf;
    if (!bFileOpen)
        return (PF_CLOSEDFILE);
    if (!IsValidPageNum(pageNum))
        return (PF_INVALIDPAGE);
    if ((rc = pBufferMgr->getPage(unixfd,
                                  pageNum,
                                  &pPageBuf,
                                  FALSE)))
        return (rc);
    if (((PF_PageHdr *)pPageBuf)->nextFree != PF_PAGE_USED)
    {
        if ((rc = UnpinPage(pageNum)))
            return (rc);
        // Return page already free
        return (PF_PAGEFREE);
    }
    ((PF_PageHdr *)pPageBuf)->nextFree = hdr.firstFree;
    hdr.firstFree = pageNum;
    bHdrChanged = TRUE;
    if ((rc = MarkDirty(pageNum)))
        return (rc);

    // Unpin the page
    if ((rc = UnpinPage(pageNum)))
        return (rc);

    return (0);
}
RC PF_FileHandle::MarkDirty(PageNum pageNum) const
{
    if (!bFileOpen)
        return (PF_CLOSEDFILE);
    if (!IsValidPageNum(pageNum))
        return (PF_INVALIDPAGE);
    return (pBufferMgr->MarkDirty(unixfd, pageNum));
}
RC PF_FileHandle::UnpinPage(PageNum pageNum) const
{
    if (!bFileOpen)
        return (PF_CLOSEDFILE);
    if (!IsValidPageNum(pageNum))
        return (PF_INVALIDPAGE);
    
    return (pBufferMgr->UnpinPage(unixfd, pageNum));
}
RC PF_FileHandle::FlushPages() const
{
    // File must be open
    if (!bFileOpen)
        return (PF_CLOSEDFILE);

    // If the file header has changed, write it back to the file
    if (bHdrChanged)
    {

        // First seek to the appropriate place
        if (lseek(unixfd, 0, L_SET) < 0)
            return (PF_UNIX);

        // Write header
        int numBytes = write(unixfd,
                             (char *)&hdr,
                             sizeof(PF_FileHdr));
        if (numBytes < 0)
            return (PF_UNIX);
        if (numBytes != sizeof(PF_FileHdr))
            return (PF_HDRWRITE);

        // This function is declared const, but we need to change the
        // bHdrChanged variable.  Cast away the constness
        PF_FileHandle *dummy = (PF_FileHandle *)this;
        dummy->bHdrChanged = FALSE;
    }

    // Tell Buffer Manager to flush pages
    return (pBufferMgr->FlushPages(unixfd));
}

RC PF_FileHandle::ForcePages(PageNum pageNum) const
{
    // File must be open
    if (!bFileOpen)
        return (PF_CLOSEDFILE);

    // If the file header has changed, write it back to the file
    if (bHdrChanged)
    {

        // First seek to the appropriate place
        if (lseek(unixfd, 0, L_SET) < 0)
            return (PF_UNIX);

        // Write header
        int numBytes = write(unixfd,
                             (char *)&hdr,
                             sizeof(PF_FileHdr));
        if (numBytes < 0)
            return (PF_UNIX);
        if (numBytes != sizeof(PF_FileHdr))
            return (PF_HDRWRITE);

        // This function is declared const, but we need to change the
        // bHdrChanged variable.  Cast away the constness
        PF_FileHandle *dummy = (PF_FileHandle *)this;
        dummy->bHdrChanged = FALSE;
    }

    // Tell Buffer Manager to Force the page
    return (pBufferMgr->ForcePages(unixfd, pageNum));
}

int PF_FileHandle::IsValidPageNum(PageNum pageNum) const
{
    return (bFileOpen &&
            pageNum >= 0 &&
            pageNum < hdr.numPages);
}
PF_FileHdr *PF_FileHandle::getHdr()
{
    PF_FileHdr *hdr = new PF_FileHdr();
    hdr->firstFree = this->hdr.firstFree;
    // cout << "Yes!" << endl;
    hdr->numPages = this->hdr.numPages;
    // cout << "Yes!" << endl;
    hdr->FreeSlotPage = this->hdr.FreeSlotPage;
    return hdr;
}

RC PF_FileHandle::setHdr(const PF_FileHdr &header)
{
    // hdr.firstFree = header.firstFree;
    hdr.FreeSlotPage = header.FreeSlotPage;
    hdr.numPages = header.numPages;
    bHdrChanged = true;
    return (0);
}