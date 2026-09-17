#include <gtest/gtest.h>

#include "UniquePtr.hpp"

#include <memory>
#include <type_traits>
#include <utility>

namespace {

struct Tracked {
    static inline int alive = 0;
    static inline int destroyed = 0;

    int value;

    explicit Tracked(int value = 0) : value(value) {
        ++alive;
    }

    ~Tracked() {
        --alive;
        ++destroyed;
    }

    Tracked(const Tracked&) = delete;
    Tracked& operator=(const Tracked&) = delete;
};

class UniquePtrTest : public ::testing::Test {
protected:
    void SetUp() override {
        Tracked::alive = 0;
        Tracked::destroyed = 0;
    }

    void TearDown() override {
        EXPECT_EQ(Tracked::alive, 0);
    }
};

static_assert(!std::is_copy_constructible_v<UniquePtr<int>>);
static_assert(!std::is_copy_assignable_v<UniquePtr<int>>);
static_assert(std::is_nothrow_move_constructible_v<UniquePtr<int>>);
static_assert(std::is_nothrow_move_assignable_v<UniquePtr<int>>);

TEST_F(UniquePtrTest, DefaultConstructorCreatesEmptyPointer) {
    UniquePtr<int> ptr;

    EXPECT_EQ(ptr.get(), nullptr);
    EXPECT_FALSE(ptr);
}

TEST_F(UniquePtrTest, NullptrConstructorCreatesEmptyPointer) {
    UniquePtr<int> ptr(nullptr);

    EXPECT_EQ(ptr.get(), nullptr);
    EXPECT_FALSE(ptr);
}

TEST_F(UniquePtrTest, OwnsObjectAndDestroysItOnScopeExit) {
    {
        UniquePtr<Tracked> ptr(new Tracked(42));

        ASSERT_TRUE(ptr);
        EXPECT_EQ(Tracked::alive, 1);
        EXPECT_EQ(ptr->value, 42);
        EXPECT_EQ((*ptr).value, 42);

        ptr->value = 100;
        EXPECT_EQ((*ptr).value, 100);
    }

    EXPECT_EQ(Tracked::destroyed, 1);
}

TEST_F(UniquePtrTest, MoveConstructorTransfersOwnership) {
    {
        UniquePtr<Tracked> source(new Tracked(42));
        Tracked* raw = source.get();

        UniquePtr<Tracked> destination(std::move(source));

        EXPECT_EQ(source.get(), nullptr);
        EXPECT_FALSE(source);
        EXPECT_EQ(destination.get(), raw);
        EXPECT_EQ(Tracked::alive, 1);
        EXPECT_EQ(Tracked::destroyed, 0);
    }

    EXPECT_EQ(Tracked::destroyed, 1);
}

TEST_F(UniquePtrTest, MoveAssignmentDestroysPreviousObject) {
    {
        UniquePtr<Tracked> source(new Tracked(42));
        UniquePtr<Tracked> destination(new Tracked(10));
        Tracked* raw = source.get();

        auto& result = (destination = std::move(source));

        EXPECT_EQ(&result, &destination);
        EXPECT_EQ(source.get(), nullptr);
        EXPECT_EQ(destination.get(), raw);
        EXPECT_EQ(Tracked::alive, 1);
        EXPECT_EQ(Tracked::destroyed, 1);
    }

    EXPECT_EQ(Tracked::destroyed, 2);
}

TEST_F(UniquePtrTest, MovingEmptyPointerClearsDestination) {
    UniquePtr<Tracked> source;
    UniquePtr<Tracked> destination(new Tracked);

    destination = std::move(source);

    EXPECT_FALSE(source);
    EXPECT_FALSE(destination);
    EXPECT_EQ(Tracked::alive, 0);
    EXPECT_EQ(Tracked::destroyed, 1);
}

TEST_F(UniquePtrTest, MoveConstructionFromEmptyPointerIsSafe) {
    UniquePtr<Tracked> source;
    UniquePtr<Tracked> destination(std::move(source));

    EXPECT_FALSE(source);
    EXPECT_FALSE(destination);
    EXPECT_EQ(Tracked::destroyed, 0);
}

// Проверяет выбранное в твоей реализации поведение self-move.
TEST_F(UniquePtrTest, SelfMovePreservesOwnership) {
    UniquePtr<Tracked> ptr(new Tracked(42));
    Tracked* raw = ptr.get();
    auto& alias = ptr;

    ptr = std::move(alias);

    EXPECT_EQ(ptr.get(), raw);
    EXPECT_EQ(Tracked::alive, 1);
    EXPECT_EQ(Tracked::destroyed, 0);
}

TEST_F(UniquePtrTest, ReleaseTransfersResponsibilityWithoutDeleting) {
    std::unique_ptr<Tracked> owner;

    {
        UniquePtr<Tracked> ptr(new Tracked(42));
        Tracked* original = ptr.get();

        owner.reset(ptr.release());

        EXPECT_EQ(owner.get(), original);
        EXPECT_FALSE(ptr);
        EXPECT_EQ(Tracked::alive, 1);
        EXPECT_EQ(Tracked::destroyed, 0);
    }

    // Уничтожение опустевшего UniquePtr не удалило объект.
    EXPECT_EQ(Tracked::destroyed, 0);

    owner.reset();
    EXPECT_EQ(Tracked::destroyed, 1);
}

TEST_F(UniquePtrTest, ReleaseEmptyPointerReturnsNullptr) {
    UniquePtr<Tracked> ptr;

    EXPECT_EQ(ptr.release(), nullptr);
    EXPECT_FALSE(ptr);
}

TEST_F(UniquePtrTest, ResetDestroysObjectAndClearsPointer) {
    UniquePtr<Tracked> ptr(new Tracked);

    ptr.reset();

    EXPECT_EQ(ptr.get(), nullptr);
    EXPECT_EQ(Tracked::alive, 0);
    EXPECT_EQ(Tracked::destroyed, 1);

    ptr.reset();

    EXPECT_EQ(Tracked::destroyed, 1);
}

TEST_F(UniquePtrTest, ResetReplacesOwnedObject) {
    {
        UniquePtr<Tracked> ptr(new Tracked(10));
        auto* replacement = new Tracked(42);

        ptr.reset(replacement);

        ASSERT_EQ(ptr.get(), replacement);
        EXPECT_EQ(ptr->value, 42);
        EXPECT_EQ(Tracked::alive, 1);
        EXPECT_EQ(Tracked::destroyed, 1);
    }

    EXPECT_EQ(Tracked::destroyed, 2);
}

TEST_F(UniquePtrTest, ResetEmptyPointerTakesOwnership) {
    UniquePtr<Tracked> ptr;
    auto* raw = new Tracked(42);

    ptr.reset(raw);

    EXPECT_EQ(ptr.get(), raw);
    EXPECT_EQ(Tracked::alive, 1);
    EXPECT_EQ(Tracked::destroyed, 0);
}

TEST_F(UniquePtrTest, MakeUniqueForwardsConstructorArguments) {
    auto ptr = UniquePtr<Tracked>::make_unique(42);

    ASSERT_TRUE(ptr);
    EXPECT_EQ(ptr->value, 42);
    EXPECT_EQ(Tracked::alive, 1);
}

TEST_F(UniquePtrTest, MakeUniqueAcceptsMoveOnlyArgument) {
    struct Holder {
        std::unique_ptr<int> value;

        explicit Holder(std::unique_ptr<int> input)
            : value(std::move(input)) {}
    };

    auto input = std::make_unique<int>(42);
    auto ptr = UniquePtr<Holder>::make_unique(std::move(input));

    EXPECT_EQ(input.get(), nullptr);
    ASSERT_TRUE(ptr);
    ASSERT_NE(ptr->value.get(), nullptr);
    EXPECT_EQ(*ptr->value, 42);
}

TEST_F(UniquePtrTest, ConstOwnerAllowsAccessToMutableObject) {
    const UniquePtr<Tracked> ptr(new Tracked(42));

    ptr->value = 100;

    EXPECT_EQ((*ptr).value, 100);
    EXPECT_EQ(ptr.get()->value, 100);
}

TEST_F(UniquePtrTest, ArraySupportsIndexing) {
    UniquePtr<int[]> ptr(new int[3]{10, 20, 30});

    EXPECT_EQ(ptr[0], 10);
    EXPECT_EQ(ptr[1], 20);
    EXPECT_EQ(ptr[2], 30);

    ptr[1] = 42;

    EXPECT_EQ(ptr.get()[1], 42);
}

TEST_F(UniquePtrTest, ArrayDestructionDestroysEveryElement) {
    {
        UniquePtr<Tracked[]> ptr(new Tracked[3]);

        EXPECT_EQ(Tracked::alive, 3);
    }

    EXPECT_EQ(Tracked::destroyed, 3);
}

TEST_F(UniquePtrTest, MakeUniqueArrayZeroInitializesIntegers) {
    auto ptr = UniquePtr<int[]>::make_unique(4);

    ASSERT_TRUE(ptr);

    for (std::size_t i = 0; i < 4; ++i) {
        EXPECT_EQ(ptr[i], 0);
    }
}

TEST_F(UniquePtrTest, ArrayMoveAssignmentDestroysOldArray) {
    {
        UniquePtr<Tracked[]> source(new Tracked[3]);
        UniquePtr<Tracked[]> destination(new Tracked[2]);
        Tracked* raw = source.get();

        destination = std::move(source);

        EXPECT_FALSE(source);
        EXPECT_EQ(destination.get(), raw);
        EXPECT_EQ(Tracked::alive, 3);
        EXPECT_EQ(Tracked::destroyed, 2);
    }

    EXPECT_EQ(Tracked::destroyed, 5);
}

TEST_F(UniquePtrTest, ArrayResetDestroysEveryElement) {
    UniquePtr<Tracked[]> ptr(new Tracked[3]);

    ptr.reset();

    EXPECT_FALSE(ptr);
    EXPECT_EQ(Tracked::alive, 0);
    EXPECT_EQ(Tracked::destroyed, 3);
}

}