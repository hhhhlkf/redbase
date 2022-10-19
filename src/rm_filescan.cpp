#include "rm_filescan.h"
#include <cassert>
RM_FileScan::RM_FileScan(/* args */) : bOpen(false)
{
    pred = NULL;
    prmh = NULL;
    current = RID(0, -1);
}

RM_FileScan::~RM_FileScan()
{
}

RC RM_FileScan::OpenScan(const RM_FileHandle &fileHandle,
                         AttrType attrType,
                         int attrLength,
                         int attrOffset,
                         CompOp compOp,
                         void *value,
                         ClientHint pinHint)
{
    if (bOpen)
    {
        return RM_HANDLEOPEN;
    }
    prmh = const_cast<RM_FileHandle *>(&fileHandle);
    if ((prmh == NULL))
        return RM_FCREATEFAIL;

    if (value != NULL)
    {
        if ((compOp < NO_OP) ||
            (compOp > GE_OP))
            return RM_FCREATEFAIL;
        if ((attrType < INT) ||
            (attrType > STRING))
            return RM_FCREATEFAIL;
        if (attrLength >= PF_PAGE_SIZE - (int)sizeof(RID) ||
            attrLength <= 0)
            return RM_RECSIZEMISMATCH;
        if ((attrType == INT && (unsigned int)attrLength != sizeof(int)) ||
            (attrType == FLOAT && (unsigned int)attrLength != sizeof(float)) ||
            (attrType == STRING &&
             ((unsigned int)attrLength <= 0 ||
              (unsigned int)attrLength > MAXSTRINGLEN)))
            return RM_FCREATEFAIL;
        if (attrOffset < 0)
            return RM_FCREATEFAIL;
    }
    bOpen = true;
    pred = new Predicate(attrType,
                         attrLength,
                         attrOffset,
                         compOp,
                         value,
                         pinHint);
    return 0;
}

RC RM_FileScan::GetNextRec(RM_Record &rec)
{
    if (!bOpen)
        return RM_FNOTOPEN;
    PF_PageHandle ph;
    RC rc;
    PF_PageHdr *hdr;
    // current 记录了当前读取到的位置
    for (int j = current.Page(); j < prmh->header.numPages; j++)
    {
        if ((rc = prmh->pfh.GetThisPage(j, ph))
            // Needs to be called everytime GetThisPage is called.
            || (rc = prmh->pfh.UnpinPage(j)))
            return rc;
        char *pData;
        ph.GetData(pData);
        hdr = (PF_PageHdr *)pData;
        int i = -1;
        // 表明是旧页，从cuurrent.slot()+1进行判断查找
        if (current.Page() == j)
            i = current.Slot() + 1;
        // 表明是新页
        else
            i = 0;
        for (; i < hdr->slotNum; i++)
        {
            char *pos = &pData[sizeof(PF_PageHdr) + sizeof(int) * 2 * i];
            int *begin = (int *)pos;
            if (*begin != -1)
            {
                current = RID(j, i);
                // 获取当前RID所对应的记录
                prmh->GetRec(current, rec);
                char *constData = NULL;
                int length = 0;
                rec.GetData(constData, length);
                if (pred->eval(constData, pred->initOp()))
                {
                    return 0;
                }
            }
        }
    }
    return RM_EOF;
}
