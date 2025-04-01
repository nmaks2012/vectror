#pragma once

#include <stdexcept>
#include <utility>

// Исключение этого типа должно генерироватся при обращении к пустому optional
class BadOptionalAccess : public std::exception {
public:
    using exception::exception;

    virtual const char* what() const noexcept override {
        return "Bad optional access";
    }
};

template <typename T>
class Optional {
public:
    Optional() = default;
    Optional(const T& value);
    Optional(T&& value);
    Optional(const Optional& other);
    Optional(Optional&& other);

    Optional& operator=(const T& value);
    Optional& operator=(T&& rhs);
    Optional& operator=(const Optional& rhs);
    Optional& operator=(Optional&& rhs);

    ~Optional();

    bool HasValue() const;

    // Операторы * и -> не должны делать никаких проверок на пустоту Optional.
    // Эти проверки остаются на совести программиста
    T& operator*()&;
    T&& operator*()&&;
    const T& operator*() const&;
    T* operator->()&;
    const T* operator->() const&;

    // Метод Value() генерирует исключение BadOptionalAccess, если Optional пуст
    T& Value() &;
    T&& Value()&&;
    const T& Value() const&;    

    void Reset();

    template<typename... Params>
    void Emplace(Params&&... params) {
        if (HasValue()) {
            Reset();
        }
        ptr_ = new(&data_[0]) T(std::forward<Params>(params)...);
    }

private:
    // alignas нужен для правильного выравнивания блока памяти
    alignas(T) char data_[sizeof(T)];
    bool is_initialized_ = false;
    T* ptr_ = nullptr;
};

template <typename T>
Optional<T>::Optional(const T& value) {
    ptr_ = new(data_) T(value);
}

template <typename T>
Optional<T>::Optional(T&& value) {
    ptr_ = new(data_) T(std::move(value));
}

template <typename T>
Optional<T>::Optional(const Optional& other) {
    if (other.HasValue()) {
        ptr_ = new(data_) T(*other);
    }
}

template <typename T>
Optional<T>::Optional(Optional&& other) {
    if (other.HasValue()) {
        ptr_ = new(data_) T(std::move(*other));
    }
}

template <typename T>
Optional<T>& Optional<T>::operator=(const T& value) {
    if (!ptr_) {
        ptr_ = new(data_) T(value);
    }
    *ptr_ = value;
    return *this;
}

template <typename T>
Optional<T>& Optional<T>::operator=(T&& rhs) {
    if (!ptr_) {
        ptr_ = new(data_) T(std::move(rhs));
    }
    else {
        *ptr_ = std::move(rhs);
    }
    
    return *this;
}

template <typename T>
Optional<T>& Optional<T>::operator=(const Optional<T>& rhs) {
    if (!ptr_ && rhs.HasValue()) {
        ptr_ = new(data_) T(*rhs);
    }
    else if(rhs.HasValue()) {
        *ptr_ = *rhs;
    }
    else {
        Reset();
    }
    return *this;
}

template <typename T>
Optional<T>& Optional<T>::operator=(Optional<T>&& rhs){
    if (!ptr_ && rhs.HasValue()) {
        ptr_ = new(data_) T(std::move(*rhs));
    }
    else if (rhs.HasValue()) {
        *ptr_ = std::move(*rhs);
    }
    else {
        Reset();
    }
    return *this;
}

template <typename T>
T& Optional<T>::operator*() & {
    return *ptr_;
}

template<typename T>
T&& Optional<T>::operator*()&& {
    return std::move(*ptr_);
}

template <typename T>
const T& Optional<T>::operator*() const& {
    return *ptr_;
}

template <typename T>
T* Optional<T>::operator->() & {
    return ptr_;
}

template <typename T>
const T* Optional<T>::operator->() const& {
    return ptr_;
}

template <typename T>
bool Optional<T>::HasValue() const {
    return ptr_ != nullptr;
}

template <typename T>
Optional<T>::~Optional() {
    Reset();
}

template <typename T>
T& Optional<T>::Value() & {
    if (!ptr_) {
        throw BadOptionalAccess("Trying to get the values of an empty pointer");
    }
    return *ptr_;
}

template <typename T>
const T& Optional<T>::Value() const& {
    if (!ptr_) {
        throw BadOptionalAccess("Trying to get the values of an empty pointer");
    }
    return *ptr_;
}

template<typename T>
T&& Optional<T>::Value()&& {
    if (!ptr_) {
        throw BadOptionalAccess("Trying to get the values of an empty pointer");
    }
    return std::move(*ptr_);
}

template <typename T>
void Optional<T>::Reset() {
    if (ptr_) {
        ptr_->~T();
        ptr_ = nullptr;
    }
}