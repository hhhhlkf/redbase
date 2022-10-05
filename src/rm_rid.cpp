#include "rm_rid.h"

RC RID::GetPageNum(PageNum &pageNum) const
{
    pageNum = page;
    return 0;
}
RC RID::GetSlotNum(SlotNum &slotNum) const
{
    slotNum = slot;
    return 0;
}
RID::RID(PageNum pageNum, SlotNum slotNum)
{
    page = pageNum;
    slot = slotNum;
}
RID::RID()
{
    page = INVALID_PAGE; // 初始化指向不可用页或slot的RID
    slot = INVALID_SLOT;
}
RID &RID::operator=(const RID &rid)
{
    if (this != &rid)
    {
        this->page = rid.page;
        this->slot = rid.slot;
    }
    return (*this);
}
bool RID::operator==(const RID &rid) const
{
    return (this->page == rid.page && this->slot == rid.slot);
}
RC RID::isValidRID() const
{
    if (page > 0 && slot >= 0)
        return 0;
    else
        return RM_INVALIDRID;
}