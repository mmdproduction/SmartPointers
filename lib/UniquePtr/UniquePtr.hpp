#pragma once

#include <type_traits>
#include <utility>

template <typename T>
struct Deleter{
    using type_element = std::remove_extent_t<T>;
    void operator()(type_element* ptr) const {
        if constexpr (std::is_array_v<T>) {
            delete[] ptr;
        }
        else {
            delete ptr;
        }
    }
};

template<typename T, typename  Deleter_t = Deleter<T>>
class UniquePtr {
    using type_element = std::remove_extent_t<T>;
private:
    type_element* ptr_;
    Deleter_t deleter_;
public:

    UniquePtr(type_element* ptr): ptr_(ptr){}

    UniquePtr(const UniquePtr<T, Deleter_t>& other) = delete;
    UniquePtr<T, Deleter_t>& operator= (const UniquePtr<T, Deleter_t>& other) = delete;

    UniquePtr(UniquePtr<T, Deleter_t>&& other) noexcept(std::is_nothrow_move_constructible_v<Deleter_t>) : ptr_(other.ptr_), deleter_(std::move(other.deleter_)){
        other.ptr_ = nullptr;
    }

    UniquePtr<T, Deleter_t>& operator= (UniquePtr<T, Deleter_t>&& other) noexcept(std::is_nothrow_move_assignable_v<Deleter_t>) {
        if(this == &other){
            return *this;
        }
        else if(ptr_!= nullptr){
            deleter_(ptr_);
        }
        
        ptr_ = other.ptr_;
        deleter_ = std::move(other.deleter_);
        other.ptr_ = nullptr;

        return *this;
    }

    template<typename... Args>
    static UniquePtr<T, Deleter_t> make_unique(Args&&... args){
        type_element* ptr = new T(std::forward<Args>(args)...);
        return UniquePtr<T, Deleter_t>(ptr);
    }

    type_element* release() noexcept {
        type_element* ptr = ptr_;
        ptr_ = nullptr;
        return ptr;
    }
    void reset(type_element* ptr = nullptr) noexcept {
        if(ptr_ != ptr){
            deleter_(ptr_);
            ptr_ = ptr;
        }
    }

    ~UniquePtr(){
        deleter_(ptr_);
    }


};