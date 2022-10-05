
#if !defined(PF_MANAGER_H)
#define PF_MANAGER_H
#include "pf_filehandle.h"
#include "pf.h"
#include "pf_buffermgr.h"
class PF_Manager
{
private:
public:
    PF_Manager();
    ~PF_Manager();
    RC CreateFile(const char *fileName,int length);
    RC DestroyFile(const char *fileName);
    RC OpenFile(const char *fileName, PF_FileHandle &fileHandle);
    RC CloseFile(PF_FileHandle &fileHandle);
    RC ClearBuffer();
    RC AllocateBlock(char *&buffer);
    RC DisposeBlock(char *buffer);

private:
    PF_BufferMgr *pBufferMgr;
};

#endif // PF_MANAGER_H
