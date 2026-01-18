/**
 * @file memory_manager_test.cpp
 * @brief Unit tests for MemoryManager
 */

#include <gtest/gtest.h>
#include "memory_core/memory/memory_manager.hpp"
#include "memory_core/persistence/database.hpp"
#include "tests/mocks/test_helpers.hpp"
#include <thread>
#include <chrono>

using namespace DearTs::Plugins::MemoryCore;
using namespace DearTs::Plugins::MemoryCore::Persistence;
using MemType = DearTs::Plugins::MemoryCore::Memory::MemoryType;
using Mem = DearTs::Plugins::MemoryCore::Memory::Memory;
using MemFilter = DearTs::Plugins::MemoryCore::Memory::MemoryFilter;

// ============================================================================
// Test Fixture
// ============================================================================

class MemoryManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create temporary database
        m_temp_dir = std::make_unique<DearTs::Tests::TempDirectory>();
        m_db_path = (m_temp_dir->path() / "test_memory.db").string();

        // Initialize database
        auto db_result = SQLiteDatabase::instance().initialize(m_db_path);
        ASSERT_TRUE(db_result.isOk()) << "Failed to initialize database: " << db_result.error();
    }

    void TearDown() override {
        // Database will be cleaned up automatically
    }

    /**
     * @brief Create a test memory
     */
    Mem create_test_memory(const std::string& content, double importance = 0.8) {
        Mem memory;
        memory.id = 0;  // Will be set by add_memory
        memory.type = MemType::Fact;
        memory.content = content;
        memory.importance = importance;
        memory.source_conversation_id = "test_conv_123";
        memory.source_message_id = 456;
        memory.created_at = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
        memory.accessed_count = 0;
        return memory;
    }

    std::unique_ptr<DearTs::Tests::TempDirectory> m_temp_dir;
    std::string m_db_path;
};

// ============================================================================
// Initialization Tests
// ============================================================================

TEST_F(MemoryManagerTest, InitializeManager) {
    auto& manager = Memory::MemoryManager::instance();
    // Singleton should always be accessible
    EXPECT_TRUE(true);
}

// ============================================================================
// CRUD Operations Tests
// ============================================================================

TEST_F(MemoryManagerTest, AddAndRetrieveMemory) {
    auto& manager = Memory::MemoryManager::instance();

    // Create a memory
    Mem memory = create_test_memory("User prefers dark theme");

    // Add memory
    auto result = manager.add_memory(memory);
    ASSERT_TRUE(result.isOk()) << "Failed to add memory: " << result.error();

    int64_t memory_id = result.unwrap();
    EXPECT_GT(memory_id, 0);

    // Retrieve the memory
    auto retrieve_result = manager.get_memory(memory_id);
    ASSERT_TRUE(retrieve_result.isOk()) << "Failed to retrieve memory: " << retrieve_result.error();

    auto retrieved = retrieve_result.unwrap();
    EXPECT_EQ(retrieved.type, MemType::Fact);
    EXPECT_EQ(retrieved.content, "User prefers dark theme");
    EXPECT_DOUBLE_EQ(retrieved.importance, 0.8);
    EXPECT_EQ(retrieved.source_conversation_id, "test_conv_123");
    EXPECT_EQ(retrieved.source_message_id, 456);
}

TEST_F(MemoryManagerTest, AddMultipleMemories) {
    auto& manager = Memory::MemoryManager::instance();

    // Create multiple memories
    std::vector<Mem> memories;
    for (int i = 0; i < 5; ++i) {
        Mem memory = create_test_memory("Memory " + std::to_string(i), 0.5 + i / 10.0);
        memories.push_back(memory);
    }

    // Add memories
    for (const auto& mem : memories) {
        auto result = manager.add_memory(mem);
        ASSERT_TRUE(result.isOk()) << "Failed to add memory";
    }

    // Get all memories
    auto all_result = manager.get_all_memories(100);
    ASSERT_TRUE(all_result.isOk());

    auto all_memories = all_result.unwrap();
    EXPECT_GE(all_memories.size(), 5);
}

TEST_F(MemoryManagerTest, UpdateMemory) {
    auto& manager = Memory::MemoryManager::instance();

    // Create a memory
    Mem memory = create_test_memory("Original content");
    auto create_result = manager.add_memory(memory);
    ASSERT_TRUE(create_result.isOk());
    int64_t memory_id = create_result.unwrap();

    // Update the memory
    Mem updated = create_test_memory("Updated content", 0.9);
    auto update_result = manager.update_memory(memory_id, updated);
    ASSERT_TRUE(update_result.isOk()) << "Failed to update memory: " << update_result.error();

    // Verify update
    auto retrieve_result = manager.get_memory(memory_id);
    ASSERT_TRUE(retrieve_result.isOk());

    auto retrieved = retrieve_result.unwrap();
    EXPECT_EQ(retrieved.content, "Updated content");
    EXPECT_DOUBLE_EQ(retrieved.importance, 0.9);
}

TEST_F(MemoryManagerTest, DeleteMemory) {
    auto& manager = Memory::MemoryManager::instance();

    // Create a memory
    Mem memory = create_test_memory("To be deleted");
    auto create_result = manager.add_memory(memory);
    ASSERT_TRUE(create_result.isOk());
    int64_t memory_id = create_result.unwrap();

    // Delete the memory
    auto delete_result = manager.delete_memory(memory_id);
    ASSERT_TRUE(delete_result.isOk()) << "Failed to delete memory: " << delete_result.error();

    // Verify deletion
    auto retrieve_result = manager.get_memory(memory_id);
    ASSERT_TRUE(retrieve_result.isErr()) << "Memory should not exist after deletion";
}

TEST_F(MemoryManagerTest, GetNonExistentMemory) {
    auto& manager = Memory::MemoryManager::instance();

    auto result = manager.get_memory(999999);
    ASSERT_TRUE(result.isErr()) << "Should fail with non-existent memory";
}

// ============================================================================
// Search Tests
// ============================================================================

TEST_F(MemoryManagerTest, SearchMemoriesByType) {
    auto& manager = Memory::MemoryManager::instance();

    // Create test memories
    Mem pref_mem = create_test_memory("User prefers dark mode");
    pref_mem.type = MemType::Preference;

    Mem fact_mem = create_test_memory("System version is 1.0.0");
    fact_mem.type = MemType::Fact;

    manager.add_memory(pref_mem);
    manager.add_memory(fact_mem);

    // Search for preferences
    MemFilter filter;
    filter.type = MemType::Preference;
    filter.limit = 10;

    auto search_result = manager.search_memories(filter);
    ASSERT_TRUE(search_result.isOk()) << "Search failed: " << search_result.error();

    auto memories = search_result.unwrap();
    bool found_pref = false;
    for (const auto& mem : memories) {
        if (mem.type == MemType::Preference) {
            found_pref = true;
            break;
        }
    }
    EXPECT_TRUE(found_pref) << "Should find preference memory";
}

TEST_F(MemoryManagerTest, SearchMemoriesByImportance) {
    auto& manager = Memory::MemoryManager::instance();

    // Create memories with different importance levels
    for (int i = 0; i < 10; ++i) {
        Mem memory = create_test_memory("Fact " + std::to_string(i), 0.3 + i / 10.0);
        manager.add_memory(memory);
    }

    // Search for high importance memories
    MemFilter filter;
    filter.min_importance = 0.7;
    filter.limit = 10;

    auto result = manager.search_memories(filter);
    ASSERT_TRUE(result.isOk()) << "Search failed: " << result.error();

    auto memories = result.unwrap();
    for (const auto& mem : memories) {
        EXPECT_GE(mem.importance, 0.7) << "All memories should have importance >= 0.7";
    }
}

TEST_F(MemoryManagerTest, GetMemoriesByType) {
    auto& manager = Memory::MemoryManager::instance();

    // Create memories of different types
    Mem pref_mem = create_test_memory("Preference memory");
    pref_mem.type = MemType::Preference;

    Mem fact_mem = create_test_memory("Fact memory");
    fact_mem.type = MemType::Fact;

    manager.add_memory(pref_mem);
    manager.add_memory(fact_mem);

    // Get memories by type
    auto result = manager.get_memories_by_type(MemType::Preference, 10);
    ASSERT_TRUE(result.isOk()) << "Failed to get memories by type: " << result.error();

    auto memories = result.unwrap();
    bool found_pref = false;
    for (const auto& mem : memories) {
        if (mem.type == MemType::Preference) {
            found_pref = true;
            break;
        }
    }
    EXPECT_TRUE(found_pref) << "Should find preference memory";
}

TEST_F(MemoryManagerTest, GetMemoriesByImportance) {
    auto& manager = Memory::MemoryManager::instance();

    // Create memories with different importance
    for (int i = 0; i < 5; ++i) {
        Mem memory = create_test_memory("Memory " + std::to_string(i), 0.5 + i / 10.0);
        manager.add_memory(memory);
    }

    // Get memories with importance > 0.7
    auto result = manager.get_memories_by_importance(0.7, 10);
    ASSERT_TRUE(result.isOk()) << "Failed to get memories by importance: " << result.error();

    auto memories = result.unwrap();
    for (const auto& mem : memories) {
        EXPECT_GE(mem.importance, 0.7) << "All memories should have importance >= 0.7";
    }
}

TEST_F(MemoryManagerTest, GetTopMemories) {
    auto& manager = Memory::MemoryManager::instance();

    // Create memories with different importance and access counts
    for (int i = 0; i < 5; ++i) {
        Mem memory = create_test_memory("Memory " + std::to_string(i), 0.8 - i / 10.0);
        manager.add_memory(memory);
    }

    // Get top memories
    auto result = manager.get_top_memories(3);
    ASSERT_TRUE(result.isOk()) << "Failed to get top memories: " << result.error();

    auto memories = result.unwrap();
    EXPECT_LE(memories.size(), 3) << "Should return at most 3 memories";
}

// ============================================================================
// Statistics Tests
// ============================================================================

TEST_F(MemoryManagerTest, IncrementAccessCount) {
    auto& manager = Memory::MemoryManager::instance();

    // Create a memory
    Mem memory = create_test_memory("Test memory");
    auto create_result = manager.add_memory(memory);
    ASSERT_TRUE(create_result.isOk());
    int64_t memory_id = create_result.unwrap();

    // Get initial access count
    auto get_result = manager.get_memory(memory_id);
    ASSERT_TRUE(get_result.isOk());
    int initial_count = get_result.unwrap().accessed_count;

    // Increment access count
    auto inc_result = manager.increment_access_count(memory_id);
    ASSERT_TRUE(inc_result.isOk()) << "Failed to increment access count: " << inc_result.error();

    // Verify increment
    auto verify_result = manager.get_memory(memory_id);
    ASSERT_TRUE(verify_result.isOk());
    EXPECT_EQ(verify_result.unwrap().accessed_count, initial_count + 1);
}

TEST_F(MemoryManagerTest, UpdateImportance) {
    auto& manager = Memory::MemoryManager::instance();

    // Create a memory
    Mem memory = create_test_memory("Test memory", 0.5);
    auto create_result = manager.add_memory(memory);
    ASSERT_TRUE(create_result.isOk());
    int64_t memory_id = create_result.unwrap();

    // Update importance
    auto update_result = manager.update_importance(memory_id, 0.9);
    ASSERT_TRUE(update_result.isOk()) << "Failed to update importance: " << update_result.error();

    // Verify update
    auto verify_result = manager.get_memory(memory_id);
    ASSERT_TRUE(verify_result.isOk());
    EXPECT_DOUBLE_EQ(verify_result.unwrap().importance, 0.9);
}

TEST_F(MemoryManagerTest, GetMemoryCount) {
    auto& manager = Memory::MemoryManager::instance();

    // Add some memories
    for (int i = 0; i < 5; ++i) {
        Mem memory = create_test_memory("Memory " + std::to_string(i));
        manager.add_memory(memory);
    }

    // Get count
    auto result = manager.get_memory_count();
    ASSERT_TRUE(result.isOk()) << "Failed to get memory count: " << result.error();

    size_t count = result.unwrap();
    EXPECT_GE(count, 5) << "Should have at least 5 memories";
}

TEST_F(MemoryManagerTest, GetMemoryCountByType) {
    auto& manager = Memory::MemoryManager::instance();

    // Create memories of different types
    Mem pref_mem = create_test_memory("Preference");
    pref_mem.type = MemType::Preference;

    Mem fact_mem1 = create_test_memory("Fact 1");
    fact_mem1.type = MemType::Fact;

    Mem fact_mem2 = create_test_memory("Fact 2");
    fact_mem2.type = MemType::Fact;

    manager.add_memory(pref_mem);
    manager.add_memory(fact_mem1);
    manager.add_memory(fact_mem2);

    // Get count by type
    auto result = manager.get_memory_count_by_type();
    ASSERT_TRUE(result.isOk()) << "Failed to get count by type: " << result.error();

    auto counts = result.unwrap();
    bool found_fact = false;
    for (const auto& [type, count] : counts) {
        if (type == MemType::Fact && count >= 2) {
            found_fact = true;
            break;
        }
    }
    EXPECT_TRUE(found_fact) << "Should have at least 2 Fact memories";
}

// ============================================================================
// Error Handling Tests
// ============================================================================

TEST_F(MemoryManagerTest, UpdateNonExistentMemory) {
    auto& manager = Memory::MemoryManager::instance();

    Mem memory = create_test_memory("This doesn't exist");
    auto result = manager.update_memory(999999, memory);
    ASSERT_TRUE(result.isErr()) << "Should fail to update non-existent memory";
}

TEST_F(MemoryManagerTest, DeleteNonExistentMemory) {
    auto& manager = Memory::MemoryManager::instance();

    auto result = manager.delete_memory(999999);
    ASSERT_TRUE(result.isErr()) << "Should fail to delete non-existent memory";
}

TEST_F(MemoryManagerTest, IncrementCountNonExistentMemory) {
    auto& manager = Memory::MemoryManager::instance();

    auto result = manager.increment_access_count(999999);
    ASSERT_TRUE(result.isErr()) << "Should fail to increment count for non-existent memory";
}

TEST_F(MemoryManagerTest, UpdateImportanceNonExistentMemory) {
    auto& manager = Memory::MemoryManager::instance();

    auto result = manager.update_importance(999999, 0.9);
    ASSERT_TRUE(result.isErr()) << "Should fail to update importance for non-existent memory";
}
