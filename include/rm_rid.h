#if !defined(RM_RID_H)
#define RM_RID_H
#include "redbase.h"
#include "rm.h"
typedef int PageNum;
typedef int SlotNum;
class RID
{
    static const PageNum INVALID_PAGE = -1;
    static const SlotNum INVALID_SLOT = -1;

private:
    PageNum page;
    SlotNum slot;

public:
    RID();
    RID(PageNum pageNum, SlotNum slotNum);
    ~RID();
    bool operator==(const RID &rid) const;
    RID &operator=(const RID &rid);
    RC GetPageNum(PageNum &pageNum) const; // Return page number
    RC GetSlotNum(SlotNum &slotNum) const; // Return slot number
    RC isValidRID() const;                 // checks if it is a valid RID
};
#endif // RM_RID_H
