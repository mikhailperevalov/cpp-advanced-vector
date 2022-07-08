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
    explicit RawMemory(size_t capacity);
    RawMemory(const RawMemory&) = delete;
    RawMemory& operator=(const RawMemory& rhs) = delete;
    RawMemory(RawMemory&& other) noexcept;
    RawMemory& operator=(RawMemory&& rhs) noexcept;
    ~RawMemory();

    T* operator+(size_t offset) noexcept;
    const T* operator+(size_t offset) const noexcept;
    const T& operator[](size_t index) const noexcept;
    T& operator[](size_t index) noexcept;
    void Swap(RawMemory& other) noexcept;
    const T* GetAddress() const noexcept;
    T* GetAddress() noexcept;
    size_t Capacity() const;
private:
    static T* Allocate(size_t n);
    static void Deallocate(T* buf) noexcept;

    T* buffer_ = nullptr;
    size_t capacity_ = 0;
};

template <typename T>
class Vector
{
public:
    using iterator = T*;
    using const_iterator = const T*;

    Vector() = default;
    explicit Vector(size_t size);
    Vector(const Vector& other);
    Vector& operator=(const Vector& rhs);
    Vector(Vector&& other) noexcept;
    Vector& operator=(Vector&& rhs) noexcept;
    ~Vector();

    iterator begin() noexcept;
    iterator end() noexcept;
    const_iterator begin() const noexcept;
    const_iterator end() const noexcept;
    const_iterator cbegin() const noexcept;
    const_iterator cend() const noexcept;

    size_t Size() const noexcept;
    size_t Capacity() const noexcept;
    void Reserve(size_t new_capacity);
    const T& operator[](size_t index) const noexcept;
    T& operator[](size_t index) noexcept;
    void Swap(Vector& other) noexcept;
    void Resize(size_t new_size);
    template<typename F>
    void PushBack(F&& value);
    void PopBack() noexcept;
    template<typename... Ts>
    T& EmplaceBack(Ts&&... vs);
    template <typename... Ts>
    iterator Emplace(const_iterator pos, Ts&&... vs);
    template<typename F>
    iterator Insert(const_iterator pos, F&& value);
    iterator Erase(const_iterator pos) noexcept(std::is_nothrow_move_assignable_v<T>);

private:
    RawMemory<T> data_;
    size_t size_ = 0;
};

template<typename T>
RawMemory<T>::RawMemory(size_t capacity)
    : buffer_(Allocate(capacity))
    , capacity_(capacity)
{
}

template<typename T>
RawMemory<T>::RawMemory(RawMemory&& other) noexcept
{
    Swap(other);
}

template<typename T>
RawMemory<T>& RawMemory<T>::operator=(RawMemory&& rhs) noexcept
{
    if (this != &rhs)
    {
        Deallocate(buffer_);
        //RawMemory<T> rhs_move(std::move(rhs));
        Swap(rhs);
    }
    return *this;
}

template<typename T>
RawMemory<T>::~RawMemory()
{
    Deallocate(buffer_);
}

template<typename T>
T* RawMemory<T>::operator+(size_t offset) noexcept
{
    assert(offset <= capacity_);
    return buffer_ + offset;
}

template<typename T>
const T* RawMemory<T>::operator+(size_t offset) const noexcept
{
    return const_cast<RawMemory&>(*this) + offset;
}

template<typename T>
const T& RawMemory<T>::operator[](size_t index) const noexcept
{
    return const_cast<RawMemory&>(*this)[index];
}

template<typename T>
T& RawMemory<T>::operator[](size_t index) noexcept
{
    assert(index < capacity_);
    return buffer_[index];
}

template<typename T>
void RawMemory<T>::Swap(RawMemory& other) noexcept
{
    std::swap(buffer_, other.buffer_);
    std::swap(capacity_, other.capacity_);
}

template<typename T>
const T* RawMemory<T>::GetAddress() const noexcept
{
    return buffer_;
}

template<typename T>
T* RawMemory<T>::GetAddress() noexcept
{
    return buffer_;
}

template<typename T>
size_t RawMemory<T>::Capacity() const
{
    return capacity_;
}

template<typename T>
T* RawMemory<T>::Allocate(size_t n)
{
    return n != 0 ? static_cast<T*>(operator new(n * sizeof(T))) : nullptr;
}

template<typename T>
void RawMemory<T>::Deallocate(T* buf) noexcept
{
    operator delete(buf);
}

template<typename T>
Vector<T>::Vector(size_t size)
    : data_(size)
    , size_(size)
{
    std::uninitialized_value_construct_n(data_.GetAddress(), size);
}

template<typename T>
Vector<T>::Vector(const Vector& other)
    : data_(other.size_)
    , size_(other.size_)
{
    std::uninitialized_copy_n(other.data_.GetAddress(), size_, data_.GetAddress());
}

template<typename T>
Vector<T>& Vector<T>::operator=(const Vector& rhs)
{
    if (this != &rhs)
    {
        if (rhs.size_ > data_.Capacity())
        {
            Vector<T> rhs_copy(rhs);
            Swap(rhs_copy);
        }
        else
        {
            if (rhs.size_ < size_)
            {
                std::copy_n(rhs.data_.GetAddress(), rhs.size_, data_.GetAddress());
                std::destroy_n(data_ + rhs.size_, size_ - rhs.size_);
            }
            else
            {
                std::copy_n(rhs.data_.GetAddress(), size_, data_.GetAddress());
                std::uninitialized_copy_n(rhs.data_ + size_, rhs.size_ - size_, data_ + size_);
            }
            size_ = rhs.size_;
        }
    }
    return *this;
}

template<typename T>
Vector<T>::Vector(Vector&& other) noexcept
    : data_(std::move(other.data_))
    , size_(std::exchange(other.size_, 0))
{
}

template<typename T>
Vector<T>& Vector<T>::operator=(Vector&& rhs) noexcept
{
    if (this != &rhs)
    {
        data_.Swap(rhs.data_);
        std::swap(size_, rhs.size_);
    }
    return *this;
}


template<typename T>
Vector<T>::~Vector()
{
    std::destroy_n(data_.GetAddress(), size_);
}

template<typename T>
typename Vector<T>::iterator Vector<T>::begin() noexcept
{
    return data_.GetAddress();
}

template<typename T>
typename Vector<T>::iterator Vector<T>::end() noexcept
{
    return data_ + size_;
}

template<typename T>
typename Vector<T>::const_iterator Vector<T>::begin() const noexcept
{
    return static_cast<const T*>(data_.GetAddress());
}

template<typename T>
typename Vector<T>::const_iterator Vector<T>::end() const noexcept
{
    return static_cast<const T*>(data_ + size_);
}

template<typename T>
typename Vector<T>::const_iterator Vector<T>::cbegin() const noexcept
{
    return begin();
}

template<typename T>
typename Vector<T>::const_iterator Vector<T>::cend() const noexcept
{
    return end();
}

template<typename T>
size_t Vector<T>::Size() const noexcept
{
    return size_;
}

template<typename T>
size_t Vector<T>::Capacity() const noexcept
{
    return data_.Capacity();
}

template<typename T>
void Vector<T>::Reserve(size_t new_capacity)
{
    if (new_capacity <= data_.Capacity())
    {
        return;
    }
    RawMemory<T> new_data(new_capacity);
    if constexpr (std::is_nothrow_move_constructible_v<T> ||
        !std::is_copy_constructible_v<T>)
    {
        std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
    }
    else
    {
        std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
    }
    std::destroy_n(data_.GetAddress(), size_);
    data_.Swap(new_data);
}

template<typename T>
const T& Vector<T>::operator[](size_t index) const noexcept
{
    return const_cast<Vector&>(*this)[index];
}

template<typename T>
T& Vector<T>::operator[](size_t index) noexcept
{
    assert(index < size_);
    return data_[index];
}

template<typename T>
void Vector<T>::Swap(Vector& other) noexcept
{
    data_.Swap(other.data_);
    std::swap(size_, other.size_);
}

template<typename T>
void Vector<T>::Resize(size_t new_size)
{
    if (new_size < size_)
    {
        std::destroy_n(&data_[new_size], size_ - new_size);
    }
    else if (new_size > size_)
    {
        Reserve(new_size);
        std::uninitialized_value_construct_n(&data_[size_], new_size - size_);
    }
    size_ = new_size;
}

template<typename T>
template<typename F>
void Vector<T>::PushBack(F&& value)
{
    EmplaceBack(std::forward<F>(value));
}

template<typename T>
void Vector<T>::PopBack() noexcept
{
    std::destroy_at(&data_[size_ - 1]);
    --size_;
}

template<typename T>
template<typename ...Ts>
T& Vector<T>::EmplaceBack(Ts && ...vs)
{
    T* result;
    if (size_ == data_.Capacity())
    {
        size_t new_capacity = size_ == 0 ? 1 : size_ * 2;
        RawMemory<T> new_data(new_capacity);
        result = new (new_data + size_) T(std::forward<Ts>(vs)...);
        if constexpr (std::is_nothrow_move_constructible_v<T> ||
            !std::is_copy_constructible_v<T>)
        {
            std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
        }
        else
        {
            std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
        }
        std::destroy_n(data_.GetAddress(), size_);
        data_.Swap(new_data);
    }
    else
    {
        result = new (data_ + size_) T(std::forward<Ts>(vs)...);
    }
    ++size_;
    return *result;
}

template<typename T>
template<typename... Ts>
typename Vector<T>::iterator Vector<T>::Emplace(const_iterator pos, Ts&& ...vs)
{
    assert(pos >= begin() && pos <= end());
    if (pos == end())
    {
        return &EmplaceBack(std::forward<Ts>(vs)...);
    }
    iterator result;
    size_t pos_index = pos - begin();
    if (size_ == data_.Capacity())
    {
        size_t pos_index = pos - begin();
        size_t new_capacity = size_ == 0 ? 1 : size_ * 2;
        RawMemory<T> new_data(new_capacity);
        result = new (new_data + pos_index) T(std::forward<Ts>(vs)...);
        if constexpr (std::is_nothrow_move_constructible_v<T> ||
            !std::is_copy_constructible_v<T>)
        {
            std::uninitialized_move(begin(), begin() + pos_index, new_data.GetAddress());
            std::uninitialized_move(begin() + pos_index, end(), result + 1);
        }
        else
        {
            try
            {
                std::uninitialized_copy(begin(), begin() + pos_index, new_data.GetAddress());
            }
            catch (...)
            {
                std::destroy_at(result);
                throw;
            }
            try
            {
                std::uninitialized_copy(begin() + pos_index, end(), result + 1);
            }
            catch (...)
            {
                std::destroy_n(new_data.GetAddress(), pos_index + 1);
                throw;
            }
        }
        std::destroy_n(data_.GetAddress(), size_);
        data_.Swap(new_data);
    }
    else
    {
        new (data_ + size_) T(std::move(*(end() - 1)));
        T buffer(std::forward<Ts>(vs)...);
        std::move_backward(begin() + pos_index, end() - 1, end());
        *(data_ + pos_index) = std::move(buffer);
        result = data_ + pos_index;
    }
    ++size_;
    return result;
}

template<typename T>
template<typename F>
typename Vector<T>::iterator Vector<T>::Insert(const_iterator pos, F&& value)
{
    return Emplace(pos, std::forward<F>(value));
}

template<typename T>
typename Vector<T>::iterator Vector<T>::Erase(const_iterator pos) noexcept(std::is_nothrow_move_assignable_v<T>)
{
    assert(pos >= begin() && pos <= end());
    size_t pos_index = pos - begin();
    if constexpr (std::is_nothrow_move_constructible_v<T> ||
        !std::is_copy_constructible_v<T>)
    {
        std::move(begin() + pos_index + 1, end(), begin() + pos_index);
    }
    else
    {
        std::copy(begin() + pos_index + 1, end(), begin() + pos_index);
    }
    std::destroy_at(std::prev(end()));
    --size_;
    return begin() + pos_index;
}
