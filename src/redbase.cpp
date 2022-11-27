
#include "initLib.h"
#include "configloader.h"
#include "pf_manager.h"
#include "rm_manager.h"
#include <fstream>
#include "rm_filescan.h"
#include <ctime>
#include <string>
PF_Manager PF;
RM_Manager RM(PF);
int main(int argc, char const *argv[])
{
    configloader loader("../config.json");
    loader.loadConfig();
    config *cfg = loader.getConfig();
    PF.SetBufferSize(cfg->getBufferSize(), LRU);
    initLib *lib = new initLib();
    return 0;
}
