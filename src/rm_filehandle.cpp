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

/*
    检测是否有删除后空余的slot，如果有则返回相应的页(ph接收)、可使用的槽的起始地址(addr接受)
*/
RC RM_FileHandle::InsertSlotCheck(int length, int &addr, PF_PageHandle &ph, int &currPage)
{
    RC rc;
    // 将addr赋值成-1，便于之后判断
    addr = -1;
    // 如果发现空闲页槽链为空
    if (header.FreeSlotPage == -1)
        return 0;
    else
    {
        // 获取当前第一位FreeSlotPage
        if ((rc = pfh.GetThisPage(header.FreeSlotPage, ph)))
            return rc;
        // 接下来涉及链表的删除操作，所以需要记录当前的块的PrePage
        int prePage = -1;
        int selectedPage = header.FreeSlotPage;
        while (1)
        {
            char *pData;
            // 通过ph获取当前块数据 ( 块头+数据 )
            if ((rc = ph.GetData(pData)))
                return rc;
            // 通过pData获取hdr
            PF_PageHdr *hdr = (PF_PageHdr *)pData;

            for (int i = 0; i < hdr->slotNum; i++)
            {
                // 通过块内扫描的形式进行空slot的查询
                // for (int i = sizeof(PF_PageHdr); i < 4096; i++)
                // {
                //     cout << short(pData[i]) << " ";
                // }
                // cout << endl;
                int *SlotHeader = (int *)(&pData[sizeof(PF_PageHdr) + i * 3 * sizeof(int)]);
                // 获取该slot的长度
                int *SlotLen = SlotHeader + 1;
                // 如果当前slot长度符合要求
                int *flag = SlotHeader + 2;
                if (*flag == 0 && *SlotLen >= length)
                {
                    addr = *SlotHeader;
                    currPage = header.FreeSlotPage;
                    // 如果该块的空闲slot号变为零，就将其从空闲页号中删除。
                    if (!(--hdr->emptySlotNum))
                    {
                        // 若为头部
                        if (prePage == -1)
                        {
                            header.FreeSlotPage = hdr->nextSlotPage;
                            hdr->nextSlotPage = -1;
                            pfh.setHdr(header);
                            // cout << "I am a header" << endl;
                        }
                        // 若不为头部
                        else
                        {
                            PF_PageHandle Prph;
                            if ((rc = pfh.GetThisPage(prePage, Prph)))
                                return rc;
                            if ((rc = Prph.GetData(pData)))
                                return rc;
                            // 获取前块的头部
                            PF_PageHdr *preHdr = (PF_PageHdr *)pData;
                            // 改变preHdr中的指针指向
                            preHdr->nextSlotPage = hdr->nextSlotPage;
                            // 令当前的nextSlotPage作为 -1
                            hdr->nextSlotPage = -1;
                            if ((rc = pfh.MarkDirty(prePage)) ||
                                (rc = pfh.UnpinPage(prePage)))
                                return rc;
                            // cout << "I am not a header" << endl;
                        }
                        *flag = 1;
                    }
                    if ((rc = pfh.MarkDirty(currPage)) ||
                        (rc = pfh.UnpinPage(currPage)))
                        return rc;
                    return (0);
                }
            }
            // 将selectPage解除Pin
            if ((rc = pfh.UnpinPage(selectedPage)))
                return rc;
            prePage = selectedPage;
            selectedPage = hdr->nextSlotPage;
            if (selectedPage == -1)
                return 0;
            // 下一循环，获取下一个Page
            if ((rc = pfh.GetThisPage(header.FreeSlotPage, ph)))
                return rc;
        }
    }
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
    int currAddr = 0;
    RC rc;
    PF_PageHandle ph;
    char *pDa = nullptr;
    PF_PageHdr *pf_Hdr = nullptr;
    int currPage = -1;
    if ((rc = InsertSlotCheck(length, currAddr, ph, currPage)))
        return rc;
    if (currAddr != -1)
    {
        ph.GetData(pDa);
        pf_Hdr = (PF_PageHdr *)pDa;
    }
    else
    {
        currPage = header.firstFree;
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
            pfh.setHdr(header);
        }
        // 将currPage设置为脏
        ph.GetPageNum(currPage);
        pfh.MarkDirty(currPage);
        ph.GetData(pDa);
        pf_Hdr = (PF_PageHdr *)pDa;
        // cout << sizeof(PF_PageHdr);
        // 如果页满或者页的剩余空间小于当前记录长度
        if (pf_Hdr->full ||
            pf_Hdr->freeCnt < (length + sizeof(int) * 3))
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
            // PF_FileHdr *hdr;
            // hdr = pfh.getHdr();
            ph.GetData(pDa);
            pf_Hdr = (PF_PageHdr *)pDa;
            // cout << "pf_Hdr->slotNum: " << pf_Hdr->slotNum << endl;
        }

        if (pf_Hdr->slotNum != 0)
        {
            // 确定当前页中上一条记录的标识位置
            // cout << "i:" << pf_Hdr->slotNum << endl;
            char *curr = &pDa[sizeof(int) * 3 * (pf_Hdr->slotNum - 1) + sizeof(PF_PageHdr)];
            // 强制类型转换成int类型，方便后文取值
            int *beginAddr = (int *)curr;
            // cout << "beginAddr: " << *beginAddr << endl;
            // 将currAddr计算出来
            currAddr = *beginAddr - length;
            // cout << "currAddr: " << currAddr << endl;
            // 将地址指针转移到下一个标识应填写的位置
            curr = curr + sizeof(int) * 3;
            int *pos = (int *)curr;
            //将即将存入的数据的头地址写入pos
            *pos = currAddr;
            curr += sizeof(int);
            pos = (int *)curr;
            //将当前即将存入的数据的长度写入pos+sizeof(int)
            *pos = length;
            curr += sizeof(int);
            pos = (int *)curr;
            *pos = 1;
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
            curr += sizeof(int);
            pos = (int *)curr;
            *pos = 1;
        }
        pf_Hdr->slotNum++;
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
    // strcpy(&pDa[currAddr], pData);

    pf_Hdr->freeCnt -= (sizeof(int) * 3 + length);
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
    char *pos = &pData[sizeof(PF_PageHdr) + sizeof(int) * s * 3];
    int *slotHeader = (int *)pos;
    int *slotLen = (int *)(pos + sizeof(int));
    int *flag = (int *)(pos + sizeof(int) * 2);
    if (*flag != 0)
    {
        rec.Set(&pData[*slotHeader], *slotLen, rid);
    }
    return (0);
}

RC RM_FileHandle::DeleteRec(const RID &rid)
{
    PageNum p;
    SlotNum s;
    rid.GetPageNum(p);
    rid.GetSlotNum(s);
    // 判断是否numPages越界
    if ((p < 0) || (p > header.numPages) || (s < 0))
    {
        return RM_BAD_RID;
    }

    RC rc = 0;
    PF_PageHandle ph;
    if ((rc = pfh.GetThisPage(p, ph)))
    {
        return rc;
    }

    char *pData;
    ph.GetData(pData);
    PF_PageHdr *hdr = (PF_PageHdr *)pData;
    // cout << "PageNextFree is :" << hdr->nextFree << endl;
    // 获取当前要删除的slot的方位
    char *pos = &pData[sizeof(PF_PageHdr) + sizeof(int) * s * 3];
    int *slotHeader = (int *)pos;
    int *flag = (int *)(pos + sizeof(int) * 2);
    // 判断是否该RID所指示位置有数据
    if (*slotHeader == 0)
    {
        if ((rc = pfh.MarkDirty(p)) || (rc = pfh.UnpinPage(p)))
            return rc;
        return RM_BAD_RID;
    }
    *flag = 0;
    // 如果当前emptySlotNum为0，则说明该块并没有在FreeSlotList中记录，需要将其添加进来，采用头插法
    if (hdr->emptySlotNum == 0)
    {
        hdr->emptySlotNum++;
        hdr->nextSlotPage = header.FreeSlotPage;
        header.FreeSlotPage = (int)p;
        // hdr->slotNum--;
        pfh.setHdr(header);
    }
    // 如果当前只存在一个非空闲slot，则删除后该块将回归空闲页链
    else if (hdr->emptySlotNum != 0 && hdr->slotNum == 1)
    {
        rc = RemoveDeleteBlock(p);
        if (rc != 0)
            return rc;
        hdr->freeCnt = PF_PAGE_SIZE;
        hdr->nextSlotPage = -1;
        hdr->emptySlotNum = 0;
        hdr->slotNum = 0;
    }
    // 当该块存在多个slot且依旧存在多个slot被占用，则执行以下代码
    else
    {
        hdr->emptySlotNum++;
        // hdr->slotNum--;
    }
    if ((rc = pfh.MarkDirty(p)) || (rc = pfh.UnpinPage(p)))
        return rc;
    // cout << "PageNextFree is :" << hdr->nextFree << endl;
    return 0;
}
/*
将对应页块id的从FreeSlotList里面删除，并添加到FreeBlockList里面
*/
RC RM_FileHandle::RemoveDeleteBlock(int pageID)
{
    RC rc;
    int currPage = header.FreeSlotPage;
    int prePage = -1;
    // 涉及到链表的删除操作
    PF_PageHdr *hdr = nullptr;
    PF_PageHdr *preHdr = nullptr;
    PF_PageHandle ph;
    char *pData;

    if ((rc = pfh.GetThisPage(currPage, ph)) || (rc = ph.GetData(pData)))
        return (0);
    hdr = (PF_PageHdr *)pData;
    while (currPage != -1)
    {
        if (hdr->PageID == pageID)
        {
            if (prePage != -1)
            {
                preHdr->nextSlotPage = hdr->nextSlotPage;
                if ((rc = pfh.MarkDirty(prePage)) || (rc = pfh.MarkDirty(currPage)))
                    return rc;
                if ((rc = pfh.UnpinPage(prePage)) || (rc = pfh.UnpinPage(currPage)))
                    return (rc);
                return (0);
            }
            else
            {

                header.FreeSlotPage = hdr->nextSlotPage;
                pfh.setHdr(header);
                if ((rc = pfh.UnpinPage(currPage)))
                    return rc;
                return (0);
            }
        }
        if (prePage != -1)
        {
            if ((rc = pfh.UnpinPage(prePage)))
                return (rc);
        }
        prePage = currPage;
        currPage = hdr->nextSlotPage;
        preHdr = hdr;
        if ((rc = pfh.GetThisPage(currPage, ph)) || (rc = ph.GetData(pData)))
            return (0);
        hdr = (PF_PageHdr *)pData;
    }
    if ((rc = pfh.UnpinPage(prePage)))
        return rc;
    return 0;
}