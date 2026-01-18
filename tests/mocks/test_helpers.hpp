/**
 * @file test_helpers.hpp
 * @brief Test helper functions and utilities
 */

#pragma once

#include <gtest/gtest.h>
#include <string>
#include <filesystem>
#include <random>
#include <sstream>
#include <iomanip>
#include <fstream>

namespace DearTs::Tests {

namespace {
    /**
     * @brief Generate a unique string for temp directory names
     */
    std::string generate_unique_suffix() {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<> dis(0, 0xFFFF);

        std::ostringstream oss;
        oss << std::hex << std::setw(4) << std::setfill('0') << dis(gen);
        oss << std::hex << std::setw(4) << std::setfill('0') << dis(gen);
        return oss.str();
    }
}

/**
 * @brief Create a temporary directory for testing
 *
 * Creates a unique temporary directory that is automatically
 * cleaned up when the object goes out of scope.
 */
class TempDirectory {
public:
    TempDirectory() {
        // Create a unique temp directory using random hex string
        auto temp_path = std::filesystem::temp_directory_path() / ("dearts_test_" + generate_unique_suffix());
        m_path = temp_path;
        std::filesystem::create_directories(m_path);
    }

    ~TempDirectory() {
        // Clean up
        if (exists(m_path)) {
            std::filesystem::remove_all(m_path);
        }
    }

    // Prevent copying
    TempDirectory(const TempDirectory&) = delete;
    TempDirectory& operator=(const TempDirectory&) = delete;

    /**
     * @brief Get the path to the temp directory
     */
    const std::filesystem::path& path() const { return m_path; }

    /**
     * @brief Create a file in the temp directory
     */
    std::filesystem::path create_file(const std::string& filename, const std::string& content) {
        auto file_path = m_path / filename;
        std::ofstream(file_path) << content;
        return file_path;
    }

private:
    std::filesystem::path m_path;
};

/**
 * @brief Scoped environment variable setter
 *
 * Sets an environment variable in the constructor and
 * restores the original value in the destructor.
 */
class ScopedEnvVar {
public:
    ScopedEnvVar(const char* name, const char* value)
        : m_name(name)
    {
        // Get original value
        const char* original = std::getenv(name);
        m_had_value = (original != nullptr);
        if (m_had_value) {
            m_original_value = original;
        }

        // Set new value
        #ifdef _WIN32
            _putenv_s(name, value ? value : "");
        #else
            if (value) {
                setenv(name, value, 1);
            } else {
                unsetenv(name);
            }
        #endif
    }

    ~ScopedEnvVar() {
        // Restore original value
        #ifdef _WIN32
            if (m_had_value) {
                _putenv_s(m_name.c_str(), m_original_value.c_str());
            } else {
                _putenv_s(m_name.c_str(), "");
            }
        #else
            if (m_had_value) {
                setenv(m_name.c_str(), m_original_value.c_str(), 1);
            } else {
                unsetenv(m_name.c_str());
            }
        #endif
    }

private:
    std::string m_name;
    std::string m_original_value;
    bool m_had_value;
};

/**
 * @brief RAII helper for capturing stdout/stderr
 */
class CaptureOutput {
public:
    CaptureOutput() {
        // TODO: Implement stdout/stderr capture
    }

    ~CaptureOutput() {
        // TODO: Restore stdout/stderr
    }

    std::string get_output() const {
        return m_output;
    }

private:
    std::string m_output;
};

} // namespace DearTs::Tests
