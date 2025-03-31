#pragma once

#include <cassert>
#include <cstdlib>
#include <new>
#include <utility>
#include <memory>

template <typename T>
class RawMemory {
public:

    RawMemory() = default;

    explicit RawMemory(size_t capacity)
        : buffer_(Allocate(capacity))
        , capacity_(capacity) {
    }

    RawMemory(const RawMemory&) = delete;
    RawMemory& operator=(const RawMemory&) = delete;
    
    RawMemory(RawMemory&& other) noexcept
        : buffer_(Allocate(other.capacity_)) {

        std::uninitialized_move_n(other.buffer_, other.capacity_, buffer_);

    }

    RawMemory& operator=(RawMemory&& rhs) noexcept {  
        buffer_ = rhs.buffer_;
        capacity_ = rhs.capacity_;
        rhs.buffer_ = nullptr;
        rhs.capacity_ = 0;
        return *this;
    }

    ~RawMemory() {
        Deallocate(buffer_);
    }

    T* operator+(size_t offset) noexcept {
        // Разрешается получать адрес ячейки памяти, следующей за последним элементом массива
        assert(offset <= capacity_);
        return buffer_ + offset;
    }

    const T* operator+(size_t offset) const noexcept {
        return const_cast<RawMemory&>(*this) + offset;
    }

    const T& operator[](size_t index) const noexcept {
        return const_cast<RawMemory&>(*this)[index];
    }

    T& operator[](size_t index) noexcept {
        assert(index < capacity_);
        return buffer_[index];
    }

    void Swap(RawMemory& other) noexcept {
        std::swap(buffer_, other.buffer_);
        std::swap(capacity_, other.capacity_);
    }

    const T* GetAddress() const noexcept {
        return buffer_;
    }

    T* GetAddress() noexcept {
        return buffer_;
    }

    size_t Capacity() const {
        return capacity_;
    }

private:
    // Выделяет сырую память под n элементов и возвращает указатель на неё
    static T* Allocate(size_t n) {
        return n != 0 ? static_cast<T*>(operator new(n * sizeof(T))) : nullptr;
    }

    // Освобождает сырую память, выделенную ранее по адресу buf при помощи Allocate
    static void Deallocate(T* buf) noexcept {
        operator delete(buf);
    }

    T* buffer_ = nullptr;
    size_t capacity_ = 0;

};


template <typename T>
class Vector {
public:

    using iterator = T*;
    using const_iterator = const T*;

    Vector() = default;

    explicit Vector(size_t size)
        : data_(size)
        , size_(size)
    {
        std::uninitialized_value_construct_n(data_.GetAddress(), size_);
    }

    Vector(const Vector& other)
        : data_(other.size_)
        , size_(other.size_)
    {
        std::uninitialized_copy_n(other.data_.GetAddress(), size_, data_.GetAddress());
    }

    Vector(Vector&& other) noexcept {
        Swap(other);
    }

    iterator begin() noexcept {
        return data_.GetAddress();
    }

    iterator end() noexcept {
        return data_.GetAddress() + size_;
    }

    const_iterator begin() const noexcept {
        return data_.GetAddress();
    }

    const_iterator end() const noexcept {
        return data_.GetAddress() + size_;
    }

    const_iterator cbegin() const noexcept {
        return begin();
    }

    const_iterator cend() const noexcept {
        return end();
    }

    Vector& operator=(const Vector& rhs) {
        if (this != &rhs) {
            if (rhs.size_ > data_.Capacity()) {
                Vector rhs_copy(rhs);
                Swap(rhs_copy);
            }
            else {
                if (rhs.size_ < size_) {
                    std::copy(rhs.data_.GetAddress(), rhs.data_.GetAddress() + rhs.size_, data_.GetAddress());
                    std::destroy_n(data_.GetAddress() + rhs.size_, size_ - rhs.size_);
                }
                else {
                    std::copy(rhs.data_.GetAddress(), rhs.data_.GetAddress() + size_, data_.GetAddress());
                    std::uninitialized_copy_n(rhs.data_.GetAddress() + size_, rhs.size_ - size_, data_.GetAddress() + size_);
                }
                size_ = rhs.size_;
            }
        }
        return *this;
    }

    Vector& operator=(Vector&& rhs) noexcept {
        if (this != &rhs) {
            if (rhs.size_ > data_.Capacity()) {
                Vector rhs_copy(std::move(rhs));
                Swap(rhs_copy);
            }
            else {
                std::uninitialized_move_n(rhs.data_.GetAddress(), rhs.size_, data_.GetAddress());
                std::destroy_n(data_.GetAddress() + rhs.size_, data_.Capacity() - rhs.size_);
                size_ = rhs.size_;
            }
        }
        return *this;
    }

    void Swap(Vector& other) noexcept {
        data_.Swap(other.data_);
        std::swap(size_, other.size_);
    }

    size_t Size() const noexcept {
        return size_;
    }

    size_t Capacity() const noexcept {
        return data_.Capacity();
    }

    const T& operator[](size_t index) const noexcept {
        return const_cast<Vector&>(*this)[index];
    }

    T& operator[](size_t index) noexcept {
        assert(index < size_);
        return data_[index];
    }

    void Reserve(size_t new_capacity) {
        if (new_capacity <= data_.Capacity()) {
            return;
        }

        RawMemory<T> new_data(new_capacity);

        if constexpr (std::is_nothrow_move_constructible_v<T> || std::is_copy_constructible_v<T>) {
            std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
        }
        else {
            std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
        }
        std::destroy_n(data_.GetAddress(), size_);
        data_.Swap(new_data);

    }

    void Resize(size_t new_size) {
        if (size_ > new_size) {
            std::destroy_n(data_.GetAddress() + new_size, size_ - new_size);
            size_ = new_size;
        }
        else if (size_ < new_size) {
            Reserve(new_size);
            std::uninitialized_value_construct_n(data_.GetAddress() + size_, new_size);
            size_ = new_size;
        }
    }

    template<typename Ref>
    void PushBack(Ref&& value) {
        if (size_ < data_.Capacity()) {
            new (data_.GetAddress() + size_) T(std::forward<Ref>(value));
            ++size_;
        }
        else {
            RawMemory<T> new_data(Capacity() == 0 ? 1 : Capacity() * 2);
            new(new_data.GetAddress() + size_) T(std::forward<Ref>(value));
            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
            }
            else {
                std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
            }
            std::destroy_n(data_.GetAddress(), size_);
            data_ = std::move(new_data);
            ++size_;
        }
    }

    void PopBack() {
        std::destroy_n(data_.GetAddress() + size_ - 1, 1);
        --size_;
    }

    template<typename ... Args>
    T& EmplaceBack(Args&& ... args) {

        T* result_ptr = nullptr;

        if (size_ < data_.Capacity()) {
            result_ptr = new (data_.GetAddress() + size_) T(std::forward<Args>(args)...);
            ++size_;
        }
        else {
            RawMemory<T> new_data(Capacity() == 0 ? 1 : Capacity() * 2);
            result_ptr = new(new_data.GetAddress() + size_) T(std::forward<Args>(args)...);
            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
            }
            else {
                std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
            }
            std::destroy_n(data_.GetAddress(), size_);
            data_ = std::move(new_data);
            ++size_;
        }

        return *result_ptr;

    }

    template <typename... Args>
    iterator Emplace(const_iterator pos, Args&&... args) {

        iterator result = nullptr;
        size_t shift = pos - begin();
        if (size_ == Capacity()) {
            RawMemory<T> new_data(size_ == 0 ? 1 : size_ * 2);
            result = new (new_data + shift) T(std::forward<Args>(args)...);
            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                std::uninitialized_move_n(begin(), shift, new_data.GetAddress());
                std::uninitialized_move_n(begin() + shift, size_ - shift, new_data.GetAddress() + shift + 1);
            }
            else {
                try {
                    std::uninitialized_copy_n(begin(), shift, new_data.GetAddress());
                    std::uninitialized_copy_n(begin() + shift, size_ - shift, new_data.GetAddress() + shift + 1);
                }
                catch (...) {
                    std::destroy_n(new_data.GetAddress() + shift, 1);
                    throw;
                }
            }
            std::destroy_n(begin(), size_);
            data_.Swap(new_data);
        }
        else {
            if (size_ != 0) {
                new (data_ + size_) T(std::move(*(end() - 1)));
                try {
                    std::move_backward(begin() + shift, end(), end() + 1);
                }
                catch (...) {
                    std::destroy_n(end(), 1);
                    throw;
                }
                std::destroy_at(begin() + shift);
            }
            result = new (data_ + shift) T(std::forward<Args>(args)...);
        }
        ++size_;
        return result;

    }

    iterator Erase(const_iterator pos)/*noexcept(std::is_nothrow_move_assignable_v<T>)*/ {
        
        size_t shift = pos - begin();
        std::move(begin() + shift + 1, end(), begin() + shift);
        PopBack();

        return begin() + shift;

    }

    iterator Insert(const_iterator pos, const T& value) {
        return Emplace(pos, value);
    }

    iterator Insert(const_iterator pos, T&& value) {
        return Emplace(pos, std::move(value));
    }

    ~Vector() {
        std::destroy_n(data_.GetAddress(), size_);
    }

private:
    RawMemory<T> data_;
    size_t size_ = 0;
};