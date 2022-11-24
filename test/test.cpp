#include <gtest/gtest.h>
#include "../include/configloader.h"

TEST(test, configload)
{
    configloader loader("./config.json");
    loader.loadConfig();
    config *cfg = loader.getConfig();
    ASSERT_EQ(cfg->getBufferBlock(), 4096) << "读取文件失败！" << endl;
}

