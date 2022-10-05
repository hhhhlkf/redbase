#if !defined(PF_PAGEHANDLE_H)
#define PF_PAGEHANDLE_H
#include "pf.h"
class PF_PageHandle
{
    friend class PF_FileHandle;

private:
    int pageNum;     // page number
    char *pPageData; // pointer to page data
public:
    PF_PageHandle(/* args */);
    ~PF_PageHandle();
    PF_PageHandle(const PF_PageHandle &pageHandle);
    RC GetData(char *&pData) const;
    RC GetPageNum(PageNum &pageNum) const;
    PF_PageHandle& operator=(const PF_PageHandle &pageHandle);
};

#endif // PF_PAGEHANDLE_H
