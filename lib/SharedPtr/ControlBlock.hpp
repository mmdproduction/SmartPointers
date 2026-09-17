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
    using Type = std::remove_extent_t<T>;
        void deleter(Type* ptr) const noexcept {
            if constexpr (std::is_array_v<T>) {
                delete[] ptr;
            }
            else {
                delete ptr;
            }
        }
    Type* ptr_;

    public:
    ControlBlock(Type* ptr) noexcept: ptr_(ptr){}

    ControlBlock(const ControlBlock&) = delete;
    ControlBlock& operator= (const ControlBlock&) = delete;

    ~ControlBlock() override{
        deleter(ptr_);
    }
};