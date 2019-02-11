#include <cstdlib>
#include <iostream>
#include <new>
#include <deque>
#include <vector>

template <class T>
struct Mallocator {
    typedef T value_type;

    Mallocator()
    {
        std::cout << "----- Start Mallocator() -----" << std::endl;
        std::cout << "----- End Mallocator() -----" << std::endl;
    }

    ~Mallocator()
    {
        std::cout << "----- ~Mallocator() -----" << std::endl;
    }

    template <class U>
    constexpr Mallocator(const Mallocator<U>&) noexcept
    {
        std::cout << "----- Mallocator(const Mallocator<U>&) -----" << std::endl;
    }

    template <class U>
    constexpr Mallocator(const Mallocator<U>&&) noexcept
    {
        std::cout << "----- Mallocator(const Mallocator<U>&&) -----" << std::endl;
    }

    [[nodiscard]]
    T* allocate(std::size_t n)
    {
        std::cout << "----- Start allocate(" << n << ") -----" << std::endl;
        if(n > std::size_t(-1) / sizeof(T))
            throw std::bad_alloc();
        if(auto p = static_cast<T*>(std::malloc(n*sizeof(T))))
            return p;
        throw std::bad_alloc();
        std::cout << "----- End allocate -----\n" << std::endl;
    }

    void deallocate(T* p, std::size_t n) noexcept
    {
        std::cout << "----- Start deallocate(" << p << ", " << n << ") -----" << std::endl;
        std::free(p);
        std::cout << "----- End deallocate -----\n" << std::endl;
    }
};

template <class T, class U>
bool operator==(const Mallocator<T>&, const Mallocator<U>&)
{
    return true;
}

template <class T, class U>
bool operator!=(const Mallocator<T>&, const Mallocator<U>&)
{
    return false;
}

int main(void)
{
    Mallocator<int> m;
    {
    std::vector<int, Mallocator<int>> lst(m);
    std::cout << "Finished construction" << std::endl;
    lst.push_back(1);
    lst.push_back(1);
    lst.push_back(1);
    lst.push_back(1);
    lst.push_back(1);
    std::cout << "Before end of scope" << std::endl;
    }
    std::cout << "Out of scope. Returning..." << std::endl;
    return 0;
}
