#if !defined(RM_MANAGER_H)
#define RM_MANAGER_H
#include "rm.h"
// #include "rm_rid.h"
#include "redbase.h"
#include <iostream>
#include "pf_manager.h"
#include "rm_filehandle.h"
class RM_Manager
{
private:
    RC SetUpFH(RM_FileHandle &fileHandle, PF_FileHandle &fh, struct RM_FileHeader *header);

    PF_Manager &pfm;

public:
    RM_Manager(PF_Manager &pfm);
    ~RM_Manager();
    RC CreateFile(const char *fileName, int length);
    RC DestroyFile(const char *fileName);
    RC OpenFile(const char *fileName, RM_FileHandle &fileHandle);
    RC CloseFile(RM_FileHandle &fileHandle);
    RC SetUpFH(RM_FileHandle &fileHandle, PF_FileHandle &fh, PF_FileHdr *header);
    RC CleanUpFH(RM_FileHandle &fileHandle);
};

#endif // RM_MANAGER_H
