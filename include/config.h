#if !defined(CONFIG_H)
#define CONFIG_H
class config
{
private:
    int bufferSize;
    int diskSize;
    int bufferBlock;

public:
    config(int bufferSize,
           int diskSize,
           int bufferBlock)
        : bufferSize(bufferSize),
          diskSize(diskSize),
          bufferBlock(bufferBlock){

          };
    int getBufferSize() { return bufferSize; }
    int getDiskSize() { return diskSize; }
    int getBufferBlock() { return bufferBlock; }
    ~config();
};
#endif // CONFIG_H
