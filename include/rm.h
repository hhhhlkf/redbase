#if !defined(RM_H)
#include "pf.h"
#include "redbase.h"
#define RM_H
struct RM_FileHeader
{
    int recordSize;        // record size in file
    int numRecordsPerPage; // calculated max # of recs per page
    int numPages;          // number of pages
    PageNum firstFreePage; // pointer to first free object
    int bitmapOffset;      // location in bytes of where the bitmap starts
                           // in the page headers
    int bitmapSize;        // size of bitmaps in the page headers
};
#define RM_INVALIDRID (START_RM_WARN + 0)          // invalid RID
#define RM_BADRECORDSIZE (START_RM_WARN + 1)       // record size is invalid
#define RM_INVALIDRECORD (START_RM_WARN + 2)       // invalid record
#define RM_INVALIDBITOPERATION (START_RM_WARN + 3) // invalid page header bit ops
#define RM_PAGEFULL (START_RM_WARN + 4)            // no more free slots on page
#define RM_INVALIDFILE (START_RM_WARN + 5)         // file is corrupt/not there
#define RM_INVALIDFILEHANDLE (START_RM_WARN + 6)   // filehandle is improperly set up
#define RM_INVALIDSCAN (START_RM_WARN + 7)         // scan is improperly set up
#define RM_ENDOFPAGE (START_RM_WARN + 8)           // end of a page
#define RM_EOF (START_RM_WARN + 9)                 // end of file
#define RM_BADFILENAME (START_RM_WARN + 10)

#define RM_ERROR (START_RM_ERR - 0) // error
#define RM_LASTERROR RM_ERROR

#define RM_BADRECSIZE (START_RM_WARN + 0) // rec size invalid <= 0
#define RM_NORECATRID (START_RM_WARN + 1) // This rid has no record

#define RM_LASTWARN RM_NORECATRID

#define RM_SIZETOOBIG (START_RM_ERR - 0) // record size too big
#define RM_PF (START_RM_ERR - 1)         // error in PF
#define RM_NULLRECORD (START_RM_ERR - 2)
#define RM_RECSIZEMISMATCH (START_RM_ERR - 3) // record size mismatch
#define RM_HANDLEOPEN (START_RM_ERR - 4)
#define RM_FCREATEFAIL (START_RM_ERR - 5)
#define RM_FNOTOPEN (START_RM_ERR - 6)
#define RM_BAD_RID (START_RM_ERR - 7)
// #define RM_EOF (START_RM_ERR - 8) // end of file

// #define RM_LASTERROR RM_EOF
#endif // RM_H
