/**
 * @file wuthering_waves_plugin.cpp
 * @brief 鸣潮抽卡记录插件实现
 */

#include "wuthering_waves/wuthering_waves_plugin.hpp"
#include "wuthering_waves/gacha_record_view.hpp"
#include "core/ui/view.h"
#include "liblogger/logger.h"

using namespace DearTs;
using namespace DearTs::Core;
using namespace DearTs::Core::Plugin;
using namespace DearTs::Core::ContentRegistry;

namespace DearTs::Plugins::WutheringWaves {

Result<void, std::string> WutheringWavesPlugin::on_load() {
    LOG_INFO("WutheringWaves plugin loading...");

    // 注册抽卡记录视图
    Views::add<GachaRecordView>();

    LOG_INFO("WutheringWaves plugin loaded successfully");
    return Result<void, std::string>::ok();
}

void WutheringWavesPlugin::on_unload() {
    LOG_INFO("WutheringWaves plugin unloaded");
}

void WutheringWavesPlugin::on_enable() {
    LOG_INFO("WutheringWaves plugin enabled");
}

void WutheringWavesPlugin::on_disable() {
    LOG_INFO("WutheringWaves plugin disabled");
}

} // namespace DearTs::Plugins::WutheringWaves
