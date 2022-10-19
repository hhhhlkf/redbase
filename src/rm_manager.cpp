#include "rm_manager.h"
RC RM_Manager::CreateFile(const char *fileName, int length)
{
    RC rc = 0;
    if (fileName == NULL)
        return (RM_BADFILENAME);
    if (rc = pfm.CreateFile(fileName, length))
    {
        return (rc);
    }
    // PF_PageHandle PHF;
    // PF_FileHandle FHF;
    // // RM_FileHeader *header;
    // PageNum page;
    // if ((rc = FHF.AllocatePage(PHF)) || (rc = PHF.GetPageNum(page)))
    //     return (rc);
    // char *pData;
    // rc = PHF.GetData(pData);
    // if (rc == 0)
    // {
    //     if ((rc = FHF.MarkDirty(page)) || (rc = FHF.UnpinPage(page)) || (rc = pfm.CloseFile(FHF)))
    //     {
    //         return (rc);
    //     }
    // }
    return (0);
}
RM_Manager::RM_Manager(PF_Manager &pfm) : pfm(pfm)
{
}
RM_Manager::~RM_Manager()
{
}
RC RM_Manager::DestroyFile(const char *fileName)
{
    if (fileName == NULL)
        return (RM_BADFILENAME);
    int rc;
    rc = pfm.DestroyFile(fileName);
    return (rc);
}
RC RM_Manager::OpenFile(const char *fileName, RM_FileHandle &fileHandle)
{
    if (fileName == NULL)
    {
        return (RM_BADFILENAME);
    }
    int rc;
    PF_FileHandle fh;
    if ((rc = pfm.OpenFile(fileName, fh)))
        return (rc);
    // cout << "I am Here" << endl;
    PF_PageHandle ph;
    PageNum page;
    PF_FileHdr *header = nullptr;
    header = fh.getHdr();
    // cout << header->firstFree<< endl;
    if ((rc = SetUpFH(fileHandle, fh, header)))
    {
        pfm.CloseFile(fh);
        return (rc);
    }
    return (0);
}
RC RM_Manager::SetUpFH(RM_FileHandle &fileHandle, PF_FileHandle &fh, PF_FileHdr *header)
{
    memcpy(&fileHandle.header, header, sizeof(PF_FileHdr));
    fileHandle.pfh = fh;
    fileHandle.header_modified = FALSE;
    fileHandle.openedFH = TRUE;
    //确保头文件可用
    return (0);
}
RC RM_Manager::CleanUpFH(RM_FileHandle &fileHandle)
{
    if (fileHandle.openedFH == false)
        return (RM_INVALIDFILEHANDLE);
    fileHandle.openedFH = false;
    return (0);
}
RC RM_Manager::CloseFile(RM_FileHandle &fileHandle)
{
    RC rc;
    PF_PageHandle ph;
    PageNum page;
    char *pData;
    // if (fileHandle.header_modified != false)
    // {
    //     if (rc = (fileHandle.pfh.GetFirstPage(ph) || ph.GetPageNum(page)))
    //     {
    //         return rc;
    //     }
    //     if ((rc = ph.GetData(pData)))
    //     {
    //         if ((rc = fileHandle.pfh.UnpinPage(page)))
    //         {
    //             return rc;
    //         }
    //         return rc;
    //     }
    //     if ((rc = (fileHandle.pfh.MarkDirty(page) ||
    //                fileHandle.pfh.UnpinPage(page))))
    //     {
    //         return rc;
    //     }
    // }
    if ((rc = pfm.CloseFile(fileHandle.pfh)))
        return (rc);
    if ((rc = CleanUpFH(fileHandle)))
        return (rc);

    return (0);
}
