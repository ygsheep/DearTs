/**
 * @file singleton.h
 * @brief 单例模式基类实现
 * @details 提供线程安全和单线程版本的单例模式基类
 * @author DearTs Team
 * @date 2025
 */

#pragma once

#include <mutex>
#include <memory>

namespace DearTs {
namespace Core {
namespace Patterns {

/**
 * @brief 线程安全的单例模式基类
 * @tparam T 派生类类型
 * @details 使用CRTP（奇异递归模板模式）实现单例
 *          提供线程安全的实例获取和销毁
 */
template<typename T>
class Singleton {
public:
    /**
     * @brief 获取单例实例
     * @return T& 单例实例的引用
     * @details 线程安全的懒汉式单例实现
     */
    static T& getInstance() {
        std::call_once(s_onceFlag, []() {
            s_instance = std::make_unique<T>();
        });
        return *s_instance;
    }

    /**
     * @brief 销毁单例实例
     * @details 手动销毁单例实例，释放资源
     */
    static void destroyInstance() {
        std::lock_guard<std::mutex> lock(s_mutex);
        s_instance.reset();
        s_onceFlag = std::once_flag{};
    }

    /**
     * @brief 检查实例是否存在
     * @return bool 实例是否已创建
     */
    static bool hasInstance() {
        std::lock_guard<std::mutex> lock(s_mutex);
        return s_instance != nullptr;
    }

protected:
    /**
     * @brief 构造函数
     * @details 受保护的构造函数，防止外部直接创建实例
     */
    Singleton() = default;

    /**
     * @brief 析构函数
     * @details 虚析构函数，确保派生类正确析构
     */
    virtual ~Singleton() = default;

    // 禁用拷贝构造和赋值操作
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;
    Singleton(Singleton&&) = delete;
    Singleton& operator=(Singleton&&) = delete;

private:
    static std::unique_ptr<T> s_instance;     ///< 单例实例
    static std::once_flag s_onceFlag;         ///< 一次性标志
    static std::mutex s_mutex;                ///< 互斥锁
};

// 静态成员定义
template<typename T>
std::unique_ptr<T> Singleton<T>::s_instance = nullptr;

template<typename T>
std::once_flag Singleton<T>::s_onceFlag;

template<typename T>
std::mutex Singleton<T>::s_mutex;

/**
 * @brief 单线程版本的单例模式基类
 * @tparam T 派生类类型
 * @details 不提供线程安全保证，但性能更好
 *          适用于单线程环境或已知线程安全的场景
 */
template<typename T>
class SingletonST {
public:
    /**
     * @brief 获取单例实例
     * @return T& 单例实例的引用
     * @details 非线程安全的懒汉式单例实现
     */
    static T& getInstance() {
        if (!s_instance) {
            s_instance = std::make_unique<T>();
        }
        return *s_instance;
    }

    /**
     * @brief 销毁单例实例
     * @details 手动销毁单例实例，释放资源
     */
    static void destroyInstance() {
        s_instance.reset();
    }

    /**
     * @brief 检查实例是否存在
     * @return bool 实例是否已创建
     */
    static bool hasInstance() {
        return s_instance != nullptr;
    }

protected:
    /**
     * @brief 构造函数
     * @details 受保护的构造函数，防止外部直接创建实例
     */
    SingletonST() = default;

    /**
     * @brief 析构函数
     * @details 虚析构函数，确保派生类正确析构
     */
    virtual ~SingletonST() = default;

    // 禁用拷贝构造和赋值操作
    SingletonST(const SingletonST&) = delete;
    SingletonST& operator=(const SingletonST&) = delete;
    SingletonST(SingletonST&&) = delete;
    SingletonST& operator=(SingletonST&&) = delete;

private:
    static std::unique_ptr<T> s_instance;     ///< 单例实例
};

// 静态成员定义
template<typename T>
std::unique_ptr<T> SingletonST<T>::s_instance = nullptr;

/**
 * @brief 管理器基类
 * @tparam T 派生类类型
 * @details 继承自线程安全单例，为各种管理器类提供统一基类
 *          添加了初始化和清理的虚函数接口
 */
template<typename T>
class Manager : public Singleton<T> {
public:
    /**
     * @brief 初始化管理器
     * @return bool 初始化是否成功
     * @details 派生类应重写此方法实现具体的初始化逻辑
     */
    virtual bool initialize() { return true; }

    /**
     * @brief 清理管理器
     * @details 派生类应重写此方法实现具体的清理逻辑
     */
    virtual void cleanup() {}

    /**
     * @brief 检查管理器是否已初始化
     * @return bool 是否已初始化
     */
    bool isInitialized() const { return m_initialized; }

protected:
    /**
     * @brief 构造函数
     */
    Manager() : m_initialized(false) {}

    /**
     * @brief 析构函数
     */
    virtual ~Manager() {
        if (m_initialized) {
            cleanup();
        }
    }

    /**
     * @brief 设置初始化状态
     * @param initialized 初始化状态
     */
    void setInitialized(bool initialized) { m_initialized = initialized; }

private:
    bool m_initialized;  ///< 初始化状态标志
};

/**
 * @brief 单线程版本的管理器基类
 * @tparam T 派生类类型
 * @details 继承自单线程单例，性能更好但不提供线程安全保证
 */
template<typename T>
class ManagerST : public SingletonST<T> {
public:
    /**
     * @brief 初始化管理器
     * @return bool 初始化是否成功
     */
    virtual bool initialize() { return true; }

    /**
     * @brief 清理管理器
     */
    virtual void cleanup() {}

    /**
     * @brief 检查管理器是否已初始化
     * @return bool 是否已初始化
     */
    bool isInitialized() const { return m_initialized; }

protected:
    /**
     * @brief 构造函数
     */
    ManagerST() : m_initialized(false) {}

    /**
     * @brief 析构函数
     */
    virtual ~ManagerST() {
        if (m_initialized) {
            cleanup();
        }
    }

    /**
     * @brief 设置初始化状态
     * @param initialized 初始化状态
     */
    void setInitialized(bool initialized) { m_initialized = initialized; }

private:
    bool m_initialized;  ///< 初始化状态标志
};

} // namespace Patterns
} // namespace Core
} // namespace DearTs
