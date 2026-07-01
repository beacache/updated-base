#pragma once
#include <string_view>
#include <cstdint>
#include <string>

template < typename T > struct vmvalue1 {
private:
    T storage;
public:
    __forceinline operator const T() const {
        return (T)((uintptr_t)storage - (uintptr_t)this);
    }

    __forceinline void operator=(const T& value) {
        storage = (T)((uintptr_t)value + (uintptr_t)this);
    }

    __forceinline const T operator->() const {
        return operator const T();
    }

    __forceinline T get() {
        return operator const T();
    }

    __forceinline void set(const T& value) {
        operator=(value);
    }
};

template < typename T > struct vmvalue2 {
private:
    T storage;
public:
    __forceinline operator const T() const {
        return (T)((uintptr_t)this - (uintptr_t)storage);
    }

    __forceinline void operator=(const T& value) {
        storage = (T)((uintptr_t)this - (uintptr_t)value);
    }

    __forceinline const T operator->() const {
        return operator const T();
    }

    __forceinline T get() {
        return operator const T();
    }

    __forceinline void set(const T& value) {
        operator=(value);
    }
};

template < typename T > struct vmvalue3 {
private:
    T storage;
public:
    __forceinline operator const T() const {
        return (T)((uintptr_t)this ^ (uintptr_t)storage);
    }

    __forceinline void operator=(const T& value) {
        storage = (T)((uintptr_t)value ^ (uintptr_t)this);
    }

    __forceinline const T operator->() const {
        return operator const T();
    }

    __forceinline T get() {
        return operator const T();
    }

    __forceinline void set(const T& value) {
        operator=(value);
    }
};

template < typename T > struct vmvalue4 {
private:
    T storage;
public:
    __forceinline operator const T() const {
        return (T)((uintptr_t)this + (uintptr_t)storage);
    }

    __forceinline void operator=(const T& value) {
        storage = (T)((uintptr_t)value - (uintptr_t)this);
    }

    __forceinline const T operator->() const {
        return operator const T();
    }

    __forceinline T get() {
        return operator const T();
    }

    __forceinline void set(const T& value) {
        operator=(value);
    }
};