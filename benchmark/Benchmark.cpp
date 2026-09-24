#include "Benchmark.hpp"
#include "UniquePtr.hpp"
#include "SharedPtr.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string_view>
#include <utility>

namespace {

using Clock = std::chrono::steady_clock;

constexpr std::size_t Iterations = 1'000'000;
constexpr std::size_t Repeats = 7;


template<typename T>
inline void escape(T& value) noexcept {
    asm volatile("" : : "g"(std::addressof(value)) : "memory");
}


struct Raw {
    using Pointer = int*;
    static constexpr std::string_view name = "raw";

    static Pointer create() {
        return new int(42);
    }

    static Pointer make() {
        return new int(42);
    }

    static void destroy(Pointer& ptr) {
        delete ptr;
        ptr = nullptr;
    }
};

struct CustomUnique {
    using Pointer = UniquePtr<int>;
    static constexpr std::string_view name = "UniquePtr";

    static Pointer create() {
        return Pointer(new int(42));
    }

    static Pointer make() {
        return Pointer::make_unique(42);
    }

    static void destroy(Pointer&) noexcept {}
};

struct StandardUnique {
    using Pointer = std::unique_ptr<int>;
    static constexpr std::string_view name = "std::unique_ptr";

    static Pointer create() {
        return Pointer(new int(42));
    }

    static Pointer make() {
        return std::make_unique<int>(42);
    }

    static void destroy(Pointer&) noexcept {}
};

struct CustomShared {
    using Pointer = SharedPtr<int>;
    static constexpr std::string_view name = "SharedPtr";

    static Pointer create() {
        return Pointer(new int(42));
    }

    static Pointer make() {
        return Pointer::make_shared(42);
    }

    static void destroy(Pointer&) noexcept {}
};

struct StandardShared {
    using Pointer = std::shared_ptr<int>;
    static constexpr std::string_view name = "std::shared_ptr";

    static Pointer create() {
        return Pointer(new int(42));
    }

    static Pointer make() {
        return std::make_shared<int>(42);
    }

    static void destroy(Pointer&) noexcept {}
};


template<typename Function>
double measure(std::size_t count, Function&& operation) {
    const auto begin = Clock::now();

    for (std::size_t i = 0; i < count; ++i) {
        operation();
    }

    const auto end = Clock::now();

    const double nanoseconds =
        std::chrono::duration<double, std::nano>(end - begin).count();

    return nanoseconds / static_cast<double>(count);
}

using Runner = double (*)(std::size_t);

struct Entry {
    std::string_view name;
    Runner run;
};

template<std::size_t Count>
void runGroup(
    std::string_view title,
    const std::array<Entry, Count>& entries
) {
    std::array<std::array<double, Repeats>, Count> samples{};

    for (const auto& entry : entries) {
        entry.run(Iterations / 10);
    }

    for (std::size_t repeat = 0; repeat < Repeats; ++repeat) {
        for (std::size_t offset = 0; offset < Count; ++offset) {
            const std::size_t index = (repeat + offset) % Count;
            samples[index][repeat] = entries[index].run(Iterations);
        }
    }

    std::cout << "\n" << title << "\n";
    std::cout << std::left << std::setw(20) << "Pointer"
              << std::right << std::setw(16) << "median ns/iter"
              << std::setw(16) << "min ns/iter"
              << "\n";

    for (std::size_t i = 0; i < Count; ++i) {
        auto values = samples[i];
        std::sort(values.begin(), values.end());

        std::cout << std::left << std::setw(20) << entries[i].name
                  << std::right << std::setw(16) << values[Repeats / 2]
                  << std::setw(16) << values.front()
                  << "\n";
    }
}


template<typename Adapter, bool UseFactory>
double createDestroy(std::size_t count) {
    return measure(count, [] {
        auto ptr = [] {
            if constexpr (UseFactory) {
                return Adapter::make();
            } else {
                return Adapter::create();
            }
        }();

        escape(ptr);
        Adapter::destroy(ptr);
    });
}


template<typename Adapter>
double dereference(std::size_t count) {
    auto ptr = Adapter::create();

    const double elapsed = measure(count, [&] {
        escape(ptr);

        int value = *ptr;

        escape(value);
    });

    Adapter::destroy(ptr);
    return elapsed;
}


template<typename Adapter>
double moveRoundTrip(std::size_t count) {
    auto ptr = Adapter::create();

    const double elapsed = measure(count, [&] {
        auto temporary = std::move(ptr);
        escape(temporary);
        escape(ptr);

        ptr = std::move(temporary);
        escape(ptr);
        escape(temporary);
    });

    Adapter::destroy(ptr);
    return elapsed;
}


template<typename Adapter>
double copyDestroy(std::size_t count) {
    auto original = Adapter::create();

    const double elapsed = measure(count, [&] {
        auto copy = original;
        escape(copy);
    });

    Adapter::destroy(original);
    return elapsed;
}

double barrierBaseline(std::size_t count) {
    int value = 42;

    return measure(count, [&] {
        escape(value);
    });
}

} 

void run_benchmark() {
    std::cout << std::fixed << std::setprecision(3);
    std::cout << "Iterations: " << Iterations
              << ", repeats: " << Repeats << "\n";

    runGroup(
        "Loop + barrier baseline",
        std::array<Entry, 1>{{
            {"baseline", barrierBaseline},
        }}
    );

    runGroup(
        "new + destruction",
        std::array<Entry, 5>{{
            {Raw::name, createDestroy<Raw, false>},
            {CustomUnique::name, createDestroy<CustomUnique, false>},
            {StandardUnique::name, createDestroy<StandardUnique, false>},
            {CustomShared::name, createDestroy<CustomShared, false>},
            {StandardShared::name, createDestroy<StandardShared, false>},
        }}
    );

    runGroup(
        "make_* + destruction (raw uses new/delete)",
        std::array<Entry, 5>{{
            {Raw::name, createDestroy<Raw, true>},
            {CustomUnique::name, createDestroy<CustomUnique, true>},
            {StandardUnique::name, createDestroy<StandardUnique, true>},
            {CustomShared::name, createDestroy<CustomShared, true>},
            {StandardShared::name, createDestroy<StandardShared, true>},
        }}
    );

    runGroup(
        "Dereference",
        std::array<Entry, 5>{{
            {Raw::name, dereference<Raw>},
            {CustomUnique::name, dereference<CustomUnique>},
            {StandardUnique::name, dereference<StandardUnique>},
            {CustomShared::name, dereference<CustomShared>},
            {StandardShared::name, dereference<StandardShared>},
        }}
    );

    runGroup(
        "Move round trip (two transfers per iteration)",
        std::array<Entry, 5>{{
            {Raw::name, moveRoundTrip<Raw>},
            {CustomUnique::name, moveRoundTrip<CustomUnique>},
            {StandardUnique::name, moveRoundTrip<StandardUnique>},
            {CustomShared::name, moveRoundTrip<CustomShared>},
            {StandardShared::name, moveRoundTrip<StandardShared>},
        }}
    );

    runGroup(
        "Copy + copy destruction",
        std::array<Entry, 3>{{
            {Raw::name, copyDestroy<Raw>},
            {CustomShared::name, copyDestroy<CustomShared>},
            {StandardShared::name, copyDestroy<StandardShared>},
        }}
    );
}