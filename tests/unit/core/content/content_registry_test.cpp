/**
 * @file content_registry_test.cpp
 * @brief Unit tests for ContentRegistry (Commands, Tools, Settings)
 */

#include <gtest/gtest.h>
#include "core/content/commands.h"
#include "core/content/tools.h"
#include "core/content/settings.h"
#include <string>
#include <vector>

using namespace DearTs::Core;

// ============================================================================
// ContentRegistry Test Fixture
// ============================================================================

class ContentRegistryTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Note: ContentRegistry is likely a singleton or static registry
        // Tests should use unique names to avoid conflicts
    }

    void TearDown() override {
        // Clean up - unregister test commands/tools if needed
    }
};

