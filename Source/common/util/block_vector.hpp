#ifndef BLOCK_VECTOR_HPP
#define BLOCK_VECTOR_HPP


template<typename T, size_t VALUES_PER_BLOCK = 1000>
class block_vector {
    T* _blocks[];
    size_t _block_count = 0;
    size_t _size = 0;

public:
    ~block_vector() {
        for(size_t i = 0; i < _block_count; ++i) {
            delete[] _blocks[i];
        }
        delete[] _blocks;
    }

    void    clear() {
        for(size_t i = 0; i < _block_count; ++i) {
            delete[] _blocks[i];
        }
        delete[] _blocks;
        _blocks = 0;
        _block_count = 0;
        _size = 0;
    }

    size_t  block_count() const { return _block_count; }
    size_t  size() const { return _size; } // actually count
    void    push_back(const T& value) {
        size_t index = _size;
        size_t block_index = index / VALUES_PER_BLOCK;
        size_t block_local_index = index - (block_index * VALUES_PER_BLOCK);
        if(_block_count <= block_index) {
            T* new_block = new T[VALUES_PER_BLOCK];
            T* new_blocks_array = new T*[_block_count + 1];
            if(_blocks) {
                memcpy(new_blocks_array, _blocks, sizeof(T*) * _block_count);
                delete[] _blocks;
            } else {
                _blocks = new_blocks_array;
            }
            _blocks[block_index] = new_block;
            ++_block_count;
        }

        _blocks[block_index][block_local_index] = value;
    }
    void    push_back(T& value) { push_back(const_cast<const T&>(value)); }
    void    emplace_back(T&& value) { push_back(value); }

    void    pop_back() {
        --_size;
    }
    
    const T& operator[](size_t index) const {
        return const_cast<const block_vector>(this)->operator[index];       
    }
    T&       operator[](size_t index) {
        size_t block_index = index / VALUES_PER_BLOCK;
        size_t block_local_index = index - (block_index * VALUES_PER_BLOCK);
        return _blocks[block_index][block_local_index];
    }

};


#endif
