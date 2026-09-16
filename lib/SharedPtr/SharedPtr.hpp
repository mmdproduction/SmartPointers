#pragma once

#include "ControlBlock.hpp"
#include <cstddef>
#include <type_traits>



template<typename T>
class SharedPtr{
    private:
    using Type = std::remove_extent_t<T>;
    ControlBlockBase* control_ = nullptr;
    Type* storage_ptr_ = nullptr;

    void release_control() noexcept{
        if(control_ && --control_->shared_counter == 0){
            delete control_;    
        }
        control_ = nullptr;
        storage_ptr_ = nullptr;

    }



    template<typename U>
    static constexpr bool compatible =
        (!std::is_array_v<T> &&
        !std::is_array_v<U> &&
        std::is_convertible_v<U*, Type*>)
        ||
        (std::is_unbounded_array_v<T> &&
        std::is_unbounded_array_v<U> &&
        std::is_convertible_v<
            std::remove_extent_t<U>(*)[],
            Type(*)[]
        >);

    public:

    SharedPtr() = default;

    SharedPtr(std::nullptr_t) noexcept {}
    
    template<typename U>
    requires (!std::is_array_v<T> &&
              std::is_convertible_v<U*, Type*>)
    SharedPtr(U* ptr): storage_ptr_(ptr){
        if(ptr)
        {
            try {
                control_ = new ControlBlock<U>(ptr);
            } 
            catch (...) {
                delete ptr;
                throw;
            }
        }

    }

    template<typename U>
        requires (
            std::is_unbounded_array_v<T> &&
            std::is_convertible_v<U(*)[], Type(*)[]>
        )
    SharedPtr(U* ptr) : storage_ptr_(ptr) {
        if (ptr) {
            try {
                control_ = new ControlBlock<U[]>(ptr);
            } catch (...) {
                delete[] ptr;
                throw;
            }
        }
    }

    template<typename U>
        requires compatible<U>
    SharedPtr(const SharedPtr<U>& other) noexcept: control_(other.control_), storage_ptr_(other.storage_ptr_){
        if(other.control_)
            control_->shared_counter++;
    }

    template<typename U>
        requires compatible<U>
    SharedPtr(SharedPtr<U>&& other) noexcept: control_(other.control_), storage_ptr_(other.storage_ptr_){
            other.control_ = nullptr;
            other.storage_ptr_ = nullptr;
    }

    

    SharedPtr& operator=(const SharedPtr<T>& other) noexcept{
        if(this == &other){
            return *this;
        }
        release_control();
        control_ = other.control_;
        storage_ptr_ = other.storage_ptr_;
        if(other.control_)
            control_->shared_counter++;

        return *this;
    }

    SharedPtr& operator=(SharedPtr<T>&& other) noexcept{
        if(this == &other){
            return *this;
        }
        release_control();
        control_ = other.control_;
        storage_ptr_ = other.storage_ptr_; 

        other.control_ = nullptr;
        other.storage_ptr_ = nullptr;

        return *this;
    }

    std::size_t use_count() const noexcept{
        return control_ ? control_->shared_counter : 0;
    }

    Type* get() const noexcept{
        return storage_ptr_;
    }

    Type& operator*() const requires(!std::is_array_v<T>) {
        return *storage_ptr_;
    }

    Type* operator->() const noexcept requires(!std::is_array_v<T>){
        return storage_ptr_;
    }

    Type& operator[](std::size_t index) const requires(std::is_array_v<T>){
        return storage_ptr_[index];
    }

    explicit operator bool() const noexcept{
        return storage_ptr_ != nullptr;
    }

    ~SharedPtr() {
        release_control();
    }
};