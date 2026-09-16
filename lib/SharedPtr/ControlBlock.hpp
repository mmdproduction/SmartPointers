#pragma once

#include <cstddef>
#include <type_traits>


class ControlBlockBase{
    public:
    std::size_t shared_counter = 1;
    virtual ~ControlBlockBase() = default;
};

template<typename T>
class ControlBlock : public ControlBlockBase{
    template<typename delT>
    struct Deleter{
        using Type = std::remove_extent_t<delT>;
        void operator()(Type* ptr) const noexcept {
            if constexpr (std::is_array_v<delT>) {
                delete[] ptr;
            }
            else {
                delete ptr;
            }
        }
    };

    using Type = std::remove_extent_t<T>;
    Deleter<T> deleter_;
    Type* ptr_;

    public:
    ControlBlock(Type* ptr) noexcept: ptr_(ptr){}

    ControlBlock(const ControlBlock&) = delete;
    ControlBlock& operator= (const ControlBlock&) = delete;

    ~ControlBlock() override{
        deleter_(ptr_);
    }
};