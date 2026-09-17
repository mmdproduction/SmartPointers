#include <gtest/gtest.h>

#include "SharedPtr.hpp"

#include <memory>
#include <type_traits>
#include <utility>

namespace {

struct Tracked {
    static inline int alive = 0;
    static inline int destroyed = 0;

    int value;

    Tracked() : Tracked(0) {}

    explicit Tracked(int value) : value(value) {
        ++alive;
    }

    ~Tracked() {
        --alive;
        ++destroyed;
    }

    Tracked(const Tracked&) = delete;
    Tracked& operator=(const Tracked&) = delete;
};

class SharedPtrTest : public ::testing::Test {
protected:
    void SetUp() override {
        Tracked::alive = 0;
        Tracked::destroyed = 0;
    }

    void TearDown() override {
        EXPECT_EQ(Tracked::alive, 0);
    }
};

static_assert(std::is_nothrow_copy_constructible_v<SharedPtr<int>>);
static_assert(std::is_nothrow_move_constructible_v<SharedPtr<int>>);
static_assert(std::is_nothrow_copy_assignable_v<SharedPtr<int>>);
static_assert(std::is_nothrow_move_assignable_v<SharedPtr<int>>);

// static_assert(!std::is_convertible_v<int*, SharedPtr<int>>);
static_assert(!std::is_convertible_v<SharedPtr<int>, bool>);

TEST_F(SharedPtrTest, DefaultConstructorCreatesEmptyPointer) {
    SharedPtr<int> ptr;

    EXPECT_EQ(ptr.get(), nullptr);
    EXPECT_EQ(ptr.use_count(), 0u);
    EXPECT_FALSE(ptr);
}

TEST_F(SharedPtrTest, NullptrConstructorCreatesEmptyPointer) {
    SharedPtr<int> ptr(nullptr);

    EXPECT_EQ(ptr.get(), nullptr);
    EXPECT_EQ(ptr.use_count(), 0u);
    EXPECT_FALSE(ptr);
}

TEST_F(SharedPtrTest, OwnsAndDestroysObject) {
    {
        SharedPtr<Tracked> ptr(new Tracked(42));

        ASSERT_TRUE(ptr);
        EXPECT_EQ(ptr.use_count(), 1u);
        EXPECT_EQ(ptr->value, 42);
        EXPECT_EQ((*ptr).value, 42);
        EXPECT_EQ(Tracked::alive, 1);

        (*ptr).value = 100;
        EXPECT_EQ(ptr->value, 100);
    }

    EXPECT_EQ(Tracked::destroyed, 1);
}

TEST_F(SharedPtrTest, CopySharesObjectAndUpdatesCount) {
    {
        SharedPtr<Tracked> first(new Tracked(42));

        {
            SharedPtr<Tracked> second(first);

            EXPECT_EQ(first.get(), second.get());
            EXPECT_EQ(first.use_count(), 2u);
            EXPECT_EQ(second.use_count(), 2u);

            second->value = 100;
            EXPECT_EQ(first->value, 100);
        }

        EXPECT_EQ(first.use_count(), 1u);
        EXPECT_EQ(Tracked::destroyed, 0);
    }

    EXPECT_EQ(Tracked::destroyed, 1);
}

TEST_F(SharedPtrTest, CopyOutlivesOriginalOwner) {
    {
        SharedPtr<Tracked> survivor;

        {
            SharedPtr<Tracked> original(new Tracked(42));
            survivor = original;
        }

        ASSERT_TRUE(survivor);
        EXPECT_EQ(survivor->value, 42);
        EXPECT_EQ(survivor.use_count(), 1u);
        EXPECT_EQ(Tracked::destroyed, 0);
    }

    EXPECT_EQ(Tracked::destroyed, 1);
}

TEST_F(SharedPtrTest, CopyEmptyPointerIsSafe) {
    SharedPtr<Tracked> first;
    SharedPtr<Tracked> second(first);

    EXPECT_FALSE(first);
    EXPECT_FALSE(second);
    EXPECT_EQ(first.use_count(), 0u);
    EXPECT_EQ(second.use_count(), 0u);
}

TEST_F(SharedPtrTest, CopyAssignmentReleasesPreviousObject) {
    {
        SharedPtr<Tracked> source(new Tracked(42));
        SharedPtr<Tracked> destination(new Tracked(10));

        auto& result = (destination = source);

        EXPECT_EQ(&result, &destination);
        EXPECT_EQ(destination.get(), source.get());
        EXPECT_EQ(source.use_count(), 2u);
        EXPECT_EQ(destination.use_count(), 2u);
        EXPECT_EQ(Tracked::alive, 1);
        EXPECT_EQ(Tracked::destroyed, 1);
    }

    EXPECT_EQ(Tracked::destroyed, 2);
}

TEST_F(SharedPtrTest, CopyAssignmentPreservesOtherOwnersOfOldObject) {
    SharedPtr<Tracked> oldOwner(new Tracked(10));
    SharedPtr<Tracked> destination(oldOwner);
    SharedPtr<Tracked> source(new Tracked(42));

    destination = source;

    EXPECT_EQ(oldOwner.use_count(), 1u);
    EXPECT_EQ(oldOwner->value, 10);
    EXPECT_EQ(source.use_count(), 2u);
    EXPECT_EQ(destination.get(), source.get());
    EXPECT_EQ(Tracked::alive, 2);
    EXPECT_EQ(Tracked::destroyed, 0);
}

TEST_F(SharedPtrTest, CopyAssignmentBetweenCoOwnersPreservesCount) {
    SharedPtr<Tracked> first(new Tracked);
    SharedPtr<Tracked> second(first);

    second = first;

    EXPECT_EQ(first.get(), second.get());
    EXPECT_EQ(first.use_count(), 2u);
    EXPECT_EQ(second.use_count(), 2u);
    EXPECT_EQ(Tracked::destroyed, 0);
}

TEST_F(SharedPtrTest, CopyAssignmentFromEmptyReleasesObject) {
    SharedPtr<Tracked> source;
    SharedPtr<Tracked> destination(new Tracked);

    destination = source;

    EXPECT_FALSE(destination);
    EXPECT_EQ(destination.use_count(), 0u);
    EXPECT_EQ(Tracked::destroyed, 1);
}

TEST_F(SharedPtrTest, SelfCopyPreservesOwnership) {
    SharedPtr<Tracked> ptr(new Tracked);
    Tracked* raw = ptr.get();
    auto& alias = ptr;

    ptr = alias;

    EXPECT_EQ(ptr.get(), raw);
    EXPECT_EQ(ptr.use_count(), 1u);
    EXPECT_EQ(Tracked::destroyed, 0);
}

TEST_F(SharedPtrTest, MoveConstructorTransfersOwnership) {
    SharedPtr<Tracked> source(new Tracked(42));
    SharedPtr<Tracked> observer(source);
    Tracked* raw = source.get();

    SharedPtr<Tracked> destination(std::move(source));

    EXPECT_EQ(source.get(), nullptr);
    EXPECT_EQ(source.use_count(), 0u);
    EXPECT_EQ(destination.get(), raw);
    EXPECT_EQ(destination.use_count(), 2u);
    EXPECT_EQ(observer.use_count(), 2u);
    EXPECT_EQ(Tracked::destroyed, 0);
}

TEST_F(SharedPtrTest, MoveEmptyPointerIsSafe) {
    SharedPtr<Tracked> source;
    SharedPtr<Tracked> destination(std::move(source));

    EXPECT_FALSE(source);
    EXPECT_FALSE(destination);
    EXPECT_EQ(source.use_count(), 0u);
    EXPECT_EQ(destination.use_count(), 0u);
}

TEST_F(SharedPtrTest, MoveAssignmentReleasesPreviousObject) {
    {
        SharedPtr<Tracked> source(new Tracked(42));
        SharedPtr<Tracked> destination(new Tracked(10));
        Tracked* raw = source.get();

        auto& result = (destination = std::move(source));

        EXPECT_EQ(&result, &destination);
        EXPECT_EQ(source.get(), nullptr);
        EXPECT_EQ(source.use_count(), 0u);
        EXPECT_EQ(destination.get(), raw);
        EXPECT_EQ(destination.use_count(), 1u);
        EXPECT_EQ(Tracked::alive, 1);
        EXPECT_EQ(Tracked::destroyed, 1);
    }

    EXPECT_EQ(Tracked::destroyed, 2);
}

TEST_F(SharedPtrTest, MoveAssignmentBetweenCoOwnersReducesCount) {
    SharedPtr<Tracked> first(new Tracked(42));
    SharedPtr<Tracked> second(first);
    Tracked* raw = first.get();

    second = std::move(first);

    EXPECT_FALSE(first);
    EXPECT_EQ(first.use_count(), 0u);
    EXPECT_EQ(second.get(), raw);
    EXPECT_EQ(second.use_count(), 1u);
    EXPECT_EQ(Tracked::destroyed, 0);
}

TEST_F(SharedPtrTest, MoveAssignmentFromEmptyReleasesObject) {
    SharedPtr<Tracked> source;
    SharedPtr<Tracked> destination(new Tracked);

    destination = std::move(source);

    EXPECT_FALSE(source);
    EXPECT_FALSE(destination);
    EXPECT_EQ(destination.use_count(), 0u);
    EXPECT_EQ(Tracked::destroyed, 1);
}

TEST_F(SharedPtrTest, SelfMovePreservesOwnership) {
    SharedPtr<Tracked> ptr(new Tracked);
    Tracked* raw = ptr.get();
    auto& alias = ptr;

    ptr = std::move(alias);

    EXPECT_EQ(ptr.get(), raw);
    EXPECT_EQ(ptr.use_count(), 1u);
    EXPECT_EQ(Tracked::destroyed, 0);
}

TEST_F(SharedPtrTest, ConstOwnerAllowsObjectModification) {
    const SharedPtr<Tracked> ptr(new Tracked(42));

    ptr->value = 100;

    EXPECT_EQ((*ptr).value, 100);
}

// Намеренно невиртуальный деструктор.
struct Base {
    int value = 42;
    ~Base() = default;
};

struct Derived : Base {
    Tracked lifetime;
};

TEST_F(SharedPtrTest, RawPointerConstructorPreservesDerivedType) {
    {
        SharedPtr<Base> ptr(new Derived);

        EXPECT_EQ(ptr->value, 42);
        EXPECT_EQ(Tracked::alive, 1);
    }

    // Подобъект lifetime уничтожится только при удалении Derived.
    EXPECT_EQ(Tracked::destroyed, 1);
}

TEST_F(SharedPtrTest, ConvertingCopySharesExistingControlBlock) {
    {
        SharedPtr<Base> base;

        {
            SharedPtr<Derived> derived(new Derived);
            SharedPtr<Base> converted(derived);

            EXPECT_EQ(converted.get(),
                      static_cast<Base*>(derived.get()));
            EXPECT_EQ(derived.use_count(), 2u);
            EXPECT_EQ(converted.use_count(), 2u);

            base = converted;
        }

        EXPECT_EQ(base.use_count(), 1u);
        EXPECT_EQ(Tracked::alive, 1);
        EXPECT_EQ(Tracked::destroyed, 0);
    }

    EXPECT_EQ(Tracked::destroyed, 1);
}

TEST_F(SharedPtrTest, ConvertingMoveTransfersExistingControlBlock) {
    {
        SharedPtr<Derived> derived(new Derived);
        Base* raw = derived.get();

        SharedPtr<Base> base(std::move(derived));

        EXPECT_FALSE(derived);
        EXPECT_EQ(derived.use_count(), 0u);
        EXPECT_EQ(base.get(), raw);
        EXPECT_EQ(base.use_count(), 1u);
    }

    EXPECT_EQ(Tracked::destroyed, 1);
}

TEST_F(SharedPtrTest, ConvertingCopyCanAddConst) {
    SharedPtr<int> mutablePtr(new int(42));
    SharedPtr<const int> constPtr(mutablePtr);

    EXPECT_EQ(mutablePtr.get(), constPtr.get());
    EXPECT_EQ(constPtr.use_count(), 2u);

    *mutablePtr = 100;
    EXPECT_EQ(*constPtr, 100);
}

// Недопустимые преобразования.
static_assert(!std::is_constructible_v<
    SharedPtr<Derived>, const SharedPtr<Base>&>);

static_assert(!std::is_constructible_v<
    SharedPtr<int>, const SharedPtr<const int>&>);

static_assert(!std::is_constructible_v<
    SharedPtr<Base[]>, const SharedPtr<Derived[]>&>);

static_assert(!std::is_constructible_v<
    SharedPtr<Base[]>, Derived*>);

static_assert(!std::is_constructible_v<
    SharedPtr<int>, const SharedPtr<int[]>&>);

static_assert(!std::is_constructible_v<
    SharedPtr<int[]>, const SharedPtr<int>&>);

TEST_F(SharedPtrTest, ArraySupportsIndexingAndSharedChanges) {
    SharedPtr<int[]> first(new int[3]{10, 20, 30});
    SharedPtr<int[]> second(first);

    EXPECT_EQ(first.use_count(), 2u);
    EXPECT_EQ(first.get(), second.get());

    second[1] = 42;

    EXPECT_EQ(first[0], 10);
    EXPECT_EQ(first[1], 42);
    EXPECT_EQ(first[2], 30);
}

TEST_F(SharedPtrTest, ArrayLivesUntilLastOwnerIsDestroyed) {
    {
        SharedPtr<Tracked[]> survivor;

        {
            SharedPtr<Tracked[]> original(new Tracked[3]);
            survivor = original;

            EXPECT_EQ(original.use_count(), 2u);
        }

        EXPECT_EQ(survivor.use_count(), 1u);
        EXPECT_EQ(Tracked::alive, 3);
        EXPECT_EQ(Tracked::destroyed, 0);
    }

    EXPECT_EQ(Tracked::destroyed, 3);
}

TEST_F(SharedPtrTest, ArrayConvertingCopyCanAddConst) {
    SharedPtr<int[]> values(new int[2]{10, 20});
    SharedPtr<const int[]> readonly(values);

    EXPECT_EQ(values.get(), readonly.get());
    EXPECT_EQ(values.use_count(), 2u);
    EXPECT_EQ(readonly.use_count(), 2u);

    values[1] = 42;
    EXPECT_EQ(readonly[1], 42);
}

TEST_F(SharedPtrTest, ArrayConvertingMoveTransfersOwnership) {
    SharedPtr<int[]> values(new int[2]{10, 20});
    int* raw = values.get();

    SharedPtr<const int[]> readonly(std::move(values));

    EXPECT_FALSE(values);
    EXPECT_EQ(values.use_count(), 0u);
    EXPECT_EQ(readonly.get(), raw);
    EXPECT_EQ(readonly.use_count(), 1u);
    EXPECT_EQ(readonly[1], 20);
}

TEST_F(SharedPtrTest, MakeSharedConstructsObject) {
    {
        auto ptr = SharedPtr<Tracked>::make_shared(42);

        ASSERT_TRUE(ptr);
        EXPECT_EQ(ptr->value, 42);
        EXPECT_EQ(ptr.use_count(), 1u);
        EXPECT_EQ(Tracked::alive, 1);

        auto copy = ptr;
        EXPECT_EQ(copy.use_count(), 2u);
    }

    EXPECT_EQ(Tracked::destroyed, 1);
}

TEST_F(SharedPtrTest, MakeSharedForwardsMoveOnlyArgument) {
    struct Holder {
        std::unique_ptr<int> value;

        explicit Holder(std::unique_ptr<int> input)
            : value(std::move(input)) {}
    };

    auto input = std::make_unique<int>(42);
    auto ptr = SharedPtr<Holder>::make_shared(std::move(input));

    EXPECT_EQ(input.get(), nullptr);
    ASSERT_TRUE(ptr);
    ASSERT_NE(ptr->value.get(), nullptr);
    EXPECT_EQ(*ptr->value, 42);
}

TEST_F(SharedPtrTest, MakeSharedPreservesLvalueReference) {
    struct RefHolder {
        int& value;

        explicit RefHolder(int& input) : value(input) {}
    };

    int value = 10;
    auto ptr = SharedPtr<RefHolder>::make_shared(value);

    EXPECT_EQ(&ptr->value, &value);

    ptr->value = 42;
    EXPECT_EQ(value, 42);
}

struct ConstructionError {};

struct ThrowingObject {
    Tracked member;

    ThrowingObject() {
        throw ConstructionError{};
    }
};

TEST_F(SharedPtrTest, MakeSharedPropagatesConstructorException) {
    EXPECT_THROW(
        (void)SharedPtr<ThrowingObject>::make_shared(),
        ConstructionError
    );

    EXPECT_EQ(Tracked::alive, 0);
    EXPECT_EQ(Tracked::destroyed, 1);
}

TEST_F(SharedPtrTest, MakeSharedArrayZeroInitializesIntegers) {
    auto ptr = SharedPtr<int[]>::make_shared(4);

    ASSERT_TRUE(ptr);
    EXPECT_EQ(ptr.use_count(), 1u);

    for (std::size_t i = 0; i < 4; ++i) {
        EXPECT_EQ(ptr[i], 0);
    }
}

TEST_F(SharedPtrTest, MakeSharedArrayDestroysEveryElement) {
    {
        auto ptr = SharedPtr<Tracked[]>::make_shared(3);
        auto copy = ptr;

        EXPECT_EQ(Tracked::alive, 3);
        EXPECT_EQ(ptr.use_count(), 2u);
    }

    EXPECT_EQ(Tracked::destroyed, 3);
}

} 