#include "pf_buffermgr.h"

#include <unistd.h>
#include <sys/types.h>
#define MEMORY_FD -1
PF_BufferMgr::PF_BufferMgr(int numPages, ALGORITHM algor, ALGORITHM algor)
{
    this->numPages = numPages;
    pageSize = PF_PAGE_SIZE + sizeof(PF_PageHdr);
    bufTable = new PF_BufPageDesc[numPages];
    for (int i = 0; i < numPages; i++)
    {
        if ((bufTable[i].pData = new char[pageSize]) == NULL)
        {
            cerr << "Not enough memory for buffer\n";
            exit(1);
        }
        memset((void *)bufTable[i].pData, 0, pageSize);
        bufTable[i].prev = i - 1;
        bufTable[i].next = i + 1;
        bufTable[i].flag = 0;
        bufTable[i].pinCount = 0;
        bufTable[i].bDirty = FALSE;
        bufTable[i].pageNum = -1;
        bufTable[i].bDirty = FALSE;
        bufTable[i].pageNum = -1;
    }
    bufTable[0].prev = bufTable[numPages - 1].next = INVALID_SLOT;
    //将第一个空闲缓存指向0号缓存块
    free = 0;
    //将最近使用和最不常使用的slot标识符作为
    first = last = INVALID_SLOT;
    if (algor == LRU)
    {
        algorithm = new PF_Lru(first, last, free);
    }
    else if (algor == CLOCK)
    {
        algorithm = new PF_Clock(numPages);
    }
    else
    {
        cout << "error!" << endl;
    }
    if (algor == LRU)
    {
        algorithm = new PF_Lru(first, last, free);
    }
    else if (algor == CLOCK)
    {
        algorithm = new PF_Clock(numPages);
    }
    else
    {
        cout << "error!" << endl;
    }
}

RC PF_BufferMgr::getPage(int fd, PageNum pageNum, char **ppBuffer,
                         int bMultiplePins)
{
    RC rc;         //返回值
    int slot = -1; //页所在的缓冲槽
    // for (int i = 0; i < numPages; i++)
    // {
    //     cout << bufTable[i].pageNum << " ";
    // }
    // cout << endl;
    for (int i = 0; i < numPages; i++)
    {

        if (bufTable[i].fd == fd && bufTable[i].pageNum == pageNum)
        {
            bufTable[i].pinCount++;
            *ppBuffer = bufTable[i].pData;
            // cout << "I am here :" << pageNum << endl;

            return (0);
        }
    }
    // cout << "here i am 1" << pageNum << endl;
    if (rc = InternalAlloc(slot))
        return (rc);
    // cout << "here i am 2" << pageNum << endl;
    if ((rc = ReadPage(fd, pageNum, bufTable[slot].pData)) ||
        (rc = InitPageDesc(fd, pageNum, slot)))
    {
        // // Unlink(slot);
        // // InsertFree(slot);
        algorithm->InsertFree(slot, bufTable);
        algorithm->InsertFree(slot, bufTable);
        // cout << "read:" << pageNum << endl;
        return (rc);
    }
    *ppBuffer = bufTable[slot].pData;
    // cout << "here i am 3" << pageNum << endl;
    return (0);
}

RC PF_BufferMgr::InitPageDesc(int fd, PageNum pageNum, int slot)
{
    // set the slot to refer to a newly-pinned page
    bufTable[slot].fd = fd;
    bufTable[slot].pageNum = pageNum;
    bufTable[slot].bDirty = FALSE;
    bufTable[slot].pinCount = 1;
    bufTable[slot].flag = 1;
    // Return ok
    return (0);
}

RC PF_BufferMgr::AllocatePage(int fd, PageNum pageNum, char **ppBuffer)
{
    RC rc;
    int slot;
    for (int i = 0; i < numPages; i++)
    {
        if (bufTable[i].pageNum == pageNum &&
            bufTable[i].fd == fd)
        {
            return (PF_PAGEINBUF);
        }
    }
    if ((rc = InternalAlloc(slot)))
    {
        cout << "I am here!" << endl;
    {
        cout << "I am here!" << endl;
        return (rc);
    }

    }

    if ((rc = InitPageDesc(fd, pageNum, slot)))
    {
        // // Unlink(slot);
        // // InsertFree(slot);
        algorithm->InsertFree(slot, bufTable);
        algorithm->InsertFree(slot, bufTable);
        return (rc);
    }
    *ppBuffer = bufTable[slot].pData;
    // cout << "finished" << endl;
    return (0);
}

RC PF_BufferMgr::MarkDirty(int fd, PageNum pageNum)
{
    RC rc;
    int slot = -1;
    for (int i = 0; i < numPages; i++)
    {
        if (fd == bufTable[i].fd &&
            pageNum == bufTable[i].pageNum)
        {
            slot = i;
            break;
        }
    }
    if (slot == -1)
        return (PF_PAGENOTINBUF);
    bufTable[slot].bDirty = TRUE;
    // if ((rc = Unlink(slot)) ||
    //     (rc = LinkHead(slot)))
    if ((rc = algorithm->LinkHead(slot, bufTable)))
        return (rc);
    return 0;
}
RC PF_BufferMgr::UnpinPage(int fd, PageNum pageNum)
{
    RC rc;
    int slot;
    for (int i = 0; i < numPages; i++)
    {
        if (fd == bufTable[i].fd &&
            pageNum == bufTable[i].pageNum)
        {
            slot = i;
            break;
        }
    }
    if (slot == -1)
        return (PF_PAGENOTINBUF);
    if (bufTable[slot].pinCount == 0)
        return (0);
    // cout << "bufTable[slot].pinCount:" << bufTable[slot].pinCount << endl;
    if (--(bufTable[slot].pinCount) == 0)
    {
        // if ((rc = Unlink(slot)) ||
        //     (rc = LinkHead(slot)))

        if ((rc = algorithm->LinkHead(slot, bufTable)))
            return (rc);
    }
    return (0);
}
RC PF_BufferMgr::FlushPages(int fd)
{
    RC rc, rcWarn = 0;
    int slot = first;
    while (slot != INVALID_SLOT)
    {
        int next = bufTable[slot].next;
        if (bufTable[slot].fd == fd)
        {
            if (bufTable[slot].pinCount)
            {
                rcWarn = PF_PAGEPINNED;
            }
            else
            {
                if (bufTable[slot].bDirty)
                {
                    if ((rc = WritePage(fd, bufTable[slot].pageNum, bufTable[slot].pData)))
                        return (rc);
                    bufTable[slot].bDirty = FALSE;
                    // if((rc = InsertFree(slot)))return rc;
                }
                // if ((rc = Unlink(slot)) ||
                //     (rc = InsertFree(slot)))
                if ((rc = algorithm->InsertFree(slot, bufTable)))
                    return (rc);
            }
        }
        slot = next;
    }
    return (rcWarn);
    return (rcWarn);
}
RC PF_BufferMgr::ClearBuffer()
{
    RC rc;

    int slot, next;
    slot = first;
    while (slot != INVALID_SLOT)
    {
        next = bufTable[slot].next;
        if (bufTable[slot].pinCount == 0)
            // // if ((rc = Unlink(slot)) ||
            // //     (rc = InsertFree(slot)))
            if (rc = algorithm->InsertFree(slot, bufTable))
            if (rc = algorithm->InsertFree(slot, bufTable))
                return (rc);
        slot = next;
    }
    return 0;
}
RC PF_BufferMgr::InsertFree(int slot)
{
    bufTable[slot].next = free;
    free = slot;
    // Return ok
    return (0);
}
RC PF_BufferMgr::LinkHead(int slot)
{
    // Set next and prev pointers of slot entry
    bufTable[slot].next = first;
    bufTable[slot].prev = INVALID_SLOT;

    // If list isn't empty, point old first back to slot
    if (first != INVALID_SLOT)
        bufTable[first].prev = slot;

    first = slot;

    // if list was empty, set last to slot
    if (last == INVALID_SLOT)
        last = first;

    // Return ok
    return (0);
}
RC PF_BufferMgr::Unlink(int slot)
{
    // If slot is at head of list, set first to next element
    if (first == slot)
        first = bufTable[slot].next;

    // If slot is at end of list, set last to previous element
    if (last == slot)
        last = bufTable[slot].prev;

    // If slot not at end of list, point next back to previous
    if (bufTable[slot].next != INVALID_SLOT)
        bufTable[bufTable[slot].next].prev = bufTable[slot].prev;

    // If slot not at head of list, point prev forward to next
    if (bufTable[slot].prev != INVALID_SLOT)
        bufTable[bufTable[slot].prev].next = bufTable[slot].next;

    // Set next and prev pointers of slot entry
    bufTable[slot].prev = bufTable[slot].next = INVALID_SLOT;

    // Return ok
    return (0);
}
RC PF_BufferMgr::ForcePages(int fd, PageNum PageNum)
{


    RC rc;
    int slot = first;
    while (slot != INVALID_SLOT)
    {
        int next = bufTable[slot].next;
        if (bufTable[slot].fd == fd &&
            (PageNum == ALL_PAGES || bufTable[slot].pageNum == PageNum))
        {
            if (bufTable[slot].bDirty)
            {
                if ((rc = WritePage(fd, bufTable[slot].pageNum, bufTable[slot].pData)))
                    return (rc);
                bufTable[slot].bDirty = FALSE;
            }
        }
        slot = next;
    }
    return (0);
    return (0);
}
RC PF_BufferMgr::ReadPage(int fd, PageNum pageNum, char *dest)
{
    int offset = pageNum * (int)pageSize + PF_FILE_HDR_SIZE;
    if (lseek(fd, offset, L_SET) < 0)
        return (PF_UNIX);
    int numBytes = read(fd, dest, pageSize);
    PF_PageHdr *header = (PF_PageHdr *)dest;
    // if (pageNum < 50)
    //     cout << pageNum << " : " << header->freeCnt << " " << header->nextFree << endl;
    if (numBytes < 0)
        return (PF_UNIX);
    else if (numBytes != pageSize)
        return (PF_INCOMPLETEREAD);
    else
        return (0);
}
RC PF_BufferMgr::WritePage(int fd, PageNum pageNum, char *source)
{
    int offset = pageNum * (int)pageSize + PF_FILE_HDR_SIZE;
    if (lseek(fd, offset, L_SET) < 0)
    {
        return (PF_UNIX);
    }

    // PF_PageHdr *header = (PF_PageHdr *)source;
    // if (pageNum < 50)
    //     cout << pageNum << " : " << header->freeCnt << " " << header->nextFree << endl;
    int numBytes = write(fd, source, pageSize);
    if (numBytes < 0)
        return (PF_UNIX);
    else if (numBytes != pageSize)
        return (PF_INCOMPLETEWRITE);
    else
        return (0);
}
RC PF_BufferMgr::InternalAlloc(int &slot)
{
    RC rc;
    algorithm->SelectSlot(slot, bufTable);
    // Write out the page if it is dirty
    // cout << "dirty: " << bufTable[slot].bDirty << endl;
    if (bufTable[slot].bDirty)
    {
        if ((rc = WritePage(bufTable[slot].fd, bufTable[slot].pageNum,
                            bufTable[slot].pData)))
            return (rc);

        bufTable[slot].bDirty = FALSE;
    }
    if (algorithm->flag)
    {
        algorithm->LinkHead(slot, bufTable);
    }
    // Remove page from the hash table and slot from the used buffer list
    return (0);
}
RC PF_BufferMgr::AllocateBlock(char *&buffer)
{
    RC rc = OK_RC;

    // Get an empty slot from the buffer pool
    int slot;
    if ((rc = InternalAlloc(slot)) != OK_RC)
        return rc;

    // Create artificial page number (just needs to be unique for hash table)
    PageNum pageNum = long(bufTable[slot].pData);

    // Insert the page into the hash table, and initialize the page description entry
    if ((rc = InitPageDesc(MEMORY_FD, pageNum, slot)) != OK_RC)
    {
        // Put the slot back on the free list before returning the error
        // // Unlink(slot);
        // // InsertFree(slot);
        algorithm->InsertFree(slot, bufTable);
        algorithm->InsertFree(slot, bufTable);
        return rc;
    }

    // Return pointer to buffer
    buffer = bufTable[slot].pData;

    // Return success code
    return OK_RC;
}
RC PF_BufferMgr::DisposeBlock(char *buffer)
{
    return UnpinPage(MEMORY_FD, long(buffer));
}