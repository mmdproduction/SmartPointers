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

template<typename T>
class UniquePtr {
private:
    T* ptr_;
    Deleter<T> deleter_;
public:

    UniquePtr(T* ptr): ptr_(ptr){}

    UniquePtr(const UniquePtr<T>& other) = delete;
    UniquePtr<T> operator= (const UniquePtr<T>& other) = delete;

    template<typename... Args>
    static UniquePtr<T> make_unique(Args&&... args){
        T* ptr = new T(std::forward<Args>(args)...);
        return UniquePtr<T>(ptr);
    }

    T* release(){

    }
    void reset(T* ptr = nullptr){

    }



};