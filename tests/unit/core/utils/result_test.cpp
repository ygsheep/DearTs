/**
 * @file result_test.cpp
 * @brief Unit tests for Result<T,E> type
 */

#include <gtest/gtest.h>
#include "core/result.h"
#include <string>

using namespace DearTs::Core;

/**
 * @brief Test fixture for Result tests
 */
class ResultTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Reset any state before each test
    }

    void TearDown() override {
        // Clean up after each test
    }
};

// ============================================================================
// Basic Operations
// ============================================================================

TEST_F(ResultTest, OkIntValue_ReturnsSuccess) {
    auto result = Result<int, std::string>::ok(42);

    EXPECT_TRUE(result.isOk());
    EXPECT_FALSE(result.isErr());
    EXPECT_EQ(result.unwrap(), 42);
}

TEST_F(ResultTest, ErrStringValue_ReturnsError) {
    auto result = Result<int, std::string>::err("Something went wrong");

    EXPECT_TRUE(result.isErr());
    EXPECT_FALSE(result.isOk());
    EXPECT_EQ(result.error(), "Something went wrong");
}

TEST_F(ResultTest, OkVoidValue_ReturnsSuccess) {
    auto result = Result<void, std::string>::ok();

    EXPECT_TRUE(result.isOk());
    EXPECT_FALSE(result.isErr());
}

TEST_F(ResultTest, ErrVoidValue_ReturnsError) {
    auto result = Result<void, std::string>::err("Error");

    EXPECT_TRUE(result.isErr());
    EXPECT_FALSE(result.isOk());
    EXPECT_EQ(result.error(), "Error");
}

// ============================================================================
// unwrap/unwrap_or
// ============================================================================

TEST_F(ResultTest, UnwrapOk_ReturnsValue) {
    auto result = Result<int, std::string>::ok(123);
    EXPECT_EQ(result.unwrap(), 123);
}

TEST_F(ResultTest, UnwrapErr_ThrowsException) {
    auto result = Result<int, std::string>::err("Error");

    // unwrap() on error should throw an exception
    EXPECT_THROW(result.unwrap(), std::runtime_error);
}

TEST_F(ResultTest, UnwrapOr_OnOk_ReturnsValue) {
    auto result = Result<int, std::string>::ok(42);
    EXPECT_EQ(result.unwrap_or(0), 42);
}

TEST_F(ResultTest, UnwrapOr_OnErr_ReturnsDefaultValue) {
    auto result = Result<int, std::string>::err("Error");
    EXPECT_EQ(result.unwrap_or(0), 0);
}

TEST_F(ResultTest, UnwrapOr_OnErr_ReturnsCustomDefault) {
    auto result = Result<int, std::string>::err("Error");
    EXPECT_EQ(result.unwrap_or(99), 99);
}
