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

TEST_F(ResultTest, UnwrapErr_TerminatesProgram) {
    auto result = Result<int, std::string>::err("Error");

    // unwrap() on error should cause program termination
    // In a real test, you might use death test
    EXPECT_DEATH_IF_SUPPORTED(result.unwrap(), ".*");
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

// ============================================================================
// map
// ============================================================================

TEST_F(ResultTest, Map_OnOk_TransformsValue) {
    auto result = Result<int, std::string>::ok(5);
    auto mapped = result.map([](int value) { return value * 2; });

    EXPECT_TRUE(mapped.isOk());
    EXPECT_EQ(mapped.unwrap(), 10);
}

TEST_F(ResultTest, Map_OnErr_PropagatesError) {
    auto result = Result<int, std::string>::err("Error");
    auto mapped = result.map([](int value) { return value * 2; });

    EXPECT_TRUE(mapped.isErr());
    EXPECT_EQ(mapped.error(), "Error");
}

TEST_F(ResultTest, Map_ChainedOperations) {
    auto result = Result<int, std::string>::ok(2);

    auto mapped = result
        .map([](int x) { return x + 3; })      // 2 + 3 = 5
        .map([](int x) { return x * 4; });     // 5 * 4 = 20

    EXPECT_TRUE(mapped.isOk());
    EXPECT_EQ(mapped.unwrap(), 20);
}

TEST_F(ResultTest,Map_ChangingType) {
    auto result = Result<int, std::string>::ok(42);

    auto mapped = result.map([](int value) {
        return "Value is: " + std::to_string(value);
    });

    EXPECT_TRUE(mapped.isOk());
    EXPECT_EQ(mapped.unwrap(), "Value is: 42");
}

// ============================================================================
// map_err
// ============================================================================

TEST_F(ResultTest, MapErr_OnErr_TransformsError) {
    auto result = Result<int, std::string>::err("Network error");
    auto mapped = result.map_err([](const std::string& err) {
        return "CRITICAL: " + err;
    });

    EXPECT_TRUE(mapped.isErr());
    EXPECT_EQ(mapped.error(), "CRITICAL: Network error");
}

TEST_F(ResultTest, MapErr_OnOk_PreservesValue) {
    auto result = Result<int, std::string>::ok(42);
    auto mapped = result.map_err([](const std::string& err) {
        return "Modified error";
    });

    EXPECT_TRUE(mapped.isOk());
    EXPECT_EQ(mapped.unwrap(), 42);
}

// ============================================================================
// and_then / or_else
// ============================================================================

TEST_F(ResultTest, AndThen_OnOk_ChainsResult) {
    auto result = Result<int, std::string>::ok(5);

    auto chained = result.and_then([](int value) {
        if (value > 0) {
            return Result<double, std::string>::ok(value * 2.5);
        } else {
            return Result<double, std::string>::err("Invalid value");
        }
    });

    EXPECT_TRUE(chained.isOk());
    EXPECT_DOUBLE_EQ(chained.unwrap(), 12.5);
}

TEST_F(ResultTest, AndThen_OnErr_ShortCircuits) {
    auto result = Result<int, std::string>::err("First error");

    auto chained = result.and_then([](int value) {
        return Result<double, std::string>::ok(value * 2.0);
    });

    EXPECT_TRUE(chained.isErr());
    EXPECT_EQ(chained.error(), "First error");
}

TEST_F(ResultTest, OrElse_OnErr_Recovery) {
    auto result = Result<int, std::string>::err("Primary failed");

    auto recovered = result.or_else([](const std::string& err) {
        return Result<int, std::string>::ok(100);  // Fallback value
    });

    EXPECT_TRUE(recovered.isOk());
    EXPECT_EQ(recovered.unwrap(), 100);
}

TEST_F(ResultTest, OrElse_OnOk_PreservesValue) {
    auto result = Result<int, std::string>::ok(42);

    auto recovered = result.or_else([](const std::string& err) {
        return Result<int, std::string>::ok(0);
    });

    EXPECT_TRUE(recovered.isOk());
    EXPECT_EQ(recovered.unwrap(), 42);
}

// ============================================================================
// Comparison Operators
// ============================================================================

TEST_F(ResultTest, Equality_SameOkValues) {
    auto result1 = Result<int, std::string>::ok(42);
    auto result2 = Result<int, std::string>::ok(42);

    EXPECT_EQ(result1, result2);
}

TEST_F(ResultTest, Inequality_DifferentOkValues) {
    auto result1 = Result<int, std::string>::ok(42);
    auto result2 = Result<int, std::string>::ok(43);

    EXPECT_NE(result1, result2);
}

TEST_F(ResultTest, Equality_SameErrValues) {
    auto result1 = Result<int, std::string>::err("Error");
    auto result2 = Result<int, std::string>::err("Error");

    EXPECT_EQ(result1, result2);
}

// ============================================================================
// Complex Scenarios
// ============================================================================

TEST_F(ResultTest, ComplexChain_RealWorldScenario) {
    // Simulate a real-world operation chain
    auto parse_int = [](const std::string& str) {
        try {
            return Result<int, std::string>::ok(std::stoi(str));
        } catch (...) {
            return Result<int, std::string>::err("Failed to parse int");
        }
    };

    auto validate_positive = [](int value) {
        if (value > 0) {
            return Result<int, std::string>::ok(value);
        } else {
            return Result<int, std::string>::err("Value must be positive");
        }
    };

    auto double_value = [](int value) {
        return Result<int, std::string>::ok(value * 2);
    };

    // Test success path
    auto result = parse_int(std::string("42"))
        .and_then(validate_positive)
        .and_then(double_value);

    EXPECT_TRUE(result.isOk());
    EXPECT_EQ(result.unwrap(), 84);

    // Test failure path
    auto result2 = parse_int(std::string("invalid"))
        .and_then(validate_positive)
        .and_then(double_value);

    EXPECT_TRUE(result2.isErr());
    EXPECT_STREQ(result2.error().c_str(), "Failed to parse int");
}
