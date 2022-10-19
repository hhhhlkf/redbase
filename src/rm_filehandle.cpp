#include "rm_filehandle.h"
#include <string.h>
#define DISK_EXCEEDED -1;
#define PAGE_EXCEEDED -2;
RM_FileHandle::RM_FileHandle()
{
    header_modified = FALSE;
    openedFH = FALSE;
}

RM_FileHandle::~RM_FileHandle()
{
    openedFH = false; // disassociate from fileHandle from an open file
}
RC RM_FileHandle::InsertRec(const char *pData, int length)
{
    /*
    存储格式:
    块
        header
        记录标识链
        反向存储的记录
    */

    RC rc;
    int currPage = header.firstFree;
    // cout << header.firstFree << endl;
    if (currPage == PF_PAGE_LIST_END)
    {
        return DISK_EXCEEDED;
    }
    if (PF_PAGE_SIZE < length)
        return PAGE_EXCEEDED;
    //通过ph获取数据和pageNum
    PF_PageHandle ph;
    if ((rc = pfh.GetThisPage(currPage, ph)))
    {
        if ((rc = pfh.AllocatePage(ph)))
            return (rc);
        ph.GetPageNum(currPage);
        PF_FileHdr *hdr;
        hdr = pfh.getHdr();
        header.firstFree = currPage;
        header.numPages = hdr->numPages;
        header_modified = true;
    }
    // 将currPage设置为脏
    ph.GetPageNum(currPage);
    pfh.MarkDirty(currPage);
    char *pDa;
    ph.GetData(pDa);
    PF_PageHdr *pf_Hdr = (PF_PageHdr *)pDa;
    // cout << sizeof(PF_PageHdr);
    // 如果页没满或者页的剩余空间小于当前记录长度
    if (pf_Hdr->full || pf_Hdr->freeCnt < (length + sizeof(int) * 2))
    {
        pfh.UnpinPage(currPage);
        if ((rc = pfh.AllocatePage(ph)))
        {
            return (rc);
        }
        ph.GetPageNum(currPage);
        // cout << "full is currPage:" << currPage << endl;
        header.firstFree = currPage;
        pfh.MarkDirty(currPage);
        PF_FileHdr *hdr;
        hdr = pfh.getHdr();
        header.numPages = hdr->numPages;
        header_modified = true;
        ph.GetData(pDa);
        pf_Hdr = (PF_PageHdr *)pDa;
        // cout << "pf_Hdr->slotNum: " << pf_Hdr->slotNum << endl;
    }
    int currAddr = 0;

    if (pf_Hdr->slotNum != 0)
    {
        // 确定当前页中上一条记录的标识位置
        // cout << "i:" << pf_Hdr->slotNum << endl;
        char *curr = &pDa[sizeof(int) * 2 * (pf_Hdr->slotNum - 1) + sizeof(PF_PageHdr)];
        // 强制类型转换成int类型，方便后文取值
        int *beginAddr = (int *)curr;
        // cout << "beginAddr: " << *beginAddr << endl;
        // 将currAddr计算出来
        currAddr = *beginAddr - length;
        // cout << "currAddr: " << currAddr << endl;
        // 将地址指针转移到下一个标识应填写的位置
        curr = curr + sizeof(int) * 2;
        int *pos = (int *)curr;
        //将即将存入的数据的头地址写入pos
        *pos = currAddr;
        curr += sizeof(int);
        pos = (int *)curr;
        //将当前即将存入的数据的长度写入pos+sizeof(int)
        *pos = length;
    }
    else
    {
        char *curr = &pDa[sizeof(PF_PageHdr)];
        // 同理
        int *pos = (int *)curr;
        //不存在前一条块内记录，因此可以放心存储
        currAddr = 4096 - length;
        // cout << "currAddr: " << currAddr << endl;
        *pos = currAddr;
        curr += sizeof(int);
        pos = (int *)curr;
        *pos = length;
    }
    memcpy(&pDa[currAddr], pData, length);
    // if (pf_Hdr->slotNum <= 5 && currPage == 0)
    // {
    //     for (int i = sizeof(PF_PageHdr); i < 4096; i++)
    //     {
    //         cout << short(pDa[i]) << " ";
    //     }
    //     cout << endl;
    // }
    strcpy(&pDa[currAddr], pData);
    pf_Hdr->slotNum++;
    pf_Hdr->freeCnt -= (sizeof(int) * 2 + length);
    pfh.UnpinPage(currPage);
    if ((pf_Hdr->freeCnt / 4096.0) < 0.2)
    {
        pf_Hdr->full = TRUE;
    }
    return (0);
}
RC RM_FileHandle::GetRec(const RID &rid, RM_Record &rec) const
{
    PageNum p;
    SlotNum s;
    rid.GetPageNum(p);
    rid.GetSlotNum(s);
    if ((p < 0) || (p > header.numPages) || (s < 0))
    {
        return RM_BAD_RID;
    }
    RC rc = 0;
    PF_PageHandle ph;
    if ((rc = pfh.GetThisPage(p, ph)) || (rc = pfh.UnpinPage(p)))
    {
        return rc;
    }
    char *pData;
    ph.GetData(pData);
    PF_PageHdr *hdr = (PF_PageHdr *)pData;
    char *pos = &pData[sizeof(PF_PageHdr) + sizeof(int) * s * 2];
    int *slotHeader = (int *)pos;
    int *slotLen = (int *)(pos + sizeof(int));
    rec.Set(&pData[*slotHeader], *slotLen, rid);
    return (0);
}