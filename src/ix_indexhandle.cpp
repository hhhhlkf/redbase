#include "ix_indexhandle.h"
IX_IndexHandle::IX_IndexHandle()
    : bFileOpen(false), pfHandle(NULL), bHdrChanged(false)
{
  root = NULL;
  path = NULL;
  pathP = NULL;
  treeLargest = NULL;
  hdr.height = 0;
}
// 重置IX_IndexHandle的各项参数
IX_IndexHandle::~IX_IndexHandle()
{
  if (root != NULL && pfHandle != NULL)
  {
    // unpin old root page
    pfHandle->UnpinPage(hdr.rootPage);
    delete root;
    root = NULL;
  }
  if (pathP != NULL)
  {
    delete[] pathP;
    pathP = NULL;
  }
  if (path != NULL)
  {
    for (int i = 1; i < hdr.height; i++)
      if (path[i] != NULL)
      {
        if (pfHandle != NULL)
          pfHandle->UnpinPage(path[i]->GetPageRID().Page());
      }
    delete[] path;
    path = NULL;
  }
  if (pfHandle != NULL)
  {
    delete pfHandle;
    pfHandle = NULL;
  }
  if (treeLargest != NULL)
  {
    delete[](char *) treeLargest;
    treeLargest = NULL;
  }
}

RC IX_IndexHandle::InsertEntry(void *pData, const RID &rid)
{
  RC invalid = IsValid();
  if (invalid)
    return invalid;
  if (pData == NULL)
    return IX_BADKEY;
  bool newLargest = false;
  void *prevKey = NULL;
  int level = hdr.height - 1;
  // 查询是否有相同的索引值
  BtreeNode *node = FindLeaf(pData);
  BtreeNode *newNode = NULL;
  int pos2 = node->FindKey((const void *&)pData, rid);
  if ((pos2 != -1))
    return IX_ENTRYEXISTS;
  if (node->GetNumKeys() == 0 ||
      node->CmpKey(pData, treeLargest) > 0)
  {
    newLargest = true;
    prevKey = treeLargest;
  }
  int result = node->Insert(pData, rid);
  // 将每个块的largest作为标识点
  if (newLargest)
  {
    for (int i = 0; i < hdr.height - 1; i++)
    {
      int pos = path[i]->FindKey((const void *&)prevKey);
      if (pos != -1)
        path[i]->SetKey(pos, pData);
      else
      {
        // assert(false); //largest key should be everywhere
        // return IX_BADKEY;
        cerr << "Fishy intermediate node ?" << endl;
      }
    }
    // copy from pData into new treeLargest
    memcpy(treeLargest,
           pData,
           hdr.attrLength);
    // cerr << "new treeLargest " << *(int*)treeLargest << endl;
  }
  void *failedKey = pData;
  RID failedRid = rid;
  // 循环直到有位置了再插入
  while (result == -1)
  {
    char *charPtr = new char[hdr.attrLength];
    void *oldLargest = charPtr;
    if (node->LargestKey() == NULL)
      oldLargest = NULL;
    else
      node->CopyKey(node->GetNumKeys() - 1, oldLargest);
    delete[] charPtr;
    // make new  node
    PF_PageHandle ph;
    PageNum p;
    RC rc = GetNewPage(p);
    if (rc != 0)
      return rc;
    rc = GetThisPage(p, ph);
    if (rc != 0)
      return rc;
    newNode = new BtreeNode(hdr.attrType, hdr.attrLength,
                            ph, true,
                            hdr.pageSize);
    rc = node->Split(newNode);
    if (rc != 0)
      return IX_PF;
    // split adjustment
    BtreeNode *currRight = FetchNode(newNode->GetRight());
    if (currRight != NULL)
    {
      currRight->SetLeft(newNode->GetPageRID().Page());
      delete currRight;
    }
    if (node->CmpKey(pData, node->LargestKey()) >= 0)
    {
      newNode->Insert(failedKey, failedRid);
    }
    else
    {
      node->Insert(failedKey, failedRid);
    }
    level--;
    if (level < 0)
      break; // root !
    int posAtParent = pathP[level];
    BtreeNode *parent = path[level];
    parent->Remove(NULL, posAtParent);
    result = parent->Insert((const void *)node->LargestKey(),
                            node->GetPageRID());
    result = parent->Insert(newNode->LargestKey(), newNode->GetPageRID());
    node = parent;
    failedKey = newNode->LargestKey(); // failure cannot be in node -
    failedRid = newNode->GetPageRID();
    delete newNode;
    newNode = NULL;
  }
  if (level >= 0)
  {
    // insertion done
    return 0;
  }
  else
  {
    RC rc = pfHandle->UnpinPage(hdr.rootPage);
    if (rc < 0)
      return rc;
    PF_PageHandle ph;
    PageNum p;
    rc = GetNewPage(p);
    if (rc != 0)
      return IX_PF;
    rc = GetThisPage(p, ph);
    if (rc != 0)
      return IX_PF;

    root = new BtreeNode(hdr.attrType, hdr.attrLength,
                         ph, true,
                         hdr.pageSize);
    root->Insert(node->LargestKey(), node->GetPageRID());
    root->Insert(newNode->LargestKey(), newNode->GetPageRID());

    // pin root page - should always be valid
    hdr.rootPage = root->GetPageRID().Page();
    PF_PageHandle rootph;
    rc = pfHandle->GetThisPage(hdr.rootPage, rootph);
    if (rc != 0)
      return rc;
    if (newNode != NULL)
    {
      delete newNode;
      newNode = NULL;
    }
    SetHeight(hdr.height + 1);
    return 0;
  }
}
RC IX_IndexHandle::DeleteEntry(void *pData, const RID &rid)
{
  RC rc;
  if (pData == NULL)
    return IX_BADKEY;
  if ((rc = IsValid()))
    return rc;
  bool nodeLargest = false;
  BtreeNode *node = FindLeaf(pData);
  assert(node != NULL);
  int pos = node->FindKey((const void *&)pData, rid);
  if (pos == -1)
  {
    int p = node->FindKey((const void *&)pData, RID(-1, -1));
    if (p != -1)
    {
      // 向左进行顺序查找(不局限于当前结点)-->替代了桶结构
      BtreeNode *other = DupScanLeftFind(node, pData, rid);
      if (other != NULL)
      {
        int pos = other->FindKey((const void *&)pData, rid);
        other->Remove(pData, pos); // ignore result - not dealing with
                                   // underflow here
        return 0;
      }
    }
    return IX_NOSUCHENTRY;
  }
  else if (pos == node->GetNumKeys() - 1)
    nodeLargest = true;
  // 将想要删除的key从内节点中删除
  if (nodeLargest)
  {
    for (int i = hdr.height - 2; i >= 0; i--)
    {
      int pos = path[i]->FindKey((const void *&)pData);

      if (pos != -1)
      {
        void *leftKey = NULL;
        leftKey = path[i + 1]->LargestKey();
        if (node->CmpKey(pData, leftKey) == 0)
        {
          int pos = path[i + 1]->GetNumKeys() - 2;
          if (pos < 0)
          {
            continue; // underflow will happen
          }
          path[i + 1]->GetKey(path[i + 1]->GetNumKeys() - 2, leftKey);
        }
        if ((i == hdr.height - 2) ||           // leaf's parent level
            (pos == path[i]->GetNumKeys() - 1) // largest at
                                               // intermediate node too
        )
          path[i]->SetKey(pos, leftKey);
      }
    }
  }
  // 将想要删除的key从叶子结点中抹除
  int result = node->Remove(pData, pos);

  int level = hdr.height - 1;
  // 将删除后出现的空node以及其内节点的记录删除
  while (result == -1)
  {
    level--;
    if (level < 0)
      break;
    int posAtParent = pathP[level];
    BtreeNode *parent = path[level];
    result = parent->Remove(NULL, posAtParent);
    if (level == 0 && parent->GetNumKeys() == 1 && result == 0)
      result = -1;
    BtreeNode *left = FetchNode(node->GetLeft());
    BtreeNode *right = FetchNode(node->GetRight());

    if (left != NULL)
    {
      if (right != NULL)
        left->SetRight(right->GetPageRID().Page());
      else
        left->SetRight(-1);
    }
    if (right != NULL)
    {
      if (left != NULL)
        right->SetLeft(left->GetPageRID().Page());
      else
        right->SetLeft(-1);
    }
    if (right != NULL)
      delete right;
    if (left != NULL)
      delete left;
    node->Destroy();
    RC rc = DisposePage(node->GetPageRID().Page());
    if (rc < 0)
      return IX_PF;

    node = parent;
  }
  if (level >= 0)
  {
    // deletion done
    return 0;
  }
  else
  {
    if (hdr.height == 1)
    {
      return 0;
    }
    // 替换新的root
    root = FetchNode(root->GetAddr(0));
    // pin root page - should always be valid
    hdr.rootPage = root->GetPageRID().Page();
    PF_PageHandle rootph;
    RC rc = pfHandle->GetThisPage(hdr.rootPage, rootph);
    if (rc != 0)
      return rc;

    node->Destroy();
    rc = DisposePage(node->GetPageRID().Page());
    if (rc < 0)
      return IX_PF;

    SetHeight(hdr.height - 1); // do all other init
    return 0;
  }
}
BtreeNode *IX_IndexHandle::FindLeaf(const void *pData)
{
  RC invalid = IsValid();
  if (invalid)
    return NULL;
  if (root == NULL)
    return NULL;
  RID addr;
  if (hdr.height == 1)
  {
    path[0] = root;
    return root;
  }
  for (int i = 1; i < hdr.height; i++)
  {
    RID r = path[i - 1]->FindAddrAtPosition(pData);
    int pos = path[i - 1]->FindKeyPosition(pData);
    if (r.Page() == -1)
    {
      // pData is bigger than any other key - return address of node
      // that largest key points to.
      const void *p = path[i - 1]->LargestKey();
      // cerr << "p was " << *(int*)p << endl;
      r = path[i - 1]->FindAddr((const void *&)(p));
      // cerr << "r was " << r << endl;
      pos = path[i - 1]->FindKey((const void *&)(p));
    }

    if (path[i] != NULL)
    {
      RC rc = pfHandle->UnpinPage(path[i]->GetPageRID().Page());
      // if (rc != 0) return NULL;
      delete path[i];
      path[i] = NULL;
    }

    path[i] = FetchNode(r.Page());
    PF_PageHandle dummy;

    RC rc = pfHandle->GetThisPage(path[i]->GetPageRID().Page(), dummy);
    if (rc != 0)
      return NULL;

    pathP[i - 1] = pos;
  }
  return path[hdr.height - 1];
}
RC IX_IndexHandle::Open(PF_FileHandle *pfh, int pairSize,
                        PageNum p, int pageSize)
{
  if (bFileOpen || pfHandle != NULL)
  {
    return IX_HANDLEOPEN;
  }
  if (pfh == NULL ||
      pairSize <= 0)
  {
    return IX_BADOPEN;
  }
  bFileOpen = true;
  pfHandle = new PF_FileHandle;
  *pfHandle = *pfh;

  PF_PageHandle ph;
  GetThisPage(0, ph);

  this->GetFileHeader(ph);

  PF_PageHandle rootph;

  bool newPage = true;
  if (hdr.height > 0)
  {
    // 存在rootNode信息，直接
    SetHeight(hdr.height); // do all other init
    newPage = false;
    RC rc = GetThisPage(hdr.rootPage, rootph);
    if (rc != 0)
      return rc;
  }
  else
  {
    // 初始化rootNode信息
    PageNum p;
    RC rc = GetNewPage(p);
    if (rc != 0)
      return rc;
    rc = GetThisPage(p, rootph);
    if (rc != 0)
      return rc;
    hdr.rootPage = p;
    SetHeight(1); // do all other init
  }
  RC rc = pfHandle->GetThisPage(hdr.rootPage, rootph);
  if (rc != 0)
    return rc;

  // rc = pfHandle->MarkDirty(hdr.rootPage);
  // if(rc!=0) return NULL;

  root = new BtreeNode(hdr.attrType, hdr.attrLength,
                       rootph, newPage,
                       hdr.pageSize);
  path[0] = root;
  hdr.order = root->GetMaxKeys();
  bHdrChanged = true;
  RC invalid = IsValid();
  if (invalid)
    return invalid;
  treeLargest = (void *)new char[hdr.attrLength];
  if (!newPage)
  {
    BtreeNode *node = FindLargestLeaf();
    // set treeLargest
    if (node->GetNumKeys() > 0)
      node->CopyKey(node->GetNumKeys() - 1, treeLargest);
  }
  if (rc = (pfHandle->UnpinPage(hdr.rootPage)))
  {
    return rc;
  }
  return 0;
}

BtreeNode *IX_IndexHandle::DupScanLeftFind(BtreeNode *right, void *pData, const RID &rid)
{
  BtreeNode *currNode = FetchNode(right->GetLeft());
  int currPos = -1;
  for (BtreeNode *j = currNode;
       j != NULL;
       j = FetchNode(j->GetLeft()))
  {
    currNode = j;
    int i = currNode->GetNumKeys() - 1;
    for (; i >= 0; i--)
    {
      currPos = i; // save Node in object state for later.
      char *key = NULL;
      int ret = currNode->GetKey(i, (void *&)key);
      if (ret == -1)
        break;
      if (currNode->CmpKey(pData, key) < 0)
        return NULL;
      if (currNode->CmpKey(pData, key) > 0)
        return NULL; // should never happen
      if (currNode->CmpKey(pData, key) == 0)
      {
        if (currNode->GetAddr(currPos) == rid)
          return currNode;
      }
    }
  }
  return NULL;
}
// 等会看看
BtreeNode *IX_IndexHandle::FindLeafForceRight(const void *pData)
{
  FindLeaf(pData);
  // see if duplicates for this value exist in the next node to the right and
  // return that node if true.
  //  have a right node ?
  if (path[hdr.height - 1]->GetRight() != -1)
  {

    // at last position in node ?
    int pos = path[hdr.height - 1]->FindKey(pData);
    if (pos != -1 &&
        pos == path[hdr.height - 1]->GetNumKeys() - 1)
    {

      // cerr << "bingo: dups to the right at leaf " << *(int*)pData << " lastPos\n";
      // cerr << "bingo: right page at " << path[hdr.height-1]->GetRight() << "\n";

      BtreeNode *r = FetchNode(path[hdr.height - 1]->GetRight());
      if (r != NULL)
      {
        // cerr << "bingo: dups to the right at leaf " << *(int*)pData << " nonNUllR\n";

        void *k = NULL;
        r->GetKey(0, k);
        if (r->CmpKey(k, pData) == 0)
        {
          // cerr << "bingo: dups to the right at leaf " << *(int*)pData << "\n";

          RC rc = pfHandle->UnpinPage(path[hdr.height - 1]->GetPageRID().Page());
          //   if (rc < 0)
          //   {
          //     PrintErrorAll(rc);
          //   }
          if (rc < 0)
            return NULL;
          delete path[hdr.height - 1];
          path[hdr.height - 1] = FetchNode(r->GetPageRID());
          pathP[hdr.height - 2]++;
        }
      }
    }
  }
  return path[hdr.height - 1];
}

RC IX_IndexHandle::Pin(PageNum p)
{
  PF_PageHandle ph;
  RC rc = pfHandle->GetThisPage(p, ph);
  return rc;
}

RC IX_IndexHandle::UnPin(PageNum p)
{
  RC rc = pfHandle->UnpinPage(p);
  return rc;
}

RC IX_IndexHandle::GetThisPage(PageNum p, PF_PageHandle &ph) const
{
  RC rc = pfHandle->GetThisPage(p, ph);
  if (rc != 0)
    return rc;
  // Needs to be called everytime GetThisPage is called.
  rc = pfHandle->MarkDirty(p);
  if (rc != 0)
    return rc;

  rc = pfHandle->UnpinPage(p);
  if (rc != 0)
    return rc;
  return 0;
}

RC IX_IndexHandle::GetFileHeader(PF_PageHandle ph)
{
  char *buf;
  RC rc = ph.GetData(buf);
  if (rc != 0)
    return rc;
  memcpy(&hdr, buf + sizeof(PF_PageHdr), sizeof(hdr));
  return 0;
}

RC IX_IndexHandle::SetFileHeader(PF_PageHandle ph) const
{
  char *buf;
  RC rc = ph.GetData(buf);
  if (rc != 0)
    return rc;
  memcpy(buf + sizeof(PF_PageHdr), &hdr, sizeof(hdr));
  return 0;
}

RC IX_IndexHandle::ForcePages()
{
  RC invalid = IsValid();
  if (invalid)
    return invalid;
  return pfHandle->ForcePages(ALL_PAGES);
}

RC IX_IndexHandle::IsValid() const
{
  bool ret = true;
  ret = ret && (pfHandle != NULL);
  if (hdr.height > 0)
  {
    ret = ret && (hdr.rootPage > 0); // 0 is for file header
    ret = ret && (hdr.numPages >= hdr.height + 1);
    ret = ret && (root != NULL);
    ret = ret && (path != NULL);
  }
  return ret ? 0 : IX_BADIXPAGE;
}
//获取新的节点
RC IX_IndexHandle::GetNewPage(PageNum &pageNum)
{
  RC invalid = IsValid();
  if (invalid)
    return invalid;
  PF_PageHandle ph;

  RC rc;
  if ((rc = pfHandle->AllocatePage(ph)) ||
      (rc = ph.GetPageNum(pageNum)))
    return (rc);

  // the default behavior of the buffer pool is to pin pages
  // let us make sure that we unpin explicitly after setting
  // things up
  if (
      //  (rc = pfHandle->MarkDirty(pageNum)) ||
      (rc = pfHandle->UnpinPage(pageNum)))
    return rc;
  // cerr << "GetNewPage called to get page " << pageNum << endl;
  hdr.numPages++;
  assert(hdr.numPages > 1); // page 0 is this page in worst case
  bHdrChanged = true;
  return 0; // pageNum is set correctly
}

RC IX_IndexHandle::DisposePage(const PageNum &pageNum)
{
  RC invalid = IsValid();
  if (invalid)
    return invalid;

  RC rc;
  if ((rc = pfHandle->DisposePage(pageNum)))
    return (rc);

  hdr.numPages--;
  assert(hdr.numPages > 0); // page 0 is this page in worst case
  bHdrChanged = true;
  return 0; // pageNum is set correctly
}

BtreeNode *IX_IndexHandle::FindSmallestLeaf()
{
  RC invalid = IsValid();
  if (invalid)
    return NULL;
  if (root == NULL)
    return NULL;
  RID addr;
  if (hdr.height == 1)
  {
    path[0] = root;
    return root;
  }

  for (int i = 1; i < hdr.height; i++)
  {
    RID r = path[i - 1]->GetAddr(0);
    if (r.Page() == -1)
    {
      // no such position or other error
      // no entries in node ?
      assert("should not run into empty node");
      return NULL;
    }
    // start with a fresh path
    if (path[i] != NULL)
    {
      RC rc = pfHandle->UnpinPage(path[i]->GetPageRID().Page());
      if (rc < 0)
        return NULL;
      delete path[i];
      path[i] = NULL;
    }
    path[i] = FetchNode(r);
    PF_PageHandle dummy;
    // pin path pages
    RC rc = pfHandle->GetThisPage(path[i]->GetPageRID().Page(), dummy);
    if (rc != 0)
      return NULL;
    pathP[i - 1] = 0; // dummy
  }
  return path[hdr.height - 1];
}

BtreeNode *IX_IndexHandle::FindLargestLeaf()
{
  RC invalid = IsValid();
  if (invalid)
    return NULL;
  if (root == NULL)
    return NULL;
  RID addr;
  if (hdr.height == 1)
  {
    path[0] = root;
    return root;
  }

  for (int i = 1; i < hdr.height; i++)
  {
    RID r = path[i - 1]->GetAddr(path[i - 1]->GetNumKeys() - 1);
    if (r.Page() == -1)
    {
      // no such position or other error
      // no entries in node ?
      assert("should not run into empty node");
      return NULL;
    }
    // start with a fresh path
    if (path[i] != NULL)
    {
      RC rc = pfHandle->UnpinPage(path[i]->GetPageRID().Page());
      if (rc < 0)
        return NULL;
      delete path[i];
      path[i] = NULL;
    }
    path[i] = FetchNode(r);
    PF_PageHandle dummy;
    // pin path pages
    RC rc = pfHandle->GetThisPage(path[i]->GetPageRID().Page(), dummy);
    if (rc != 0)
      return NULL;
    pathP[i - 1] = 0; // dummy
  }
  return path[hdr.height - 1];
}

BtreeNode *IX_IndexHandle::FetchNode(PageNum p) const
{
  return FetchNode(RID(p, -1));
}

BtreeNode *IX_IndexHandle::FetchNode(RID r) const
{
  RC invalid = IsValid();
  if (invalid)
    return NULL;
  if (r.Page() < 0)
    return NULL;
  PF_PageHandle ph;
  RC rc = GetThisPage(r.Page(), ph);
  if (rc != 0)
    return NULL;

  // rc = pfHandle->MarkDirty(r.Page());
  // if(rc!=0) return NULL;

  return new BtreeNode(hdr.attrType, hdr.attrLength,
                       ph, false,
                       hdr.pageSize);
}

RC IX_IndexHandle::Search(void *pData, RID &rid)
{
  RC invalid = IsValid();
  if (invalid)
    return invalid;
  if (pData == NULL)
    return IX_BADKEY;
  BtreeNode *leaf = FindLeaf(pData);
  if (leaf == NULL)
    return IX_BADKEY;
  rid = leaf->FindAddr((const void *&)pData);
  if (rid == RID(-1, -1))
    return IX_KEYNOTFOUND;
  return 0;
}

int IX_IndexHandle::GetHeight() const
{
  return hdr.height;
}

void IX_IndexHandle::SetHeight(const int &h)
{
  for (int i = 1; i < hdr.height; i++)
    if (path != NULL && path[i] != NULL)
    {
      delete path[i];
      path[i] = NULL;
    }
  if (path != NULL)
    delete[] path;
  if (pathP != NULL)
    delete[] pathP;

  hdr.height = h;
  path = new BtreeNode *[hdr.height];
  for (int i = 1; i < hdr.height; i++)
    path[i] = NULL;
  path[0] = root;

  pathP = new int[hdr.height - 1]; // leaves don't point
  for (int i = 0; i < hdr.height - 1; i++)
    pathP[i] = -1;
}

BtreeNode *IX_IndexHandle::GetRoot() const
{
  return root;
}

void IX_IndexHandle::Print(ostream &os, int level, RID r) const
{
  RC invalid = IsValid();
  if (invalid)
    assert(invalid);
  // level -1 signals first call to recursive function - root
  // os << "Print called with level " << level << endl;
  BtreeNode *node = NULL;
  if (level == -1)
  {
    level = hdr.height;
    node = FetchNode(root->GetPageRID());
  }
  else
  {
    if (level < 1)
    {
      return;
    }
    else
    {
      node = FetchNode(r);
    }
  }

  node->Print(os);
  if (level >= 2) // non leaf
  {
    for (int i = 0; i < node->GetNumKeys(); i++)
    {
      RID newr = node->GetAddr(i);
      Print(os, level - 1, newr);
    }
  }
  // else level == 1 - recursion ends
  if (level == 1 && node->GetRight() == -1)
    os << endl; // blank after rightmost leaf
  if (node != NULL)
    delete node;
}