// tests/test_allocator.cpp
// Tests for kalloc<T>(), kalloc_array<T>(), kfree_obj(), and GFP flag selection.
#include "cpp_lkm/runtime/kalloc.hpp"
#include "tests/support/mock_globals.hpp"

#include <cassert>
#include <cerrno>
#include <cstdio>
#include <utility>

class TestObj
{
  public:
    static int destructor_count;

    explicit TestObj(int val) : x_(val) {}
    ~TestObj()
    {
        ++destructor_count;
    }

    TestObj(const TestObj&) = delete;
    TestObj& operator=(const TestObj&) = delete;
    TestObj(TestObj&&) = delete;
    TestObj& operator=(TestObj&&) = delete;

    [[nodiscard]] int value() const noexcept
    {
        return x_;
    }

  private:
    int x_{};
};

int TestObj::destructor_count = 0;

// ---- GFP flag selection ----

static void test_kalloc_success()
{
    reset_mock_state();
    auto ptr = kalloc<TestObj>(42);
    assert(ptr.has_value());
    assert(ptr.value()->value() == 42);
    assert(g_mock_last_gfp == CPP_GFP_KERNEL);
    kfree_obj(*ptr);
}

static void test_kalloc_atomic()
{
    reset_mock_state();
    g_mock_preempt_count = 1; // in_atomic() → true
    auto ptr = kalloc<TestObj>(10);
    assert(ptr.has_value());
    assert(g_mock_last_gfp == CPP_GFP_ATOMIC);
    kfree_obj(*ptr);
}

static void test_kalloc_irqs_disabled()
{
    reset_mock_state();
    g_mock_irqs_disabled = 1;
    auto ptr = kalloc<TestObj>(10);
    assert(ptr.has_value());
    assert(g_mock_last_gfp == CPP_GFP_ATOMIC);
    kfree_obj(*ptr);
}

static void test_kalloc_nmi()
{
    reset_mock_state();
    g_mock_in_nmi = 1;
    auto ptr = kalloc<TestObj>(10);
    assert(ptr.has_value());
    assert(g_mock_last_gfp == CPP_GFP_ATOMIC);
    kfree_obj(*ptr);
}

// ---- Allocation failure ----

static void test_kalloc_fail()
{
    reset_mock_state();
    g_mock_kmalloc_fail = 1;
    auto ptr = kalloc<TestObj>(99);
    assert(!ptr.has_value());
    assert(ptr.error() == -ENOMEM);
}

static void test_kalloc_array_fail()
{
    reset_mock_state();
    g_mock_kmalloc_fail = 1;
    auto arr = kalloc_array<int>(4);
    assert(!arr.has_value());
    assert(arr.error() == -ENOMEM);
}

static void test_kalloc_array_overflow_guard()
{
    reset_mock_state();
    auto arr = kalloc_array<int>(static_cast<size_t>(-1));
    assert(!arr.has_value());
    assert(arr.error() == -ENOMEM);
}

// ---- kfree_obj ----

static void test_kfree_obj_null()
{
    TestObj* ptr = nullptr;
    kfree_obj(ptr); // Must be a silent no-op
}

static void test_kfree_obj_calls_destructor()
{
    reset_mock_state();
    TestObj::destructor_count = 0;
    auto result = kalloc<TestObj>(7);
    assert(result.has_value());
    TestObj* raw = *result;
    assert(raw->value() == 7);
    kfree_obj(raw);
    assert(TestObj::destructor_count == 1);
}

// ---- KOwned / kalloc_owned ----

static void test_kalloc_owned_success()
{
    reset_mock_state();
    TestObj::destructor_count = 0;
    {
        auto owned_result = kalloc_owned<TestObj>(42);
        assert(owned_result.has_value());
        KOwned<TestObj> owned = std::move(*owned_result);
        assert(owned);
        assert(owned->value() == 42);
    }
    assert(TestObj::destructor_count == 1);
}

static void test_kalloc_owned_fail()
{
    reset_mock_state();
    g_mock_kmalloc_fail = 1;
    auto owned = kalloc_owned<TestObj>(99);
    assert(!owned.has_value());
    assert(owned.error() == -ENOMEM);
}

static void test_kowned_move_semantics()
{
    reset_mock_state();
    TestObj::destructor_count = 0;
    {
        auto owned_result = kalloc_owned<TestObj>(7);
        assert(owned_result.has_value());
        KOwned<TestObj> first = std::move(*owned_result);
        assert(first);
        KOwned<TestObj> second = std::move(first);
        assert(second);
        assert(second->value() == 7);
        KOwned<TestObj> third;
        third = std::move(second);
        assert(third);
        assert(third->value() == 7);
    }
    assert(TestObj::destructor_count == 1);
}

static void test_kowned_reset()
{
    reset_mock_state();
    TestObj::destructor_count = 0;
    auto owned_result = kalloc_owned<TestObj>(11);
    assert(owned_result.has_value());
    KOwned<TestObj> owned = std::move(*owned_result);
    owned.reset();
    assert(TestObj::destructor_count == 1);
    owned.reset();
    assert(TestObj::destructor_count == 1);
}

static void test_kowned_release()
{
    reset_mock_state();
    TestObj::destructor_count = 0;
    auto owned_result = kalloc_owned<TestObj>(13);
    assert(owned_result.has_value());
    KOwned<TestObj> owned = std::move(*owned_result);
    TestObj* raw = owned.release();
    assert(raw != nullptr);
    assert(!owned);
    assert(TestObj::destructor_count == 0);
    kfree_obj(raw);
    assert(TestObj::destructor_count == 1);
}

// ---- Array allocation ----

static void test_kalloc_array_success()
{
    reset_mock_state();
    auto arr = kalloc_array<int>(10);
    assert(arr.has_value());
    int* raw = *arr;
    for (int idx = 0; idx < 10; ++idx)
    {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        raw[idx] = idx;
    }
    cpp_kfree(raw);
}

static void test_gfp_priority_with_multiple_signals()
{
    reset_mock_state();
    g_mock_preempt_count = 1;
    g_mock_irqs_disabled = 1;
    g_mock_in_nmi = 1;
    auto ptr = kalloc<TestObj>(5);
    assert(ptr.has_value());
    assert(g_mock_last_gfp == CPP_GFP_ATOMIC);
    kfree_obj(*ptr);
}

extern "C" int main()
{
    test_kalloc_success();
    test_kalloc_atomic();
    test_kalloc_irqs_disabled();
    test_kalloc_nmi();
    test_gfp_priority_with_multiple_signals();
    test_kalloc_fail();
    test_kalloc_array_fail();
    test_kalloc_array_overflow_guard();
    test_kfree_obj_null();
    test_kfree_obj_calls_destructor();
    test_kalloc_owned_success();
    test_kalloc_owned_fail();
    test_kowned_move_semantics();
    test_kowned_reset();
    test_kowned_release();
    test_kalloc_array_success();
    printf("All allocator tests passed!\n");
    return 0;
}
