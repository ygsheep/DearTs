/**
 * DearTs Audio Management System - Simplified Version
 * 
 * 
 * @author DearTs Team
 * @version 1.0.0
 * @date 2025
 */

#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include <mutex>
#include <atomic>

// SDL库包含
// #include <SDL_mixer.h>  // 暂时注释掉，避免编译错误

namespace DearTs {
namespace Core {
namespace Audio {

// ============================================================================

/**
 * @brief 音频状态枚举
 */
enum class AudioState {
    STOPPED = 0,
    PLAYING,
    PAUSED
};

// ============================================================================

/**
 * @brief 音频配置结构
 */
struct AudioConfig {
    int frequency = 44100;          // 采样率
    int channels = 2;               // 声道数（1=单声道，2=立体声）
    int chunk_size = 1024;          // 缓冲区大小
    int max_channels = 16;          // 最大混音通道数
    float master_volume = 1.0f;     // 主音量
    float music_volume = 0.7f;      // 音乐音量
    float sfx_volume = 0.8f;        // 音效音量
};

/**
 * @brief 音频统计信息
 */
struct AudioStats {
    int active_channels = 0;        // 活跃通道数
    int total_sounds_loaded = 0;    // 已加载音效数
    int total_music_loaded = 0;     // 已加载音乐数
    size_t memory_usage = 0;        // 内存使用量
};

// ============================================================================

/**
 * @brief 简化版音频管理器
 */
class AudioManager {
public:
    /**
     * @brief 获取单例实例
     */
    static AudioManager& getInstance();

    /**
     * @brief 初始化音频系统
     * @param config 音频配置
     * @return 初始化是否成功
     */
    bool initialize(const AudioConfig& config = AudioConfig{});

    /**
     * @brief 关闭音频系统
     */
    void shutdown();

    // ========================================================================
    // 音效管理
    // ========================================================================

    /**
     * @brief 加载音效文件
     * @param id 音效ID
     * @param file_path 文件路径
     * @return 加载是否成功
     */
    bool loadSound(const std::string& id, const std::string& file_path);

    /**
     * @brief 播放音效
     * @param id 音效ID
     * @param volume 音量 (0.0-1.0)
     * @param loops 循环次数 (0=播放一次, -1=无限循环)
     * @return 播放通道号，-1表示失败
     */
    int playSound(const std::string& id, float volume = 1.0f, int loops = 0);

    /**
     * @brief 停止指定通道的音效
     * @param channel 通道号
     */
    void stopSound(int channel);

    /**
     * @brief 停止所有音效
     */
    void stopAllSounds();

    /**
     * @brief 卸载音效
     * @param id 音效ID
     */
    void unloadSound(const std::string& id);

    // ========================================================================
    // 音乐管理
    // ========================================================================

    /**
     * @brief 加载音乐文件
     * @param id 音乐ID
     * @param file_path 文件路径
     * @return 加载是否成功
     */
    bool loadMusic(const std::string& id, const std::string& file_path);

    /**
     * @brief 播放音乐
     * @param id 音乐ID
     * @param loops 循环次数 (-1=无限循环)
     * @return 播放是否成功
     */
    bool playMusic(const std::string& id, int loops = -1);

    /**
     * @brief 停止音乐
     */
    void stopMusic();

    /**
     * @brief 暂停音乐
     */
    void pauseMusic();

    /**
     * @brief 恢复音乐
     */
    void resumeMusic();

    /**
     * @brief 卸载音乐
     * @param id 音乐ID
     */
    void unloadMusic(const std::string& id);

    /**
     * @brief 检查音乐是否正在播放
     */
    bool isMusicPlaying() const;

    // ========================================================================
    // 音量控制
    // ========================================================================

    /**
     * @brief 设置主音量
     * @param volume 音量 (0.0-1.0)
     */
    void setMasterVolume(float volume);

    /**
     * @brief 获取主音量
     */
    float getMasterVolume() const;

    /**
     * @brief 设置音效音量
     * @param volume 音量 (0.0-1.0)
     */
    void setSoundVolume(float volume);

    /**
     * @brief 获取音效音量
     */
    float getSoundVolume() const;

    /**
     * @brief 设置音乐音量
     * @param volume 音量 (0.0-1.0)
     */
    void setMusicVolume(float volume);

    /**
     * @brief 获取音乐音量
     */
    float getMusicVolume() const;

    // ========================================================================
    // 状态查询
    // ========================================================================

    /**
     * @brief 获取音频统计信息
     */
    AudioStats getStats() const;

    /**
     * @brief 检查是否已初始化
     */
    bool isInitialized() const { return initialized_; }

private:
    AudioManager() = default;
    ~AudioManager() = default;
    AudioManager(const AudioManager&) = delete;
    AudioManager& operator=(const AudioManager&) = delete;

    // 内部数据结构
    struct SoundData {
        void* chunk;  // 暂时使用 void* 替代 Mix_Chunk*
        std::string file_path;
        size_t size;
    };

    struct MusicData {
        void* music;  // 暂时使用 void* 替代 Mix_Music*
        std::string file_path;
    };

    // 成员变量
    bool initialized_ = false;
    AudioConfig config_;

    mutable std::mutex sounds_mutex_;
    std::unordered_map<std::string, SoundData> sounds_;

    mutable std::mutex music_mutex_;
    std::unordered_map<std::string, MusicData> music_library_;
    std::string current_music_id_;

    std::atomic<float> master_volume_{1.0f};
    std::atomic<float> sound_volume_{0.8f};
    std::atomic<float> music_volume_{0.7f};
};

// ============================================================================

#define AUDIO_MANAGER() DearTs::Core::Audio::AudioManager::getInstance()
#define PLAY_SOUND(id) AUDIO_MANAGER().playSound(id)
#define PLAY_MUSIC(id) AUDIO_MANAGER().playMusic(id)
#define STOP_MUSIC() AUDIO_MANAGER().stopMusic()
#define PAUSE_MUSIC() AUDIO_MANAGER().pauseMusic()
#define RESUME_MUSIC() AUDIO_MANAGER().resumeMusic()
#define SET_MASTER_VOLUME(vol) AUDIO_MANAGER().setMasterVolume(vol)
#define SET_SOUND_VOLUME(vol) AUDIO_MANAGER().setSoundVolume(vol)
#define SET_MUSIC_VOLUME(vol) AUDIO_MANAGER().setMusicVolume(vol)

} // namespace Audio
} // namespace Core
} // namespace DearTs

