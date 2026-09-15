#include "ControlBlock.hpp"
#include <cstddef>

template<typename T>
class SharedPtr{
    private:
    ControlBlockBase* control_ = nullptr;
    T* storage_ptr_ = nullptr;

    void release_control(){
        if(control_){
            control_->shared_counter--;
            if(control_->shared_counter == 0){
                delete control_;
                
            }
        }
        control_ = nullptr;
    }

    public:

    SharedPtr() = default;
    
    SharedPtr(T* ptr){
        if(ptr == nullptr)
            control_ = nullptr;
        else
            control_ = new ControlBlock<T>(ptr);
    }

    SharedPtr(const SharedPtr<T>& other) noexcept: control_(other.control_){
        if(other.control_)
            control_->shared_counter++;
    }

    SharedPtr(SharedPtr<T>&& other) noexcept: control_(other.control_){
            other.control_ = nullptr;
    }

    SharedPtr& operator=(const SharedPtr<T>& other) noexcept{
        if(this == &other){
            return *this;
        }
        release_control();
        control_ = other.control_;
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
        other.control_ = nullptr;

        return *this;
    }

    std::size_t use_count() const noexcept{
        return control_ ? control_->shared_counter : 0;
    }


    
    ~SharedPtr(){
        release_control();
    }
};