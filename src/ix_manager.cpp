#include "ix_manager.h"
//
// IX_Manager
//
// Desc: Constructor - intended to be called once at begin of program
//       Handles creation, deletion, opening and closing of files.
//
IX_Manager::IX_Manager(PF_Manager &pfm) : pfm(pfm)
{
}
IX_Manager::~IX_Manager()
{
}
//
// CreateIndex
//
// Desc: Create a new IX table/file named fileName
// In:   fileName - name of file to create
// Ret:  IX return code
//
RC IX_Manager::CreateIndex(const char *fileName, int indexNo,
                           AttrType attrType, int attrLength,
                           int length, int pageSize)
{
    if (indexNo < 0 ||
        attrType < INT ||
        attrType > STRING ||
        fileName == NULL)
        return IX_FCREATEFAIL;
    if (attrLength >= pageSize - (int)sizeof(RID) ||
        attrLength <= 0)
        return IX_INVALIDSIZE;
    if ((attrType == INT && (unsigned int)attrLength != sizeof(int)) ||
        (attrType == FLOAT && (unsigned int)attrLength != sizeof(float)) ||
        (attrType == STRING &&
         ((unsigned int)attrLength <= 0 ||
          (unsigned int)attrLength > MAXSTRINGLEN)))
        return IX_FCREATEFAIL;
    stringstream newname;
    newname << fileName << "." << indexNo;
    int RC = pfm.CreateFile(newname.str().c_str(), length);
    if (RC < 0)
    {
        // PF_PrintError(RC);
        return IX_PF;
    }
    PF_FileHandle pfh;
    RC = pfm.OpenFile(newname.str().c_str(), pfh);
    if (RC < 0)
    {
        // PF_PrintError(RC);
        return IX_PF;
    }
    PF_PageHandle headerPage;
    char *pData;
    RC = pfh.AllocatePage(headerPage);
    if (RC < 0)
    {
        // PF_PrintError(RC);
        return IX_PF;
    }
    RC = headerPage.GetData(pData);
    if (RC < 0)
    {
        // PF_PrintError(RC);
        return IX_PF;
    }
    IX_FileHdr hdr;
    hdr.numPages = 1; // header page
    hdr.pageSize = pageSize;
    hdr.pairSize = attrLength + sizeof(RID);
    hdr.order = -1;
    hdr.height = 0;
    hdr.rootPage = -1;
    hdr.attrType = attrType;
    hdr.attrLength = attrLength;
    memcpy(pData + sizeof(PF_PageHdr), &hdr, sizeof(hdr));
    PageNum headerPageNum;
    headerPage.GetPageNum(headerPageNum);
    if ((RC = pfh.MarkDirty(headerPageNum)) || (RC = pfh.UnpinPage(headerPageNum)) || (RC = pfm.CloseFile(pfh)))
    {
        return RC;
    }
    return (0);
}

RC IX_Manager::DestroyIndex(const char *fileName, int indexNo)
{
    if (indexNo < 0 ||
        fileName == NULL)
        return IX_FCREATEFAIL;
    stringstream newname;
    newname << fileName << "." << indexNo;
    RC RC = 0;
    if ((RC = pfm.DestroyFile(newname.str().c_str())))
    {
        // PF_PrintError(RC);
        return IX_PF;
    }
    return 0;
}

RC IX_Manager::OpenIndex(const char *fileName, int indexNo, IX_IndexHandle &ixh)
{
    if (indexNo < 0 ||
        fileName == NULL)
        return IX_FCREATEFAIL;

    PF_FileHandle pfh;
    stringstream newname;
    newname << fileName << "." << indexNo;

    RC rc = pfm.OpenFile(newname.str().c_str(), pfh);
    if (rc < 0)
    {
        // PF_PrintError(rc);
        return IX_PF;
    }
    // header page is at 0
    PF_PageHandle ph;
    char *pData;
    if ((rc = pfh.GetThisPage(0, ph)) ||
        (rc = ph.GetData(pData)))
        return (rc);
    IX_FileHdr hdr;
    memcpy(&hdr + sizeof(PF_PageHdr), pData, sizeof(hdr));
    rc = ixh.Open(&pfh, hdr.pairSize, hdr.rootPage, hdr.pageSize);
    if (rc < 0)
    {
        // IX_PrintError(rc);
        return rc;
    }
    rc = pfh.UnpinPage(0);
    if (rc < 0)
    {
        // PF_PrintError(rc);
        return rc;
    }

    return 0;
}

RC IX_Manager::CloseIndex(IX_IndexHandle &ixh)
{
    if (!ixh.bFileOpen || ixh.pfHandle == NULL)
        return IX_FNOTOPEN;
    if (ixh.HdrChanged())
    {
        PF_PageHandle ph;
        RC rc;
        if ((rc = ixh.pfHandle->GetThisPage(0, ph)))
        {
            return rc;
        }
        if ((rc = ixh.SetFileHeader(ph)))
        {
            return rc;
        }
        if ((rc = ixh.pfHandle->MarkDirty(0)))
        {
            return rc;
        }
        if ((rc = ixh.pfHandle->UnpinPage(0)))
        {
            return rc;
        }
        if ((rc = ixh.ForcePages()))
        {
            return rc;
        }
    }

    RC rc2 = pfm.CloseFile(*ixh.pfHandle);
    if (rc2 < 0)
    {
        return rc2;
    }
    ixh.~IX_IndexHandle();
    ixh.pfHandle = NULL;
    ixh.bFileOpen = false;
    return 0;
}