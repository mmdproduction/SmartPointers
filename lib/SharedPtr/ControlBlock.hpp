#pragma once

#include <cstddef>
#include <type_traits>

template <typename T>
struct ControlBlock{
    private:
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

    public:
    
    Type* ptr_;
    std::size_t shared_count = 1;
    ControlBlock(Type* ptr): ptr_(ptr){}

    ~ControlBlock(){
        deleter_(ptr_);
    }

};