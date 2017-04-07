//=============================================================================
// CircularBuffer
// (c) 18-01-2016 Jeroen de Haas
//=============================================================================
#include <vector>
#include <stdexcept>

template<typename T>
class CircularBuffer {
    std::vector<T> buffer;
    int head;
    int tail;
    int inc(int x, int by) noexcept { return (x + by) % buffer.capacity(); }
    int used;
public:
    CircularBuffer() : CircularBuffer { 512 } {}
    CircularBuffer(std::size_t capacity) : buffer(capacity), head{0}, tail{0}, used{0} {}

    bool full() noexcept { return used == buffer.capacity(); }
    bool empty() noexcept { return used == 0; }
    int size() noexcept { return used; }

    void clear() noexcept { 
        head = tail = used = 0; 
    }

    void push_back(const T& t) {
        if (full()) {
            throw std::logic_error { "push_back called on full circular buffer" };
        }
        buffer[tail] = t;
        tail = inc(tail, 1);
        ++used;
    }

    T pop_front() {
        if (empty()) {
            throw std::logic_error { "pop_front called on empty circular buffer" };
        }
        T t = std::move(buffer[head]);
        head = inc(head, 1);
        --used;
        return t;
    }

    const T& operator[](std::size_t index) const {
        return buffer[(head+index)%buffer.capacity()];
    }

    T& operator[](std::size_t index) {
        return buffer[(head+index)%buffer.capacity()];
    }
};    

