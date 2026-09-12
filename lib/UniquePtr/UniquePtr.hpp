#pragma once

#include <cstddef>
#include <type_traits>
#include <utility>

template <typename T>
struct Deleter{
    using type_element = std::remove_extent_t<T>;
    void operator()(type_element* ptr) const noexcept {
        if constexpr (std::is_array_v<T>) {
            delete[] ptr;
        }
        else {
            delete ptr;
        }
    }
};

template<typename T>
class UniquePtr {
    using Type = std::remove_extent_t<T>;
private:
    Type* ptr_ = nullptr;
    Deleter<T> deleter_;
public:
    UniquePtr() = default;

    UniquePtr(Type* ptr): ptr_(ptr){}

    UniquePtr(const UniquePtr<T>& other) = delete;
    UniquePtr<T>& operator= (const UniquePtr<T>& other) = delete;

    UniquePtr(UniquePtr<T>&& other) noexcept : ptr_(other.ptr_), deleter_(){
        other.ptr_ = nullptr;
    }

    UniquePtr<T>& operator= (UniquePtr<T>&& other) noexcept {
        if(this == &other){
            return *this;
        }
        if(ptr_!= nullptr){
            deleter_(ptr_);
        }
        
        ptr_ = other.ptr_;
        other.ptr_ = nullptr;

        return *this;
    }

    template<typename... Args>
    static UniquePtr<T> make_unique(Args&&... args) requires(!std::is_array_v<T>){
        return UniquePtr<T>(new T(std::forward<Args>(args)...));    
    }

    static UniquePtr<T> make_unique(std::size_t size) requires(std::is_array_v<T>){
        return UniquePtr<T>(new Type[size]{});    
    }

    Type* release() noexcept {
        Type* ptr = ptr_;
        ptr_ = nullptr;
        return ptr;
    }
    void reset(Type* ptr = nullptr) noexcept {
        if(ptr_ != ptr){
            deleter_(ptr_);
            ptr_ = ptr;
        }
    }

    Type* get() const noexcept{
        return ptr_;
    }

    Type& operator*() const requires(!std::is_array_v<T>) {
        return *ptr_;
    }

    Type* operator->() const noexcept requires(!std::is_array_v<T>){
        return ptr_;
    }

    Type& operator[](std::size_t index) const requires(std::is_array_v<T>){
        return ptr_[index];
    }

    explicit operator bool() const noexcept{
        return ptr_ != nullptr;
    }

    ~UniquePtr(){
        deleter_(ptr_);
    }


};