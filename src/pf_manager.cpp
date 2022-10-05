#include "pf_manager.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
PF_Manager::PF_Manager()
{
    pBufferMgr = new PF_BufferMgr(PF_BUFFER_SIZE);
}
PF_Manager::~PF_Manager()
{
}
RC PF_Manager::CreateFile(const char *fileName, int length)
{
    int fd;
    int numBytes;
    // O_CREAT 如果文件不存在则创建该文件
    // O_EXCL 如果使用O_CREAT选项且文件存在，则返回错误消息
    // O_NOCTTY 如果文件为终端，那么终端不可以调用open系统调用的那个进程的控制终端
    // O_TRUNC 如果文件已经存在泽删除文件中原有数据
    // O_APPEND 以追加的方式打开
    if ((fd = open(fileName, O_CREAT | O_EXCL | O_WRONLY,
                   CREATION_MASK)) < 0)
        return (PF_UNIX);
    int reback = 0;
    if ((reback = fallocate(fd, 0, 0, 1 << length)) < 0)
    // cout << "here I am" << endl;
    {
        return (PF_UNIX);
    }
    // cout << "here I am" << endl;
    char hdrBuf[PF_FILE_HDR_SIZE];
    memset(hdrBuf, 0, PF_FILE_HDR_SIZE);
    PF_FileHdr *hdr = (PF_FileHdr *)hdrBuf;
    hdr->firstFree = 0;
    hdr->numPages = (1 << length) / 4096 - 1;
    // cout << "here I am" << endl;
    if ((numBytes = write(fd, hdrBuf, PF_FILE_HDR_SIZE)) != PF_FILE_HDR_SIZE)
    {
        // Error while writing: close and remove file
        // cout << "wrong !!!!!" << endl;
        close(fd);
        unlink(fileName);
        // Return an error
        if (numBytes < 0)
            return (PF_UNIX);
        else
            return (PF_HDRWRITE);
    }
    char *pbuffer = nullptr;
    RC rc;
    // cout << hdr->numPages << endl;
    for (long i = 0; i <= hdr->numPages - 1; i++)
    {
        if ((rc = pBufferMgr->AllocatePage(fd, i, &pbuffer)))
            return (rc);
        memset(pbuffer, 0, sizeof(pbuffer));
        PF_PageHdr *pf_pageHdr = (PF_PageHdr *)pbuffer;
        pf_pageHdr->full = FALSE;
        if (i != hdr->numPages)
            pf_pageHdr->nextFree = i + 1;
        else
            pf_pageHdr->nextFree = PF_PAGE_LIST_END;
        pf_pageHdr->slotNum = 0;
        pf_pageHdr->freeCnt = PF_PAGE_SIZE;
        pf_pageHdr->pageId = 0 | (fd << 24) | (i);
        // cout << "pbuffer is:";
        // for (int i = 0; i < 100; i++)
        // {
        //     cout << u_short(*pbuffer)<<" ";
        //     pbuffer++;
        // }
        // cout << endl;
        pBufferMgr->UnpinPage(fd, i);
        pBufferMgr->MarkDirty(fd, i);
    }
    // cout << "ok!" << endl;
    pBufferMgr->FlushPages(fd);
    if (close(fd) < 0)
        return (PF_UNIX);
    return (0);
}
RC PF_Manager::DestroyFile(const char *fileName)
{
    if (unlink(fileName) < 0)
        return (PF_UNIX);
    // Return ok
    return (0);
}
RC PF_Manager::OpenFile(const char *fileName, PF_FileHandle &fileHandle)
{
    int rc;
    if (fileHandle.bFileOpen)
        return (PF_FILEOPEN);
    if ((fileHandle.unixfd = open(fileName, O_RDWR)) < 0)
        return (PF_UNIX);
    int numBytes = read(fileHandle.unixfd, (char *)&fileHandle.hdr,
                        sizeof(PF_FileHdr));
    if (numBytes != sizeof(PF_FileHdr))
    {
        rc = (numBytes < 0) ? PF_UNIX : PF_HDRREAD;
        close(fileHandle.unixfd);
        fileHandle.bFileOpen = TRUE;
        return (rc);
    }
    fileHandle.bHdrChanged = FALSE;
    fileHandle.pBufferMgr = pBufferMgr;
    fileHandle.bFileOpen = TRUE;
    return (0);
}
RC PF_Manager::CloseFile(PF_FileHandle &fileHandle)
{
    RC rc;

    // Ensure fileHandle refers to open file
    if (!fileHandle.bFileOpen)
        return (PF_CLOSEDFILE);

    // Flush all buffers for this file and write out the header
    if ((rc = fileHandle.FlushPages()))
        return (rc);

    // Close the file
    if (close(fileHandle.unixfd) < 0)
        return (PF_UNIX);
    fileHandle.bFileOpen = FALSE;

    // Reset the buffer manager pointer in the file handle
    fileHandle.pBufferMgr = NULL;

    // Return ok
    return 0;
}
RC PF_Manager::ClearBuffer()
{
    return pBufferMgr->ClearBuffer();
}
RC PF_Manager::AllocateBlock(char *&buffer)
{
    return pBufferMgr->AllocateBlock(buffer);
}
RC PF_Manager::DisposeBlock(char *buffer)
{
    return pBufferMgr->DisposeBlock(buffer);
}