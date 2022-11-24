#include "pf_algomgr.h"
RC PF_Clock::LinkHead(int slot, PF_BufPageDesc *&bufTable)
{
    return (0);
}

RC PF_Clock::InsertFree(int slot, PF_BufPageDesc *&bufTable)
{
    return (0);
}

RC PF_Clock::SelectSlot(int &slot, PF_BufPageDesc *&bufTable)
{
    for (int i = 0; i < bufferSize * 2; i++)
    {
        if (!bufTable[currentP].bDirty && !bufTable[currentP].pinCount)
        {
            slot = currentP++;
            cout << "block num is: " << slot << endl;
            currentP %= bufferSize;
            return (0);
        }
        else if (bufTable[currentP].pinCount)
        {
            bufTable[currentP++].pinCount = 0;
            currentP %= bufferSize;
        }
    }
    for (int i = 0; i < bufferSize; i++)
    {
        if (!bufTable[currentP].pinCount)
        {
            slot = currentP++;
            cout << "block num is: " << slot << endl;
            currentP %= bufferSize;
            return (0);
        }
    }
    flag = false;
    return (0);
}

PF_Clock::PF_Clock(int size)
{
    bufferSize = size;
    currentP = 0;
}

PF_Clock::~PF_Clock()
{
}

PF_Lru::PF_Lru(int &first, int &last, int &free)
{
    this->first = &first;
    this->last = &last;
    this->free = &free;
}

PF_Lru::~PF_Lru()
{
}

RC PF_Lru::LinkHead(int slot, PF_BufPageDesc *&bufTable)
{
    RC rc;
    if ((rc = Unlink(slot, bufTable)))
    {
        return (rc);
    }
    bufTable[slot].next = *first;
    bufTable[slot].prev = INVALID_SLOT;

    // If list isn't empty, point old first back to slot
    if (*first != INVALID_SLOT)
        bufTable[*first].prev = slot;

    *first = slot;

    // if list was empty, set last to slot
    if (*last == INVALID_SLOT)
        *last = *first;

    // Return ok
    return (0);
}

RC PF_Lru::InsertFree(int slot, PF_BufPageDesc *&bufTable)
{
    RC rc;
    if ((rc = Unlink(slot, bufTable)))
    {
        return (rc);
    }
    bufTable[slot].next = *free;
    *free = slot;
    // Return ok
    return (0);
}

RC PF_Lru::SelectSlot(int &slot, PF_BufPageDesc *&bufTable)
{
    RC rc;
    if (*free != INVALID_SLOT)
    {
        slot = *free;
        *free = bufTable[slot].next;
        cout << "block num is: " << slot << endl;
        // cout << "free = bufTable[slot].next;" << endl;
        flag = false;
        return (0);
    }
    else
    {
        for (slot = *last; slot != INVALID_SLOT; slot = bufTable[slot].prev)
        {
            if (bufTable[slot].pinCount == 0)
            {
                cout << "block num is: " << slot << endl;
                break;
            }
        }
        if (slot == INVALID_SLOT)
            return (PF_NOBUF);
        flag = true;
    }
    return (0);
}

RC PF_Lru::Unlink(int slot, PF_BufPageDesc *&bufTable)
{
    // If slot is at head of list, set first to next element
    if (*first == slot)
        *first = bufTable[slot].next;

    // If slot is at end of list, set last to previous element
    if (*last == slot)
        *last = bufTable[slot].prev;

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
PF_AlgoMgr::PF_AlgoMgr()
{
}
PF_AlgoMgr::~PF_AlgoMgr()
{
}