#include <cmath>
#include <iostream>
#include "initLib.h"
initLib::initLib()
{
    dataLibrary = new relationTable(0, "account", "root", 3, 1000);
    propertyTable *propslib = new propertyTable("id", 4, INT, "notnull");
    dataLibrary->propsLink.push_back(propslib);
    propslib = new propertyTable("name", 10, VARCHAR, "notnull");
    dataLibrary->propsLink.push_back(propslib);
    propslib = new propertyTable("money", 4, INT, "notnull");
    dataLibrary->propsLink.push_back(propslib);
}
initLib::~initLib()
{
}
/*
    record大致组成:
    ————————————————————————————————————————————————————————————————————————————————————————————————
   | 状态位(1) | 整条记录长度(4) | 位图(nullMapNum) | 属性值(int,char) | 属性值长度(2)+属性值(varchar) |
   —————————————————————————————————————————————————————————————————————————————————————————————————
*/
char *initLib::exdata(char *record, int &size)
{

    string str = string(record);
    //将record进行分词操作
    vector<string> result = split(str, ",");
    // totlen = 1字节的状态位TagA + sizeof(int)字节的数量,其中sizeof(int)为了存整条记录的长度
    int totalLen = 5;
    // nullMapNum是判断numMap的位图长度，需要占几个字节。
    int nullMapNum = (int)ceil(result.size() / 8.0);
    // cout << "nullMapNum:" << nullMapNum << endl;
    // 定义空位图，为numMap分配空间
    char *numMap = new char[nullMapNum];
    // memset(numMap, 0, nullMapNum);
    // 初始化
    memset(numMap, 0, nullMapNum);
    // 将total的数值增加numMapNum
    totalLen += nullMapNum;
    //定义主信息的位数
    int constLength = 0;
    //通过属性值进行遍历
    for (int i = 0; i < result.size(); i++)
    {
        if (dataLibrary->propsLink[i]->typeName == INT)
        {
            constLength += dataLibrary->propsLink[i]->length;
        }
        else if (dataLibrary->propsLink[i]->typeName == STRING)
        {
            constLength++;
            constLength += dataLibrary->propsLink[i]->length;
        }
        else
        {
            //如果是varchar类型，需要取当前数据的长度
            constLength++;
            constLength++;
            constLength += result[i].length();
            //加上存储该信息的长度空间
            constLength += sizeof(short);
        }
        if (result[i] == "null")
        {
            // 求出null信息所在的反向块编号
            int posBlock = (int)ceil((i + 1) / 8.0);
            // 求出正向块编号
            char *pos = &numMap[nullMapNum - posBlock];
            // 求出准确的判断位
            *pos = *pos | (1 << (i % 8));
        }
    }
    // 将数据长度加入整个totalLen中
    totalLen += constLength;
    // cout << "constLength:" << constLength << endl;
    // 预申请一块数据空间
    char *rcd = (char *)malloc(totalLen);
    // 初始化空间
    memset(rcd, 0, totalLen);
    // 令header指向rcd头部，进行指针地址保存
    char *header = rcd;
    // 向后偏移一位，到
    rcd = rcd + sizeof(char);
    // 找到存储总字节数的记录位
    int *constp = (int *)rcd;
    // 存入对应信息
    *constp = constLength;
    // 将地址加上constLength长度的偏移量

    rcd += sizeof(int);
    // 拷贝numMap的取值，并将地址加上numMap的偏移量
    memcpy(rcd, numMap, nullMapNum);
    // cout << "sizeof(numMap):" << sizeof(numMap) << endl;
    rcd += sizeof(char) * nullMapNum;

    // 将信息循环写入rcd所指的连续空间中
    for (int i = 0; i < result.size(); i++)
    {
        // 确定类型
        AttrType type = dataLibrary->propsLink[i]->typeName;
        // cout << "result:" << result[i] << endl;
        if (result[i] == "null")
        {
            // 修改null的值
            result[i] = "";
        }
        if (type == INT)
        {
            int *pos = (int *)rcd;
            // 强转换成int类型
            int a = atoi(result[i].c_str());
            // cout << "a:" << a << endl;
            *pos = atoi(result[i].c_str());
            rcd = rcd + dataLibrary->propsLink[i]->length;


            // totalLen += dataLibrary->propsLink[i]->length;
        }
        else if (type == STRING)
        {
            char *pos = rcd;
            strcpy(pos, result[i].c_str());
            rcd = rcd + dataLibrary->propsLink[i]->length + 1 + 1;
        }
        else if (type == VARCHAR)
        {
            // 需要预存长度空间
            short *posmark = (short *)rcd;
            *posmark = result[i].length();
            rcd += sizeof(short);
            char *pos = rcd;
            strcpy(pos, result[i].c_str());
            rcd = rcd + result[i].length() + 1;
        }
        // strcpy(type, dataLibrary->propsLink[i]->typeName);
    }
    size = totalLen;
    free(numMap);
    return header;
}
vector<string> initLib::split(string str, string pattern)
{
    string::size_type pos;
    vector<string> result;
    str += pattern; //扩展字符串以方便操作
    int size = str.size();
    for (int i = 0; i < size; i++)
    {
        pos = str.find(pattern, i);
        if (pos < size)
        {
            std::string s = str.substr(i, pos - i);
            result.push_back(s);
            i = pos + pattern.size() - 1;
        }
    }
    return result;
}
/*
    record大致组成:
    ————————————————————————————————————————————————————————————————————————————————————————————————
   | 状态位(1) | 整条记录长度(4) | 位图(nullMapNum) | 属性值(int,char) | 属性值长度(2)+属性值(varchar) |
   —————————————————————————————————————————————————————————————————————————————————————————————————
*/
string initLib::rexdata(char *record, int size)
{
    //规定输出格式
    string rs;
    //求出位图的长度
    int nullMapNum = (int)ceil(dataLibrary->propsLink.size() / 8.0);
    //求出第一个属性值的开头首地址
    char *pos = &record[5 + nullMapNum];
    for (int i = 0; i < dataLibrary->propsLink.size(); i++)
    {
        // 求出当前属性所在位图的反序编号
        int posBlock = (int)ceil((i + 1) / 8.0);
        // 求出当前属性所在位图的正向编号
        char *bitPos = &record[(nullMapNum - posBlock) + 5];
        // cout << u_short(*bitPos) << endl;
        // 提取出当前位置空状态
        bool flag = (1 & ((*bitPos) >> (i % 8)));
        //如果不为空
        if (!flag)
        {
            //却定类型
            AttrType type = dataLibrary->propsLink[i]->typeName;
            //确定长度
            int attrSize = dataLibrary->propsLink[i]->length;
            //将"字符" + ","拼接到 rs 上
            if (type == INT)
            {
                int *ans = (int *)pos;
                rs = rs + to_string(*ans) + ",";
                pos += sizeof(int);
            }
            else if (type == STRING)
            {
                rs = rs + string(pos) + ",";
                pos += (attrSize + 1);
            }
            else
            {
                short *len = (short *)pos;
                pos += sizeof(short);
                rs = rs + string(pos) + ",";
                pos += (*len + 1);
            }
        }
        //如果为空
        else
        {
            rs = rs + "null,";
        }
    }
    return rs.substr(0, rs.length() - 1);
}