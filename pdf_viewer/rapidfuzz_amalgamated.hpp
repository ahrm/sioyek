//  Licensed under the MIT License <http://opensource.org/licenses/MIT>.
//  SPDX-License-Identifier: MIT
//  RapidFuzz v1.0.2
//  Generated: 2022-12-11 00:18:48.007741
//  ----------------------------------------------------------
//  This file is an amalgamation of multiple different files.
//  You probably shouldn't edit it directly.
//  ----------------------------------------------------------
#ifndef RAPIDFUZZ_AMALGAMATED_HPP_INCLUDED
#define RAPIDFUZZ_AMALGAMATED_HPP_INCLUDED

#include <algorithm>
#include <cmath>

#include <cassert>
#include <cstddef>
#include <iterator>
#include <limits>
#include <memory>
#include <numeric>

#include <array>
#include <iterator>
#include <new>
#include <stdint.h>

namespace rapidfuzz::detail {

/* hashmap for integers which can only grow, but can't remove elements */
template <typename T_Key, typename T_Entry>
struct GrowingHashmap {
    using key_type = T_Key;
    using value_type = T_Entry;
    using size_type = unsigned int;

private:
    static constexpr size_type min_size = 8;
    struct MapElem {
        key_type key;
        value_type value = value_type();
    };

    int used;
    int fill;
    int mask;
    MapElem* m_map;

public:
    GrowingHashmap() : used(0), fill(0), mask(-1), m_map(NULL)
    {}
    ~GrowingHashmap()
    {
        delete[] m_map;
    }

    GrowingHashmap(const GrowingHashmap& other) : used(other.used), fill(other.fill), mask(other.mask)
    {
        int size = mask + 1;
        m_map = new MapElem[size];
        std::copy(other.m_map, other.m_map + size, m_map);
    }

    GrowingHashmap(GrowingHashmap&& other) noexcept : GrowingHashmap()
    {
        swap(*this, other);
    }

    GrowingHashmap& operator=(GrowingHashmap other)
    {
        swap(*this, other);
        return *this;
    }

    friend void swap(GrowingHashmap& first, GrowingHashmap& second) noexcept
    {
        std::swap(first.used, second.used);
        std::swap(first.fill, second.fill);
        std::swap(first.mask, second.mask);
        std::swap(first.m_map, second.m_map);
    }

    size_type size() const
    {
        return used;
    }
    size_type capacity() const
    {
        return mask + 1;
    }
    bool empty() const
    {
        return used == 0;
    }

    value_type get(key_type key) const noexcept
    {
        if (m_map == NULL) return value_type();

        return m_map[lookup(key)].value;
    }

    value_type& operator[](key_type key) noexcept
    {
        if (m_map == NULL) allocate();

        size_t i = lookup(key);

        if (m_map[i].value == value_type()) {
            /* resize when 2/3 full */
            if (++fill * 3 >= (mask + 1) * 2) {
                grow((used + 1) * 2);
                i = lookup(key);
            }

            used++;
        }

        m_map[i].key = key;
        return m_map[i].value;
    }

private:
    void allocate()
    {
        mask = min_size - 1;
        m_map = new MapElem[min_size];
    }

    /**
     * lookup key inside the hashmap using a similar collision resolution
     * strategy to CPython and Ruby
     */
    size_t lookup(key_type key) const
    {
        size_t hash = static_cast<size_t>(key);
        size_t i = hash & static_cast<size_t>(mask);

        if (m_map[i].value == value_type() || m_map[i].key == key) return i;

        size_t perturb = hash;
        while (true) {
            i = (i * 5 + perturb + 1) & static_cast<size_t>(mask);
            if (m_map[i].value == value_type() || m_map[i].key == key) return i;

            perturb >>= 5;
        }
    }

    void grow(int minUsed)
    {
        int newSize = mask + 1;
        while (newSize <= minUsed)
            newSize <<= 1;

        MapElem* oldMap = m_map;
        m_map = new MapElem[static_cast<size_t>(newSize)];

        fill = used;
        mask = newSize - 1;

        for (int i = 0; used > 0; i++)
            if (oldMap[i].value != value_type()) {
                size_t j = lookup(oldMap[i].key);

                m_map[j].key = oldMap[i].key;
                m_map[j].value = oldMap[i].value;
                used--;
            }

        used = fill;
        delete[] oldMap;
    }
};

template <typename T_Key, typename T_Entry>
struct HybridGrowingHashmap {
    using key_type = T_Key;
    using value_type = T_Entry;

    HybridGrowingHashmap()
    {
        m_extendedAscii.fill(value_type());
    }

    value_type get(char key) const noexcept
    {
        /** treat char as value between 0 and 127 for performance reasons */
        return m_extendedAscii[static_cast<uint8_t>(key)];
    }

    template <typename CharT>
    value_type get(CharT key) const noexcept
    {
        if (key >= 0 && key <= 255)
            return m_extendedAscii[static_cast<uint8_t>(key)];
        else
            return m_map.get(static_cast<key_type>(key));
    }

    value_type& operator[](char key) noexcept
    {
        /** treat char as value between 0 and 127 for performance reasons */
        return m_extendedAscii[static_cast<uint8_t>(key)];
    }

    template <typename CharT>
    value_type& operator[](CharT key)
    {
        if (key >= 0 && key <= 255)
            return m_extendedAscii[static_cast<uint8_t>(key)];
        else
            return m_map[static_cast<key_type>(key)];
    }

private:
    GrowingHashmap<key_type, value_type> m_map;
    std::array<value_type, 256> m_extendedAscii;
};

} // namespace rapidfuzz::detail

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <stdint.h>
#include <stdio.h>
#include <vector>

namespace rapidfuzz::detail {

template <typename T, bool IsConst>
struct BitMatrixView {

    using value_type = T;
    using size_type = size_t;
    using pointer = std::conditional_t<IsConst, const value_type*, value_type*>;
    using reference = std::conditional_t<IsConst, const value_type&, value_type&>;

    BitMatrixView(pointer vector, size_type cols) noexcept : m_vector(vector), m_cols(cols)
    {}

    reference operator[](size_type col) noexcept
    {
        assert(col < m_cols);
        return m_vector[col];
    }

    size_type size() const noexcept
    {
        return m_cols;
    }

private:
    pointer m_vector;
    size_type m_cols;
};

template <typename T>
struct BitMatrix {

    using value_type = T;

    BitMatrix() : m_rows(0), m_cols(0), m_matrix(nullptr)
    {}

    BitMatrix(size_t rows, size_t cols, T val) : m_rows(rows), m_cols(cols), m_matrix(nullptr)
    {
        if (m_rows && m_cols) m_matrix = new T[m_rows * m_cols];
        std::fill_n(m_matrix, m_rows * m_cols, val);
    }

    BitMatrix(const BitMatrix& other) : m_rows(other.m_rows), m_cols(other.m_cols), m_matrix(nullptr)
    {
        if (m_rows && m_cols) m_matrix = new T[m_rows * m_cols];
        std::copy(other.m_matrix, other.m_matrix + m_rows * m_cols, m_matrix);
    }

    BitMatrix(BitMatrix&& other) noexcept : m_rows(0), m_cols(0), m_matrix(nullptr)
    {
        other.swap(*this);
    }

    BitMatrix& operator=(BitMatrix&& other) noexcept
    {
        other.swap(*this);
        return *this;
    }

    BitMatrix& operator=(const BitMatrix& other)
    {
        BitMatrix temp = other;
        temp.swap(*this);
        return *this;
    }

    void swap(BitMatrix& rhs) noexcept
    {
        using std::swap;
        swap(m_rows, rhs.m_rows);
        swap(m_cols, rhs.m_cols);
        swap(m_matrix, rhs.m_matrix);
    }

    ~BitMatrix()
    {
        delete[] m_matrix;
    }

    BitMatrixView<value_type, false> operator[](size_t row) noexcept
    {
        assert(row < m_rows);
        return {&m_matrix[row * m_cols], m_cols};
    }

    BitMatrixView<value_type, true> operator[](size_t row) const noexcept
    {
        assert(row < m_rows);
        return {&m_matrix[row * m_cols], m_cols};
    }

    size_t rows() const noexcept
    {
        return m_rows;
    }

    size_t cols() const noexcept
    {
        return m_cols;
    }

private:
    size_t m_rows;
    size_t m_cols;
    T* m_matrix;
};

template <typename T>
struct ShiftedBitMatrix {
    using value_type = T;

    ShiftedBitMatrix()
    {}

    ShiftedBitMatrix(size_t rows, size_t cols, T val) : m_matrix(rows, cols, val), m_offsets(rows)
    {}

    ShiftedBitMatrix(const ShiftedBitMatrix& other) : m_matrix(other.m_matrix), m_offsets(other.m_offsets)
    {}

    ShiftedBitMatrix(ShiftedBitMatrix&& other) noexcept
    {
        other.swap(*this);
    }

    ShiftedBitMatrix& operator=(ShiftedBitMatrix&& other) noexcept
    {
        other.swap(*this);
        return *this;
    }

    ShiftedBitMatrix& operator=(const ShiftedBitMatrix& other)
    {
        ShiftedBitMatrix temp = other;
        temp.swap(*this);
        return *this;
    }

    void swap(ShiftedBitMatrix& rhs) noexcept
    {
        using std::swap;
        swap(m_matrix, rhs.m_matrix);
        swap(m_offsets, rhs.m_offsets);
    }

    bool test_bit(size_t row, size_t col, bool default_ = false) const noexcept
    {
        ptrdiff_t offset = static_cast<ptrdiff_t>(m_offsets[row]);

        if (offset < 0) {
            col += static_cast<size_t>(-offset);
        }
        else if (col >= static_cast<size_t>(offset)) {
            col -= static_cast<size_t>(offset);
        }
        /* bit on the left of the band */
        else {
            return default_;
        }

        size_t word_size = sizeof(value_type) * 8;
        size_t col_word = col / word_size;
        uint64_t col_mask = value_type(1) << (col % word_size);

        return bool(m_matrix[row][col_word] & col_mask);
    }

    auto operator[](size_t row) noexcept
    {
        return m_matrix[row];
    }

    auto operator[](size_t row) const noexcept
    {
        return m_matrix[row];
    }

    void set_offset(size_t row, ptrdiff_t offset)
    {
        m_offsets[row] = offset;
    }

private:
    BitMatrix<value_type> m_matrix;
    std::vector<ptrdiff_t> m_offsets;
};

} // namespace rapidfuzz::detail

#include <iterator>
#include <limits>
#include <ostream>
#include <stdexcept>
#include <vector>

namespace rapidfuzz::detail {

static inline void assume(bool b)
{
#if defined(__clang__)
    __builtin_assume(b);
#elif defined(__GNUC__) || defined(__GNUG__)
    if (!b) __builtin_unreachable();
#elif defined(_MSC_VER)
    __assume(b);
#endif
}

template <typename CharT>
CharT* to_begin(CharT* s)
{
    return s;
}

template <typename T>
auto to_begin(T& x)
{
    using std::begin;
    return begin(x);
}

template <typename CharT>
CharT* to_end(CharT* s)
{
    assume(s != nullptr);
    while (*s != 0)
        ++s;

    return s;
}

template <typename T>
auto to_end(T& x)
{
    using std::end;
    return end(x);
}

template <typename Iter>
class Range {
    Iter _first;
    Iter _last;

public:
    using value_type = typename std::iterator_traits<Iter>::value_type;
    using iterator = Iter;
    using reverse_iterator = std::reverse_iterator<iterator>;

    constexpr Range(Iter first, Iter last) : _first(first), _last(last)
    {}

    template <typename T>
    constexpr Range(T& x) : _first(to_begin(x)), _last(to_end(x))
    {}

    constexpr iterator begin() const noexcept
    {
        return _first;
    }
    constexpr iterator end() const noexcept
    {
        return _last;
    }

    constexpr reverse_iterator rbegin() const noexcept
    {
        return reverse_iterator(end());
    }
    constexpr reverse_iterator rend() const noexcept
    {
        return reverse_iterator(begin());
    }

    constexpr ptrdiff_t size() const
    {
        return std::distance(_first, _last);
    }
    constexpr bool empty() const
    {
        return size() == 0;
    }
    explicit constexpr operator bool() const
    {
        return !empty();
    }
    constexpr decltype(auto) operator[](ptrdiff_t n) const
    {
        return _first[n];
    }

    constexpr void remove_prefix(ptrdiff_t n)
    {
        _first += n;
    }
    constexpr void remove_suffix(ptrdiff_t n)
    {
        _last -= n;
    }

    constexpr Range subseq(ptrdiff_t pos = 0, ptrdiff_t count = std::numeric_limits<ptrdiff_t>::max())
    {
        if (pos > size()) throw std::out_of_range("Index out of range in Range::substr");

        auto start = _first + pos;
        if (std::distance(start, _last) < count) return {start, _last};
        return {start, start + count};
    }

    constexpr decltype(auto) front() const
    {
        return *(_first);
    }

    constexpr decltype(auto) back() const
    {
        return *(_last - 1);
    }

    constexpr Range<reverse_iterator> reversed() const
    {
        return {rbegin(), rend()};
    }

    friend std::ostream& operator<<(std::ostream& os, const Range& seq)
    {
        os << "[";
        for (auto x : seq)
            os << static_cast<uint64_t>(x) << ", ";
        os << "]";
        return os;
    }
};

template <typename T>
Range(T& x) -> Range<decltype(to_begin(x))>;

template <typename InputIt1, typename InputIt2>
inline bool operator==(const Range<InputIt1>& a, const Range<InputIt2>& b)
{
    return std::equal(a.begin(), a.end(), b.begin(), b.end());
}

template <typename InputIt1, typename InputIt2>
inline bool operator!=(const Range<InputIt1>& a, const Range<InputIt2>& b)
{
    return !(a == b);
}

template <typename InputIt1, typename InputIt2>
inline bool operator<(const Range<InputIt1>& a, const Range<InputIt2>& b)
{
    return (std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end()));
}

template <typename InputIt1, typename InputIt2>
inline bool operator>(const Range<InputIt1>& a, const Range<InputIt2>& b)
{
    return b < a;
}

template <typename InputIt1, typename InputIt2>
inline bool operator<=(const Range<InputIt1>& a, const Range<InputIt2>& b)
{
    return !(b < a);
}

template <typename InputIt1, typename InputIt2>
inline bool operator>=(const Range<InputIt1>& a, const Range<InputIt2>& b)
{
    return !(a < b);
}

template <typename InputIt>
using RangeVec = std::vector<Range<InputIt>>;

} // namespace rapidfuzz::detail

#include <array>
#include <cmath>
#include <cstring>
#include <limits>

#include <algorithm>

#include <algorithm>
#include <cassert>
#include <stddef.h>
#include <stdexcept>
#include <stdint.h>
#include <type_traits>
#include <vector>

namespace rapidfuzz {

struct StringAffix {
    size_t prefix_len;
    size_t suffix_len;
};

struct LevenshteinWeightTable {
    int64_t insert_cost;
    int64_t delete_cost;
    int64_t replace_cost;
};

/**
 * @brief Edit operation types used by the Levenshtein distance
 */
enum class EditType {
    None = 0,    /**< No Operation required */
    Replace = 1, /**< Replace a character if a string by another character */
    Insert = 2,  /**< Insert a character into a string */
    Delete = 3   /**< Delete a character from a string */
};

/**
 * @brief Edit operations used by the Levenshtein distance
 *
 * This represents an edit operation of type type which is applied to
 * the source string
 *
 * Replace: replace character at src_pos with character at dest_pos
 * Insert:  insert character from dest_pos at src_pos
 * Delete:  delete character at src_pos
 */
struct EditOp {
    EditType type;   /**< type of the edit operation */
    size_t src_pos;  /**< index into the source string */
    size_t dest_pos; /**< index into the destination string */

    EditOp() : type(EditType::None), src_pos(0), dest_pos(0)
    {}

    EditOp(EditType type_, size_t src_pos_, size_t dest_pos_)
        : type(type_), src_pos(src_pos_), dest_pos(dest_pos_)
    {}
};

inline bool operator==(EditOp a, EditOp b)
{
    return (a.type == b.type) && (a.src_pos == b.src_pos) && (a.dest_pos == b.dest_pos);
}

inline bool operator!=(EditOp a, EditOp b)
{
    return !(a == b);
}

/**
 * @brief Edit operations used by the Levenshtein distance
 *
 * This represents an edit operation of type type which is applied to
 * the source string
 *
 * None:    s1[src_begin:src_end] == s1[dest_begin:dest_end]
 * Replace: s1[i1:i2] should be replaced by s2[dest_begin:dest_end]
 * Insert:  s2[dest_begin:dest_end] should be inserted at s1[src_begin:src_begin].
 *          Note that src_begin==src_end in this case.
 * Delete:  s1[src_begin:src_end] should be deleted.
 *          Note that dest_begin==dest_end in this case.
 */
struct Opcode {
    EditType type;     /**< type of the edit operation */
    size_t src_begin;  /**< index into the source string */
    size_t src_end;    /**< index into the source string */
    size_t dest_begin; /**< index into the destination string */
    size_t dest_end;   /**< index into the destination string */

    Opcode() : type(EditType::None), src_begin(0), src_end(0), dest_begin(0), dest_end(0)
    {}

    Opcode(EditType type_, size_t src_begin_, size_t src_end_, size_t dest_begin_, size_t dest_end_)
        : type(type_), src_begin(src_begin_), src_end(src_end_), dest_begin(dest_begin_), dest_end(dest_end_)
    {}
};

inline bool operator==(Opcode a, Opcode b)
{
    return (a.type == b.type) && (a.src_begin == b.src_begin) && (a.src_end == b.src_end) &&
           (a.dest_begin == b.dest_begin) && (a.dest_end == b.dest_end);
}

inline bool operator!=(Opcode a, Opcode b)
{
    return !(a == b);
}

namespace detail {
template <typename Vec>
auto vector_slice(const Vec& vec, int start, int stop, int step) -> Vec
{
    Vec new_vec;

    if (step == 0) throw std::invalid_argument("slice step cannot be zero");
    if (step < 0) throw std::invalid_argument("step sizes below 0 lead to an invalid order of editops");

    if (start < 0)
        start = std::max<int>(start + static_cast<int>(vec.size()), 0);
    else if (start > static_cast<int>(vec.size()))
        start = static_cast<int>(vec.size());

    if (stop < 0)
        stop = std::max<int>(stop + static_cast<int>(vec.size()), 0);
    else if (stop > static_cast<int>(vec.size()))
        stop = static_cast<int>(vec.size());

    if (start >= stop) return new_vec;

    int count = (stop - 1 - start) / step + 1;
    new_vec.reserve(static_cast<size_t>(count));

    for (int i = start; i < stop; i += step)
        new_vec.push_back(vec[static_cast<size_t>(i)]);

    return new_vec;
}

template <typename Vec>
void vector_remove_slice(Vec& vec, int start, int stop, int step)
{
    if (step == 0) throw std::invalid_argument("slice step cannot be zero");
    if (step < 0) throw std::invalid_argument("step sizes below 0 lead to an invalid order of editops");

    if (start < 0)
        start = std::max<int>(start + static_cast<int>(vec.size()), 0);
    else if (start > static_cast<int>(vec.size()))
        start = static_cast<int>(vec.size());

    if (stop < 0)
        stop = std::max<int>(stop + static_cast<int>(vec.size()), 0);
    else if (stop > static_cast<int>(vec.size()))
        stop = static_cast<int>(vec.size());

    if (start >= stop) return;

    auto iter = vec.begin() + start;
    for (int i = start; i < static_cast<int>(vec.size()); i++)
        if (i >= stop || ((i - start) % step != 0)) *(iter++) = vec[static_cast<size_t>(i)];

    vec.resize(static_cast<size_t>(std::distance(vec.begin(), iter)));
    vec.shrink_to_fit();
}

} // namespace detail

class Opcodes;

class Editops : private std::vector<EditOp> {
public:
    using std::vector<EditOp>::size_type;

    Editops() noexcept : src_len(0), dest_len(0)
    {}

    Editops(size_type count, const EditOp& value) : std::vector<EditOp>(count, value), src_len(0), dest_len(0)
    {}

    explicit Editops(size_type count) : std::vector<EditOp>(count), src_len(0), dest_len(0)
    {}

    Editops(const Editops& other)
        : std::vector<EditOp>(other), src_len(other.src_len), dest_len(other.dest_len)
    {}

    Editops(const Opcodes& other);

    Editops(Editops&& other) noexcept
    {
        swap(other);
    }

    Editops& operator=(Editops other) noexcept
    {
        swap(other);
        return *this;
    }

    /* Element access */
    using std::vector<EditOp>::at;
    using std::vector<EditOp>::operator[];
    using std::vector<EditOp>::front;
    using std::vector<EditOp>::back;
    using std::vector<EditOp>::data;

    /* Iterators */
    using std::vector<EditOp>::begin;
    using std::vector<EditOp>::cbegin;
    using std::vector<EditOp>::end;
    using std::vector<EditOp>::cend;
    using std::vector<EditOp>::rbegin;
    using std::vector<EditOp>::crbegin;
    using std::vector<EditOp>::rend;
    using std::vector<EditOp>::crend;

    /* Capacity */
    using std::vector<EditOp>::empty;
    using std::vector<EditOp>::size;
    using std::vector<EditOp>::max_size;
    using std::vector<EditOp>::reserve;
    using std::vector<EditOp>::capacity;
    using std::vector<EditOp>::shrink_to_fit;

    /* Modifiers */
    using std::vector<EditOp>::clear;
    using std::vector<EditOp>::insert;
    using std::vector<EditOp>::emplace;
    using std::vector<EditOp>::erase;
    using std::vector<EditOp>::push_back;
    using std::vector<EditOp>::emplace_back;
    using std::vector<EditOp>::pop_back;
    using std::vector<EditOp>::resize;

    void swap(Editops& rhs) noexcept
    {
        std::swap(src_len, rhs.src_len);
        std::swap(dest_len, rhs.dest_len);
        std::vector<EditOp>::swap(rhs);
    }

    Editops slice(int start, int stop, int step = 1) const
    {
        Editops ed_slice = detail::vector_slice(*this, start, stop, step);
        ed_slice.src_len = src_len;
        ed_slice.dest_len = dest_len;
        return ed_slice;
    }

    void remove_slice(int start, int stop, int step = 1)
    {
        detail::vector_remove_slice(*this, start, stop, step);
    }

    Editops reverse() const
    {
        Editops reversed = *this;
        std::reverse(reversed.begin(), reversed.end());
        return reversed;
    }

    size_t get_src_len() const noexcept
    {
        return src_len;
    }
    void set_src_len(size_t len) noexcept
    {
        src_len = len;
    }
    size_t get_dest_len() const noexcept
    {
        return dest_len;
    }
    void set_dest_len(size_t len) noexcept
    {
        dest_len = len;
    }

    Editops inverse() const
    {
        Editops inv_ops = *this;
        std::swap(inv_ops.src_len, inv_ops.dest_len);
        for (auto& op : inv_ops) {
            std::swap(op.src_pos, op.dest_pos);
            if (op.type == EditType::Delete)
                op.type = EditType::Insert;
            else if (op.type == EditType::Insert)
                op.type = EditType::Delete;
        }
        return inv_ops;
    }

    Editops remove_subsequence(const Editops& subsequence) const
    {
        Editops result;
        result.set_src_len(src_len);
        result.set_dest_len(dest_len);

        if (subsequence.size() > size()) throw std::invalid_argument("subsequence is not a subsequence");

        result.resize(size() - subsequence.size());

        /* offset to correct removed edit operations */
        int offset = 0;
        auto op_iter = begin();
        auto op_end = end();
        size_t result_pos = 0;
        for (const auto& sop : subsequence) {
            for (; op_iter != op_end && sop != *op_iter; op_iter++) {
                result[result_pos] = *op_iter;
                result[result_pos].src_pos =
                    static_cast<size_t>(static_cast<ptrdiff_t>(result[result_pos].src_pos) + offset);
                result_pos++;
            }
            /* element of subsequence not part of the sequence */
            if (op_iter == op_end) throw std::invalid_argument("subsequence is not a subsequence");

            if (sop.type == EditType::Insert)
                offset++;
            else if (sop.type == EditType::Delete)
                offset--;
            op_iter++;
        }

        /* add remaining elements */
        for (; op_iter != op_end; op_iter++) {
            result[result_pos] = *op_iter;
            result[result_pos].src_pos =
                static_cast<size_t>(static_cast<ptrdiff_t>(result[result_pos].src_pos) + offset);
            result_pos++;
        }

        return result;
    }

private:
    size_t src_len;
    size_t dest_len;
};

inline bool operator==(const Editops& lhs, const Editops& rhs)
{
    if (lhs.get_src_len() != rhs.get_src_len() || lhs.get_dest_len() != rhs.get_dest_len()) {
        return false;
    }

    if (lhs.size() != rhs.size()) {
        return false;
    }
    return std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

inline bool operator!=(const Editops& lhs, const Editops& rhs)
{
    return !(lhs == rhs);
}

inline void swap(Editops& lhs, Editops& rhs) noexcept(noexcept(lhs.swap(rhs)))
{
    lhs.swap(rhs);
}

class Opcodes : private std::vector<Opcode> {
public:
    using std::vector<Opcode>::size_type;

    Opcodes() noexcept : src_len(0), dest_len(0)
    {}

    Opcodes(size_type count, const Opcode& value) : std::vector<Opcode>(count, value), src_len(0), dest_len(0)
    {}

    explicit Opcodes(size_type count) : std::vector<Opcode>(count), src_len(0), dest_len(0)
    {}

    Opcodes(const Opcodes& other)
        : std::vector<Opcode>(other), src_len(other.src_len), dest_len(other.dest_len)
    {}

    Opcodes(const Editops& other);

    Opcodes(Opcodes&& other) noexcept
    {
        swap(other);
    }

    Opcodes& operator=(Opcodes other) noexcept
    {
        swap(other);
        return *this;
    }

    /* Element access */
    using std::vector<Opcode>::at;
    using std::vector<Opcode>::operator[];
    using std::vector<Opcode>::front;
    using std::vector<Opcode>::back;
    using std::vector<Opcode>::data;

    /* Iterators */
    using std::vector<Opcode>::begin;
    using std::vector<Opcode>::cbegin;
    using std::vector<Opcode>::end;
    using std::vector<Opcode>::cend;
    using std::vector<Opcode>::rbegin;
    using std::vector<Opcode>::crbegin;
    using std::vector<Opcode>::rend;
    using std::vector<Opcode>::crend;

    /* Capacity */
    using std::vector<Opcode>::empty;
    using std::vector<Opcode>::size;
    using std::vector<Opcode>::max_size;
    using std::vector<Opcode>::reserve;
    using std::vector<Opcode>::capacity;
    using std::vector<Opcode>::shrink_to_fit;

    /* Modifiers */
    using std::vector<Opcode>::clear;
    using std::vector<Opcode>::insert;
    using std::vector<Opcode>::emplace;
    using std::vector<Opcode>::erase;
    using std::vector<Opcode>::push_back;
    using std::vector<Opcode>::emplace_back;
    using std::vector<Opcode>::pop_back;
    using std::vector<Opcode>::resize;

    void swap(Opcodes& rhs) noexcept
    {
        std::swap(src_len, rhs.src_len);
        std::swap(dest_len, rhs.dest_len);
        std::vector<Opcode>::swap(rhs);
    }

    Opcodes slice(int start, int stop, int step = 1) const
    {
        Opcodes ed_slice = detail::vector_slice(*this, start, stop, step);
        ed_slice.src_len = src_len;
        ed_slice.dest_len = dest_len;
        return ed_slice;
    }

    Opcodes reverse() const
    {
        Opcodes reversed = *this;
        std::reverse(reversed.begin(), reversed.end());
        return reversed;
    }

    size_t get_src_len() const noexcept
    {
        return src_len;
    }
    void set_src_len(size_t len) noexcept
    {
        src_len = len;
    }
    size_t get_dest_len() const noexcept
    {
        return dest_len;
    }
    void set_dest_len(size_t len) noexcept
    {
        dest_len = len;
    }

    Opcodes inverse() const
    {
        Opcodes inv_ops = *this;
        std::swap(inv_ops.src_len, inv_ops.dest_len);
        for (auto& op : inv_ops) {
            std::swap(op.src_begin, op.dest_begin);
            std::swap(op.src_end, op.dest_end);
            if (op.type == EditType::Delete)
                op.type = EditType::Insert;
            else if (op.type == EditType::Insert)
                op.type = EditType::Delete;
        }
        return inv_ops;
    }

private:
    size_t src_len;
    size_t dest_len;
};

inline bool operator==(const Opcodes& lhs, const Opcodes& rhs)
{
    if (lhs.get_src_len() != rhs.get_src_len() || lhs.get_dest_len() != rhs.get_dest_len()) return false;

    if (lhs.size() != rhs.size()) return false;

    return std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

inline bool operator!=(const Opcodes& lhs, const Opcodes& rhs)
{
    return !(lhs == rhs);
}

inline void swap(Opcodes& lhs, Opcodes& rhs) noexcept(noexcept(lhs.swap(rhs)))
{
    lhs.swap(rhs);
}

inline Editops::Editops(const Opcodes& other)
{
    src_len = other.get_src_len();
    dest_len = other.get_dest_len();
    for (const auto& op : other) {
        switch (op.type) {
        case EditType::None: break;

        case EditType::Replace:
            for (size_t j = 0; j < op.src_end - op.src_begin; j++)
                push_back({EditType::Replace, op.src_begin + j, op.dest_begin + j});
            break;

        case EditType::Insert:
            for (size_t j = 0; j < op.dest_end - op.dest_begin; j++)
                push_back({EditType::Insert, op.src_begin, op.dest_begin + j});
            break;

        case EditType::Delete:
            for (size_t j = 0; j < op.src_end - op.src_begin; j++)
                push_back({EditType::Delete, op.src_begin + j, op.dest_begin});
            break;
        }
    }
}

inline Opcodes::Opcodes(const Editops& other)
{
    src_len = other.get_src_len();
    dest_len = other.get_dest_len();
    size_t src_pos = 0;
    size_t dest_pos = 0;
    for (size_t i = 0; i < other.size();) {
        if (src_pos < other[i].src_pos || dest_pos < other[i].dest_pos) {
            push_back({EditType::None, src_pos, other[i].src_pos, dest_pos, other[i].dest_pos});
            src_pos = other[i].src_pos;
            dest_pos = other[i].dest_pos;
        }

        size_t src_begin = src_pos;
        size_t dest_begin = dest_pos;
        EditType type = other[i].type;
        do {
            switch (type) {
            case EditType::None: break;

            case EditType::Replace:
                src_pos++;
                dest_pos++;
                break;

            case EditType::Insert: dest_pos++; break;

            case EditType::Delete: src_pos++; break;
            }
            i++;
        } while (i < other.size() && other[i].type == type && src_pos == other[i].src_pos &&
                 dest_pos == other[i].dest_pos);

        push_back({type, src_begin, src_pos, dest_begin, dest_pos});
    }

    if (src_pos < other.get_src_len() || dest_pos < other.get_dest_len()) {
        push_back({EditType::None, src_pos, other.get_src_len(), dest_pos, other.get_dest_len()});
    }
}

template <typename T>
struct ScoreAlignment {
    T score;           /**< resulting score of the algorithm */
    size_t src_start;  /**< index into the source string */
    size_t src_end;    /**< index into the source string */
    size_t dest_start; /**< index into the destination string */
    size_t dest_end;   /**< index into the destination string */

    ScoreAlignment() : score(T()), src_start(0), src_end(0), dest_start(0), dest_end(0)
    {}

    ScoreAlignment(T score_, size_t src_start_, size_t src_end_, size_t dest_start_, size_t dest_end_)
        : score(score_),
          src_start(src_start_),
          src_end(src_end_),
          dest_start(dest_start_),
          dest_end(dest_end_)
    {}
};

template <typename T>
inline bool operator==(const ScoreAlignment<T>& a, const ScoreAlignment<T>& b)
{
    return (a.score == b.score) && (a.src_start == b.src_start) && (a.src_end == b.src_end) &&
           (a.dest_start == b.dest_start) && (a.dest_end == b.dest_end);
}

} // namespace rapidfuzz

#include <functional>
#include <iterator>
#include <type_traits>
#include <utility>

namespace rapidfuzz {

namespace detail {
template <typename T>
auto inner_type(T const*) -> T;

template <typename T>
auto inner_type(T const&) -> typename T::value_type;
} // namespace detail

template <typename T>
using char_type = decltype(detail::inner_type(std::declval<T const&>()));

/* backport of std::iter_value_t from C++20
 * This does not cover the complete functionality, but should be enough for
 * the use cases in this library
 */
template <typename T>
using iter_value_t = typename std::iterator_traits<T>::value_type;

// taken from
// https://stackoverflow.com/questions/16893992/check-if-type-can-be-explicitly-converted
template <typename From, typename To>
struct is_explicitly_convertible {
    template <typename T>
    static void f(T);

    template <typename F, typename T>
    static constexpr auto test(int /*unused*/) -> decltype(f(static_cast<T>(std::declval<F>())), true)
    {
        return true;
    }

    template <typename F, typename T>
    static constexpr auto test(...) -> bool
    {
        return false;
    }

    static bool const value = test<From, To>(0);
};

} // namespace rapidfuzz

#include <string>

namespace rapidfuzz::detail {

template <typename InputIt>
class SplittedSentenceView {
public:
    using CharT = iter_value_t<InputIt>;

    SplittedSentenceView(RangeVec<InputIt> sentence) noexcept(
        std::is_nothrow_move_constructible_v<RangeVec<InputIt>>)
        : m_sentence(std::move(sentence))
    {}

    size_t dedupe();
    size_t size() const;

    size_t length() const
    {
        return size();
    }

    bool empty() const
    {
        return m_sentence.empty();
    }

    size_t word_count() const
    {
        return m_sentence.size();
    }

    std::basic_string<CharT> join() const;

    const RangeVec<InputIt>& words() const
    {
        return m_sentence;
    }

private:
    RangeVec<InputIt> m_sentence;
};

template <typename InputIt>
size_t SplittedSentenceView<InputIt>::dedupe()
{
    size_t old_word_count = word_count();
    m_sentence.erase(std::unique(m_sentence.begin(), m_sentence.end()), m_sentence.end());
    return old_word_count - word_count();
}

template <typename InputIt>
size_t SplittedSentenceView<InputIt>::size() const
{
    if (m_sentence.empty()) return 0;

    // there is a whitespace between each word
    size_t result = m_sentence.size() - 1;
    for (const auto& word : m_sentence) {
        result += static_cast<size_t>(std::distance(word.begin(), word.end()));
    }

    return result;
}

template <typename InputIt>
auto SplittedSentenceView<InputIt>::join() const -> std::basic_string<CharT>
{
    if (m_sentence.empty()) {
        return std::basic_string<CharT>();
    }

    auto sentence_iter = m_sentence.begin();
    std::basic_string<CharT> joined(sentence_iter->begin(), sentence_iter->end());
    const std::basic_string<CharT> whitespace{0x20};
    ++sentence_iter;
    for (; sentence_iter != m_sentence.end(); ++sentence_iter) {
        joined.append(whitespace)
            .append(std::basic_string<CharT>(sentence_iter->begin(), sentence_iter->end()));
    }
    return joined;
}

} // namespace rapidfuzz::detail

#include <bitset>
#include <cassert>
#include <cstddef>
#include <limits>
#include <stdint.h>
#include <type_traits>

#if defined(_MSC_VER) && !defined(__clang__)
#    include <intrin.h>
#endif

namespace rapidfuzz::detail {

template <typename T>
T bit_mask_lsb(int n)
{
    T mask = static_cast<T>(-1);
    if (n < static_cast<int>(sizeof(T) * 8)) {
        mask += static_cast<T>(1) << n;
    }
    return mask;
}

template <typename T>
bool bittest(T a, int bit)
{
    return (a >> bit) & 1;
}

/*
 * shift right without undefined behavior for shifts > bit width
 */
template <typename U>
constexpr uint64_t shr64(uint64_t a, U shift)
{
    return (shift < 64) ? a >> shift : 0;
}

/*
 * shift left without undefined behavior for shifts > bit width
 */
template <typename U>
constexpr uint64_t shl64(uint64_t a, U shift)
{
    return (shift < 64) ? a << shift : 0;
}

constexpr uint64_t addc64(uint64_t a, uint64_t b, uint64_t carryin, uint64_t* carryout)
{
    /* todo should use _addcarry_u64 when available */
    a += carryin;
    *carryout = a < carryin;
    a += b;
    *carryout |= a < b;
    return a;
}

template <typename T, typename U>
constexpr T ceil_div(T a, U divisor)
{
    T _div = static_cast<T>(divisor);
    return a / _div + static_cast<T>(a % _div != 0);
}

static inline int popcount(uint64_t x)
{
    return static_cast<int>(std::bitset<64>(x).count());
}

static inline int popcount(uint32_t x)
{
    return static_cast<int>(std::bitset<32>(x).count());
}

static inline int popcount(uint16_t x)
{
    return static_cast<int>(std::bitset<16>(x).count());
}

static inline int popcount(uint8_t x)
{
    static constexpr int bit_count[256] = {
        0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
        1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
        1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
        2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
        1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
        2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
        2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
        3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8};
    return bit_count[x];
}

template <typename T>
constexpr T rotl(T x, unsigned int n)
{
    unsigned int num_bits = std::numeric_limits<T>::digits;
    assert(n < num_bits);
    unsigned int count_mask = num_bits - 1;

#if _MSC_VER && !defined(__clang__)
#    pragma warning(push)
/* unary minus operator applied to unsigned type, result still unsigned */
#    pragma warning(disable : 4146)
#endif
    return (x << n) | (x >> (-n & count_mask));
#if _MSC_VER && !defined(__clang__)
#    pragma warning(pop)
#endif
}

/**
 * Extract the lowest set bit from a. If no bits are set in a returns 0.
 */
template <typename T>
constexpr T blsi(T a)
{
#if _MSC_VER && !defined(__clang__)
#    pragma warning(push)
/* unary minus operator applied to unsigned type, result still unsigned */
#    pragma warning(disable : 4146)
#endif
    return a & -a;
#if _MSC_VER && !defined(__clang__)
#    pragma warning(pop)
#endif
}

/**
 * Clear the lowest set bit in a.
 */
template <typename T>
constexpr T blsr(T x)
{
    return x & (x - 1);
}

/**
 * Sets all the lower bits of the result to 1 up to and including lowest set bit (=1) in a.
 * If a is zero, blsmsk sets all bits to 1.
 */
template <typename T>
constexpr T blsmsk(T a)
{
    return a ^ (a - 1);
}

#if defined(_MSC_VER) && !defined(__clang__)
static inline int countr_zero(uint32_t x)
{
    unsigned long trailing_zero = 0;
    _BitScanForward(&trailing_zero, x);
    return trailing_zero;
}

#    if defined(_M_ARM) || defined(_M_X64)
static inline int countr_zero(uint64_t x)
{
    unsigned long trailing_zero = 0;
    _BitScanForward64(&trailing_zero, x);
    return trailing_zero;
}
#    else
static inline int countr_zero(uint64_t x)
{
    uint32_t msh = (uint32_t)(x >> 32);
    uint32_t lsh = (uint32_t)(x & 0xFFFFFFFF);
    if (lsh != 0) return countr_zero(lsh);
    return 32 + countr_zero(msh);
}
#    endif

#else /*  gcc / clang */
static inline int countr_zero(uint32_t x)
{
    return __builtin_ctz(x);
}

static inline int countr_zero(uint64_t x)
{
    return __builtin_ctzll(x);
}
#endif

template <class T, T... inds, class F>
constexpr void unroll_impl(std::integer_sequence<T, inds...>, F&& f)
{
    (f(std::integral_constant<T, inds>{}), ...);
}

template <class T, T count, class F>
constexpr void unroll(F&& f)
{
    unroll_impl(std::make_integer_sequence<T, count>{}, std::forward<F>(f));
}

} // namespace rapidfuzz::detail

#include <vector>

namespace rapidfuzz::detail {

template <typename InputIt1, typename InputIt2, typename InputIt3>
struct DecomposedSet {
    SplittedSentenceView<InputIt1> difference_ab;
    SplittedSentenceView<InputIt2> difference_ba;
    SplittedSentenceView<InputIt3> intersection;
    DecomposedSet(SplittedSentenceView<InputIt1> diff_ab, SplittedSentenceView<InputIt2> diff_ba,
                  SplittedSentenceView<InputIt3> intersect)
        : difference_ab(std::move(diff_ab)),
          difference_ba(std::move(diff_ba)),
          intersection(std::move(intersect))
    {}
};

/**
 * @defgroup Common Common
 * Common utilities shared among multiple functions
 * @{
 */

static inline double NormSim_to_NormDist(double score_cutoff, double imprecision = 0.00001)
{
    return std::min(1.0, 1.0 - score_cutoff + imprecision);
}

template <typename InputIt1, typename InputIt2>
DecomposedSet<InputIt1, InputIt2, InputIt1> set_decomposition(SplittedSentenceView<InputIt1> a,
                                                              SplittedSentenceView<InputIt2> b);

constexpr double result_cutoff(double result, double score_cutoff)
{
    return (result >= score_cutoff) ? result : 0;
}

template <int Max = 1>
constexpr double norm_distance(int64_t dist, int64_t lensum, double score_cutoff = 0)
{
    double max = static_cast<double>(Max);
    return result_cutoff((lensum > 0) ? (max - max * static_cast<double>(dist) / static_cast<double>(lensum))
                                      : max,
                         score_cutoff);
}

template <int Max = 1>
static inline int64_t score_cutoff_to_distance(double score_cutoff, int64_t lensum)
{
    return static_cast<int64_t>(std::ceil(static_cast<double>(lensum) * (1.0 - score_cutoff / Max)));
}

template <typename InputIt1, typename InputIt2>
StringAffix remove_common_affix(Range<InputIt1>& s1, Range<InputIt2>& s2);

template <typename InputIt1, typename InputIt2>
size_t remove_common_prefix(Range<InputIt1>& s1, Range<InputIt2>& s2);

template <typename InputIt1, typename InputIt2>
size_t remove_common_suffix(Range<InputIt1>& s1, Range<InputIt2>& s2);

template <typename InputIt, typename CharT = iter_value_t<InputIt>>
SplittedSentenceView<InputIt> sorted_split(InputIt first, InputIt last);

/**@}*/

} // namespace rapidfuzz::detail

#include <algorithm>
#include <array>
#include <cctype>
#include <cwctype>
#include <iterator>

namespace rapidfuzz::detail {

template <typename InputIt1, typename InputIt2>
DecomposedSet<InputIt1, InputIt2, InputIt1> set_decomposition(SplittedSentenceView<InputIt1> a,
                                                              SplittedSentenceView<InputIt2> b)
{
    a.dedupe();
    b.dedupe();

    RangeVec<InputIt1> intersection;
    RangeVec<InputIt1> difference_ab;
    RangeVec<InputIt2> difference_ba = b.words();

    for (const auto& current_a : a.words()) {
        auto element_b = std::find(difference_ba.begin(), difference_ba.end(), current_a);

        if (element_b != difference_ba.end()) {
            difference_ba.erase(element_b);
            intersection.push_back(current_a);
        }
        else {
            difference_ab.push_back(current_a);
        }
    }

    return {difference_ab, difference_ba, intersection};
}

/**
 * Removes common prefix of two string views
 */
template <typename InputIt1, typename InputIt2>
size_t remove_common_prefix(Range<InputIt1>& s1, Range<InputIt2>& s2)
{
    auto first1 = std::begin(s1);
    auto prefix =
        std::distance(first1, std::mismatch(first1, std::end(s1), std::begin(s2), std::end(s2)).first);
    s1.remove_prefix(prefix);
    s2.remove_prefix(prefix);
    return static_cast<size_t>(prefix);
}

/**
 * Removes common suffix of two string views
 */
template <typename InputIt1, typename InputIt2>
size_t remove_common_suffix(Range<InputIt1>& s1, Range<InputIt2>& s2)
{
    auto rfirst1 = std::rbegin(s1);
    auto suffix =
        std::distance(rfirst1, std::mismatch(rfirst1, std::rend(s1), std::rbegin(s2), std::rend(s2)).first);
    s1.remove_suffix(suffix);
    s2.remove_suffix(suffix);
    return static_cast<size_t>(suffix);
}

/**
 * Removes common affix of two string views
 */
template <typename InputIt1, typename InputIt2>
StringAffix remove_common_affix(Range<InputIt1>& s1, Range<InputIt2>& s2)
{
    return StringAffix{remove_common_prefix(s1, s2), remove_common_suffix(s1, s2)};
}

template <typename, typename = void>
struct is_space_dispatch_tag : std::integral_constant<int, 0> {};

template <typename CharT>
struct is_space_dispatch_tag<CharT, typename std::enable_if<sizeof(CharT) == 1>::type>
    : std::integral_constant<int, 1> {};

/*
 * Implementation of is_space for char types that are at least 2 Byte in size
 */
template <typename CharT>
bool is_space_impl(const CharT ch, std::integral_constant<int, 0>)
{
    switch (ch) {
    case 0x0009:
    case 0x000A:
    case 0x000B:
    case 0x000C:
    case 0x000D:
    case 0x001C:
    case 0x001D:
    case 0x001E:
    case 0x001F:
    case 0x0020:
    case 0x0085:
    case 0x00A0:
    case 0x1680:
    case 0x2000:
    case 0x2001:
    case 0x2002:
    case 0x2003:
    case 0x2004:
    case 0x2005:
    case 0x2006:
    case 0x2007:
    case 0x2008:
    case 0x2009:
    case 0x200A:
    case 0x2028:
    case 0x2029:
    case 0x202F:
    case 0x205F:
    case 0x3000: return true;
    }
    return false;
}

/*
 * Implementation of is_space for char types that are 1 Byte in size
 */
template <typename CharT>
bool is_space_impl(const CharT ch, std::integral_constant<int, 1>)
{
    switch (ch) {
    case 0x0009:
    case 0x000A:
    case 0x000B:
    case 0x000C:
    case 0x000D:
    case 0x001C:
    case 0x001D:
    case 0x001E:
    case 0x001F:
    case 0x0020: return true;
    }
    return false;
}

/*
 * checks whether unicode characters have the bidirectional
 * type 'WS', 'B' or 'S' or the category 'Zs'
 */
template <typename CharT>
bool is_space(const CharT ch)
{
    return is_space_impl(ch, is_space_dispatch_tag<CharT>{});
}

template <typename InputIt, typename CharT>
SplittedSentenceView<InputIt> sorted_split(InputIt first, InputIt last)
{
    RangeVec<InputIt> splitted;
    auto second = first;

    for (; first != last; first = second + 1) {
        second = std::find_if(first, last, is_space<CharT>);

        if (first != second) {
            splitted.emplace_back(first, second);
        }

        if (second == last) break;
    }

    std::sort(splitted.begin(), splitted.end());

    return SplittedSentenceView<InputIt>(splitted);
}

} // namespace rapidfuzz::detail

#include <cmath>

/* RAPIDFUZZ_LTO_HACK is used to differentiate functions between different
 * translation units to avoid warnings when using lto */
#ifndef RAPIDFUZZ_EXCLUDE_SIMD
#    if __AVX2__
#        define RAPIDFUZZ_SIMD
#        define RAPIDFUZZ_AVX2
#        define RAPIDFUZZ_LTO_HACK 0

#        include <array>
#        include <immintrin.h>
#        include <ostream>
#        include <stdint.h>

namespace rapidfuzz {
namespace detail {
namespace simd_avx2 {

template <typename T>
class native_simd;

template <>
class native_simd<uint64_t> {
public:
    using value_type = uint64_t;

    static const int _size = 4;
    __m256i xmm;

    native_simd() noexcept
    {}

    native_simd(__m256i val) noexcept : xmm(val)
    {}

    native_simd(uint64_t a) noexcept
    {
        xmm = _mm256_set1_epi64x(static_cast<int64_t>(a));
    }

    native_simd(const uint64_t* p) noexcept
    {
        load(p);
    }

    operator __m256i() const noexcept
    {
        return xmm;
    }

    constexpr static int size() noexcept
    {
        return _size;
    }

    native_simd load(const uint64_t* p) noexcept
    {
        xmm = _mm256_set_epi64x(static_cast<int64_t>(p[3]), static_cast<int64_t>(p[2]),
                                static_cast<int64_t>(p[1]), static_cast<int64_t>(p[0]));
        return *this;
    }

    void store(uint64_t* p) const noexcept
    {
        _mm256_store_si256(reinterpret_cast<__m256i*>(p), xmm);
    }

    native_simd operator+(const native_simd b) const noexcept
    {
        return _mm256_add_epi64(xmm, b);
    }

    native_simd& operator+=(const native_simd b) noexcept
    {
        xmm = _mm256_add_epi64(xmm, b);
        return *this;
    }

    native_simd operator-(const native_simd b) const noexcept
    {
        return _mm256_sub_epi64(xmm, b);
    }

    native_simd& operator-=(const native_simd b) noexcept
    {
        xmm = _mm256_sub_epi64(xmm, b);
        return *this;
    }
};

template <>
class native_simd<uint32_t> {
public:
    using value_type = uint32_t;

    static const int _size = 8;
    __m256i xmm;

    native_simd() noexcept
    {}

    native_simd(__m256i val) noexcept : xmm(val)
    {}

    native_simd(uint32_t a) noexcept
    {
        xmm = _mm256_set1_epi32(static_cast<int>(a));
    }

    native_simd(const uint64_t* p) noexcept
    {
        load(p);
    }

    operator __m256i() const
    {
        return xmm;
    }

    constexpr static int size() noexcept
    {
        return _size;
    }

    native_simd load(const uint64_t* p) noexcept
    {
        xmm = _mm256_set_epi64x(static_cast<int64_t>(p[3]), static_cast<int64_t>(p[2]),
                                static_cast<int64_t>(p[1]), static_cast<int64_t>(p[0]));
        return *this;
    }

    void store(uint32_t* p) const noexcept
    {
        _mm256_store_si256(reinterpret_cast<__m256i*>(p), xmm);
    }

    native_simd operator+(const native_simd b) const noexcept
    {
        return _mm256_add_epi32(xmm, b);
    }

    native_simd& operator+=(const native_simd b) noexcept
    {
        xmm = _mm256_add_epi32(xmm, b);
        return *this;
    }

    native_simd operator-(const native_simd b) const noexcept
    {
        return _mm256_sub_epi32(xmm, b);
    }

    native_simd& operator-=(const native_simd b) noexcept
    {
        xmm = _mm256_sub_epi32(xmm, b);
        return *this;
    }
};

template <>
class native_simd<uint16_t> {
public:
    using value_type = uint16_t;

    static const int _size = 16;
    __m256i xmm;

    native_simd() noexcept
    {}

    native_simd(__m256i val) : xmm(val)
    {}

    native_simd(uint16_t a) noexcept
    {
        xmm = _mm256_set1_epi16(static_cast<short>(a));
    }

    native_simd(const uint64_t* p) noexcept
    {
        load(p);
    }

    operator __m256i() const noexcept
    {
        return xmm;
    }

    constexpr static int size() noexcept
    {
        return _size;
    }

    native_simd load(const uint64_t* p) noexcept
    {
        xmm = _mm256_set_epi64x(static_cast<int64_t>(p[3]), static_cast<int64_t>(p[2]),
                                static_cast<int64_t>(p[1]), static_cast<int64_t>(p[0]));
        return *this;
    }

    void store(uint16_t* p) const noexcept
    {
        _mm256_store_si256(reinterpret_cast<__m256i*>(p), xmm);
    }

    native_simd operator+(const native_simd b) const noexcept
    {
        return _mm256_add_epi16(xmm, b);
    }

    native_simd& operator+=(const native_simd b) noexcept
    {
        xmm = _mm256_add_epi16(xmm, b);
        return *this;
    }

    native_simd operator-(const native_simd b) const noexcept
    {
        return _mm256_sub_epi16(xmm, b);
    }

    native_simd& operator-=(const native_simd b) noexcept
    {
        xmm = _mm256_sub_epi16(xmm, b);
        return *this;
    }
};

template <>
class native_simd<uint8_t> {
public:
    using value_type = uint8_t;

    static const int _size = 32;
    __m256i xmm;

    native_simd() noexcept
    {}

    native_simd(__m256i val) noexcept : xmm(val)
    {}

    native_simd(uint8_t a) noexcept
    {
        xmm = _mm256_set1_epi8(static_cast<char>(a));
    }

    native_simd(const uint64_t* p) noexcept
    {
        load(p);
    }

    operator __m256i() const noexcept
    {
        return xmm;
    }

    constexpr static int size() noexcept
    {
        return _size;
    }

    native_simd load(const uint64_t* p) noexcept
    {
        xmm = _mm256_set_epi64x(static_cast<int64_t>(p[3]), static_cast<int64_t>(p[2]),
                                static_cast<int64_t>(p[1]), static_cast<int64_t>(p[0]));
        return *this;
    }

    void store(uint8_t* p) const noexcept
    {
        _mm256_store_si256(reinterpret_cast<__m256i*>(p), xmm);
    }

    native_simd operator+(const native_simd b) const noexcept
    {
        return _mm256_add_epi8(xmm, b);
    }

    native_simd& operator+=(const native_simd b) noexcept
    {
        xmm = _mm256_add_epi8(xmm, b);
        return *this;
    }

    native_simd operator-(const native_simd b) const noexcept
    {
        return _mm256_sub_epi8(xmm, b);
    }

    native_simd& operator-=(const native_simd b) noexcept
    {
        xmm = _mm256_sub_epi8(xmm, b);
        return *this;
    }
};

template <typename T>
std::ostream& operator<<(std::ostream& os, const native_simd<T>& a)
{
    alignas(32) std::array<T, native_simd<T>::size()> res;
    a.store(&res[0]);

    for (size_t i = res.size() - 1; i != 0; i--)
        os << std::bitset<std::numeric_limits<T>::digits>(res[i]) << "|";

    os << std::bitset<std::numeric_limits<T>::digits>(res[0]);
    return os;
}

template <typename T>
__m256i hadd_impl(__m256i x) noexcept;

template <>
inline __m256i hadd_impl<uint8_t>(__m256i x) noexcept
{
    return x;
}

template <>
inline __m256i hadd_impl<uint16_t>(__m256i x) noexcept
{
    const __m256i mask = _mm256_set1_epi16(0x001f);
    __m256i y = _mm256_srli_si256(x, 1);
    x = _mm256_add_epi16(x, y);
    return _mm256_and_si256(x, mask);
}

template <>
inline __m256i hadd_impl<uint32_t>(__m256i x) noexcept
{
    const __m256i mask = _mm256_set1_epi32(0x0000003F);
    x = hadd_impl<uint16_t>(x);
    __m256i y = _mm256_srli_si256(x, 2);
    x = _mm256_add_epi32(x, y);
    return _mm256_and_si256(x, mask);
}

template <>
inline __m256i hadd_impl<uint64_t>(__m256i x) noexcept
{
    return _mm256_sad_epu8(x, _mm256_setzero_si256());
}

/* based on the paper `Faster Population Counts Using AVX2 Instructions` */
template <typename T>
native_simd<T> popcount_impl(const native_simd<T>& v) noexcept
{
    __m256i lookup = _mm256_setr_epi8(0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4, 0, 1, 1, 2, 1, 2, 2, 3,
                                      1, 2, 2, 3, 2, 3, 3, 4);
    const __m256i low_mask = _mm256_set1_epi8(0x0F);
    __m256i lo = _mm256_and_si256(v, low_mask);
    __m256i hi = _mm256_and_si256(_mm256_srli_epi32(v, 4), low_mask);
    __m256i popcnt1 = _mm256_shuffle_epi8(lookup, lo);
    __m256i popcnt2 = _mm256_shuffle_epi8(lookup, hi);
    __m256i total = _mm256_add_epi8(popcnt1, popcnt2);
    return hadd_impl<T>(total);
}

template <typename T>
std::array<T, native_simd<T>::size()> popcount(const native_simd<T>& a) noexcept
{
    alignas(32) std::array<T, native_simd<T>::size()> res;
    popcount_impl(a).store(&res[0]);
    return res;
}

// function andnot: a & ~ b
template <typename T>
native_simd<T> andnot(const native_simd<T>& a, const native_simd<T>& b)
{
    return _mm256_andnot_si256(b, a);
}

static inline native_simd<uint8_t> operator==(const native_simd<uint8_t>& a,
                                              const native_simd<uint8_t>& b) noexcept
{
    return _mm256_cmpeq_epi8(a, b);
}

static inline native_simd<uint16_t> operator==(const native_simd<uint16_t>& a,
                                               const native_simd<uint16_t>& b) noexcept
{
    return _mm256_cmpeq_epi16(a, b);
}

static inline native_simd<uint32_t> operator==(const native_simd<uint32_t>& a,
                                               const native_simd<uint32_t>& b) noexcept
{
    return _mm256_cmpeq_epi32(a, b);
}

static inline native_simd<uint64_t> operator==(const native_simd<uint64_t>& a,
                                               const native_simd<uint64_t>& b) noexcept
{
    return _mm256_cmpeq_epi64(a, b);
}

static inline native_simd<uint8_t> operator<<(const native_simd<uint8_t>& a, int b) noexcept
{
    return _mm256_and_si256(_mm256_slli_epi16(a, b),
                            _mm256_set1_epi8(static_cast<char>(0xFF << (b & 0b1111))));
}

static inline native_simd<uint16_t> operator<<(const native_simd<uint16_t>& a, int b) noexcept
{
    return _mm256_slli_epi16(a, b);
}

static inline native_simd<uint32_t> operator<<(const native_simd<uint32_t>& a, int b) noexcept
{
    return _mm256_slli_epi32(a, b);
}

static inline native_simd<uint64_t> operator<<(const native_simd<uint64_t>& a, int b) noexcept
{
    return _mm256_slli_epi64(a, b);
}

template <typename T>
native_simd<T> operator&(const native_simd<T>& a, const native_simd<T>& b) noexcept
{
    return _mm256_and_si256(a, b);
}

template <typename T>
native_simd<T> operator&=(native_simd<T>& a, const native_simd<T>& b) noexcept
{
    a = a & b;
    return a;
}

template <typename T>
native_simd<T> operator|(const native_simd<T>& a, const native_simd<T>& b) noexcept
{
    return _mm256_or_si256(a, b);
}

template <typename T>
native_simd<T> operator|=(native_simd<T>& a, const native_simd<T>& b) noexcept
{
    a = a | b;
    return a;
}

template <typename T>
native_simd<T> operator^(const native_simd<T>& a, const native_simd<T>& b) noexcept
{
    return _mm256_xor_si256(a, b);
}

template <typename T>
native_simd<T> operator^=(native_simd<T>& a, const native_simd<T>& b) noexcept
{
    a = a ^ b;
    return a;
}

template <typename T>
native_simd<T> operator~(const native_simd<T>& a) noexcept
{
    return _mm256_xor_si256(a, _mm256_set1_epi32(-1));
}

} // namespace simd_avx2
} // namespace detail
} // namespace rapidfuzz

#    elif (defined(_M_AMD64) || defined(_M_X64)) || defined(__SSE2__)
#        define RAPIDFUZZ_SIMD
#        define RAPIDFUZZ_SSE2
#        define RAPIDFUZZ_LTO_HACK 1

#        include <array>
#        include <emmintrin.h>
#        include <ostream>
#        include <stdint.h>

namespace rapidfuzz {
namespace detail {
namespace simd_sse2 {

template <typename T>
class native_simd;

template <>
class native_simd<uint64_t> {
public:
    static const int _size = 2;
    __m128i xmm;

    native_simd() noexcept
    {}

    native_simd(__m128i val) noexcept : xmm(val)
    {}

    native_simd(uint64_t a) noexcept
    {
        xmm = _mm_set1_epi64x(static_cast<int64_t>(a));
    }

    native_simd(const uint64_t* p) noexcept
    {
        load(p);
    }

    operator __m128i() const noexcept
    {
        return xmm;
    }

    constexpr static int size() noexcept
    {
        return _size;
    }

    native_simd load(const uint64_t* p) noexcept
    {
        xmm = _mm_set_epi64x(static_cast<int64_t>(p[1]), static_cast<int64_t>(p[0]));
        return *this;
    }

    void store(uint64_t* p) const noexcept
    {
        _mm_store_si128(reinterpret_cast<__m128i*>(p), xmm);
    }

    native_simd operator+(const native_simd b) const noexcept
    {
        return _mm_add_epi64(xmm, b);
    }

    native_simd& operator+=(const native_simd b) noexcept
    {
        xmm = _mm_add_epi64(xmm, b);
        return *this;
    }

    native_simd operator-(const native_simd b) const noexcept
    {
        return _mm_sub_epi64(xmm, b);
    }

    native_simd& operator-=(const native_simd b) noexcept
    {
        xmm = _mm_sub_epi64(xmm, b);
        return *this;
    }
};

template <>
class native_simd<uint32_t> {
public:
    static const int _size = 4;
    __m128i xmm;

    native_simd() noexcept
    {}

    native_simd(__m128i val) noexcept : xmm(val)
    {}

    native_simd(uint32_t a) noexcept
    {
        xmm = _mm_set1_epi32(static_cast<int>(a));
    }

    native_simd(const uint64_t* p) noexcept
    {
        load(p);
    }

    operator __m128i() const noexcept
    {
        return xmm;
    }

    constexpr static int size() noexcept
    {
        return _size;
    }

    native_simd load(const uint64_t* p) noexcept
    {
        xmm = _mm_set_epi64x(static_cast<int64_t>(p[1]), static_cast<int64_t>(p[0]));
        return *this;
    }

    void store(uint32_t* p) const noexcept
    {
        _mm_store_si128(reinterpret_cast<__m128i*>(p), xmm);
    }

    native_simd operator+(const native_simd b) const noexcept
    {
        return _mm_add_epi32(xmm, b);
    }

    native_simd& operator+=(const native_simd b) noexcept
    {
        xmm = _mm_add_epi32(xmm, b);
        return *this;
    }

    native_simd operator-(const native_simd b) const noexcept
    {
        return _mm_sub_epi32(xmm, b);
    }

    native_simd& operator-=(const native_simd b) noexcept
    {
        xmm = _mm_sub_epi32(xmm, b);
        return *this;
    }
};

template <>
class native_simd<uint16_t> {
public:
    static const int _size = 8;
    __m128i xmm;

    native_simd() noexcept
    {}

    native_simd(__m128i val) noexcept : xmm(val)
    {}

    native_simd(uint16_t a) noexcept
    {
        xmm = _mm_set1_epi16(static_cast<short>(a));
    }

    native_simd(const uint64_t* p) noexcept
    {
        load(p);
    }

    operator __m128i() const noexcept
    {
        return xmm;
    }

    constexpr static int size() noexcept
    {
        return _size;
    }

    native_simd load(const uint64_t* p) noexcept
    {
        xmm = _mm_set_epi64x(static_cast<int64_t>(p[1]), static_cast<int64_t>(p[0]));
        return *this;
    }

    void store(uint16_t* p) const noexcept
    {
        _mm_store_si128(reinterpret_cast<__m128i*>(p), xmm);
    }

    native_simd operator+(const native_simd b) const noexcept
    {
        return _mm_add_epi16(xmm, b);
    }

    native_simd& operator+=(const native_simd b) noexcept
    {
        xmm = _mm_add_epi16(xmm, b);
        return *this;
    }

    native_simd operator-(const native_simd b) const noexcept
    {
        return _mm_sub_epi16(xmm, b);
    }

    native_simd& operator-=(const native_simd b) noexcept
    {
        xmm = _mm_sub_epi16(xmm, b);
        return *this;
    }
};

template <>
class native_simd<uint8_t> {
public:
    static const int _size = 16;
    __m128i xmm;

    native_simd() noexcept
    {}

    native_simd(__m128i val) noexcept : xmm(val)
    {}

    native_simd(uint8_t a) noexcept
    {
        xmm = _mm_set1_epi8(static_cast<char>(a));
    }

    native_simd(const uint64_t* p) noexcept
    {
        load(p);
    }

    operator __m128i() const noexcept
    {
        return xmm;
    }

    constexpr static int size() noexcept
    {
        return _size;
    }

    native_simd load(const uint64_t* p) noexcept
    {
        xmm = _mm_set_epi64x(static_cast<int64_t>(p[1]), static_cast<int64_t>(p[0]));
        return *this;
    }

    void store(uint8_t* p) const noexcept
    {
        _mm_store_si128(reinterpret_cast<__m128i*>(p), xmm);
    }

    native_simd operator+(const native_simd b) const noexcept
    {
        return _mm_add_epi8(xmm, b);
    }

    native_simd& operator+=(const native_simd b) noexcept
    {
        xmm = _mm_add_epi8(xmm, b);
        return *this;
    }

    native_simd operator-(const native_simd b) const noexcept
    {
        return _mm_sub_epi8(xmm, b);
    }

    native_simd& operator-=(const native_simd b) noexcept
    {
        xmm = _mm_sub_epi8(xmm, b);
        return *this;
    }
};

template <typename T>
std::ostream& operator<<(std::ostream& os, const native_simd<T>& a)
{
    alignas(32) std::array<T, native_simd<T>::size()> res;
    a.store(&res[0]);

    for (size_t i = res.size() - 1; i != 0; i--)
        os << std::bitset<std::numeric_limits<T>::digits>(res[i]) << "|";

    os << std::bitset<std::numeric_limits<T>::digits>(res[0]);
    return os;
}

template <typename T>
__m128i hadd_impl(__m128i x) noexcept;

template <>
inline __m128i hadd_impl<uint8_t>(__m128i x) noexcept
{
    return x;
}

template <>
inline __m128i hadd_impl<uint16_t>(__m128i x) noexcept
{
    const __m128i mask = _mm_set1_epi16(0x001f);
    __m128i y = _mm_srli_si128(x, 1);
    x = _mm_add_epi16(x, y);
    return _mm_and_si128(x, mask);
}

template <>
inline __m128i hadd_impl<uint32_t>(__m128i x) noexcept
{
    const __m128i mask = _mm_set1_epi32(0x0000003f);
    x = hadd_impl<uint16_t>(x);
    __m128i y = _mm_srli_si128(x, 2);
    x = _mm_add_epi32(x, y);
    return _mm_and_si128(x, mask);
}

template <>
inline __m128i hadd_impl<uint64_t>(__m128i x) noexcept
{
    return _mm_sad_epu8(x, _mm_setzero_si128());
}

template <typename T>
native_simd<T> popcount_impl(const native_simd<T>& v) noexcept
{
    const __m128i m1 = _mm_set1_epi8(0x55);
    const __m128i m2 = _mm_set1_epi8(0x33);
    const __m128i m3 = _mm_set1_epi8(0x0F);

    /* Note: if we returned x here it would be like _mm_popcnt_epi1(x) */
    __m128i y;
    __m128i x = v;
    /* add even and odd bits*/
    y = _mm_srli_epi64(x, 1); // put even bits in odd place
    y = _mm_and_si128(y, m1); // mask out the even bits (0x55)
    x = _mm_subs_epu8(x, y);  // shortcut to mask even bits and add
    /* if we just returned x here it would be like popcnt_epi2(x) */
    /* now add the half nibbles */
    y = _mm_srli_epi64(x, 2); // move half nibbles in place to add
    y = _mm_and_si128(y, m2); // mask off the extra half nibbles (0x0f)
    x = _mm_and_si128(x, m2); // ditto
    x = _mm_adds_epu8(x, y);  // totals are a maximum of 5 bits (0x1f)
    /* if we just returned x here it would be like popcnt_epi4(x) */
    /* now add the nibbles */
    y = _mm_srli_epi64(x, 4); // move nibbles in place to add
    x = _mm_adds_epu8(x, y);  // totals are a maximum of 6 bits (0x3f)
    x = _mm_and_si128(x, m3); // mask off the extra bits

    /* todo use when sse3 available
    __m128i lookup = _mm_setr_epi8(0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4);
    const __m128i low_mask = _mm_set1_epi8(0x0F);
    __m128i lo = _mm_and_si128(v, low_mask);
    __m128i hi = _mm_and_si256(_mm_srli_epi32(v, 4), low_mask);
    __m128i popcnt1 = _mm_shuffle_epi8(lookup, lo);
    __m128i popcnt2 = _mm_shuffle_epi8(lookup, hi);
    __m128i total = _mm_add_epi8(popcnt1, popcnt2);*/

    return hadd_impl<T>(x);
}

template <typename T>
std::array<T, native_simd<T>::size()> popcount(const native_simd<T>& a) noexcept
{
    alignas(16) std::array<T, native_simd<T>::size()> res;
    popcount_impl(a).store(&res[0]);
    return res;
}

// function andnot: a & ~ b
template <typename T>
native_simd<T> andnot(const native_simd<T>& a, const native_simd<T>& b)
{
    return _mm_andnot_si128(b, a);
}

static inline native_simd<uint8_t> operator==(const native_simd<uint8_t>& a,
                                              const native_simd<uint8_t>& b) noexcept
{
    return _mm_cmpeq_epi8(a, b);
}

static inline native_simd<uint16_t> operator==(const native_simd<uint16_t>& a,
                                               const native_simd<uint16_t>& b) noexcept
{
    return _mm_cmpeq_epi16(a, b);
}

static inline native_simd<uint32_t> operator==(const native_simd<uint32_t>& a,
                                               const native_simd<uint32_t>& b) noexcept
{
    return _mm_cmpeq_epi32(a, b);
}

static inline native_simd<uint64_t> operator==(const native_simd<uint64_t>& a,
                                               const native_simd<uint64_t>& b) noexcept
{
    // no 64 compare instruction. Do two 32 bit compares
    __m128i com32 = _mm_cmpeq_epi32(a, b);           // 32 bit compares
    __m128i com32s = _mm_shuffle_epi32(com32, 0xB1); // swap low and high dwords
    __m128i test = _mm_and_si128(com32, com32s);     // low & high
    __m128i teste = _mm_srai_epi32(test, 31);        // extend sign bit to 32 bits
    __m128i testee = _mm_shuffle_epi32(teste, 0xF5); // extend sign bit to 64 bits
    return testee;
}

static inline native_simd<uint8_t> operator<<(const native_simd<uint8_t>& a, int b) noexcept
{
    return _mm_and_si128(_mm_slli_epi16(a, b), _mm_set1_epi8(static_cast<char>(0xFF << (b & 0b1111))));
}

static inline native_simd<uint16_t> operator<<(const native_simd<uint16_t>& a, int b) noexcept
{
    return _mm_slli_epi16(a, b);
}

static inline native_simd<uint32_t> operator<<(const native_simd<uint32_t>& a, int b) noexcept
{
    return _mm_slli_epi32(a, b);
}

static inline native_simd<uint64_t> operator<<(const native_simd<uint64_t>& a, int b) noexcept
{
    return _mm_slli_epi64(a, b);
}

template <typename T>
native_simd<T> operator&(const native_simd<T>& a, const native_simd<T>& b) noexcept
{
    return _mm_and_si128(a, b);
}

template <typename T>
native_simd<T> operator&=(native_simd<T>& a, const native_simd<T>& b) noexcept
{
    a = a & b;
    return a;
}

template <typename T>
native_simd<T> operator|(const native_simd<T>& a, const native_simd<T>& b) noexcept
{
    return _mm_or_si128(a, b);
}

template <typename T>
native_simd<T> operator|=(native_simd<T>& a, const native_simd<T>& b) noexcept
{
    a = a | b;
    return a;
}

template <typename T>
native_simd<T> operator^(const native_simd<T>& a, const native_simd<T>& b) noexcept
{
    return _mm_xor_si128(a, b);
}

template <typename T>
native_simd<T> operator^=(native_simd<T>& a, const native_simd<T>& b) noexcept
{
    a = a ^ b;
    return a;
}

template <typename T>
native_simd<T> operator~(const native_simd<T>& a) noexcept
{
    return _mm_xor_si128(a, _mm_set1_epi32(-1));
}

} // namespace simd_sse2
} // namespace detail
} // namespace rapidfuzz

#    endif
#endif
#include <type_traits>

namespace rapidfuzz::detail {

template <typename T, typename... Args>
struct NormalizedMetricBase {
    template <typename InputIt1, typename InputIt2,
              typename = std::enable_if_t<!std::is_same_v<InputIt2, double>>>
    static double normalized_distance(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2,
                                      Args... args, double score_cutoff, double score_hint)
    {
        return _normalized_distance(Range(first1, last1), Range(first2, last2), std::forward<Args>(args)...,
                                    score_cutoff, score_hint);
    }

    template <typename Sentence1, typename Sentence2>
    static double normalized_distance(const Sentence1& s1, const Sentence2& s2, Args... args,
                                      double score_cutoff, double score_hint)
    {
        return _normalized_distance(Range(s1), Range(s2), std::forward<Args>(args)..., score_cutoff,
                                    score_hint);
    }

    template <typename InputIt1, typename InputIt2,
              typename = std::enable_if_t<!std::is_same_v<InputIt2, double>>>
    static double normalized_similarity(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2,
                                        Args... args, double score_cutoff, double score_hint)
    {
        return _normalized_similarity(Range(first1, last1), Range(first2, last2), std::forward<Args>(args)...,
                                      score_cutoff, score_hint);
    }

    template <typename Sentence1, typename Sentence2>
    static double normalized_similarity(const Sentence1& s1, const Sentence2& s2, Args... args,
                                        double score_cutoff, double score_hint)
    {
        return _normalized_similarity(Range(s1), Range(s2), std::forward<Args>(args)..., score_cutoff,
                                      score_hint);
    }

protected:
    template <typename InputIt1, typename InputIt2>
    static double _normalized_distance(Range<InputIt1> s1, Range<InputIt2> s2, Args... args,
                                       double score_cutoff, double score_hint)
    {
        auto maximum = T::maximum(s1, s2, args...);
        auto cutoff_distance =
            static_cast<decltype(maximum)>(std::ceil(static_cast<double>(maximum) * score_cutoff));
        auto hint_distance =
            static_cast<decltype(maximum)>(std::ceil(static_cast<double>(maximum) * score_hint));
        auto dist = T::_distance(s1, s2, std::forward<Args>(args)..., cutoff_distance, hint_distance);
        double norm_dist = (maximum != 0) ? static_cast<double>(dist) / static_cast<double>(maximum) : 0.0;
        return (norm_dist <= score_cutoff) ? norm_dist : 1.0;
    }

    template <typename InputIt1, typename InputIt2>
    static double _normalized_similarity(Range<InputIt1> s1, Range<InputIt2> s2, Args... args,
                                         double score_cutoff, double score_hint)
    {
        double cutoff_score = NormSim_to_NormDist(score_cutoff);
        double hint_score = NormSim_to_NormDist(score_hint);
        double norm_dist =
            _normalized_distance(s1, s2, std::forward<Args>(args)..., cutoff_score, hint_score);
        double norm_sim = 1.0 - norm_dist;
        return (norm_sim >= score_cutoff) ? norm_sim : 0.0;
    }

    NormalizedMetricBase()
    {}
    friend T;
};

template <typename T, typename ResType, int64_t WorstSimilarity, int64_t WorstDistance, typename... Args>
struct DistanceBase : public NormalizedMetricBase<T, Args...> {
    template <typename InputIt1, typename InputIt2,
              typename = std::enable_if_t<!std::is_same_v<InputIt2, double>>>
    static ResType distance(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2, Args... args,
                            ResType score_cutoff, ResType score_hint)
    {
        return T::_distance(Range(first1, last1), Range(first2, last2), std::forward<Args>(args)...,
                            score_cutoff, score_hint);
    }

    template <typename Sentence1, typename Sentence2>
    static ResType distance(const Sentence1& s1, const Sentence2& s2, Args... args, ResType score_cutoff,
                            ResType score_hint)
    {
        return T::_distance(Range(s1), Range(s2), std::forward<Args>(args)..., score_cutoff, score_hint);
    }

    template <typename InputIt1, typename InputIt2,
              typename = std::enable_if_t<!std::is_same_v<InputIt2, double>>>
    static ResType similarity(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2, Args... args,
                              ResType score_cutoff, ResType score_hint)
    {
        return _similarity(Range(first1, last1), Range(first2, last2), std::forward<Args>(args)...,
                           score_cutoff, score_hint);
    }

    template <typename Sentence1, typename Sentence2>
    static ResType similarity(const Sentence1& s1, const Sentence2& s2, Args... args, ResType score_cutoff,
                              ResType score_hint)
    {
        return _similarity(Range(s1), Range(s2), std::forward<Args>(args)..., score_cutoff, score_hint);
    }

protected:
    template <typename InputIt1, typename InputIt2>
    static ResType _similarity(Range<InputIt1> s1, Range<InputIt2> s2, Args... args, ResType score_cutoff,
                               ResType score_hint)
    {
        auto maximum = T::maximum(s1, s2, args...);
        if (score_cutoff > maximum) return 0;

        score_hint = std::min(score_cutoff, score_hint);
        ResType cutoff_distance = maximum - score_cutoff;
        ResType hint_distance = maximum - score_hint;
        ResType dist = T::_distance(s1, s2, std::forward<Args>(args)..., cutoff_distance, hint_distance);
        ResType sim = maximum - dist;
        return (sim >= score_cutoff) ? sim : 0;
    }

    DistanceBase()
    {}
    friend T;
};

template <typename T, typename ResType, int64_t WorstSimilarity, int64_t WorstDistance, typename... Args>
struct SimilarityBase : public NormalizedMetricBase<T, Args...> {
    template <typename InputIt1, typename InputIt2,
              typename = std::enable_if_t<!std::is_same_v<InputIt2, double>>>
    static ResType distance(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2, Args... args,
                            ResType score_cutoff, ResType score_hint)
    {
        return _distance(Range(first1, last1), Range(first2, last2), std::forward<Args>(args)...,
                         score_cutoff, score_hint);
    }

    template <typename Sentence1, typename Sentence2>
    static ResType distance(const Sentence1& s1, const Sentence2& s2, Args... args, ResType score_cutoff,
                            ResType score_hint)
    {
        return _distance(Range(s1), Range(s2), std::forward<Args>(args)..., score_cutoff, score_hint);
    }

    template <typename InputIt1, typename InputIt2,
              typename = std::enable_if_t<!std::is_same_v<InputIt2, double>>>
    static ResType similarity(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2, Args... args,
                              ResType score_cutoff, ResType score_hint)
    {
        return T::_similarity(Range(first1, last1), Range(first2, last2), std::forward<Args>(args)...,
                              score_cutoff, score_hint);
    }

    template <typename Sentence1, typename Sentence2>
    static ResType similarity(const Sentence1& s1, const Sentence2& s2, Args... args, ResType score_cutoff,
                              ResType score_hint)
    {
        return T::_similarity(Range(s1), Range(s2), std::forward<Args>(args)..., score_cutoff, score_hint);
    }

protected:
    template <typename InputIt1, typename InputIt2>
    static ResType _distance(Range<InputIt1> s1, Range<InputIt2> s2, Args... args, ResType score_cutoff,
                             ResType score_hint)
    {
        auto maximum = T::maximum(s1, s2, args...);
        ResType cutoff_similarity =
            (maximum >= score_cutoff) ? maximum - score_cutoff : static_cast<ResType>(WorstSimilarity);
        ResType hint_similarity =
            (maximum >= score_hint) ? maximum - score_hint : static_cast<ResType>(WorstSimilarity);
        ResType sim = T::_similarity(s1, s2, std::forward<Args>(args)..., cutoff_similarity, hint_similarity);
        ResType dist = maximum - sim;

        if constexpr (std::is_floating_point_v<ResType>)
            return (dist <= score_cutoff) ? dist : 1.0;
        else
            return (dist <= score_cutoff) ? dist : score_cutoff + 1;
    }

    SimilarityBase()
    {}
    friend T;
};

template <typename T>
struct CachedNormalizedMetricBase {
    template <typename InputIt2>
    double normalized_distance(InputIt2 first2, InputIt2 last2, double score_cutoff = 1.0,
                               double score_hint = 1.0) const
    {
        return _normalized_distance(Range(first2, last2), score_cutoff, score_hint);
    }

    template <typename Sentence2>
    double normalized_distance(const Sentence2& s2, double score_cutoff = 1.0, double score_hint = 1.0) const
    {
        return _normalized_distance(Range(s2), score_cutoff, score_hint);
    }

    template <typename InputIt2>
    double normalized_similarity(InputIt2 first2, InputIt2 last2, double score_cutoff = 0.0,
                                 double score_hint = 0.0) const
    {
        return _normalized_similarity(Range(first2, last2), score_cutoff, score_hint);
    }

    template <typename Sentence2>
    double normalized_similarity(const Sentence2& s2, double score_cutoff = 0.0,
                                 double score_hint = 0.0) const
    {
        return _normalized_similarity(Range(s2), score_cutoff, score_hint);
    }

protected:
    template <typename InputIt2>
    double _normalized_distance(Range<InputIt2> s2, double score_cutoff, double score_hint) const
    {
        const T& derived = static_cast<const T&>(*this);
        auto maximum = derived.maximum(s2);
        auto cutoff_distance =
            static_cast<decltype(maximum)>(std::ceil(static_cast<double>(maximum) * score_cutoff));
        auto hint_distance =
            static_cast<decltype(maximum)>(std::ceil(static_cast<double>(maximum) * score_hint));
        double dist = static_cast<double>(derived._distance(s2, cutoff_distance, hint_distance));
        double norm_dist = (maximum != 0) ? dist / static_cast<double>(maximum) : 0.0;
        return (norm_dist <= score_cutoff) ? norm_dist : 1.0;
    }

    template <typename InputIt2>
    double _normalized_similarity(Range<InputIt2> s2, double score_cutoff, double score_hint) const
    {
        double cutoff_score = NormSim_to_NormDist(score_cutoff);
        double hint_score = NormSim_to_NormDist(score_hint);
        double norm_dist = _normalized_distance(s2, cutoff_score, hint_score);
        double norm_sim = 1.0 - norm_dist;
        return (norm_sim >= score_cutoff) ? norm_sim : 0.0;
    }

    CachedNormalizedMetricBase()
    {}
    friend T;
};

template <typename T, typename ResType, int64_t WorstSimilarity, int64_t WorstDistance>
struct CachedDistanceBase : public CachedNormalizedMetricBase<T> {
    template <typename InputIt2>
    ResType distance(InputIt2 first2, InputIt2 last2,
                     ResType score_cutoff = static_cast<ResType>(WorstDistance),
                     ResType score_hint = static_cast<ResType>(WorstDistance)) const
    {
        const T& derived = static_cast<const T&>(*this);
        return derived._distance(Range(first2, last2), score_cutoff, score_hint);
    }

    template <typename Sentence2>
    ResType distance(const Sentence2& s2, ResType score_cutoff = static_cast<ResType>(WorstDistance),
                     ResType score_hint = static_cast<ResType>(WorstDistance)) const
    {
        const T& derived = static_cast<const T&>(*this);
        return derived._distance(Range(s2), score_cutoff, score_hint);
    }

    template <typename InputIt2>
    ResType similarity(InputIt2 first2, InputIt2 last2,
                       ResType score_cutoff = static_cast<ResType>(WorstSimilarity),
                       ResType score_hint = static_cast<ResType>(WorstSimilarity)) const
    {
        return _similarity(Range(first2, last2), score_cutoff, score_hint);
    }

    template <typename Sentence2>
    ResType similarity(const Sentence2& s2, ResType score_cutoff = static_cast<ResType>(WorstSimilarity),
                       ResType score_hint = static_cast<ResType>(WorstSimilarity)) const
    {
        return _similarity(Range(s2), score_cutoff, score_hint);
    }

protected:
    template <typename InputIt2>
    ResType _similarity(Range<InputIt2> s2, ResType score_cutoff, ResType score_hint) const
    {
        const T& derived = static_cast<const T&>(*this);
        ResType maximum = derived.maximum(s2);
        if (score_cutoff > maximum) return 0;

        score_hint = std::min(score_cutoff, score_hint);
        ResType cutoff_distance = maximum - score_cutoff;
        ResType hint_distance = maximum - score_hint;
        ResType dist = derived._distance(s2, cutoff_distance, hint_distance);
        ResType sim = maximum - dist;
        return (sim >= score_cutoff) ? sim : 0;
    }

    CachedDistanceBase()
    {}
    friend T;
};

template <typename T, typename ResType, int64_t WorstSimilarity, int64_t WorstDistance>
struct CachedSimilarityBase : public CachedNormalizedMetricBase<T> {
    template <typename InputIt2>
    ResType distance(InputIt2 first2, InputIt2 last2,
                     ResType score_cutoff = static_cast<ResType>(WorstDistance),
                     ResType score_hint = static_cast<ResType>(WorstDistance)) const
    {
        return _distance(Range(first2, last2), score_cutoff, score_hint);
    }

    template <typename Sentence2>
    ResType distance(const Sentence2& s2, ResType score_cutoff = static_cast<ResType>(WorstDistance),
                     ResType score_hint = static_cast<ResType>(WorstDistance)) const
    {
        return _distance(Range(s2), score_cutoff, score_hint);
    }

    template <typename InputIt2>
    ResType similarity(InputIt2 first2, InputIt2 last2,
                       ResType score_cutoff = static_cast<ResType>(WorstSimilarity),
                       ResType score_hint = static_cast<ResType>(WorstSimilarity)) const
    {
        const T& derived = static_cast<const T&>(*this);
        return derived._similarity(Range(first2, last2), score_cutoff, score_hint);
    }

    template <typename Sentence2>
    ResType similarity(const Sentence2& s2, ResType score_cutoff = static_cast<ResType>(WorstSimilarity),
                       ResType score_hint = static_cast<ResType>(WorstSimilarity)) const
    {
        const T& derived = static_cast<const T&>(*this);
        return derived._similarity(Range(s2), score_cutoff, score_hint);
    }

protected:
    template <typename InputIt2>
    ResType _distance(Range<InputIt2> s2, ResType score_cutoff, ResType score_hint) const
    {
        const T& derived = static_cast<const T&>(*this);
        ResType maximum = derived.maximum(s2);
        ResType cutoff_similarity = (maximum > score_cutoff) ? maximum - score_cutoff : 0;
        ResType hint_similarity = (maximum > score_hint) ? maximum - score_hint : 0;
        ResType sim = derived._similarity(s2, cutoff_similarity, hint_similarity);
        ResType dist = maximum - sim;

        if constexpr (std::is_floating_point_v<ResType>)
            return (dist <= score_cutoff) ? dist : 1.0;
        else
            return (dist <= score_cutoff) ? dist : score_cutoff + 1;
    }

    CachedSimilarityBase()
    {}
    friend T;
};

template <typename T>
struct MultiNormalizedMetricBase {
    template <typename InputIt2>
    void normalized_distance(double* scores, size_t score_count, InputIt2 first2, InputIt2 last2,
                             double score_cutoff = 1.0) const
    {
        _normalized_distance(scores, score_count, Range(first2, last2), score_cutoff);
    }

    template <typename Sentence2>
    void normalized_distance(double* scores, size_t score_count, const Sentence2& s2,
                             double score_cutoff = 1.0) const
    {
        _normalized_distance(scores, score_count, Range(s2), score_cutoff);
    }

    template <typename InputIt2>
    void normalized_similarity(double* scores, size_t score_count, InputIt2 first2, InputIt2 last2,
                               double score_cutoff = 0.0) const
    {
        _normalized_similarity(scores, score_count, Range(first2, last2), score_cutoff);
    }

    template <typename Sentence2>
    void normalized_similarity(double* scores, size_t score_count, const Sentence2& s2,
                               double score_cutoff = 0.0) const
    {
        _normalized_similarity(scores, score_count, Range(s2), score_cutoff);
    }

protected:
    template <typename InputIt2>
    void _normalized_distance(double* scores, size_t score_count, Range<InputIt2> s2,
                              double score_cutoff = 1.0) const
    {
        const T& derived = static_cast<const T&>(*this);
        if (score_count < derived.result_count())
            throw std::invalid_argument("scores has to have >= result_count() elements");

        // reinterpretation only works when the types have the same size
        int64_t* scores_i64 = nullptr;
        if constexpr (sizeof(double) == sizeof(int64_t))
            scores_i64 = reinterpret_cast<int64_t*>(scores);
        else
            scores_i64 = new int64_t[derived.result_count()];

        Range s2_(s2);
        derived.distance(scores_i64, derived.result_count(), s2_);

        for (size_t i = 0; i < derived.get_input_count(); ++i) {
            auto maximum = derived.maximum(i, s2);
            double norm_dist = static_cast<double>(scores_i64[i]) / static_cast<double>(maximum);
            scores[i] = (norm_dist <= score_cutoff) ? norm_dist : 1.0;
        }

        if constexpr (sizeof(double) != sizeof(int64_t)) delete[] scores_i64;
    }

    template <typename InputIt2>
    void _normalized_similarity(double* scores, size_t score_count, Range<InputIt2> s2,
                                double score_cutoff) const
    {
        const T& derived = static_cast<const T&>(*this);
        _normalized_distance(scores, score_count, s2);

        for (size_t i = 0; i < derived.get_input_count(); ++i) {
            double norm_sim = 1.0 - scores[i];
            scores[i] = (norm_sim >= score_cutoff) ? norm_sim : 0.0;
        }
    }

    MultiNormalizedMetricBase()
    {}
    friend T;
};

template <typename T, typename ResType, int64_t WorstSimilarity, int64_t WorstDistance>
struct MultiDistanceBase : public MultiNormalizedMetricBase<T> {
    template <typename InputIt2>
    void distance(ResType* scores, size_t score_count, InputIt2 first2, InputIt2 last2,
                  ResType score_cutoff = static_cast<ResType>(WorstDistance)) const
    {
        const T& derived = static_cast<const T&>(*this);
        derived._distance(scores, score_count, Range(first2, last2), score_cutoff);
    }

    template <typename Sentence2>
    void distance(ResType* scores, size_t score_count, const Sentence2& s2,
                  ResType score_cutoff = static_cast<ResType>(WorstDistance)) const
    {
        const T& derived = static_cast<const T&>(*this);
        derived._distance(scores, score_count, Range(s2), score_cutoff);
    }

    template <typename InputIt2>
    void similarity(ResType* scores, size_t score_count, InputIt2 first2, InputIt2 last2,
                    ResType score_cutoff = static_cast<ResType>(WorstSimilarity)) const
    {
        _similarity(scores, score_count, Range(first2, last2), score_cutoff);
    }

    template <typename Sentence2>
    void similarity(ResType* scores, size_t score_count, const Sentence2& s2,
                    ResType score_cutoff = static_cast<ResType>(WorstSimilarity)) const
    {
        _similarity(scores, score_count, Range(s2), score_cutoff);
    }

protected:
    template <typename InputIt2>
    void _similarity(ResType* scores, size_t score_count, Range<InputIt2> s2, ResType score_cutoff) const
    {
        const T& derived = static_cast<const T&>(*this);
        derived._distance(scores, score_count, s2);

        for (size_t i = 0; i < derived.get_input_count(); ++i) {
            ResType maximum = derived.maximum(i, s2);
            ResType sim = maximum - scores[i];
            scores[i] = (sim >= score_cutoff) ? sim : 0;
        }
    }

    MultiDistanceBase()
    {}
    friend T;
};

template <typename T, typename ResType, int64_t WorstSimilarity, int64_t WorstDistance>
struct MultiSimilarityBase : public MultiNormalizedMetricBase<T> {
    template <typename InputIt2>
    void distance(ResType* scores, size_t score_count, InputIt2 first2, InputIt2 last2,
                  ResType score_cutoff = static_cast<ResType>(WorstDistance)) const
    {
        _distance(scores, score_count, Range(first2, last2), score_cutoff);
    }

    template <typename Sentence2>
    void distance(ResType* scores, size_t score_count, const Sentence2& s2,
                  ResType score_cutoff = WorstDistance) const
    {
        _distance(scores, score_count, Range(s2), score_cutoff);
    }

    template <typename InputIt2>
    void similarity(ResType* scores, size_t score_count, InputIt2 first2, InputIt2 last2,
                    ResType score_cutoff = static_cast<ResType>(WorstSimilarity)) const
    {
        const T& derived = static_cast<const T&>(*this);
        derived._similarity(scores, score_count, Range(first2, last2), score_cutoff);
    }

    template <typename Sentence2>
    void similarity(ResType* scores, size_t score_count, const Sentence2& s2,
                    ResType score_cutoff = static_cast<ResType>(WorstSimilarity)) const
    {
        const T& derived = static_cast<const T&>(*this);
        derived._similarity(scores, score_count, Range(s2), score_cutoff);
    }

protected:
    template <typename InputIt2>
    void _distance(ResType* scores, size_t score_count, Range<InputIt2> s2, ResType score_cutoff) const
    {
        const T& derived = static_cast<const T&>(*this);
        derived._similarity(scores, score_count, s2);

        for (size_t i = 0; i < derived.get_input_count(); ++i) {
            ResType maximum = derived.maximum(i, s2);
            ResType dist = maximum - scores[i];

            if constexpr (std::is_floating_point_v<ResType>)
                scores[i] = (dist <= score_cutoff) ? dist : 1.0;
            else
                scores[i] = (dist <= score_cutoff) ? dist : score_cutoff + 1;
        }
    }

    MultiSimilarityBase()
    {}
    friend T;
};

} // namespace rapidfuzz::detail

namespace rapidfuzz::detail {

template <typename IntType>
struct RowId {
    IntType val = -1;
    friend bool operator==(const RowId& lhs, const RowId& rhs)
    {
        return lhs.val == rhs.val;
    }

    friend bool operator!=(const RowId& lhs, const RowId& rhs)
    {
        return !(lhs == rhs);
    }
};

/*
 * based on the paper
 * "Linear space string correction algorithm using the Damerau-Levenshtein distance"
 * from Chunchun Zhao and Sartaj Sahni
 */
template <typename IntType, typename InputIt1, typename InputIt2>
int64_t damerau_levenshtein_distance_zhao(Range<InputIt1> s1, Range<InputIt2> s2, int64_t max)
{
    IntType len1 = static_cast<IntType>(s1.size());
    IntType len2 = static_cast<IntType>(s2.size());
    IntType maxVal = static_cast<IntType>(std::max(len1, len2) + 1);
    assert(std::numeric_limits<IntType>::max() > maxVal);

    HybridGrowingHashmap<typename Range<InputIt1>::value_type, RowId<IntType>> last_row_id;
    size_t size = static_cast<size_t>(s2.size() + 2);
    assume(size != 0);
    std::vector<IntType> FR_arr(size, maxVal);
    std::vector<IntType> R1_arr(size, maxVal);
    std::vector<IntType> R_arr(size);
    R_arr[0] = maxVal;
    std::iota(R_arr.begin() + 1, R_arr.end(), IntType(0));

    IntType* R = &R_arr[1];
    IntType* R1 = &R1_arr[1];
    IntType* FR = &FR_arr[1];

    for (IntType i = 1; i <= len1; i++) {
        std::swap(R, R1);
        IntType last_col_id = -1;
        IntType last_i2l1 = R[0];
        R[0] = i;
        IntType T = maxVal;

        for (IntType j = 1; j <= len2; j++) {
            ptrdiff_t diag = R1[j - 1] + static_cast<IntType>(s1[i - 1] != s2[j - 1]);
            ptrdiff_t left = R[j - 1] + 1;
            ptrdiff_t up = R1[j] + 1;
            ptrdiff_t temp = std::min({diag, left, up});

            if (s1[i - 1] == s2[j - 1]) {
                last_col_id = j;   // last occurence of s1_i
                FR[j] = R1[j - 2]; // save H_k-1,j-2
                T = last_i2l1;     // save H_i-2,l-1
            }
            else {
                ptrdiff_t k = last_row_id.get(static_cast<uint64_t>(s2[j - 1])).val;
                ptrdiff_t l = last_col_id;

                if ((j - l) == 1) {
                    ptrdiff_t transpose = FR[j] + (i - k);
                    temp = std::min(temp, transpose);
                }
                else if ((i - k) == 1) {
                    ptrdiff_t transpose = T + (j - l);
                    temp = std::min(temp, transpose);
                }
            }

            last_i2l1 = R[j];
            R[j] = static_cast<IntType>(temp);
        }
        last_row_id[s1[i - 1]].val = i;
    }

    int64_t dist = R[s2.size()];
    return (dist <= max) ? dist : max + 1;
}

template <typename InputIt1, typename InputIt2>
int64_t damerau_levenshtein_distance(Range<InputIt1> s1, Range<InputIt2> s2, int64_t max)
{
    int64_t min_edits = std::abs(s1.size() - s2.size());
    if (min_edits > max) return max + 1;

    /* common affix does not effect Levenshtein distance */
    remove_common_affix(s1, s2);

    ptrdiff_t maxVal = std::max(s1.size(), s2.size()) + 1;
    if (std::numeric_limits<int16_t>::max() > maxVal)
        return damerau_levenshtein_distance_zhao<int16_t>(s1, s2, max);
    else if (std::numeric_limits<int32_t>::max() > maxVal)
        return damerau_levenshtein_distance_zhao<int32_t>(s1, s2, max);
    else
        return damerau_levenshtein_distance_zhao<int64_t>(s1, s2, max);
}

class DamerauLevenshtein
    : public DistanceBase<DamerauLevenshtein, int64_t, 0, std::numeric_limits<int64_t>::max()> {
    friend DistanceBase<DamerauLevenshtein, int64_t, 0, std::numeric_limits<int64_t>::max()>;
    friend NormalizedMetricBase<DamerauLevenshtein>;

    template <typename InputIt1, typename InputIt2>
    static int64_t maximum(Range<InputIt1> s1, Range<InputIt2> s2)
    {
        return std::max(s1.size(), s2.size());
    }

    template <typename InputIt1, typename InputIt2>
    static int64_t _distance(Range<InputIt1> s1, Range<InputIt2> s2, int64_t score_cutoff,
                             [[maybe_unused]] int64_t score_hint)
    {
        return damerau_levenshtein_distance(s1, s2, score_cutoff);
    }
};

} // namespace rapidfuzz::detail

namespace rapidfuzz {
/* the API will require a change when adding custom weights */
namespace experimental {
/**
 * @brief Calculates the Damerau Levenshtein distance between two strings.
 *
 *
 * @tparam Sentence1 This is a string that can be converted to
 * basic_string_view<char_type>
 * @tparam Sentence2 This is a string that can be converted to
 * basic_string_view<char_type>
 *
 * @param s1
 *   string to compare with s2 (for type info check Template parameters above)
 * @param s2
 *   string to compare with s1 (for type info check Template parameters above)
 * @param max
 *   Maximum Damerau Levenshtein distance between s1 and s2, that is
 *   considered as a result. If the distance is bigger than max,
 *   max + 1 is returned instead. Default is std::numeric_limits<size_t>::max(),
 *   which deactivates this behaviour.
 *
 * @return Damerau Levenshtein distance between s1 and s2
 */
template <typename InputIt1, typename InputIt2>
int64_t damerau_levenshtein_distance(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2,
                                     int64_t score_cutoff = std::numeric_limits<int64_t>::max())
{
    return detail::DamerauLevenshtein::distance(first1, last1, first2, last2, score_cutoff, score_cutoff);
}

template <typename Sentence1, typename Sentence2>
int64_t damerau_levenshtein_distance(const Sentence1& s1, const Sentence2& s2,
                                     int64_t score_cutoff = std::numeric_limits<int64_t>::max())
{
    return detail::DamerauLevenshtein::distance(s1, s2, score_cutoff, score_cutoff);
}

template <typename InputIt1, typename InputIt2>
int64_t damerau_levenshtein_similarity(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2,
                                       int64_t score_cutoff = 0)
{
    return detail::DamerauLevenshtein::similarity(first1, last1, first2, last2, score_cutoff, score_cutoff);
}

template <typename Sentence1, typename Sentence2>
int64_t damerau_levenshtein_similarity(const Sentence1& s1, const Sentence2& s2, int64_t score_cutoff = 0)
{
    return detail::DamerauLevenshtein::similarity(s1, s2, score_cutoff, score_cutoff);
}

template <typename InputIt1, typename InputIt2>
double damerau_levenshtein_normalized_distance(InputIt1 first1, InputIt1 last1, InputIt2 first2,
                                               InputIt2 last2, double score_cutoff = 1.0)
{
    return detail::DamerauLevenshtein::normalized_distance(first1, last1, first2, last2, score_cutoff,
                                                           score_cutoff);
}

template <typename Sentence1, typename Sentence2>
double damerau_levenshtein_normalized_distance(const Sentence1& s1, const Sentence2& s2,
                                               double score_cutoff = 1.0)
{
    return detail::DamerauLevenshtein::normalized_distance(s1, s2, score_cutoff, score_cutoff);
}

/**
 * @brief Calculates a normalized Damerau Levenshtein similarity
 *
 * @details
 * Both string require a similar length
 *
 *
 * @tparam Sentence1 This is a string that can be converted to
 * basic_string_view<char_type>
 * @tparam Sentence2 This is a string that can be converted to
 * basic_string_view<char_type>
 *
 * @param s1
 *   string to compare with s2 (for type info check Template parameters above)
 * @param s2
 *   string to compare with s1 (for type info check Template parameters above)
 * @param score_cutoff
 *   Optional argument for a score threshold as a float between 0 and 1.0.
 *   For ratio < score_cutoff 0 is returned instead. Default is 0,
 *   which deactivates this behaviour.
 *
 * @return Normalized Damerau Levenshtein distance between s1 and s2
 *   as a float between 0 and 1.0
 */
template <typename InputIt1, typename InputIt2>
double damerau_levenshtein_normalized_similarity(InputIt1 first1, InputIt1 last1, InputIt2 first2,
                                                 InputIt2 last2, double score_cutoff = 0.0)
{
    return detail::DamerauLevenshtein::normalized_similarity(first1, last1, first2, last2, score_cutoff,
                                                             score_cutoff);
}

template <typename Sentence1, typename Sentence2>
double damerau_levenshtein_normalized_similarity(const Sentence1& s1, const Sentence2& s2,
                                                 double score_cutoff = 0.0)
{
    return detail::DamerauLevenshtein::normalized_similarity(s1, s2, score_cutoff, score_cutoff);
}

template <typename CharT1>
struct CachedDamerauLevenshtein : public detail::CachedDistanceBase<CachedDamerauLevenshtein<CharT1>, int64_t,
                                                                    0, std::numeric_limits<int64_t>::max()> {
    template <typename Sentence1>
    explicit CachedDamerauLevenshtein(const Sentence1& s1_)
        : CachedDamerauLevenshtein(detail::to_begin(s1_), detail::to_end(s1_))
    {}

    template <typename InputIt1>
    CachedDamerauLevenshtein(InputIt1 first1, InputIt1 last1) : s1(first1, last1)
    {}

private:
    friend detail::CachedDistanceBase<CachedDamerauLevenshtein<CharT1>, int64_t, 0,
                                      std::numeric_limits<int64_t>::max()>;
    friend detail::CachedNormalizedMetricBase<CachedDamerauLevenshtein<CharT1>>;

    template <typename InputIt2>
    int64_t maximum(detail::Range<InputIt2> s2) const
    {
        return std::max(static_cast<ptrdiff_t>(s1.size()), s2.size());
    }

    template <typename InputIt2>
    int64_t _distance(detail::Range<InputIt2> s2, int64_t score_cutoff,
                      [[maybe_unused]] int64_t score_hint) const
    {
        return damerau_levenshtein_distance(s1, s2, score_cutoff);
    }

    std::basic_string<CharT1> s1;
};

template <typename Sentence1>
explicit CachedDamerauLevenshtein(const Sentence1& s1_) -> CachedDamerauLevenshtein<char_type<Sentence1>>;

template <typename InputIt1>
CachedDamerauLevenshtein(InputIt1 first1, InputIt1 last1) -> CachedDamerauLevenshtein<iter_value_t<InputIt1>>;

} // namespace experimental
} // namespace rapidfuzz

#include <cmath>
#include <numeric>

#include <stdexcept>

namespace rapidfuzz::detail {

class Hamming : public DistanceBase<Hamming, int64_t, 0, std::numeric_limits<int64_t>::max()> {
    friend DistanceBase<Hamming, int64_t, 0, std::numeric_limits<int64_t>::max()>;
    friend NormalizedMetricBase<Hamming>;

    template <typename InputIt1, typename InputIt2>
    static int64_t maximum(Range<InputIt1> s1, Range<InputIt2>)
    {
        return s1.size();
    }

    template <typename InputIt1, typename InputIt2>
    static int64_t _distance(Range<InputIt1> s1, Range<InputIt2> s2, int64_t score_cutoff,
                             [[maybe_unused]] int64_t score_hint)
    {
        if (s1.size() != s2.size()) throw std::invalid_argument("Sequences are not the same length.");

        int64_t dist = 0;
        for (ptrdiff_t i = 0; i < s1.size(); ++i)
            dist += bool(s1[i] != s2[i]);

        return (dist <= score_cutoff) ? dist : score_cutoff + 1;
    }
};

template <typename InputIt1, typename InputIt2>
Editops hamming_editops(Range<InputIt1> s1, Range<InputIt2> s2, int64_t)
{
    if (s1.size() != s2.size()) throw std::invalid_argument("Sequences are not the same length.");

    Editops ops;
    for (ptrdiff_t i = 0; i < s1.size(); ++i)
        if (s1[i] != s2[i]) ops.emplace_back(EditType::Replace, i, i);

    ops.set_src_len(static_cast<size_t>(s1.size()));
    ops.set_dest_len(static_cast<size_t>(s2.size()));
    return ops;
}

} // namespace rapidfuzz::detail

namespace rapidfuzz {

/**
 * @brief Calculates the Hamming distance between two strings.
 *
 * @details
 * Both strings require a similar length
 *
 *
 * @tparam Sentence1 This is a string that can be converted to
 * basic_string_view<char_type>
 * @tparam Sentence2 This is a string that can be converted to
 * basic_string_view<char_type>
 *
 * @param s1
 *   string to compare with s2 (for type info check Template parameters above)
 * @param s2
 *   string to compare with s1 (for type info check Template parameters above)
 * @param max
 *   Maximum Hamming distance between s1 and s2, that is
 *   considered as a result. If the distance is bigger than max,
 *   max + 1 is returned instead. Default is std::numeric_limits<size_t>::max(),
 *   which deactivates this behaviour.
 *
 * @return Hamming distance between s1 and s2
 */
template <typename InputIt1, typename InputIt2>
int64_t hamming_distance(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2,
                         int64_t score_cutoff = std::numeric_limits<int64_t>::max())
{
    return detail::Hamming::distance(first1, last1, first2, last2, score_cutoff, score_cutoff);
}

template <typename Sentence1, typename Sentence2>
int64_t hamming_distance(const Sentence1& s1, const Sentence2& s2,
                         int64_t score_cutoff = std::numeric_limits<int64_t>::max())
{
    return detail::Hamming::distance(s1, s2, score_cutoff, score_cutoff);
}

template <typename InputIt1, typename InputIt2>
int64_t hamming_similarity(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2,
                           int64_t score_cutoff = 0)
{
    return detail::Hamming::similarity(first1, last1, first2, last2, score_cutoff, score_cutoff);
}

template <typename Sentence1, typename Sentence2>
int64_t hamming_similarity(const Sentence1& s1, const Sentence2& s2, int64_t score_cutoff = 0)
{
    return detail::Hamming::similarity(s1, s2, score_cutoff, score_cutoff);
}

template <typename InputIt1, typename InputIt2>
double hamming_normalized_distance(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2,
                                   double score_cutoff = 1.0)
{
    return detail::Hamming::normalized_distance(first1, last1, first2, last2, score_cutoff, score_cutoff);
}

template <typename Sentence1, typename Sentence2>
double hamming_normalized_distance(const Sentence1& s1, const Sentence2& s2, double score_cutoff = 1.0)
{
    return detail::Hamming::normalized_distance(s1, s2, score_cutoff, score_cutoff);
}

template <typename InputIt1, typename InputIt2>
Editops hamming_editops(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2,
                        int64_t score_hint = std::numeric_limits<int64_t>::max())
{
    return detail::hamming_editops(detail::Range(first1, last1), detail::Range(first2, last2), score_hint);
}

template <typename Sentence1, typename Sentence2>
Editops hamming_editops(const Sentence1& s1, const Sentence2& s2,
                        int64_t score_hint = std::numeric_limits<int64_t>::max())
{
    return detail::hamming_editops(detail::Range(s1), detail::Range(s2), score_hint);
}

/**
 * @brief Calculates a normalized hamming similarity
 *
 * @details
 * Both string require a similar length
 *
 *
 * @tparam Sentence1 This is a string that can be converted to
 * basic_string_view<char_type>
 * @tparam Sentence2 This is a string that can be converted to
 * basic_string_view<char_type>
 *
 * @param s1
 *   string to compare with s2 (for type info check Template parameters above)
 * @param s2
 *   string to compare with s1 (for type info check Template parameters above)
 * @param score_cutoff
 *   Optional argument for a score threshold as a float between 0 and 1.0.
 *   For ratio < score_cutoff 0 is returned instead. Default is 0,
 *   which deactivates this behaviour.
 *
 * @return Normalized hamming distance between s1 and s2
 *   as a float between 0 and 1.0
 */
template <typename InputIt1, typename InputIt2>
double hamming_normalized_similarity(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2,
                                     double score_cutoff = 0.0)
{
    return detail::Hamming::normalized_similarity(first1, last1, first2, last2, score_cutoff, score_cutoff);
}

template <typename Sentence1, typename Sentence2>
double hamming_normalized_similarity(const Sentence1& s1, const Sentence2& s2, double score_cutoff = 0.0)
{
    return detail::Hamming::normalized_similarity(s1, s2, score_cutoff, score_cutoff);
}

template <typename CharT1>
struct CachedHamming : public detail::CachedDistanceBase<CachedHamming<CharT1>, int64_t, 0,
                                                         std::numeric_limits<int64_t>::max()> {
    template <typename Sentence1>
    explicit CachedHamming(const Sentence1& s1_) : CachedHamming(detail::to_begin(s1_), detail::to_end(s1_))
    {}

    template <typename InputIt1>
    CachedHamming(InputIt1 first1, InputIt1 last1) : s1(first1, last1)
    {}

private:
    friend detail::CachedDistanceBase<CachedHamming<CharT1>, int64_t, 0, std::numeric_limits<int64_t>::max()>;
    friend detail::CachedNormalizedMetricBase<CachedHamming<CharT1>>;

    template <typename InputIt2>
    int64_t maximum(detail::Range<InputIt2> s2) const
    {
        return s2.size();
    }

    template <typename InputIt2>
    int64_t _distance(detail::Range<InputIt2> s2, int64_t score_cutoff,
                      [[maybe_unused]] int64_t score_hint) const
    {
        return detail::Hamming::distance(s1, s2, score_cutoff, score_hint);
    }

    std::basic_string<CharT1> s1;
};

template <typename Sentence1>
explicit CachedHamming(const Sentence1& s1_) -> CachedHamming<char_type<Sentence1>>;

template <typename InputIt1>
CachedHamming(InputIt1 first1, InputIt1 last1) -> CachedHamming<iter_value_t<InputIt1>>;

/**@}*/

} // namespace rapidfuzz

#include <limits>

#include <array>
#include <limits>
#include <stdint.h>
#include <stdio.h>
#include <type_traits>
#include <unordered_set>
#include <vector>

namespace rapidfuzz::detail {

struct BitvectorHashmap {
    BitvectorHashmap() : m_map()
    {}

    template <typename CharT>
    uint64_t get(CharT key) const noexcept
    {
        return m_map[lookup(static_cast<uint64_t>(key))].value;
    }

    template <typename CharT>
    uint64_t& operator[](CharT key) noexcept
    {
        uint32_t i = lookup(static_cast<uint64_t>(key));
        m_map[i].key = static_cast<uint64_t>(key);
        return m_map[i].value;
    }

private:
    /**
     * lookup key inside the hashmap using a similar collision resolution
     * strategy to CPython and Ruby
     */
    uint32_t lookup(uint64_t key) const noexcept
    {
        uint32_t i = key % 128;

        if (!m_map[i].value || m_map[i].key == key) return i;

        uint64_t perturb = key;
        while (true) {
            i = (static_cast<uint64_t>(i) * 5 + perturb + 1) % 128;
            if (!m_map[i].value || m_map[i].key == key) return i;

            perturb >>= 5;
        }
    }

    struct MapElem {
        uint64_t key = 0;
        uint64_t value = 0;
    };
    std::array<MapElem, 128> m_map;
};

struct PatternMatchVector {
    PatternMatchVector() : m_extendedAscii()
    {}

    template <typename InputIt>
    PatternMatchVector(Range<InputIt> s) : m_extendedAscii()
    {
        insert(s);
    }

    size_t size() const noexcept
    {
        return 1;
    }

    template <typename InputIt>
    void insert(Range<InputIt> s) noexcept
    {
        uint64_t mask = 1;
        for (const auto& ch : s) {
            insert_mask(ch, mask);
            mask <<= 1;
        }
    }

    template <typename CharT>
    void insert(CharT key, int64_t pos) noexcept
    {
        insert_mask(key, UINT64_C(1) << pos);
    }

    uint64_t get(char key) const noexcept
    {
        /** treat char as value between 0 and 127 for performance reasons */
        return m_extendedAscii[static_cast<uint8_t>(key)];
    }

    template <typename CharT>
    uint64_t get(CharT key) const noexcept
    {
        if (key >= 0 && key <= 255)
            return m_extendedAscii[static_cast<uint8_t>(key)];
        else
            return m_map.get(key);
    }

    template <typename CharT>
    uint64_t get(size_t block, CharT key) const noexcept
    {
        assert(block == 0);
        (void)block;
        return get(key);
    }

    void insert_mask(char key, uint64_t mask) noexcept
    {
        /** treat char as value between 0 and 127 for performance reasons */
        m_extendedAscii[static_cast<uint8_t>(key)] |= mask;
    }

    template <typename CharT>
    void insert_mask(CharT key, uint64_t mask) noexcept
    {
        if (key >= 0 && key <= 255)
            m_extendedAscii[static_cast<uint8_t>(key)] |= mask;
        else
            m_map[key] |= mask;
    }

private:
    BitvectorHashmap m_map;
    std::array<uint64_t, 256> m_extendedAscii;
};

struct BlockPatternMatchVector {
    BlockPatternMatchVector() = delete;

    BlockPatternMatchVector(size_t str_len)
        : m_block_count(ceil_div(str_len, 64)), m_map(nullptr), m_extendedAscii(256, m_block_count, 0)
    {}

    template <typename InputIt>
    BlockPatternMatchVector(Range<InputIt> s) : BlockPatternMatchVector(static_cast<size_t>(s.size()))
    {
        insert(s);
    }

    ~BlockPatternMatchVector()
    {
        delete[] m_map;
    }

    size_t size() const noexcept
    {
        return m_block_count;
    }

    template <typename CharT>
    void insert(size_t block, CharT ch, int pos) noexcept
    {
        uint64_t mask = UINT64_C(1) << pos;
        insert_mask(block, ch, mask);
    }

    /**
     * @warning undefined behavior if iterator \p first is greater than \p last
     * @tparam InputIt
     * @param first
     * @param last
     */
    template <typename InputIt>
    void insert(Range<InputIt> s) noexcept
    {
        auto len = s.size();
        uint64_t mask = 1;
        for (ptrdiff_t i = 0; i < len; ++i) {
            size_t block = static_cast<size_t>(i) / 64;
            insert_mask(block, s[i], mask);
            mask = rotl(mask, 1);
        }
    }

    template <typename CharT>
    void insert_mask(size_t block, CharT key, uint64_t mask) noexcept
    {
        assert(block < size());
        if (key >= 0 && key <= 255)
            m_extendedAscii[static_cast<uint8_t>(key)][block] |= mask;
        else {
            if (!m_map) m_map = new BitvectorHashmap[m_block_count];
            m_map[block][key] |= mask;
        }
    }

    void insert_mask(size_t block, char key, uint64_t mask) noexcept
    {
        insert_mask(block, static_cast<uint8_t>(key), mask);
    }

    template <typename CharT>
    uint64_t get(size_t block, CharT key) const noexcept
    {
        if (key >= 0 && key <= 255)
            return m_extendedAscii[static_cast<uint8_t>(key)][block];
        else if (m_map)
            return m_map[block].get(key);
        else
            return 0;
    }

    uint64_t get(size_t block, char ch) const noexcept
    {
        return get(block, static_cast<uint8_t>(ch));
    }

private:
    size_t m_block_count;
    BitvectorHashmap* m_map;
    BitMatrix<uint64_t> m_extendedAscii;
};

} // namespace rapidfuzz::detail

#include <limits>

#include <algorithm>
#include <array>

namespace rapidfuzz::detail {

template <bool RecordMatrix>
struct LCSseqResult;

template <>
struct LCSseqResult<true> {
    ShiftedBitMatrix<uint64_t> S;

    int64_t sim;
};

template <>
struct LCSseqResult<false> {
    int64_t sim;
};

/*
 * An encoded mbleven model table.
 *
 * Each 8-bit integer represents an edit sequence, with using two
 * bits for a single operation.
 *
 * Each Row of 8 integers represent all possible combinations
 * of edit sequences for a gived maximum edit distance and length
 * difference between the two strings, that is below the maximum
 * edit distance
 *
 *   0x1 = 01 = DELETE,
 *   0x2 = 10 = INSERT
 *
 * 0x5 -> DEL + DEL
 * 0x6 -> DEL + INS
 * 0x9 -> INS + DEL
 * 0xA -> INS + INS
 */
static constexpr std::array<std::array<uint8_t, 7>, 14> lcs_seq_mbleven2018_matrix = {{
    /* max edit distance 1 */
    {0},
    /* case does not occur */ /* len_diff 0 */
    {0x01},                   /* len_diff 1 */
    /* max edit distance 2 */
    {0x09, 0x06}, /* len_diff 0 */
    {0x01},       /* len_diff 1 */
    {0x05},       /* len_diff 2 */
    /* max edit distance 3 */
    {0x09, 0x06},       /* len_diff 0 */
    {0x25, 0x19, 0x16}, /* len_diff 1 */
    {0x05},             /* len_diff 2 */
    {0x15},             /* len_diff 3 */
    /* max edit distance 4 */
    {0x96, 0x66, 0x5A, 0x99, 0x69, 0xA5}, /* len_diff 0 */
    {0x25, 0x19, 0x16},                   /* len_diff 1 */
    {0x65, 0x56, 0x95, 0x59},             /* len_diff 2 */
    {0x15},                               /* len_diff 3 */
    {0x55},                               /* len_diff 4 */
}};

template <typename InputIt1, typename InputIt2>
int64_t lcs_seq_mbleven2018(Range<InputIt1> s1, Range<InputIt2> s2, int64_t score_cutoff)
{
    auto len1 = s1.size();
    auto len2 = s2.size();

    if (len1 < len2) return lcs_seq_mbleven2018(s2, s1, score_cutoff);

    auto len_diff = len1 - len2;
    int64_t max_misses = static_cast<ptrdiff_t>(len1) - score_cutoff;
    auto ops_index = (max_misses + max_misses * max_misses) / 2 + len_diff - 1;
    auto& possible_ops = lcs_seq_mbleven2018_matrix[static_cast<size_t>(ops_index)];
    int64_t max_len = 0;

    for (uint8_t ops : possible_ops) {
        ptrdiff_t s1_pos = 0;
        ptrdiff_t s2_pos = 0;
        int64_t cur_len = 0;

        while (s1_pos < len1 && s2_pos < len2) {
            if (s1[s1_pos] != s2[s2_pos]) {
                if (!ops) break;
                if (ops & 1)
                    s1_pos++;
                else if (ops & 2)
                    s2_pos++;
#if defined(__GNUC__) && !defined(__clang__) && !defined(__ICC) && __GNUC__ < 10
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wconversion"
#endif
                ops >>= 2;
#if defined(__GNUC__) && !defined(__clang__) && !defined(__ICC) && __GNUC__ < 10
#    pragma GCC diagnostic pop
#endif
            }
            else {
                cur_len++;
                s1_pos++;
                s2_pos++;
            }
        }

        max_len = std::max(max_len, cur_len);
    }

    return (max_len >= score_cutoff) ? max_len : 0;
}

template <bool RecordMatrix>
struct LCSseqResult;

#ifdef RAPIDFUZZ_SIMD
template <typename VecType, typename InputIt, int _lto_hack = RAPIDFUZZ_LTO_HACK>
void lcs_simd(Range<int64_t*> scores, const BlockPatternMatchVector& block, Range<InputIt> s2,
              int64_t score_cutoff) noexcept
{
#    ifdef RAPIDFUZZ_AVX2
    using namespace simd_avx2;
#    else
    using namespace simd_sse2;
#    endif
    auto score_iter = scores.begin();
    static constexpr size_t vecs = static_cast<size_t>(native_simd<uint64_t>::size());
    assert(block.size() % vecs == 0);

    for (size_t cur_vec = 0; cur_vec < block.size(); cur_vec += vecs) {
        native_simd<VecType> S(static_cast<VecType>(-1));

        for (const auto& ch : s2) {
            alignas(32) std::array<uint64_t, vecs> stored;
            unroll<int, vecs>([&](auto i) { stored[i] = block.get(cur_vec + i, ch); });

            native_simd<VecType> Matches(stored.data());
            native_simd<VecType> u = S & Matches;
            S = (S + u) | (S - u);
        }

        S = ~S;

        auto counts = popcount(S);
        unroll<int, counts.size()>([&](auto i) {
            *score_iter =
                (static_cast<int64_t>(counts[i]) >= score_cutoff) ? static_cast<int64_t>(counts[i]) : 0;
            score_iter++;
        });
    }
}

#endif

template <size_t N, bool RecordMatrix, typename PMV, typename InputIt1, typename InputIt2>
auto lcs_unroll(const PMV& block, Range<InputIt1>, Range<InputIt2> s2, int64_t score_cutoff = 0)
    -> LCSseqResult<RecordMatrix>
{
    uint64_t S[N];
    unroll<size_t, N>([&](size_t i) { S[i] = ~UINT64_C(0); });

    LCSseqResult<RecordMatrix> res;
    if constexpr (RecordMatrix) res.S = ShiftedBitMatrix<uint64_t>(s2.size(), N, ~UINT64_C(0));

    for (ptrdiff_t i = 0; i < s2.size(); ++i) {
        uint64_t carry = 0;
        unroll<size_t, N>([&](size_t word) {
            uint64_t Matches = block.get(word, s2[i]);
            uint64_t u = S[word] & Matches;
            uint64_t x = addc64(S[word], u, carry, &carry);
            S[word] = x | (S[word] - u);

            if constexpr (RecordMatrix) res.S[i][word] = S[word];
        });
    }

    res.sim = 0;
    unroll<size_t, N>([&](size_t i) { res.sim += popcount(~S[i]); });

    if (res.sim < score_cutoff) res.sim = 0;

    return res;
}

template <bool RecordMatrix, typename PMV, typename InputIt1, typename InputIt2>
auto lcs_blockwise(const PMV& block, Range<InputIt1>, Range<InputIt2> s2, int64_t score_cutoff = 0)
    -> LCSseqResult<RecordMatrix>
{
    auto words = block.size();
    std::vector<uint64_t> S(words, ~UINT64_C(0));

    LCSseqResult<RecordMatrix> res;
    if constexpr (RecordMatrix) res.S = ShiftedBitMatrix<uint64_t>(s2.size(), words, ~UINT64_C(0));

    for (ptrdiff_t i = 0; i < s2.size(); ++i) {
        uint64_t carry = 0;
        for (size_t word = 0; word < words; ++word) {
            const uint64_t Matches = block.get(word, s2[i]);
            uint64_t Stemp = S[word];

            uint64_t u = Stemp & Matches;

            uint64_t x = addc64(Stemp, u, carry, &carry);
            S[word] = x | (Stemp - u);

            if constexpr (RecordMatrix) res.S[i][word] = S[word];
        }
    }

    res.sim = 0;
    for (uint64_t Stemp : S)
        res.sim += popcount(~Stemp);

    if (res.sim < score_cutoff) res.sim = 0;

    return res;
}

template <typename PMV, typename InputIt1, typename InputIt2>
int64_t longest_common_subsequence(const PMV& block, Range<InputIt1> s1, Range<InputIt2> s2,
                                   int64_t score_cutoff)
{
    auto nr = ceil_div(s1.size(), 64);
    switch (nr) {
    case 0: return 0;
    case 1: return lcs_unroll<1, false>(block, s1, s2, score_cutoff).sim;
    case 2: return lcs_unroll<2, false>(block, s1, s2, score_cutoff).sim;
    case 3: return lcs_unroll<3, false>(block, s1, s2, score_cutoff).sim;
    case 4: return lcs_unroll<4, false>(block, s1, s2, score_cutoff).sim;
    case 5: return lcs_unroll<5, false>(block, s1, s2, score_cutoff).sim;
    case 6: return lcs_unroll<6, false>(block, s1, s2, score_cutoff).sim;
    case 7: return lcs_unroll<7, false>(block, s1, s2, score_cutoff).sim;
    case 8: return lcs_unroll<8, false>(block, s1, s2, score_cutoff).sim;
    default: return lcs_blockwise<false>(block, s1, s2, score_cutoff).sim;
    }
}

template <typename InputIt1, typename InputIt2>
int64_t longest_common_subsequence(Range<InputIt1> s1, Range<InputIt2> s2, int64_t score_cutoff)
{
    if (s1.empty()) return 0;
    if (s1.size() <= 64) return longest_common_subsequence(PatternMatchVector(s1), s1, s2, score_cutoff);

    return longest_common_subsequence(BlockPatternMatchVector(s1), s1, s2, score_cutoff);
}

template <typename InputIt1, typename InputIt2>
int64_t lcs_seq_similarity(const BlockPatternMatchVector& block, Range<InputIt1> s1, Range<InputIt2> s2,
                           int64_t score_cutoff)
{
    auto len1 = s1.size();
    auto len2 = s2.size();
    int64_t max_misses = static_cast<int64_t>(len1) + len2 - 2 * score_cutoff;

    /* no edits are allowed */
    if (max_misses == 0 || (max_misses == 1 && len1 == len2))
        return std::equal(s1.begin(), s1.end(), s2.begin(), s2.end()) ? len1 : 0;

    if (max_misses < std::abs(len1 - len2)) return 0;

    // do this first, since we can not remove any affix in encoded form
    if (max_misses >= 5) return longest_common_subsequence(block, s1, s2, score_cutoff);

    /* common affix does not effect Levenshtein distance */
    StringAffix affix = remove_common_affix(s1, s2);
    int64_t lcs_sim = static_cast<int64_t>(affix.prefix_len + affix.suffix_len);
    if (!s1.empty() && !s2.empty()) lcs_sim += lcs_seq_mbleven2018(s1, s2, score_cutoff - lcs_sim);

    return (lcs_sim >= score_cutoff) ? lcs_sim : 0;
}

template <typename InputIt1, typename InputIt2>
int64_t lcs_seq_similarity(Range<InputIt1> s1, Range<InputIt2> s2, int64_t score_cutoff)
{
    auto len1 = s1.size();
    auto len2 = s2.size();

    // Swapping the strings so the second string is shorter
    if (len1 < len2) return lcs_seq_similarity(s2, s1, score_cutoff);

    int64_t max_misses = static_cast<int64_t>(len1) + len2 - 2 * score_cutoff;

    /* no edits are allowed */
    if (max_misses == 0 || (max_misses == 1 && len1 == len2))
        return std::equal(s1.begin(), s1.end(), s2.begin(), s2.end()) ? len1 : 0;

    if (max_misses < std::abs(len1 - len2)) return 0;

    /* common affix does not effect Levenshtein distance */
    StringAffix affix = remove_common_affix(s1, s2);
    int64_t lcs_sim = static_cast<int64_t>(affix.prefix_len + affix.suffix_len);
    if (s1.size() && s2.size()) {
        if (max_misses < 5)
            lcs_sim += lcs_seq_mbleven2018(s1, s2, score_cutoff - lcs_sim);
        else
            lcs_sim += longest_common_subsequence(s1, s2, score_cutoff - lcs_sim);
    }

    return (lcs_sim >= score_cutoff) ? lcs_sim : 0;
}

/**
 * @brief recover alignment from bitparallel Levenshtein matrix
 */
template <typename InputIt1, typename InputIt2>
Editops recover_alignment(Range<InputIt1> s1, Range<InputIt2> s2, const LCSseqResult<true>& matrix,
                          StringAffix affix)
{
    auto len1 = s1.size();
    auto len2 = s2.size();
    size_t dist = static_cast<size_t>(static_cast<int64_t>(len1) + len2 - 2 * matrix.sim);
    Editops editops(dist);
    editops.set_src_len(len1 + affix.prefix_len + affix.suffix_len);
    editops.set_dest_len(len2 + affix.prefix_len + affix.suffix_len);

    if (dist == 0) return editops;

    auto col = len1;
    auto row = len2;

    while (row && col) {
        /* Deletion */
        if (matrix.S.test_bit(row - 1, col - 1)) {
            assert(dist > 0);
            dist--;
            col--;
            editops[dist].type = EditType::Delete;
            editops[dist].src_pos = col + affix.prefix_len;
            editops[dist].dest_pos = row + affix.prefix_len;
        }
        else {
            row--;

            /* Insertion */
            if (row && !(matrix.S.test_bit(row - 1, col - 1))) {
                assert(dist > 0);
                dist--;
                editops[dist].type = EditType::Insert;
                editops[dist].src_pos = col + affix.prefix_len;
                editops[dist].dest_pos = row + affix.prefix_len;
            }
            /* Match */
            else {
                col--;
                assert(s1[col] == s2[row]);
            }
        }
    }

    while (col) {
        dist--;
        col--;
        editops[dist].type = EditType::Delete;
        editops[dist].src_pos = col + affix.prefix_len;
        editops[dist].dest_pos = row + affix.prefix_len;
    }

    while (row) {
        dist--;
        row--;
        editops[dist].type = EditType::Insert;
        editops[dist].src_pos = col + affix.prefix_len;
        editops[dist].dest_pos = row + affix.prefix_len;
    }

    return editops;
}

template <typename InputIt1, typename InputIt2>
LCSseqResult<true> lcs_matrix(Range<InputIt1> s1, Range<InputIt2> s2)
{
    auto nr = ceil_div(s1.size(), 64);
    switch (nr) {
    case 0:
    {
        LCSseqResult<true> res;
        res.sim = 0;
        return res;
    }
    case 1: return lcs_unroll<1, true>(PatternMatchVector(s1), s1, s2);
    case 2: return lcs_unroll<2, true>(BlockPatternMatchVector(s1), s1, s2);
    case 3: return lcs_unroll<3, true>(BlockPatternMatchVector(s1), s1, s2);
    case 4: return lcs_unroll<4, true>(BlockPatternMatchVector(s1), s1, s2);
    case 5: return lcs_unroll<5, true>(BlockPatternMatchVector(s1), s1, s2);
    case 6: return lcs_unroll<6, true>(BlockPatternMatchVector(s1), s1, s2);
    case 7: return lcs_unroll<7, true>(BlockPatternMatchVector(s1), s1, s2);
    case 8: return lcs_unroll<8, true>(BlockPatternMatchVector(s1), s1, s2);
    default: return lcs_blockwise<true>(BlockPatternMatchVector(s1), s1, s2);
    }
}

template <typename InputIt1, typename InputIt2>
Editops lcs_seq_editops(Range<InputIt1> s1, Range<InputIt2> s2)
{
    /* prefix and suffix are no-ops, which do not need to be added to the editops */
    StringAffix affix = remove_common_affix(s1, s2);

    return recover_alignment(s1, s2, lcs_matrix(s1, s2), affix);
}

class LCSseq : public SimilarityBase<LCSseq, int64_t, 0, std::numeric_limits<int64_t>::max()> {
    friend SimilarityBase<LCSseq, int64_t, 0, std::numeric_limits<int64_t>::max()>;
    friend NormalizedMetricBase<LCSseq>;

    template <typename InputIt1, typename InputIt2>
    static int64_t maximum(Range<InputIt1> s1, Range<InputIt2> s2)
    {
        return std::max(s1.size(), s2.size());
    }

    template <typename InputIt1, typename InputIt2>
    static int64_t _similarity(Range<InputIt1> s1, Range<InputIt2> s2, int64_t score_cutoff,
                               [[maybe_unused]] int64_t score_hint)
    {
        return lcs_seq_similarity(s1, s2, score_cutoff);
    }
};

} // namespace rapidfuzz::detail

#include <algorithm>
#include <cmath>
#include <limits>

namespace rapidfuzz {

template <typename InputIt1, typename InputIt2>
int64_t lcs_seq_distance(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2,
                         int64_t score_cutoff = std::numeric_limits<int64_t>::max())
{
    return detail::LCSseq::distance(first1, last1, first2, last2, score_cutoff, score_cutoff);
}

template <typename Sentence1, typename Sentence2>
int64_t lcs_seq_distance(const Sentence1& s1, const Sentence2& s2,
                         int64_t score_cutoff = std::numeric_limits<int64_t>::max())
{
    return detail::LCSseq::distance(s1, s2, score_cutoff, score_cutoff);
}

template <typename InputIt1, typename InputIt2>
int64_t lcs_seq_similarity(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2,
                           int64_t score_cutoff = 0)
{
    return detail::LCSseq::similarity(first1, last1, first2, last2, score_cutoff, score_cutoff);
}

template <typename Sentence1, typename Sentence2>
int64_t lcs_seq_similarity(const Sentence1& s1, const Sentence2& s2, int64_t score_cutoff = 0)
{
    return detail::LCSseq::similarity(s1, s2, score_cutoff, score_cutoff);
}

template <typename InputIt1, typename InputIt2>
double lcs_seq_normalized_distance(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2,
                                   double score_cutoff = 1.0)
{
    return detail::LCSseq::normalized_distance(first1, last1, first2, last2, score_cutoff, score_cutoff);
}

template <typename Sentence1, typename Sentence2>
double lcs_seq_normalized_distance(const Sentence1& s1, const Sentence2& s2, double score_cutoff = 1.0)
{
    return detail::LCSseq::normalized_distance(s1, s2, score_cutoff, score_cutoff);
}

template <typename InputIt1, typename InputIt2>
double lcs_seq_normalized_similarity(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2,
                                     double score_cutoff = 0.0)
{
    return detail::LCSseq::normalized_similarity(first1, last1, first2, last2, score_cutoff, score_cutoff);
}

template <typename Sentence1, typename Sentence2>
double lcs_seq_normalized_similarity(const Sentence1& s1, const Sentence2& s2, double score_cutoff = 0.0)
{
    return detail::LCSseq::normalized_similarity(s1, s2, score_cutoff, score_cutoff);
}

template <typename InputIt1, typename InputIt2>
Editops lcs_seq_editops(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2)
{
    return detail::lcs_seq_editops(detail::Range(first1, last1), detail::Range(first2, last2));
}

template <typename Sentence1, typename Sentence2>
Editops lcs_seq_editops(const Sentence1& s1, const Sentence2& s2)
{
    return detail::lcs_seq_editops(detail::Range(s1), detail::Range(s2));
}

#ifdef RAPIDFUZZ_SIMD
namespace experimental {
template <int MaxLen>
struct MultiLCSseq : public detail::MultiSimilarityBase<MultiLCSseq<MaxLen>, int64_t, 0,
                                                        std::numeric_limits<int64_t>::max()> {
private:
    friend detail::MultiSimilarityBase<MultiLCSseq<MaxLen>, int64_t, 0, std::numeric_limits<int64_t>::max()>;
    friend detail::MultiNormalizedMetricBase<MultiLCSseq<MaxLen>>;

    constexpr static size_t get_vec_size()
    {
#    ifdef RAPIDFUZZ_AVX2
        using namespace detail::simd_avx2;
#    else
        using namespace detail::simd_sse2;
#    endif
        if constexpr (MaxLen <= 8)
            return native_simd<uint8_t>::size();
        else if constexpr (MaxLen <= 16)
            return native_simd<uint16_t>::size();
        else if constexpr (MaxLen <= 32)
            return native_simd<uint32_t>::size();
        else if constexpr (MaxLen <= 64)
            return native_simd<uint64_t>::size();

        static_assert(MaxLen <= 64);
    }

    constexpr static size_t find_block_count(size_t count)
    {
        size_t vec_size = get_vec_size();
        size_t simd_vec_count = detail::ceil_div(count, vec_size);
        return detail::ceil_div(simd_vec_count * vec_size * MaxLen, 64);
    }

public:
    MultiLCSseq(size_t count) : input_count(count), pos(0), PM(find_block_count(count) * 64)
    {
        str_lens.resize(result_count());
    }

    /**
     * @brief get minimum size required for result vectors passed into
     * - distance
     * - similarity
     * - normalized_distance
     * - normalized_similarity
     *
     * @return minimum vector size
     */
    size_t result_count() const
    {
        size_t vec_size = get_vec_size();
        size_t simd_vec_count = detail::ceil_div(input_count, vec_size);
        return simd_vec_count * vec_size;
    }

    template <typename Sentence1>
    void insert(const Sentence1& s1_)
    {
        insert(detail::to_begin(s1_), detail::to_end(s1_));
    }

    template <typename InputIt1>
    void insert(InputIt1 first1, InputIt1 last1)
    {
        auto len = std::distance(first1, last1);
        int block_pos = static_cast<int>((pos * MaxLen) % 64);
        auto block = (pos * MaxLen) / 64;
        assert(len <= MaxLen);

        if (pos >= input_count) throw std::invalid_argument("out of bounds insert");

        str_lens[pos] = static_cast<size_t>(len);

        for (; first1 != last1; ++first1) {
            PM.insert(block, *first1, block_pos);
            block_pos++;
        }
        pos++;
    }

private:
    template <typename InputIt2>
    void _similarity(int64_t* scores, size_t score_count, detail::Range<InputIt2> s2,
                     int64_t score_cutoff = 0) const
    {
        if (score_count < result_count())
            throw std::invalid_argument("scores has to have >= result_count() elements");

        detail::Range scores_(scores, scores + score_count);
        if constexpr (MaxLen == 8)
            detail::lcs_simd<uint8_t>(scores_, PM, s2, score_cutoff);
        else if constexpr (MaxLen == 16)
            detail::lcs_simd<uint16_t>(scores_, PM, s2, score_cutoff);
        else if constexpr (MaxLen == 32)
            detail::lcs_simd<uint32_t>(scores_, PM, s2, score_cutoff);
        else if constexpr (MaxLen == 64)
            detail::lcs_simd<uint64_t>(scores_, PM, s2, score_cutoff);
    }

    template <typename InputIt2>
    int64_t maximum(size_t s1_idx, detail::Range<InputIt2> s2) const
    {
        return std::max(static_cast<int64_t>(str_lens[s1_idx]), static_cast<int64_t>(s2.size()));
    }

    size_t get_input_count() const noexcept
    {
        return input_count;
    }

    size_t input_count;
    size_t pos;
    detail::BlockPatternMatchVector PM;
    std::vector<size_t> str_lens;
};
} /* namespace experimental */
#endif

template <typename CharT1>
struct CachedLCSseq
    : detail::CachedSimilarityBase<CachedLCSseq<CharT1>, int64_t, 0, std::numeric_limits<int64_t>::max()> {
    template <typename Sentence1>
    explicit CachedLCSseq(const Sentence1& s1_) : CachedLCSseq(detail::to_begin(s1_), detail::to_end(s1_))
    {}

    template <typename InputIt1>
    CachedLCSseq(InputIt1 first1, InputIt1 last1) : s1(first1, last1), PM(detail::Range(first1, last1))
    {}

private:
    friend detail::CachedSimilarityBase<CachedLCSseq<CharT1>, int64_t, 0,
                                        std::numeric_limits<int64_t>::max()>;
    friend detail::CachedNormalizedMetricBase<CachedLCSseq<CharT1>>;

    template <typename InputIt2>
    int64_t maximum(detail::Range<InputIt2> s2) const
    {
        return std::max(static_cast<ptrdiff_t>(s1.size()), s2.size());
    }

    template <typename InputIt2>
    int64_t _similarity(detail::Range<InputIt2> s2, int64_t score_cutoff,
                        [[maybe_unused]] int64_t score_hint) const
    {
        return detail::lcs_seq_similarity(PM, detail::Range(s1), s2, score_cutoff);
    }

    std::basic_string<CharT1> s1;
    detail::BlockPatternMatchVector PM;
};

template <typename Sentence1>
explicit CachedLCSseq(const Sentence1& s1_) -> CachedLCSseq<char_type<Sentence1>>;

template <typename InputIt1>
CachedLCSseq(InputIt1 first1, InputIt1 last1) -> CachedLCSseq<iter_value_t<InputIt1>>;

} // namespace rapidfuzz

namespace rapidfuzz::detail {

template <typename InputIt1, typename InputIt2>
int64_t indel_distance(const BlockPatternMatchVector& block, Range<InputIt1> s1, Range<InputIt2> s2,
                       int64_t score_cutoff)
{
    int64_t maximum = s1.size() + s2.size();
    int64_t lcs_cutoff = std::max<int64_t>(0, maximum / 2 - score_cutoff);
    int64_t lcs_sim = lcs_seq_similarity(block, s1, s2, lcs_cutoff);
    int64_t dist = maximum - 2 * lcs_sim;
    return (dist <= score_cutoff) ? dist : score_cutoff + 1;
}

template <typename InputIt1, typename InputIt2>
double indel_normalized_distance(const BlockPatternMatchVector& block, Range<InputIt1> s1, Range<InputIt2> s2,
                                 double score_cutoff)
{
    int64_t maximum = s1.size() + s2.size();
    int64_t cutoff_distance = static_cast<int64_t>(std::ceil(static_cast<double>(maximum) * score_cutoff));
    int64_t dist = indel_distance(block, s1, s2, cutoff_distance);
    double norm_dist = (maximum) ? static_cast<double>(dist) / static_cast<double>(maximum) : 0.0;
    return (norm_dist <= score_cutoff) ? norm_dist : 1.0;
}

template <typename InputIt1, typename InputIt2>
double indel_normalized_similarity(const BlockPatternMatchVector& block, Range<InputIt1> s1,
                                   Range<InputIt2> s2, double score_cutoff)
{
    double cutoff_score = NormSim_to_NormDist(score_cutoff);
    double norm_dist = indel_normalized_distance(block, s1, s2, cutoff_score);
    double norm_sim = 1.0 - norm_dist;
    return (norm_sim >= score_cutoff) ? norm_sim : 0.0;
}

class Indel : public DistanceBase<Indel, int64_t, 0, std::numeric_limits<int64_t>::max()> {
    friend DistanceBase<Indel, int64_t, 0, std::numeric_limits<int64_t>::max()>;
    friend NormalizedMetricBase<Indel>;

    template <typename InputIt1, typename InputIt2>
    static int64_t maximum(Range<InputIt1> s1, Range<InputIt2> s2)
    {
        return s1.size() + s2.size();
    }

    template <typename InputIt1, typename InputIt2>
    static int64_t _distance(Range<InputIt1> s1, Range<InputIt2> s2, int64_t score_cutoff, int64_t score_hint)
    {
        int64_t maximum = Indel::maximum(s1, s2);
        int64_t lcs_cutoff = std::max<int64_t>(0, maximum / 2 - score_cutoff);
        int64_t lcs_hint = std::max<int64_t>(0, maximum / 2 - score_hint);
        int64_t lcs_sim = LCSseq::similarity(s1, s2, lcs_cutoff, lcs_hint);
        int64_t dist = maximum - 2 * lcs_sim;
        return (dist <= score_cutoff) ? dist : score_cutoff + 1;
    }
};

} // namespace rapidfuzz::detail

namespace rapidfuzz {

template <typename InputIt1, typename InputIt2>
int64_t indel_distance(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2,
                       int64_t score_cutoff = std::numeric_limits<int64_t>::max())
{
    return detail::Indel::distance(first1, last1, first2, last2, score_cutoff, score_cutoff);
}

template <typename Sentence1, typename Sentence2>
int64_t indel_distance(const Sentence1& s1, const Sentence2& s2,
                       int64_t score_cutoff = std::numeric_limits<int64_t>::max())
{
    return detail::Indel::distance(s1, s2, score_cutoff, score_cutoff);
}

template <typename InputIt1, typename InputIt2>
int64_t indel_similarity(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2,
                         int64_t score_cutoff = 0.0)
{
    return detail::Indel::similarity(first1, last1, first2, last2, score_cutoff, score_cutoff);
}

template <typename Sentence1, typename Sentence2>
int64_t indel_similarity(const Sentence1& s1, const Sentence2& s2, int64_t score_cutoff = 0.0)
{
    return detail::Indel::similarity(s1, s2, score_cutoff, score_cutoff);
}

template <typename InputIt1, typename InputIt2>
double indel_normalized_distance(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2,
                                 double score_cutoff = 1.0)
{
    return detail::Indel::normalized_distance(first1, last1, first2, last2, score_cutoff, score_cutoff);
}

template <typename Sentence1, typename Sentence2>
double indel_normalized_distance(const Sentence1& s1, const Sentence2& s2, double score_cutoff = 1.0)
{
    return detail::Indel::normalized_distance(s1, s2, score_cutoff, score_cutoff);
}

template <typename InputIt1, typename InputIt2>
double indel_normalized_similarity(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2,
                                   double score_cutoff = 0.0)
{
    return detail::Indel::normalized_similarity(first1, last1, first2, last2, score_cutoff, score_cutoff);
}

template <typename Sentence1, typename Sentence2>
double indel_normalized_similarity(const Sentence1& s1, const Sentence2& s2, double score_cutoff = 0.0)
{
    return detail::Indel::normalized_similarity(s1, s2, score_cutoff, score_cutoff);
}

template <typename InputIt1, typename InputIt2>
Editops indel_editops(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2)
{
    return lcs_seq_editops(first1, last1, first2, last2);
}

template <typename Sentence1, typename Sentence2>
Editops indel_editops(const Sentence1& s1, const Sentence2& s2)
{
    return lcs_seq_editops(s1, s2);
}

#ifdef RAPIDFUZZ_SIMD
namespace experimental {
template <int MaxLen>
struct MultiIndel
    : public detail::MultiDistanceBase<MultiIndel<MaxLen>, int64_t, 0, std::numeric_limits<int64_t>::max()> {
private:
    friend detail::MultiDistanceBase<MultiIndel<MaxLen>, int64_t, 0, std::numeric_limits<int64_t>::max()>;
    friend detail::MultiNormalizedMetricBase<MultiIndel<MaxLen>>;

public:
    MultiIndel(size_t count) : scorer(count)
    {}

    /**
     * @brief get minimum size required for result vectors passed into
     * - distance
     * - similarity
     * - normalized_distance
     * - normalized_similarity
     *
     * @return minimum vector size
     */
    size_t result_count() const
    {
        return scorer.result_count();
    }

    template <typename Sentence1>
    void insert(const Sentence1& s1_)
    {
        insert(detail::to_begin(s1_), detail::to_end(s1_));
    }

    template <typename InputIt1>
    void insert(InputIt1 first1, InputIt1 last1)
    {
        scorer.insert(first1, last1);
        str_lens.push_back(static_cast<size_t>(std::distance(first1, last1)));
    }

private:
    template <typename InputIt2>
    void _distance(int64_t* scores, size_t score_count, detail::Range<InputIt2> s2,
                   int64_t score_cutoff = std::numeric_limits<int64_t>::max()) const
    {
        scorer.similarity(scores, score_count, s2);

        for (size_t i = 0; i < get_input_count(); ++i) {
            int64_t maximum_ = maximum(i, s2);
            int64_t dist = maximum_ - 2 * scores[i];
            scores[i] = (dist <= score_cutoff) ? dist : score_cutoff + 1;
        }
    }

    template <typename InputIt2>
    int64_t maximum(size_t s1_idx, detail::Range<InputIt2> s2) const
    {
        return static_cast<int64_t>(str_lens[s1_idx]) + s2.size();
    }

    size_t get_input_count() const noexcept
    {
        return str_lens.size();
    }

    std::vector<size_t> str_lens;
    MultiLCSseq<MaxLen> scorer;
};
} /* namespace experimental */
#endif

template <typename CharT1>
struct CachedIndel : public detail::CachedDistanceBase<CachedIndel<CharT1>, int64_t, 0,
                                                       std::numeric_limits<int64_t>::max()> {
    template <typename Sentence1>
    explicit CachedIndel(const Sentence1& s1_) : CachedIndel(detail::to_begin(s1_), detail::to_end(s1_))
    {}

    template <typename InputIt1>
    CachedIndel(InputIt1 first1, InputIt1 last1) : s1_len(std::distance(first1, last1)), scorer(first1, last1)
    {}

private:
    friend detail::CachedDistanceBase<CachedIndel<CharT1>, int64_t, 0, std::numeric_limits<int64_t>::max()>;
    friend detail::CachedNormalizedMetricBase<CachedIndel<CharT1>>;

    template <typename InputIt2>
    int64_t maximum(detail::Range<InputIt2> s2) const
    {
        return s1_len + s2.size();
    }

    template <typename InputIt2>
    int64_t _distance(detail::Range<InputIt2> s2, int64_t score_cutoff, int64_t score_hint) const
    {
        int64_t maximum_ = maximum(s2);
        int64_t lcs_cutoff = std::max<int64_t>(0, maximum_ / 2 - score_cutoff);
        int64_t lcs_cutoff_hint = std::max<int64_t>(0, maximum_ / 2 - score_hint);
        int64_t lcs_sim = scorer.similarity(s2, lcs_cutoff, lcs_cutoff_hint);
        int64_t dist = maximum_ - 2 * lcs_sim;
        return (dist <= score_cutoff) ? dist : score_cutoff + 1;
    }

    int64_t s1_len;
    CachedLCSseq<CharT1> scorer;
};

template <typename Sentence1>
explicit CachedIndel(const Sentence1& s1_) -> CachedIndel<char_type<Sentence1>>;

template <typename InputIt1>
CachedIndel(InputIt1 first1, InputIt1 last1) -> CachedIndel<iter_value_t<InputIt1>>;

} // namespace rapidfuzz

#include <limits>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

namespace rapidfuzz::detail {

struct FlaggedCharsWord {
    uint64_t P_flag;
    uint64_t T_flag;
};

struct FlaggedCharsMultiword {
    std::vector<uint64_t> P_flag;
    std::vector<uint64_t> T_flag;
};

struct SearchBoundMask {
    size_t words = 0;
    size_t empty_words = 0;
    uint64_t last_mask = 0;
    uint64_t first_mask = 0;
};

struct TextPosition {
    TextPosition(int64_t Word_, int64_t WordPos_) : Word(Word_), WordPos(WordPos_)
    {}
    int64_t Word;
    int64_t WordPos;
};

static inline double jaro_calculate_similarity(int64_t P_len, int64_t T_len, size_t CommonChars,
                                               size_t Transpositions)
{
    Transpositions /= 2;
    double Sim = 0;
    Sim += static_cast<double>(CommonChars) / static_cast<double>(P_len);
    Sim += static_cast<double>(CommonChars) / static_cast<double>(T_len);
    Sim += (static_cast<double>(CommonChars) - static_cast<double>(Transpositions)) /
           static_cast<double>(CommonChars);
    return Sim / 3.0;
}

/**
 * @brief filter matches below score_cutoff based on string lengths
 */
static inline bool jaro_length_filter(int64_t P_len, int64_t T_len, double score_cutoff)
{
    if (!T_len || !P_len) return false;

    double min_len = static_cast<double>(std::min(P_len, T_len));
    double Sim = min_len / static_cast<double>(P_len) + min_len / static_cast<double>(T_len) + 1.0;
    Sim /= 3.0;
    return Sim >= score_cutoff;
}

/**
 * @brief filter matches below score_cutoff based on string lengths and common characters
 */
static inline bool jaro_common_char_filter(int64_t P_len, int64_t T_len, size_t CommonChars,
                                           double score_cutoff)
{
    if (!CommonChars) return false;

    double Sim = 0;
    Sim += static_cast<double>(CommonChars) / static_cast<double>(P_len);
    Sim += static_cast<double>(CommonChars) / static_cast<double>(T_len);
    Sim += 1.0;
    Sim /= 3.0;
    return Sim >= score_cutoff;
}

static inline size_t count_common_chars(const FlaggedCharsWord& flagged)
{
    return static_cast<size_t>(popcount(flagged.P_flag));
}

static inline size_t count_common_chars(const FlaggedCharsMultiword& flagged)
{
    size_t CommonChars = 0;
    if (flagged.P_flag.size() < flagged.T_flag.size()) {
        for (uint64_t flag : flagged.P_flag) {
            CommonChars += static_cast<size_t>(popcount(flag));
        }
    }
    else {
        for (uint64_t flag : flagged.T_flag) {
            CommonChars += static_cast<size_t>(popcount(flag));
        }
    }
    return CommonChars;
}

template <typename PM_Vec, typename InputIt1, typename InputIt2>
FlaggedCharsWord flag_similar_characters_word(const PM_Vec& PM, [[maybe_unused]] Range<InputIt1> P,
                                              Range<InputIt2> T, int Bound)
{
    assert(P.size() <= 64);
    assert(T.size() <= 64);
    assert(Bound > P.size() || P.size() - Bound <= T.size());

    FlaggedCharsWord flagged = {0, 0};

    uint64_t BoundMask = bit_mask_lsb<uint64_t>(Bound + 1);

    int64_t j = 0;
    for (; j < std::min(static_cast<int64_t>(Bound), static_cast<int64_t>(T.size())); ++j) {
        uint64_t PM_j = PM.get(0, T[j]) & BoundMask & (~flagged.P_flag);

        flagged.P_flag |= blsi(PM_j);
        flagged.T_flag |= static_cast<uint64_t>(PM_j != 0) << j;

        BoundMask = (BoundMask << 1) | 1;
    }

    for (; j < T.size(); ++j) {
        uint64_t PM_j = PM.get(0, T[j]) & BoundMask & (~flagged.P_flag);

        flagged.P_flag |= blsi(PM_j);
        flagged.T_flag |= static_cast<uint64_t>(PM_j != 0) << j;

        BoundMask <<= 1;
    }

    return flagged;
}

template <typename CharT>
void flag_similar_characters_step(const BlockPatternMatchVector& PM, CharT T_j,
                                  FlaggedCharsMultiword& flagged, size_t j, SearchBoundMask BoundMask)
{
    size_t j_word = j / 64;
    size_t j_pos = j % 64;
    size_t word = BoundMask.empty_words;
    size_t last_word = word + BoundMask.words;

    if (BoundMask.words == 1) {
        uint64_t PM_j =
            PM.get(word, T_j) & BoundMask.last_mask & BoundMask.first_mask & (~flagged.P_flag[word]);

        flagged.P_flag[word] |= blsi(PM_j);
        flagged.T_flag[j_word] |= static_cast<uint64_t>(PM_j != 0) << j_pos;
        return;
    }

    if (BoundMask.first_mask) {
        uint64_t PM_j = PM.get(word, T_j) & BoundMask.first_mask & (~flagged.P_flag[word]);

        if (PM_j) {
            flagged.P_flag[word] |= blsi(PM_j);
            flagged.T_flag[j_word] |= 1ull << j_pos;
            return;
        }
        word++;
    }

    /* unroll for better performance on long sequences when access is fast */
    if (T_j >= 0 && T_j < 256) {
        for (; word + 3 < last_word - 1; word += 4) {
            uint64_t PM_j[4];
            unroll<int, 4>([&](auto i) {
                PM_j[i] = PM.get(word + i, static_cast<uint8_t>(T_j)) & (~flagged.P_flag[word + i]);
            });

            if (PM_j[0]) {
                flagged.P_flag[word] |= blsi(PM_j[0]);
                flagged.T_flag[j_word] |= 1ull << j_pos;
                return;
            }
            if (PM_j[1]) {
                flagged.P_flag[word + 1] |= blsi(PM_j[1]);
                flagged.T_flag[j_word] |= 1ull << j_pos;
                return;
            }
            if (PM_j[2]) {
                flagged.P_flag[word + 2] |= blsi(PM_j[2]);
                flagged.T_flag[j_word] |= 1ull << j_pos;
                return;
            }
            if (PM_j[3]) {
                flagged.P_flag[word + 3] |= blsi(PM_j[3]);
                flagged.T_flag[j_word] |= 1ull << j_pos;
                return;
            }
        }
    }

    for (; word < last_word - 1; ++word) {
        uint64_t PM_j = PM.get(word, T_j) & (~flagged.P_flag[word]);

        if (PM_j) {
            flagged.P_flag[word] |= blsi(PM_j);
            flagged.T_flag[j_word] |= 1ull << j_pos;
            return;
        }
    }

    if (BoundMask.last_mask) {
        uint64_t PM_j = PM.get(word, T_j) & BoundMask.last_mask & (~flagged.P_flag[word]);

        flagged.P_flag[word] |= blsi(PM_j);
        flagged.T_flag[j_word] |= static_cast<uint64_t>(PM_j != 0) << j_pos;
    }
}

template <typename InputIt1, typename InputIt2>
static inline FlaggedCharsMultiword flag_similar_characters_block(const BlockPatternMatchVector& PM,
                                                                  Range<InputIt1> P, Range<InputIt2> T,
                                                                  int64_t Bound)
{
    assert(P.size() > 64 || T.size() > 64);
    assert(Bound > P.size() || P.size() - Bound <= T.size());
    assert(Bound >= 31);

    FlaggedCharsMultiword flagged;
    flagged.T_flag.resize(static_cast<size_t>(ceil_div(T.size(), 64)));
    flagged.P_flag.resize(static_cast<size_t>(ceil_div(P.size(), 64)));

    SearchBoundMask BoundMask;
    size_t start_range = static_cast<size_t>(std::min(Bound + 1, static_cast<int64_t>(P.size())));
    BoundMask.words = 1 + start_range / 64;
    BoundMask.empty_words = 0;
    BoundMask.last_mask = (1ull << (start_range % 64)) - 1;
    BoundMask.first_mask = ~UINT64_C(0);

    for (int64_t j = 0; j < T.size(); ++j) {
        flag_similar_characters_step(PM, T[j], flagged, static_cast<size_t>(j), BoundMask);

        if (j + Bound + 1 < P.size()) {
            BoundMask.last_mask = (BoundMask.last_mask << 1) | 1;
            if (j + Bound + 2 < P.size() && BoundMask.last_mask == ~UINT64_C(0)) {
                BoundMask.last_mask = 0;
                BoundMask.words++;
            }
        }

        if (j >= Bound) {
            BoundMask.first_mask <<= 1;
            if (BoundMask.first_mask == 0) {
                BoundMask.first_mask = ~UINT64_C(0);
                BoundMask.words--;
                BoundMask.empty_words++;
            }
        }
    }

    return flagged;
}

template <typename PM_Vec, typename InputIt1>
static inline size_t count_transpositions_word(const PM_Vec& PM, Range<InputIt1> T,
                                               const FlaggedCharsWord& flagged)
{
    uint64_t P_flag = flagged.P_flag;
    uint64_t T_flag = flagged.T_flag;
    size_t Transpositions = 0;
    while (T_flag) {
        uint64_t PatternFlagMask = blsi(P_flag);

        Transpositions += !(PM.get(0, T[countr_zero(T_flag)]) & PatternFlagMask);

        T_flag = blsr(T_flag);
        P_flag ^= PatternFlagMask;
    }

    return Transpositions;
}

template <typename InputIt1>
static inline size_t count_transpositions_block(const BlockPatternMatchVector& PM, Range<InputIt1> T,
                                                const FlaggedCharsMultiword& flagged, size_t FlaggedChars)
{
    size_t TextWord = 0;
    size_t PatternWord = 0;
    uint64_t T_flag = flagged.T_flag[TextWord];
    uint64_t P_flag = flagged.P_flag[PatternWord];

    auto T_first = T.begin();
    size_t Transpositions = 0;
    while (FlaggedChars) {
        while (!T_flag) {
            TextWord++;
            T_first += 64;
            T_flag = flagged.T_flag[TextWord];
        }

        while (T_flag) {
            while (!P_flag) {
                PatternWord++;
                P_flag = flagged.P_flag[PatternWord];
            }

            uint64_t PatternFlagMask = blsi(P_flag);

            Transpositions += !(PM.get(PatternWord, T_first[countr_zero(T_flag)]) & PatternFlagMask);

            T_flag = blsr(T_flag);
            P_flag ^= PatternFlagMask;

            FlaggedChars--;
        }
    }

    return Transpositions;
}

/**
 * @brief find bounds and skip out of bound parts of the sequences
 *
 */
template <typename InputIt1, typename InputIt2>
int64_t jaro_bounds(Range<InputIt1>& P, Range<InputIt2>& T)
{
    int64_t P_len = P.size();
    int64_t T_len = T.size();

    /* since jaro uses a sliding window some parts of T/P might never be in
     * range an can be removed ahead of time
     */
    int64_t Bound = 0;
    if (T_len > P_len) {
        Bound = T_len / 2 - 1;
        if (T_len > P_len + Bound) T.remove_suffix(T_len - (P_len + Bound));
    }
    else {
        Bound = P_len / 2 - 1;
        if (P_len > T_len + Bound) P.remove_suffix(P_len - (T_len + Bound));
    }
    return Bound;
}

template <typename InputIt1, typename InputIt2>
double jaro_similarity(Range<InputIt1> P, Range<InputIt2> T, double score_cutoff)
{
    int64_t P_len = P.size();
    int64_t T_len = T.size();

    /* filter out based on the length difference between the two strings */
    if (!jaro_length_filter(P_len, T_len, score_cutoff)) return 0.0;

    if (P_len == 1 && T_len == 1) return static_cast<double>(P[0] == T[0]);

    int64_t Bound = jaro_bounds(P, T);

    /* common prefix never includes Transpositions */
    size_t CommonChars = remove_common_prefix(P, T);
    size_t Transpositions = 0;

    if (P.empty() || T.empty()) {
        /* already has correct number of common chars and transpositions */
    }
    else if (P.size() <= 64 && T.size() <= 64) {
        PatternMatchVector PM(P);
        auto flagged = flag_similar_characters_word(PM, P, T, static_cast<int>(Bound));
        CommonChars += count_common_chars(flagged);

        if (!jaro_common_char_filter(P_len, T_len, CommonChars, score_cutoff)) return 0.0;

        Transpositions = count_transpositions_word(PM, T, flagged);
    }
    else {
        BlockPatternMatchVector PM(P);
        auto flagged = flag_similar_characters_block(PM, P, T, Bound);
        size_t FlaggedChars = count_common_chars(flagged);
        CommonChars += FlaggedChars;

        if (!jaro_common_char_filter(P_len, T_len, CommonChars, score_cutoff)) return 0.0;

        Transpositions = count_transpositions_block(PM, T, flagged, FlaggedChars);
    }

    double Sim = jaro_calculate_similarity(P_len, T_len, CommonChars, Transpositions);
    return (Sim >= score_cutoff) ? Sim : 0;
}

template <typename InputIt1, typename InputIt2>
double jaro_similarity(const BlockPatternMatchVector& PM, Range<InputIt1> P, Range<InputIt2> T,
                       double score_cutoff)
{
    int64_t P_len = P.size();
    int64_t T_len = T.size();

    /* filter out based on the length difference between the two strings */
    if (!jaro_length_filter(P_len, T_len, score_cutoff)) return 0.0;

    if (P_len == 1 && T_len == 1) return static_cast<double>(P[0] == T[0]);

    int64_t Bound = jaro_bounds(P, T);

    /* common prefix never includes Transpositions */
    size_t CommonChars = 0;
    size_t Transpositions = 0;

    if (P.empty() || T.empty()) {
        /* already has correct number of common chars and transpositions */
    }
    else if (P.size() <= 64 && T.size() <= 64) {
        auto flagged = flag_similar_characters_word(PM, P, T, static_cast<int>(Bound));
        CommonChars += count_common_chars(flagged);

        if (!jaro_common_char_filter(P_len, T_len, CommonChars, score_cutoff)) return 0.0;

        Transpositions = count_transpositions_word(PM, T, flagged);
    }
    else {
        auto flagged = flag_similar_characters_block(PM, P, T, Bound);
        size_t FlaggedChars = count_common_chars(flagged);
        CommonChars += FlaggedChars;

        if (!jaro_common_char_filter(P_len, T_len, CommonChars, score_cutoff)) return 0.0;

        Transpositions = count_transpositions_block(PM, T, flagged, FlaggedChars);
    }

    double Sim = jaro_calculate_similarity(P_len, T_len, CommonChars, Transpositions);
    return (Sim >= score_cutoff) ? Sim : 0;
}

class Jaro : public SimilarityBase<Jaro, double, 0, 1> {
    friend SimilarityBase<Jaro, double, 0, 1>;
    friend NormalizedMetricBase<Jaro>;

    template <typename InputIt1, typename InputIt2>
    static double maximum(Range<InputIt1>, Range<InputIt2>) noexcept
    {
        return 1.0;
    }

    template <typename InputIt1, typename InputIt2>
    static double _similarity(Range<InputIt1> s1, Range<InputIt2> s2, double score_cutoff,
                              [[maybe_unused]] double score_hint)
    {
        return jaro_similarity(s1, s2, score_cutoff);
    }
};

} // namespace rapidfuzz::detail

namespace rapidfuzz {

template <typename InputIt1, typename InputIt2>
double jaro_distance(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2,
                     double score_cutoff = 1.0)
{
    return detail::Jaro::distance(first1, last1, first2, last2, score_cutoff, score_cutoff);
}

template <typename Sentence1, typename Sentence2>
double jaro_distance(const Sentence1& s1, const Sentence2& s2, double score_cutoff = 1.0)
{
    return detail::Jaro::distance(s1, s2, score_cutoff, score_cutoff);
}

template <typename InputIt1, typename InputIt2>
double jaro_similarity(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2,
                       double score_cutoff = 0.0)
{
    return detail::Jaro::similarity(first1, last1, first2, last2, score_cutoff, score_cutoff);
}

template <typename Sentence1, typename Sentence2>
double jaro_similarity(const Sentence1& s1, const Sentence2& s2, double score_cutoff = 0.0)
{
    return detail::Jaro::similarity(s1, s2, score_cutoff, score_cutoff);
}

template <typename InputIt1, typename InputIt2>
double jaro_normalized_distance(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2,
                                double score_cutoff = 1.0)
{
    return detail::Jaro::normalized_distance(first1, last1, first2, last2, score_cutoff, score_cutoff);
}

template <typename Sentence1, typename Sentence2>
double jaro_normalized_distance(const Sentence1& s1, const Sentence2& s2, double score_cutoff = 1.0)
{
    return detail::Jaro::normalized_distance(s1, s2, score_cutoff, score_cutoff);
}

template <typename InputIt1, typename InputIt2>
double jaro_normalized_similarity(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2,
                                  double score_cutoff = 0.0)
{
    return detail::Jaro::normalized_similarity(first1, last1, first2, last2, score_cutoff, score_cutoff);
}

template <typename Sentence1, typename Sentence2>
double jaro_normalized_similarity(const Sentence1& s1, const Sentence2& s2, double score_cutoff = 0.0)
{
    return detail::Jaro::normalized_similarity(s1, s2, score_cutoff, score_cutoff);
}

template <typename CharT1>
struct CachedJaro : public detail::CachedSimilarityBase<CachedJaro<CharT1>, double, 0, 1> {
    template <typename Sentence1>
    explicit CachedJaro(const Sentence1& s1_) : CachedJaro(detail::to_begin(s1_), detail::to_end(s1_))
    {}

    template <typename InputIt1>
    CachedJaro(InputIt1 first1, InputIt1 last1) : s1(first1, last1), PM(detail::Range(first1, last1))
    {}

private:
    friend detail::CachedSimilarityBase<CachedJaro<CharT1>, double, 0, 1>;
    friend detail::CachedNormalizedMetricBase<CachedJaro<CharT1>>;

    template <typename InputIt2>
    double maximum(detail::Range<InputIt2>) const
    {
        return 1.0;
    }

    template <typename InputIt2>
    double _similarity(detail::Range<InputIt2> s2, double score_cutoff,
                       [[maybe_unused]] double score_hint) const
    {
        return detail::jaro_similarity(PM, detail::Range(s1), s2, score_cutoff);
    }

    std::basic_string<CharT1> s1;
    detail::BlockPatternMatchVector PM;
};

template <typename Sentence1>
explicit CachedJaro(const Sentence1& s1_) -> CachedJaro<char_type<Sentence1>>;

template <typename InputIt1>
CachedJaro(InputIt1 first1, InputIt1 last1) -> CachedJaro<iter_value_t<InputIt1>>;

} // namespace rapidfuzz

#include <limits>

namespace rapidfuzz::detail {

template <typename InputIt1, typename InputIt2>
double jaro_winkler_similarity(Range<InputIt1> P, Range<InputIt2> T, double prefix_weight,
                               double score_cutoff)
{
    int64_t P_len = P.size();
    int64_t T_len = T.size();
    int64_t min_len = std::min(P_len, T_len);
    int64_t prefix = 0;
    int64_t max_prefix = std::min<int64_t>(min_len, 4);

    for (; prefix < max_prefix; ++prefix)
        if (T[prefix] != P[prefix]) break;

    double jaro_score_cutoff = score_cutoff;
    if (jaro_score_cutoff > 0.7) {
        double prefix_sim = static_cast<double>(prefix) * prefix_weight;

        if (prefix_sim >= 1.0)
            jaro_score_cutoff = 0.7;
        else
            jaro_score_cutoff = std::max(0.7, (prefix_sim - jaro_score_cutoff) / (prefix_sim - 1.0));
    }

    double Sim = jaro_similarity(P, T, jaro_score_cutoff);
    if (Sim > 0.7) Sim += static_cast<double>(prefix) * prefix_weight * (1.0 - Sim);

    return (Sim >= score_cutoff) ? Sim : 0;
}

template <typename InputIt1, typename InputIt2>
double jaro_winkler_similarity(const BlockPatternMatchVector& PM, Range<InputIt1> P, Range<InputIt2> T,
                               double prefix_weight, double score_cutoff)
{
    int64_t P_len = P.size();
    int64_t T_len = T.size();
    int64_t min_len = std::min(P_len, T_len);
    int64_t prefix = 0;
    int64_t max_prefix = std::min<int64_t>(min_len, 4);

    for (; prefix < max_prefix; ++prefix)
        if (T[prefix] != P[prefix]) break;

    double jaro_score_cutoff = score_cutoff;
    if (jaro_score_cutoff > 0.7) {
        double prefix_sim = static_cast<double>(prefix) * prefix_weight;

        if (prefix_sim >= 1.0)
            jaro_score_cutoff = 0.7;
        else
            jaro_score_cutoff = std::max(0.7, (prefix_sim - jaro_score_cutoff) / (prefix_sim - 1.0));
    }

    double Sim = jaro_similarity(PM, P, T, jaro_score_cutoff);
    if (Sim > 0.7) Sim += static_cast<double>(prefix) * prefix_weight * (1.0 - Sim);

    return (Sim >= score_cutoff) ? Sim : 0;
}

class JaroWinkler : public SimilarityBase<JaroWinkler, double, 0, 1, double> {
    friend SimilarityBase<JaroWinkler, double, 0, 1, double>;
    friend NormalizedMetricBase<JaroWinkler, double>;

    template <typename InputIt1, typename InputIt2>
    static double maximum(Range<InputIt1>, Range<InputIt2>, double) noexcept
    {
        return 1.0;
    }

    template <typename InputIt1, typename InputIt2>
    static double _similarity(Range<InputIt1> s1, Range<InputIt2> s2, double prefix_weight,
                              double score_cutoff, [[maybe_unused]] double score_hint)
    {
        return jaro_winkler_similarity(s1, s2, prefix_weight, score_cutoff);
    }
};

} // namespace rapidfuzz::detail

namespace rapidfuzz {

template <typename InputIt1, typename InputIt2,
          typename = std::enable_if_t<!std::is_same_v<InputIt2, double>>>
double jaro_winkler_distance(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2,
                             double prefix_weight = 0.1, double score_cutoff = 1.0)
{
    return detail::JaroWinkler::distance(first1, last1, first2, last2, prefix_weight, score_cutoff,
                                         score_cutoff);
}

template <typename Sentence1, typename Sentence2>
double jaro_winkler_distance(const Sentence1& s1, const Sentence2& s2, double prefix_weight = 0.1,
                             double score_cutoff = 1.0)
{
    return detail::JaroWinkler::distance(s1, s2, prefix_weight, score_cutoff, score_cutoff);
}

template <typename InputIt1, typename InputIt2,
          typename = std::enable_if_t<!std::is_same_v<InputIt2, double>>>
double jaro_winkler_similarity(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2,
                               double prefix_weight = 0.1, double score_cutoff = 0.0)
{
    return detail::JaroWinkler::similarity(first1, last1, first2, last2, prefix_weight, score_cutoff,
                                           score_cutoff);
}

template <typename Sentence1, typename Sentence2>
double jaro_winkler_similarity(const Sentence1& s1, const Sentence2& s2, double prefix_weight = 0.1,
                               double score_cutoff = 0.0)
{
    return detail::JaroWinkler::similarity(s1, s2, prefix_weight, score_cutoff, score_cutoff);
}

template <typename InputIt1, typename InputIt2,
          typename = std::enable_if_t<!std::is_same_v<InputIt2, double>>>
double jaro_winkler_normalized_distance(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2,
                                        double prefix_weight = 0.1, double score_cutoff = 1.0)
{
    return detail::JaroWinkler::normalized_distance(first1, last1, first2, last2, prefix_weight, score_cutoff,
                                                    score_cutoff);
}

template <typename Sentence1, typename Sentence2>
double jaro_winkler_normalized_distance(const Sentence1& s1, const Sentence2& s2, double prefix_weight = 0.1,
                                        double score_cutoff = 1.0)
{
    return detail::JaroWinkler::normalized_distance(s1, s2, prefix_weight, score_cutoff, score_cutoff);
}

template <typename InputIt1, typename InputIt2,
          typename = std::enable_if_t<!std::is_same_v<InputIt2, double>>>
double jaro_winkler_normalized_similarity(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2,
                                          double prefix_weight = 0.1, double score_cutoff = 0.0)
{
    return detail::JaroWinkler::normalized_similarity(first1, last1, first2, last2, prefix_weight,
                                                      score_cutoff, score_cutoff);
}

template <typename Sentence1, typename Sentence2>
double jaro_winkler_normalized_similarity(const Sentence1& s1, const Sentence2& s2,
                                          double prefix_weight = 0.1, double score_cutoff = 0.0)
{
    return detail::JaroWinkler::normalized_similarity(s1, s2, prefix_weight, score_cutoff, score_cutoff);
}

template <typename CharT1>
struct CachedJaroWinkler : public detail::CachedSimilarityBase<CachedJaroWinkler<CharT1>, double, 0, 1> {
    template <typename Sentence1>
    explicit CachedJaroWinkler(const Sentence1& s1_, double _prefix_weight = 0.1)
        : CachedJaroWinkler(detail::to_begin(s1_), detail::to_end(s1_), _prefix_weight)
    {}

    template <typename InputIt1>
    CachedJaroWinkler(InputIt1 first1, InputIt1 last1, double _prefix_weight = 0.1)
        : prefix_weight(_prefix_weight), s1(first1, last1), PM(detail::Range(first1, last1))
    {}

private:
    friend detail::CachedSimilarityBase<CachedJaroWinkler<CharT1>, double, 0, 1>;
    friend detail::CachedNormalizedMetricBase<CachedJaroWinkler<CharT1>>;

    template <typename InputIt2>
    double maximum(detail::Range<InputIt2>) const
    {
        return 1.0;
    }

    template <typename InputIt2>
    double _similarity(detail::Range<InputIt2> s2, double score_cutoff,
                       [[maybe_unused]] double score_hint) const
    {
        return detail::jaro_winkler_similarity(PM, detail::Range(s1), s2, prefix_weight, score_cutoff);
    }

    double prefix_weight;
    std::basic_string<CharT1> s1;
    detail::BlockPatternMatchVector PM;
};

template <typename Sentence1>
explicit CachedJaroWinkler(const Sentence1& s1_, double _prefix_weight = 0.1)
    -> CachedJaroWinkler<char_type<Sentence1>>;

template <typename InputIt1>
CachedJaroWinkler(InputIt1 first1, InputIt1 last1, double _prefix_weight = 0.1)
    -> CachedJaroWinkler<iter_value_t<InputIt1>>;

} // namespace rapidfuzz

#include <limits>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>

namespace rapidfuzz::detail {

struct LevenshteinRow {
    uint64_t VP;
    uint64_t VN;

    LevenshteinRow() : VP(~UINT64_C(0)), VN(0)
    {}

    LevenshteinRow(uint64_t VP_, uint64_t VN_) : VP(VP_), VN(VN_)
    {}
};

template <bool RecordMatrix, bool RecordBitRow>
struct LevenshteinResult;

template <>
struct LevenshteinResult<true, false> {
    ShiftedBitMatrix<uint64_t> VP;
    ShiftedBitMatrix<uint64_t> VN;

    int64_t dist;
};

template <>
struct LevenshteinResult<false, true> {
    size_t first_block;
    size_t last_block;
    int64_t prev_score;
    std::vector<LevenshteinRow> vecs;

    int64_t dist;
};

template <>
struct LevenshteinResult<false, false> {
    int64_t dist;
};

template <typename InputIt1, typename InputIt2>
int64_t generalized_levenshtein_wagner_fischer(Range<InputIt1> s1, Range<InputIt2> s2,
                                               LevenshteinWeightTable weights, int64_t max)
{
    size_t cache_size = static_cast<size_t>(s1.size()) + 1;
    std::vector<int64_t> cache(cache_size);
    assume(cache_size != 0);

    cache[0] = 0;
    for (size_t i = 1; i < cache_size; ++i)
        cache[i] = cache[i - 1] + weights.delete_cost;

    for (const auto& ch2 : s2) {
        auto cache_iter = cache.begin();
        int64_t temp = *cache_iter;
        *cache_iter += weights.insert_cost;

        for (const auto& ch1 : s1) {
            if (ch1 != ch2)
                temp = std::min({*cache_iter + weights.delete_cost, *(cache_iter + 1) + weights.insert_cost,
                                 temp + weights.replace_cost});
            ++cache_iter;
            std::swap(*cache_iter, temp);
        }
    }

    int64_t dist = cache.back();
    return (dist <= max) ? dist : max + 1;
}

/**
 * @brief calculates the maximum possible Levenshtein distance based on
 * string lengths and weights
 */
static inline int64_t levenshtein_maximum(ptrdiff_t len1, ptrdiff_t len2, LevenshteinWeightTable weights)
{
    int64_t max_dist = len1 * weights.delete_cost + len2 * weights.insert_cost;

    if (len1 >= len2)
        max_dist = std::min(max_dist, len2 * weights.replace_cost + (len1 - len2) * weights.delete_cost);
    else
        max_dist = std::min(max_dist, len1 * weights.replace_cost + (len2 - len1) * weights.insert_cost);

    return max_dist;
}

/**
 * @brief calculates the minimal possible Levenshtein distance based on
 * string lengths and weights
 */
template <typename InputIt1, typename InputIt2>
int64_t levenshtein_min_distance(Range<InputIt1> s1, Range<InputIt2> s2, LevenshteinWeightTable weights)
{
    return std::max((s1.size() - s2.size()) * weights.delete_cost,
                    (s2.size() - s1.size()) * weights.insert_cost);
}

template <typename InputIt1, typename InputIt2>
int64_t generalized_levenshtein_distance(Range<InputIt1> s1, Range<InputIt2> s2,
                                         LevenshteinWeightTable weights, int64_t max)
{
    int64_t min_edits = levenshtein_min_distance(s1, s2, weights);
    if (min_edits > max) return max + 1;

    /* common affix does not effect Levenshtein distance */
    remove_common_affix(s1, s2);

    return generalized_levenshtein_wagner_fischer(s1, s2, weights, max);
}

/*
 * An encoded mbleven model table.
 *
 * Each 8-bit integer represents an edit sequence, with using two
 * bits for a single operation.
 *
 * Each Row of 8 integers represent all possible combinations
 * of edit sequences for a gived maximum edit distance and length
 * difference between the two strings, that is below the maximum
 * edit distance
 *
 *   01 = DELETE, 10 = INSERT, 11 = SUBSTITUTE
 *
 * For example, 3F -> 0b111111 means three substitutions
 */
static constexpr std::array<std::array<uint8_t, 8>, 9> levenshtein_mbleven2018_matrix = {{
    /* max edit distance 1 */
    {0x03}, /* len_diff 0 */
    {0x01}, /* len_diff 1 */
    /* max edit distance 2 */
    {0x0F, 0x09, 0x06}, /* len_diff 0 */
    {0x0D, 0x07},       /* len_diff 1 */
    {0x05},             /* len_diff 2 */
    /* max edit distance 3 */
    {0x3F, 0x27, 0x2D, 0x39, 0x36, 0x1E, 0x1B}, /* len_diff 0 */
    {0x3D, 0x37, 0x1F, 0x25, 0x19, 0x16},       /* len_diff 1 */
    {0x35, 0x1D, 0x17},                         /* len_diff 2 */
    {0x15},                                     /* len_diff 3 */
}};

template <typename InputIt1, typename InputIt2>
int64_t levenshtein_mbleven2018(Range<InputIt1> s1, Range<InputIt2> s2, int64_t max)
{
    auto len1 = s1.size();
    auto len2 = s2.size();
    assert(len1 > 0);
    assert(len2 > 0);
    assert(*s1.begin() != *s2.begin());
    assert(*(s1.end() - 1) != *(s2.end() - 1));

    if (len1 < len2) return levenshtein_mbleven2018(s2, s1, max);

    auto len_diff = len1 - len2;

    if (max == 1) return max + static_cast<int64_t>(len_diff == 1 || len1 != 1);

    auto ops_index = (max + max * max) / 2 + len_diff - 1;
    auto& possible_ops = levenshtein_mbleven2018_matrix[static_cast<size_t>(ops_index)];
    int64_t dist = max + 1;

    for (uint8_t ops : possible_ops) {
        ptrdiff_t s1_pos = 0;
        ptrdiff_t s2_pos = 0;
        int64_t cur_dist = 0;
        while (s1_pos < len1 && s2_pos < len2) {
            if (s1[s1_pos] != s2[s2_pos]) {
                cur_dist++;
                if (!ops) break;
                if (ops & 1) s1_pos++;
                if (ops & 2) s2_pos++;
#if defined(__GNUC__) && !defined(__clang__) && !defined(__ICC) && __GNUC__ < 10
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wconversion"
#endif
                ops >>= 2;
#if defined(__GNUC__) && !defined(__clang__) && !defined(__ICC) && __GNUC__ < 10
#    pragma GCC diagnostic pop
#endif
            }
            else {
                s1_pos++;
                s2_pos++;
            }
        }
        cur_dist += (len1 - s1_pos) + (len2 - s2_pos);
        dist = std::min(dist, cur_dist);
    }

    return (dist <= max) ? dist : max + 1;
}

/**
 * @brief Bitparallel implementation of the Levenshtein distance.
 *
 * This implementation requires the first string to have a length <= 64.
 * The algorithm used is described @cite hyrro_2002 and has a time complexity
 * of O(N). Comments and variable names in the implementation follow the
 * paper. This implementation is used internally when the strings are short enough
 *
 * @tparam CharT1 This is the char type of the first sentence
 * @tparam CharT2 This is the char type of the second sentence
 *
 * @param s1
 *   string to compare with s2 (for type info check Template parameters above)
 * @param s2
 *   string to compare with s1 (for type info check Template parameters above)
 *
 * @return returns the levenshtein distance between s1 and s2
 */
template <bool RecordMatrix, bool RecordBitRow, typename PM_Vec, typename InputIt1, typename InputIt2>
auto levenshtein_hyrroe2003(const PM_Vec& PM, Range<InputIt1> s1, Range<InputIt2> s2,
                            int64_t max = std::numeric_limits<int64_t>::max())
    -> LevenshteinResult<RecordMatrix, RecordBitRow>
{
    assert(s1.size() != 0);

    /* VP is set to 1^m. Shifting by bitwidth would be undefined behavior */
    uint64_t VP = ~UINT64_C(0);
    uint64_t VN = 0;

    LevenshteinResult<RecordMatrix, RecordBitRow> res;
    res.dist = s1.size();
    if constexpr (RecordMatrix) {
        res.VP = ShiftedBitMatrix<uint64_t>(static_cast<size_t>(s2.size()), 1, ~UINT64_C(0));
        res.VN = ShiftedBitMatrix<uint64_t>(static_cast<size_t>(s2.size()), 1, 0);
    }

    /* mask used when computing D[m,j] in the paper 10^(m-1) */
    uint64_t mask = UINT64_C(1) << (s1.size() - 1);

    /* Searching */
    for (ptrdiff_t i = 0; i < s2.size(); ++i) {
        /* Step 1: Computing D0 */
        uint64_t PM_j = PM.get(0, s2[i]);
        uint64_t X = PM_j;
        uint64_t D0 = (((X & VP) + VP) ^ VP) | X | VN;

        /* Step 2: Computing HP and HN */
        uint64_t HP = VN | ~(D0 | VP);
        uint64_t HN = D0 & VP;

        /* Step 3: Computing the value D[m,j] */
        res.dist += bool(HP & mask);
        res.dist -= bool(HN & mask);

        /* Step 4: Computing Vp and VN */
        HP = (HP << 1) | 1;
        HN = (HN << 1);

        VP = HN | ~(D0 | HP);
        VN = HP & D0;

        if constexpr (RecordMatrix) {
            res.VP[static_cast<size_t>(i)][0] = VP;
            res.VN[static_cast<size_t>(i)][0] = VN;
        }
    }

    if (res.dist > max) res.dist = max + 1;

    if constexpr (RecordBitRow) {
        res.first_block = 0;
        res.last_block = 0;
        res.prev_score = s2.size();
        res.vecs.emplace_back(VP, VN);
    }

    return res;
}

#ifdef RAPIDFUZZ_SIMD
template <typename VecType, typename InputIt, int _lto_hack = RAPIDFUZZ_LTO_HACK>
void levenshtein_hyrroe2003_simd(Range<int64_t*> scores, const detail::BlockPatternMatchVector& block,
                                 const std::vector<size_t>& s1_lengths, Range<InputIt> s2,
                                 int64_t score_cutoff) noexcept
{
#    ifdef RAPIDFUZZ_AVX2
    using namespace simd_avx2;
#    else
    using namespace simd_sse2;
#    endif
    static constexpr size_t vec_width = native_simd<VecType>::size();
    static constexpr size_t vecs = static_cast<size_t>(native_simd<uint64_t>::size());
    assert(block.size() % vecs == 0);

    native_simd<VecType> zero(VecType(0));
    native_simd<VecType> one(1);
    size_t result_index = 0;

    for (size_t cur_vec = 0; cur_vec < block.size(); cur_vec += vecs) {
        /* VP is set to 1^m */
        native_simd<VecType> VP(static_cast<VecType>(-1));
        native_simd<VecType> VN(VecType(0));

        alignas(32) std::array<VecType, vec_width> currDist_;
        unroll<int, vec_width>(
            [&](auto i) { currDist_[i] = static_cast<VecType>(s1_lengths[result_index + i]); });
        native_simd<VecType> currDist(reinterpret_cast<uint64_t*>(currDist_.data()));
        /* mask used when computing D[m,j] in the paper 10^(m-1) */
        alignas(32) std::array<VecType, vec_width> mask_;
        unroll<int, vec_width>([&](auto i) {
            if (s1_lengths[result_index + i] == 0)
                mask_[i] = 0;
            else
                mask_[i] = static_cast<VecType>(UINT64_C(1) << (s1_lengths[result_index + i] - 1));
        });
        native_simd<VecType> mask(reinterpret_cast<uint64_t*>(mask_.data()));

        for (const auto& ch : s2) {
            /* Step 1: Computing D0 */
            alignas(32) std::array<uint64_t, vecs> stored;
            unroll<int, vecs>([&](auto i) { stored[i] = block.get(cur_vec + i, ch); });

            native_simd<VecType> X(stored.data());
            auto D0 = (((X & VP) + VP) ^ VP) | X | VN;

            /* Step 2: Computing HP and HN */
            auto HP = VN | ~(D0 | VP);
            auto HN = D0 & VP;

            /* Step 3: Computing the value D[m,j] */
            currDist += andnot(one, (HP & mask) == zero);
            currDist -= andnot(one, (HN & mask) == zero);

            /* Step 4: Computing Vp and VN */
            HP = (HP << 1) | one;
            HN = (HN << 1);

            VP = HN | ~(D0 | HP);
            VN = HP & D0;
        }

        alignas(32) std::array<VecType, vec_width> distances;
        currDist.store(distances.data());

        unroll<int, vec_width>([&](auto i) {
            int64_t score = 0;
            /* strings of length 0 are not handled correctly */
            if (s1_lengths[result_index] == 0) {
                score = s2.size();
            }
            /* calculate score under consideration of wraparounds in parallel counter */
            else {
                if constexpr (!std::is_same_v<VecType, uint64_t>) {
                    ptrdiff_t min_dist =
                        std::abs(static_cast<ptrdiff_t>(s1_lengths[result_index]) - s2.size());
                    int64_t wraparound_score = static_cast<int64_t>(std::numeric_limits<VecType>::max()) + 1;

                    score = (min_dist / wraparound_score) * wraparound_score;
                    VecType remainder = static_cast<VecType>(min_dist % wraparound_score);

                    if (distances[i] < remainder) score += wraparound_score;
                }

                score += static_cast<int64_t>(distances[i]);
            }
            scores[static_cast<int64_t>(result_index)] = (score <= score_cutoff) ? score : score_cutoff + 1;
            result_index++;
        });
    }
}
#endif

template <typename InputIt1, typename InputIt2>
int64_t levenshtein_hyrroe2003_small_band(const BlockPatternMatchVector& PM, Range<InputIt1> s1,
                                          Range<InputIt2> s2, int64_t max)
{
    /* VP is set to 1^m. */
    uint64_t VP = ~UINT64_C(0) << (64 - max - 1);
    uint64_t VN = 0;

    const auto words = PM.size();
    int64_t currDist = max;
    uint64_t diagonal_mask = UINT64_C(1) << 63;
    uint64_t horizontal_mask = UINT64_C(1) << 62;
    ptrdiff_t start_pos = max + 1 - 64;

    /* score can decrease along the horizontal, but not along the diagonal */
    int64_t break_score = max + s2.size() - (s1.size() - max);

    /* Searching */
    ptrdiff_t i = 0;
    for (; i < s1.size() - max; ++i, ++start_pos) {
        /* Step 1: Computing D0 */
        uint64_t PM_j = 0;
        if (start_pos < 0) {
            PM_j = PM.get(0, s2[i]) << (-start_pos);
        }
        else {
            size_t word = static_cast<size_t>(start_pos) / 64;
            size_t word_pos = static_cast<size_t>(start_pos) % 64;

            PM_j = PM.get(word, s2[i]) >> word_pos;

            if (word + 1 < words && word_pos != 0) PM_j |= PM.get(word + 1, s2[i]) << (64 - word_pos);
        }
        uint64_t X = PM_j;
        uint64_t D0 = (((X & VP) + VP) ^ VP) | X | VN;

        /* Step 2: Computing HP and HN */
        uint64_t HP = VN | ~(D0 | VP);
        uint64_t HN = D0 & VP;

        /* Step 3: Computing the value D[m,j] */
        currDist += !bool(D0 & diagonal_mask);

        if (currDist > break_score) return max + 1;

        /* Step 4: Computing Vp and VN */
        VP = HN | ~((D0 >> 1) | HP);
        VN = (D0 >> 1) & HP;
    }

    for (; i < s2.size(); ++i, ++start_pos) {
        /* Step 1: Computing D0 */
        uint64_t PM_j = 0;
        if (start_pos < 0) {
            PM_j = PM.get(0, s2[i]) << (-start_pos);
        }
        else {
            size_t word = static_cast<size_t>(start_pos) / 64;
            size_t word_pos = static_cast<size_t>(start_pos) % 64;

            PM_j = PM.get(word, s2[i]) >> word_pos;

            if (word + 1 < words && word_pos != 0) PM_j |= PM.get(word + 1, s2[i]) << (64 - word_pos);
        }
        uint64_t X = PM_j;
        uint64_t D0 = (((X & VP) + VP) ^ VP) | X | VN;

        /* Step 2: Computing HP and HN */
        uint64_t HP = VN | ~(D0 | VP);
        uint64_t HN = D0 & VP;

        /* Step 3: Computing the value D[m,j] */
        currDist += bool(HP & horizontal_mask);
        currDist -= bool(HN & horizontal_mask);
        horizontal_mask >>= 1;

        if (currDist > break_score) return max + 1;

        /* Step 4: Computing Vp and VN */
        VP = HN | ~((D0 >> 1) | HP);
        VN = (D0 >> 1) & HP;
    }

    return (currDist <= max) ? currDist : max + 1;
}

template <bool RecordMatrix, typename InputIt1, typename InputIt2>
auto levenshtein_hyrroe2003_small_band(Range<InputIt1> s1, Range<InputIt2> s2, int64_t max)
    -> LevenshteinResult<RecordMatrix, false>
{
    assert(max <= s1.size());
    assert(max <= s2.size());

    /* VP is set to 1^m. Shifting by bitwidth would be undefined behavior */
    uint64_t VP = ~UINT64_C(0) << (64 - max - 1);
    uint64_t VN = 0;

    LevenshteinResult<RecordMatrix, false> res;
    res.dist = max;
    if constexpr (RecordMatrix) {
        res.VP = ShiftedBitMatrix<uint64_t>(static_cast<size_t>(s2.size()), 1, ~UINT64_C(0));
        res.VN = ShiftedBitMatrix<uint64_t>(static_cast<size_t>(s2.size()), 1, 0);

        ptrdiff_t start_offset = max + 2 - 64;
        for (ptrdiff_t i = 0; i < s2.size(); ++i) {
            res.VP.set_offset(static_cast<size_t>(i), start_offset + i);
            res.VN.set_offset(static_cast<size_t>(i), start_offset + i);
        }
    }

    uint64_t diagonal_mask = UINT64_C(1) << 63;
    uint64_t horizontal_mask = UINT64_C(1) << 62;

    /* score can decrease along the horizontal, but not along the diagonal */
    int64_t break_score = max + s2.size() - (s1.size() - max);
    HybridGrowingHashmap<typename Range<InputIt1>::value_type, std::pair<ptrdiff_t, uint64_t>> PM;

    for (ptrdiff_t j = -max; j < 0; ++j) {
        auto& x = PM[s1[j + max]];
        x.second = shr64(x.second, j - x.first) | (UINT64_C(1) << 63);
        x.first = j;
    }

    /* Searching */
    ptrdiff_t i = 0;
    for (; i < s1.size() - max; ++i) {
        /* Step 1: Computing D0 */
        /* update bitmasks online */
        uint64_t PM_j = 0;
        if (i + max < s1.size()) {
            auto& x = PM[s1[i + max]];
            x.second = shr64(x.second, i - x.first) | (UINT64_C(1) << 63);
            x.first = i;
        }
        {
            auto x = PM.get(s2[i]);
            PM_j = shr64(x.second, i - x.first);
        }

        uint64_t X = PM_j;
        uint64_t D0 = (((X & VP) + VP) ^ VP) | X | VN;

        /* Step 2: Computing HP and HN */
        uint64_t HP = VN | ~(D0 | VP);
        uint64_t HN = D0 & VP;

        /* Step 3: Computing the value D[m,j] */
        res.dist += !bool(D0 & diagonal_mask);

        if (res.dist > break_score) {
            res.dist = max + 1;
            return res;
        }

        /* Step 4: Computing Vp and VN */
        VP = HN | ~((D0 >> 1) | HP);
        VN = (D0 >> 1) & HP;

        if constexpr (RecordMatrix) {
            res.VP[static_cast<size_t>(i)][0] = VP;
            res.VN[static_cast<size_t>(i)][0] = VN;
        }
    }

    for (; i < s2.size(); ++i) {
        /* Step 1: Computing D0 */
        /* update bitmasks online */
        uint64_t PM_j = 0;
        if (i + max < s1.size()) {
            auto& x = PM[s1[i + max]];
            x.second = shr64(x.second, i - x.first) | (UINT64_C(1) << 63);
            x.first = i;
        }
        {
            auto x = PM.get(s2[i]);
            PM_j = shr64(x.second, i - x.first);
        }

        uint64_t X = PM_j;
        uint64_t D0 = (((X & VP) + VP) ^ VP) | X | VN;

        /* Step 2: Computing HP and HN */
        uint64_t HP = VN | ~(D0 | VP);
        uint64_t HN = D0 & VP;

        /* Step 3: Computing the value D[m,j] */
        res.dist += bool(HP & horizontal_mask);
        res.dist -= bool(HN & horizontal_mask);
        horizontal_mask >>= 1;

        if (res.dist > break_score) {
            res.dist = max + 1;
            return res;
        }

        /* Step 4: Computing Vp and VN */
        VP = HN | ~((D0 >> 1) | HP);
        VN = (D0 >> 1) & HP;

        if constexpr (RecordMatrix) {
            res.VP[static_cast<size_t>(i)][0] = VP;
            res.VN[static_cast<size_t>(i)][0] = VN;
        }
    }

    if (res.dist > max) res.dist = max + 1;

    return res;
}

/**
 * @param stop_row specifies the row to record when using RecordBitRow
 */
template <bool RecordMatrix, bool RecordBitRow, typename InputIt1, typename InputIt2>
auto levenshtein_hyrroe2003_block(const BlockPatternMatchVector& PM, Range<InputIt1> s1, Range<InputIt2> s2,
                                  int64_t max = std::numeric_limits<int64_t>::max(), ptrdiff_t stop_row = -1)
    -> LevenshteinResult<RecordMatrix, RecordBitRow>
{
    ptrdiff_t word_size = sizeof(uint64_t) * 8;
    auto words = PM.size();
    std::vector<LevenshteinRow> vecs(words);
    std::vector<int64_t> scores(words);
    uint64_t Last = UINT64_C(1) << ((s1.size() - 1) % word_size);

    for (size_t i = 0; i < words - 1; ++i)
        scores[i] = static_cast<int64_t>(i + 1) * word_size;

    scores[words - 1] = s1.size();

    LevenshteinResult<RecordMatrix, RecordBitRow> res;
    if constexpr (RecordMatrix) {
        int64_t full_band = std::min<int64_t>(s1.size(), 2 * max + 1);
        size_t full_band_words = std::min(words, static_cast<size_t>(full_band / word_size) + 2);
        res.VP = ShiftedBitMatrix<uint64_t>(static_cast<size_t>(s2.size()), full_band_words, ~UINT64_C(0));
        res.VN = ShiftedBitMatrix<uint64_t>(static_cast<size_t>(s2.size()), full_band_words, 0);
    }

    if constexpr (RecordBitRow) {
        res.first_block = 0;
        res.last_block = 0;
        res.prev_score = 0;
    }

    max = std::min(max, static_cast<int64_t>(std::max(s1.size(), s2.size())));

    /* first_block is the index of the first block in Ukkonen band. */
    size_t first_block = 0;
    /* last_block is the index of the last block in Ukkonen band. */
    size_t last_block =
        std::min(words, static_cast<size_t>(
                            ceil_div(std::min(max, (max + s1.size() - s2.size()) / 2) + 1, word_size))) -
        1;

    /* Searching */
    for (ptrdiff_t row = 0; row < s2.size(); ++row) {
        uint64_t HP_carry = 1;
        uint64_t HN_carry = 0;

        if constexpr (RecordMatrix) {
            res.VP.set_offset(static_cast<size_t>(row), static_cast<int64_t>(first_block) * word_size);
            res.VN.set_offset(static_cast<size_t>(row), static_cast<int64_t>(first_block) * word_size);
        }

        auto advance_block = [&](size_t word) {
            /* Step 1: Computing D0 */
            uint64_t PM_j = PM.get(word, s2[row]);
            uint64_t VN = vecs[word].VN;
            uint64_t VP = vecs[word].VP;

            uint64_t X = PM_j | HN_carry;
            uint64_t D0 = (((X & VP) + VP) ^ VP) | X | VN;

            /* Step 2: Computing HP and HN */
            uint64_t HP = VN | ~(D0 | VP);
            uint64_t HN = D0 & VP;

            uint64_t HP_carry_temp = HP_carry;
            uint64_t HN_carry_temp = HN_carry;
            if (word < words - 1) {
                HP_carry = HP >> 63;
                HN_carry = HN >> 63;
            }
            else {
                HP_carry = bool(HP & Last);
                HN_carry = bool(HN & Last);
            }

            /* Step 4: Computing Vp and VN */
            HP = (HP << 1) | HP_carry_temp;
            HN = (HN << 1) | HN_carry_temp;

            vecs[word].VP = HN | ~(D0 | HP);
            vecs[word].VN = HP & D0;

            if constexpr (RecordMatrix) {
                res.VP[static_cast<size_t>(row)][word - first_block] = vecs[word].VP;
                res.VN[static_cast<size_t>(row)][word - first_block] = vecs[word].VN;
            }

            return static_cast<int64_t>(HP_carry) - static_cast<int64_t>(HN_carry);
        };

        auto get_row_num = [&](size_t word) {
            if (word + 1 == words) return s1.size() - 1;
            return static_cast<ptrdiff_t>(word + 1) * word_size - 1;
        };

        for (size_t word = first_block; word <= last_block /* - 1*/; word++) {
            /* Step 3: Computing the value D[m,j] */
            scores[word] += advance_block(word);
        }

        max = std::min(
            max, scores[last_block] +
                     std::max(s2.size() - row - 1,
                              s1.size() - (static_cast<ptrdiff_t>(1 + last_block) * word_size - 1) - 1));

        /*---------- Adjust number of blocks according to Ukkonen ----------*/
        // todo on the last word instead of word_size often s1.size() % 64 should be used

        /* Band adjustment: last_block */
        /*  If block is not beneath band, calculate next block. Only next because others are certainly beneath
         * band. */
        if (last_block + 1 < words && !(get_row_num(last_block) > max - scores[last_block] + 2 * word_size -
                                                                      2 - s2.size() + row + s1.size()))
        {
            last_block++;
            vecs[last_block].VP = ~UINT64_C(0);
            vecs[last_block].VN = 0;

            int64_t chars_in_block = (last_block + 1 == words) ? ((s1.size() - 1) % word_size + 1) : 64;
            scores[last_block] = scores[last_block - 1] + chars_in_block -
                                 (static_cast<int64_t>(HP_carry) - static_cast<int64_t>(HN_carry));
            scores[last_block] += advance_block(last_block);
        }

        for (; last_block >= first_block; --last_block) {
            /* in band if score <= k where score >= score_last - word_size + 1 */
            bool in_band_cond1 = scores[last_block] < max + word_size;

            /* in band if row <= max - score - len2 + len1 + i
             * if the condition is met for the first cell in the block, it
             * is met for all other cells in the blocks as well
             *
             * this uses a more loose condition similar to edlib:
             * https://github.com/Martinsos/edlib
             */
            bool in_band_cond2 = get_row_num(last_block) <= max - scores[last_block] + 2 * word_size - 2 -
                                                                s2.size() + row + s1.size() + 1;

            if (in_band_cond1 && in_band_cond2) break;
        }

        /* Band adjustment: first_block */
        for (; first_block <= last_block; ++first_block) {
            /* in band if score <= k where score >= score_last - word_size + 1 */
            bool in_band_cond1 = scores[first_block] < max + word_size;

            /* in band if row >= score - max - len2 + len1 + i
             * if this condition is met for the last cell in the block, it
             * is met for all other cells in the blocks as well
             */
            bool in_band_cond2 =
                get_row_num(first_block) >= scores[first_block] - max - s2.size() + s1.size() + row;

            if (in_band_cond1 && in_band_cond2) break;
        }

        /* distance is larger than max, so band stops to exist */
        if (last_block < first_block) {
            res.dist = max + 1;
            return res;
        }

        if constexpr (RecordBitRow) {
            if (row == stop_row) {
                if (first_block == 0)
                    res.prev_score = stop_row + 1;
                else {
                    /* count backwards to find score at last position in previous block */
                    ptrdiff_t relevant_bits =
                        std::min(static_cast<ptrdiff_t>((first_block + 1) * 64), s1.size()) % 64;
                    uint64_t mask = ~UINT64_C(0);
                    if (relevant_bits) mask >>= 64 - relevant_bits;

                    res.prev_score = scores[first_block] + popcount(vecs[first_block].VN & mask) -
                                     popcount(vecs[first_block].VP & mask);
                }

                res.first_block = first_block;
                res.last_block = last_block;
                res.vecs = std::move(vecs);

                /* unknown so make sure it is <= max */
                res.dist = 0;
                return res;
            }
        }
    }

    res.dist = scores[words - 1];

    if (res.dist > max) res.dist = max + 1;

    return res;
}

template <typename InputIt1, typename InputIt2>
int64_t uniform_levenshtein_distance(const BlockPatternMatchVector& block, Range<InputIt1> s1,
                                     Range<InputIt2> s2, int64_t score_cutoff, int64_t score_hint)
{
    /* upper bound */
    score_cutoff = std::min(score_cutoff, std::max<int64_t>(s1.size(), s2.size()));
    if (score_hint < 31) score_hint = 31;

    // when no differences are allowed a direct comparision is sufficient
    if (score_cutoff == 0) return !std::equal(s1.begin(), s1.end(), s2.begin(), s2.end());

    if (score_cutoff < std::abs(s1.size() - s2.size())) return score_cutoff + 1;

    // important to catch, since this causes block to be empty -> raises exception on access
    if (s1.empty()) return (s2.size() <= score_cutoff) ? s2.size() : score_cutoff + 1;

    /* do this first, since we can not remove any affix in encoded form
     * todo actually we could at least remove the common prefix and just shift the band
     */
    if (score_cutoff >= 4) {
        // todo could safe up to 25% even without max when ignoring irrelevant paths
        // in the upper and lower corner
        int64_t full_band = std::min<int64_t>(s1.size(), 2 * score_cutoff + 1);

        if (s1.size() < 65)
            return levenshtein_hyrroe2003<false, false>(block, s1, s2, score_cutoff).dist;
        else if (full_band <= 64)
            return levenshtein_hyrroe2003_small_band(block, s1, s2, score_cutoff);

        while (score_hint < score_cutoff) {
            full_band = std::min<int64_t>(s1.size(), 2 * score_hint + 1);

            int64_t score;
            if (full_band <= 64)
                score = levenshtein_hyrroe2003_small_band(block, s1, s2, score_hint);
            else
                score = levenshtein_hyrroe2003_block<false, false>(block, s1, s2, score_hint).dist;

            if (score <= score_hint) return score;

            if (std::numeric_limits<int64_t>::max() / 2 < score_hint) break;

            score_hint *= 2;
        }

        return levenshtein_hyrroe2003_block<false, false>(block, s1, s2, score_cutoff).dist;
    }

    /* common affix does not effect Levenshtein distance */
    remove_common_affix(s1, s2);
    if (s1.empty() || s2.empty()) return s1.size() + s2.size();

    return levenshtein_mbleven2018(s1, s2, score_cutoff);
}

template <typename InputIt1, typename InputIt2>
int64_t uniform_levenshtein_distance(Range<InputIt1> s1, Range<InputIt2> s2, int64_t score_cutoff,
                                     int64_t score_hint)
{
    /* Swapping the strings so the second string is shorter */
    if (s1.size() < s2.size()) return uniform_levenshtein_distance(s2, s1, score_cutoff, score_hint);

    /* upper bound */
    score_cutoff = std::min(score_cutoff, std::max<int64_t>(s1.size(), s2.size()));
    if (score_hint < 31) score_hint = 31;

    // when no differences are allowed a direct comparision is sufficient
    if (score_cutoff == 0) return !std::equal(s1.begin(), s1.end(), s2.begin(), s2.end());

    // at least length difference insertions/deletions required
    if (score_cutoff < (s1.size() - s2.size())) return score_cutoff + 1;

    /* common affix does not effect Levenshtein distance */
    remove_common_affix(s1, s2);
    if (s1.empty() || s2.empty()) return s1.size() + s2.size();

    if (score_cutoff < 4) return levenshtein_mbleven2018(s1, s2, score_cutoff);

    // todo could safe up to 25% even without score_cutoff when ignoring irrelevant paths
    // in the upper and lower corner
    int64_t full_band = std::min<int64_t>(s1.size(), 2 * score_cutoff + 1);

    /* when the short strings has less then 65 elements Hyyrös' algorithm can be used */
    if (s2.size() < 65)
        return levenshtein_hyrroe2003<false, false>(PatternMatchVector(s2), s2, s1, score_cutoff).dist;
    else if (full_band <= 64)
        return levenshtein_hyrroe2003_small_band<false>(s1, s2, score_cutoff).dist;
    else {
        BlockPatternMatchVector PM(s1);
        while (score_hint < score_cutoff) {
            int64_t score = levenshtein_hyrroe2003_block<false, false>(PM, s1, s2, score_hint).dist;
            if (score <= score_hint) return score;

            if (std::numeric_limits<int64_t>::max() / 2 < score_hint) break;

            score_hint *= 2;
        }

        return levenshtein_hyrroe2003_block<false, false>(PM, s1, s2, score_cutoff).dist;
    }
}

/**
 * @brief recover alignment from bitparallel Levenshtein matrix
 */
template <typename InputIt1, typename InputIt2>
void recover_alignment(Editops& editops, Range<InputIt1> s1, Range<InputIt2> s2,
                       const LevenshteinResult<true, false>& matrix, size_t src_pos, size_t dest_pos,
                       size_t editop_pos)
{
    size_t dist = static_cast<size_t>(matrix.dist);
    size_t col = static_cast<size_t>(s1.size());
    size_t row = static_cast<size_t>(s2.size());

    while (row && col) {
        /* Deletion */
        if (matrix.VP.test_bit(row - 1, col - 1)) {
            assert(dist > 0);
            dist--;
            col--;
            editops[editop_pos + dist].type = EditType::Delete;
            editops[editop_pos + dist].src_pos = col + src_pos;
            editops[editop_pos + dist].dest_pos = row + dest_pos;
        }
        else {
            row--;

            /* Insertion */
            if (row && matrix.VN.test_bit(row - 1, col - 1)) {
                assert(dist > 0);
                dist--;
                editops[editop_pos + dist].type = EditType::Insert;
                editops[editop_pos + dist].src_pos = col + src_pos;
                editops[editop_pos + dist].dest_pos = row + dest_pos;
            }
            /* Match/Mismatch */
            else {
                col--;

                /* Replace (Matches are not recorded) */
                if (s1[static_cast<ptrdiff_t>(col)] != s2[static_cast<ptrdiff_t>(row)]) {
                    assert(dist > 0);
                    dist--;
                    editops[editop_pos + dist].type = EditType::Replace;
                    editops[editop_pos + dist].src_pos = col + src_pos;
                    editops[editop_pos + dist].dest_pos = row + dest_pos;
                }
            }
        }
    }

    while (col) {
        dist--;
        col--;
        editops[editop_pos + dist].type = EditType::Delete;
        editops[editop_pos + dist].src_pos = col + src_pos;
        editops[editop_pos + dist].dest_pos = row + dest_pos;
    }

    while (row) {
        dist--;
        row--;
        editops[editop_pos + dist].type = EditType::Insert;
        editops[editop_pos + dist].src_pos = col + src_pos;
        editops[editop_pos + dist].dest_pos = row + dest_pos;
    }
}

template <typename InputIt1, typename InputIt2>
void levenshtein_align(Editops& editops, Range<InputIt1> s1, Range<InputIt2> s2,
                       int64_t max = std::numeric_limits<int64_t>::max(), size_t src_pos = 0,
                       size_t dest_pos = 0, size_t editop_pos = 0)
{
    /* upper bound */
    max = std::min(max, std::max<int64_t>(s1.size(), s2.size()));
    int64_t full_band = std::min<int64_t>(s1.size(), 2 * max + 1);

    LevenshteinResult<true, false> matrix;
    if (s1.empty() || s2.empty())
        matrix.dist = s1.size() + s2.size();
    else if (s1.size() <= 64)
        matrix = levenshtein_hyrroe2003<true, false>(PatternMatchVector(s1), s1, s2);
    else if (full_band <= 64)
        matrix = levenshtein_hyrroe2003_small_band<true>(s1, s2, max);
    else
        matrix = levenshtein_hyrroe2003_block<true, false>(BlockPatternMatchVector(s1), s1, s2, max);

    assert(matrix.dist <= max);
    if (matrix.dist != 0) {
        if (editops.size() == 0) editops.resize(static_cast<size_t>(matrix.dist));

        recover_alignment(editops, s1, s2, matrix, src_pos, dest_pos, editop_pos);
    }
}

template <typename InputIt1, typename InputIt2>
LevenshteinResult<false, true> levenshtein_row(Range<InputIt1> s1, Range<InputIt2> s2, int64_t max,
                                               ptrdiff_t stop_row)
{
    return levenshtein_hyrroe2003_block<false, true>(BlockPatternMatchVector(s1), s1, s2, max, stop_row);
}

template <typename InputIt1, typename InputIt2>
int64_t levenshtein_distance(Range<InputIt1> s1, Range<InputIt2> s2,
                             LevenshteinWeightTable weights = {1, 1, 1},
                             int64_t score_cutoff = std::numeric_limits<int64_t>::max(),
                             int64_t score_hint = std::numeric_limits<int64_t>::max())
{
    if (weights.insert_cost == weights.delete_cost) {
        /* when insertions + deletions operations are free there can not be any edit distance */
        if (weights.insert_cost == 0) return 0;

        /* uniform Levenshtein multiplied with the common factor */
        if (weights.insert_cost == weights.replace_cost) {
            // score_cutoff can make use of the common divisor of the three weights
            int64_t new_score_cutoff = ceil_div(score_cutoff, weights.insert_cost);
            int64_t new_score_hint = ceil_div(score_hint, weights.insert_cost);
            int64_t distance = uniform_levenshtein_distance(s1, s2, new_score_cutoff, new_score_hint);
            distance *= weights.insert_cost;
            return (distance <= score_cutoff) ? distance : score_cutoff + 1;
        }
        /*
         * when replace_cost >= insert_cost + delete_cost no substitutions are performed
         * therefore this can be implemented as InDel distance multiplied with the common factor
         */
        else if (weights.replace_cost >= weights.insert_cost + weights.delete_cost) {
            // score_cutoff can make use of the common divisor of the three weights
            int64_t new_score_cutoff = ceil_div(score_cutoff, weights.insert_cost);
            int64_t distance = rapidfuzz::indel_distance(s1, s2, new_score_cutoff);
            distance *= weights.insert_cost;
            return (distance <= score_cutoff) ? distance : score_cutoff + 1;
        }
    }

    return generalized_levenshtein_wagner_fischer(s1, s2, weights, score_cutoff);
}
struct HirschbergPos {
    int64_t left_score;
    int64_t right_score;
    ptrdiff_t s1_mid;
    ptrdiff_t s2_mid;
};

template <typename InputIt1, typename InputIt2>
HirschbergPos find_hirschberg_pos(Range<InputIt1> s1, Range<InputIt2> s2,
                                  int64_t max = std::numeric_limits<int64_t>::max())
{
    HirschbergPos hpos = {};
    ptrdiff_t left_size = s2.size() / 2;
    ptrdiff_t right_size = s2.size() - left_size;
    hpos.s2_mid = left_size;
    size_t s1_len = static_cast<size_t>(s1.size());
    int64_t best_score = std::numeric_limits<int64_t>::max();
    size_t right_first_pos = 0;
    size_t right_last_pos = 0;
    std::vector<int64_t> right_scores;

    {
        auto right_row = levenshtein_row(s1.reversed(), s2.reversed(), max, right_size - 1);
        if (right_row.dist > max) return find_hirschberg_pos(s1, s2, max * 2);

        right_first_pos = right_row.first_block * 64;
        right_last_pos = std::min(s1_len, right_row.last_block * 64 + 64);

        right_scores.resize(right_last_pos - right_first_pos + 1, 0);
        assume(right_scores.size() != 0);
        right_scores[0] = right_row.prev_score;

        for (size_t i = right_first_pos; i < right_last_pos; ++i) {
            size_t col_pos = i % 64;
            size_t col_word = i / 64;
            uint64_t col_mask = UINT64_C(1) << col_pos;

            right_scores[i - right_first_pos + 1] = right_scores[i - right_first_pos];
            right_scores[i - right_first_pos + 1] -= bool(right_row.vecs[col_word].VN & col_mask);
            right_scores[i - right_first_pos + 1] += bool(right_row.vecs[col_word].VP & col_mask);
        }
    }

    auto left_row = levenshtein_row(s1, s2, max, left_size - 1);
    if (left_row.dist > max) return find_hirschberg_pos(s1, s2, max * 2);

    auto left_first_pos = left_row.first_block * 64;
    auto left_last_pos = std::min(s1_len, left_row.last_block * 64 + 64);

    int64_t left_score = left_row.prev_score;
    for (size_t i = left_first_pos; i < left_last_pos; ++i) {
        size_t col_pos = i % 64;
        size_t col_word = i / 64;
        uint64_t col_mask = UINT64_C(1) << col_pos;

        left_score -= bool(left_row.vecs[col_word].VN & col_mask);
        left_score += bool(left_row.vecs[col_word].VP & col_mask);

        if (s1_len < i + 1 + right_first_pos) continue;

        size_t right_index = s1_len - i - 1 - right_first_pos;
        if (right_index >= right_scores.size()) continue;

        if (right_scores[right_index] + left_score < best_score) {
            best_score = right_scores[right_index] + left_score;
            hpos.left_score = left_score;
            hpos.right_score = right_scores[right_index];
            hpos.s1_mid = static_cast<ptrdiff_t>(i + 1);
        }
    }

    assert(hpos.left_score >= 0);
    assert(hpos.right_score >= 0);

    if (hpos.left_score + hpos.right_score > max)
        return find_hirschberg_pos(s1, s2, max * 2);
    else {
        assert(levenshtein_distance(s1, s2) == hpos.left_score + hpos.right_score);
        return hpos;
    }
}

template <typename InputIt1, typename InputIt2>
void levenshtein_align_hirschberg(Editops& editops, Range<InputIt1> s1, Range<InputIt2> s2,
                                  size_t src_pos = 0, size_t dest_pos = 0, size_t editop_pos = 0,
                                  int64_t max = std::numeric_limits<int64_t>::max())
{
    /* prefix and suffix are no-ops, which do not need to be added to the editops */
    StringAffix affix = remove_common_affix(s1, s2);
    src_pos += affix.prefix_len;
    dest_pos += affix.prefix_len;

    max = std::min(max, std::max<int64_t>(s1.size(), s2.size()));
    int64_t full_band = std::min<int64_t>(s1.size(), 2 * max + 1);

    ptrdiff_t matrix_size = 2 * full_band * s2.size() / 8;
    if (matrix_size < 1024 * 1024 || s1.size() < 65 || s2.size() < 10) {
        levenshtein_align(editops, s1, s2, max, src_pos, dest_pos, editop_pos);
    }
    /* Hirschbergs algorithm */
    else {
        auto hpos = find_hirschberg_pos(s1, s2, max);

        if (editops.size() == 0) editops.resize(static_cast<size_t>(hpos.left_score + hpos.right_score));

        levenshtein_align_hirschberg(editops, s1.subseq(0, hpos.s1_mid), s2.subseq(0, hpos.s2_mid), src_pos,
                                     dest_pos, editop_pos, hpos.left_score);
        levenshtein_align_hirschberg(editops, s1.subseq(hpos.s1_mid), s2.subseq(hpos.s2_mid),
                                     src_pos + static_cast<size_t>(hpos.s1_mid),
                                     dest_pos + static_cast<size_t>(hpos.s2_mid),
                                     editop_pos + static_cast<size_t>(hpos.left_score), hpos.right_score);
    }
}

class Levenshtein : public DistanceBase<Levenshtein, int64_t, 0, std::numeric_limits<int64_t>::max(),
                                        LevenshteinWeightTable> {
    friend DistanceBase<Levenshtein, int64_t, 0, std::numeric_limits<int64_t>::max(), LevenshteinWeightTable>;
    friend NormalizedMetricBase<Levenshtein, LevenshteinWeightTable>;

    template <typename InputIt1, typename InputIt2>
    static int64_t maximum(Range<InputIt1> s1, Range<InputIt2> s2, LevenshteinWeightTable weights)
    {
        return levenshtein_maximum(s1.size(), s2.size(), weights);
    }

    template <typename InputIt1, typename InputIt2>
    static int64_t _distance(Range<InputIt1> s1, Range<InputIt2> s2, LevenshteinWeightTable weights,
                             int64_t score_cutoff, int64_t score_hint)
    {
        return levenshtein_distance(s1, s2, weights, score_cutoff, score_hint);
    }
};

template <typename InputIt1, typename InputIt2>
Editops levenshtein_editops(Range<InputIt1> s1, Range<InputIt2> s2, int64_t score_hint)
{
    Editops editops;
    if (score_hint < 31) score_hint = 31;

    int64_t score_cutoff = std::max(s1.size(), s2.size());
    /* score_hint currently leads to calculating the levenshtein distance twice
     * 1) to find the real distance
     * 2) to find the alignment
     * this is only worth it when at least 50% of the runtime could be saved
     * todo: maybe there is a way to join these two calculations in the future
     * so it is worth it in more cases
     */
    if (std::numeric_limits<int64_t>::max() / 2 > score_hint && 2 * score_hint < score_cutoff)
        score_cutoff = Levenshtein::distance(s1, s2, {1, 1, 1}, score_cutoff, score_hint);

    levenshtein_align_hirschberg(editops, s1, s2, 0, 0, 0, score_cutoff);

    editops.set_src_len(static_cast<size_t>(s1.size()));
    editops.set_dest_len(static_cast<size_t>(s2.size()));
    return editops;
}

} // namespace rapidfuzz::detail

namespace rapidfuzz {

/**
 * @brief Calculates the minimum number of insertions, deletions, and substitutions
 * required to change one sequence into the other according to Levenshtein with custom
 * costs for insertion, deletion and substitution
 *
 * @tparam Sentence1 This is a string that can be converted to
 * basic_string_view<char_type>
 * @tparam Sentence2 This is a string that can be converted to
 * basic_string_view<char_type>
 *
 * @param s1
 *   string to compare with s2 (for type info check Template parameters above)
 * @param s2
 *   string to compare with s1 (for type info check Template parameters above)
 * @param weights
 *   The weights for the three operations in the form
 *   (insertion, deletion, substitution). Default is {1, 1, 1},
 *   which gives all three operations a weight of 1.
 * @param max
 *   Maximum Levenshtein distance between s1 and s2, that is
 *   considered as a result. If the distance is bigger than max,
 *   max + 1 is returned instead. Default is std::numeric_limits<int64_t>::max(),
 *   which deactivates this behaviour.
 *
 * @return returns the levenshtein distance between s1 and s2
 *
 * @remarks
 * @parblock
 * Depending on the input parameters different optimized implementation are used
 * to improve the performance. Worst-case performance is ``O(m * n)``.
 *
 * <b>Insertion = Deletion = Substitution:</b>
 *
 *    This is known as uniform Levenshtein distance and is the distance most commonly
 *    referred to as Levenshtein distance. The following implementation is used
 *    with a worst-case performance of ``O([N/64]M)``.
 *
 *    - if max is 0 the similarity can be calculated using a direct comparision,
 *      since no difference between the strings is allowed.  The time complexity of
 *      this algorithm is ``O(N)``.
 *
 *    - A common prefix/suffix of the two compared strings does not affect
 *      the Levenshtein distance, so the affix is removed before calculating the
 *      similarity.
 *
 *    - If max is <= 3 the mbleven algorithm is used. This algorithm
 *      checks all possible edit operations that are possible under
 *      the threshold `max`. The time complexity of this algorithm is ``O(N)``.
 *
 *    - If the length of the shorter string is <= 64 after removing the common affix
 *      Hyyrös' algorithm is used, which calculates the Levenshtein distance in
 *      parallel. The algorithm is described by @cite hyrro_2002. The time complexity of this
 *      algorithm is ``O(N)``.
 *
 *    - If the length of the shorter string is >= 64 after removing the common affix
 *      a blockwise implementation of Myers' algorithm is used, which calculates
 *      the Levenshtein distance in parallel (64 characters at a time).
 *      The algorithm is described by @cite myers_1999. The time complexity of this
 *      algorithm is ``O([N/64]M)``.
 *
 *
 * <b>Insertion = Deletion, Substitution >= Insertion + Deletion:</b>
 *
 *    Since every Substitution can be performed as Insertion + Deletion, this variant
 *    of the Levenshtein distance only uses Insertions and Deletions. Therefore this
 *    variant is often referred to as InDel-Distance.  The following implementation
 *    is used with a worst-case performance of ``O([N/64]M)``.
 *
 *    - if max is 0 the similarity can be calculated using a direct comparision,
 *      since no difference between the strings is allowed.  The time complexity of
 *      this algorithm is ``O(N)``.
 *
 *    - if max is 1 and the two strings have a similar length, the similarity can be
 *      calculated using a direct comparision aswell, since a substitution would cause
 *      a edit distance higher than max. The time complexity of this algorithm
 *      is ``O(N)``.
 *
 *    - A common prefix/suffix of the two compared strings does not affect
 *      the Levenshtein distance, so the affix is removed before calculating the
 *      similarity.
 *
 *    - If max is <= 4 the mbleven algorithm is used. This algorithm
 *      checks all possible edit operations that are possible under
 *      the threshold `max`. As a difference to the normal Levenshtein distance this
 *      algorithm can even be used up to a threshold of 4 here, since the higher weight
 *      of substitutions decreases the amount of possible edit operations.
 *      The time complexity of this algorithm is ``O(N)``.
 *
 *    - If the length of the shorter string is <= 64 after removing the common affix
 *      Hyyrös' lcs algorithm is used, which calculates the InDel distance in
 *      parallel. The algorithm is described by @cite hyrro_lcs_2004 and is extended with support
 *      for UTF32 in this implementation. The time complexity of this
 *      algorithm is ``O(N)``.
 *
 *    - If the length of the shorter string is >= 64 after removing the common affix
 *      a blockwise implementation of Hyyrös' lcs algorithm is used, which calculates
 *      the Levenshtein distance in parallel (64 characters at a time).
 *      The algorithm is described by @cite hyrro_lcs_2004. The time complexity of this
 *      algorithm is ``O([N/64]M)``.
 *
 * <b>Other weights:</b>
 *
 *   The implementation for other weights is based on Wagner-Fischer.
 *   It has a performance of ``O(N * M)`` and has a memory usage of ``O(N)``.
 *   Further details can be found in @cite wagner_fischer_1974.
 * @endparblock
 *
 * @par Examples
 * @parblock
 * Find the Levenshtein distance between two strings:
 * @code{.cpp}
 * // dist is 2
 * int64_t dist = levenshtein_distance("lewenstein", "levenshtein");
 * @endcode
 *
 * Setting a maximum distance allows the implementation to select
 * a more efficient implementation:
 * @code{.cpp}
 * // dist is 2
 * int64_t dist = levenshtein_distance("lewenstein", "levenshtein", {1, 1, 1}, 1);
 * @endcode
 *
 * It is possible to select different weights by passing a `weight` struct.
 * @code{.cpp}
 * // dist is 3
 * int64_t dist = levenshtein_distance("lewenstein", "levenshtein", {1, 1, 2});
 * @endcode
 * @endparblock
 */
template <typename InputIt1, typename InputIt2>
int64_t levenshtein_distance(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2,
                             LevenshteinWeightTable weights = {1, 1, 1},
                             int64_t score_cutoff = std::numeric_limits<int64_t>::max(),
                             int64_t score_hint = std::numeric_limits<int64_t>::max())
{
    return detail::Levenshtein::distance(first1, last1, first2, last2, weights, score_cutoff, score_hint);
}

template <typename Sentence1, typename Sentence2>
int64_t levenshtein_distance(const Sentence1& s1, const Sentence2& s2,
                             LevenshteinWeightTable weights = {1, 1, 1},
                             int64_t score_cutoff = std::numeric_limits<int64_t>::max(),
                             int64_t score_hint = std::numeric_limits<int64_t>::max())
{
    return detail::Levenshtein::distance(s1, s2, weights, score_cutoff, score_hint);
}

template <typename InputIt1, typename InputIt2>
int64_t levenshtein_similarity(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2,
                               LevenshteinWeightTable weights = {1, 1, 1}, int64_t score_cutoff = 0,
                               int64_t score_hint = 0)
{
    return detail::Levenshtein::similarity(first1, last1, first2, last2, weights, score_cutoff, score_hint);
}

template <typename Sentence1, typename Sentence2>
int64_t levenshtein_similarity(const Sentence1& s1, const Sentence2& s2,
                               LevenshteinWeightTable weights = {1, 1, 1}, int64_t score_cutoff = 0,
                               int64_t score_hint = 0)
{
    return detail::Levenshtein::similarity(s1, s2, weights, score_cutoff, score_hint);
}

template <typename InputIt1, typename InputIt2>
double levenshtein_normalized_distance(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2,
                                       LevenshteinWeightTable weights = {1, 1, 1}, double score_cutoff = 1.0,
                                       double score_hint = 1.0)
{
    return detail::Levenshtein::normalized_distance(first1, last1, first2, last2, weights, score_cutoff,
                                                    score_hint);
}

template <typename Sentence1, typename Sentence2>
double levenshtein_normalized_distance(const Sentence1& s1, const Sentence2& s2,
                                       LevenshteinWeightTable weights = {1, 1, 1}, double score_cutoff = 1.0,
                                       double score_hint = 1.0)
{
    return detail::Levenshtein::normalized_distance(s1, s2, weights, score_cutoff, score_hint);
}

/**
 * @brief Calculates a normalized levenshtein distance using custom
 * costs for insertion, deletion and substitution.
 *
 * @tparam Sentence1 This is a string that can be converted to
 * basic_string_view<char_type>
 * @tparam Sentence2 This is a string that can be converted to
 * basic_string_view<char_type>
 *
 * @param s1
 *   string to compare with s2 (for type info check Template parameters above)
 * @param s2
 *   string to compare with s1 (for type info check Template parameters above)
 * @param weights
 *   The weights for the three operations in the form
 *   (insertion, deletion, substitution). Default is {1, 1, 1},
 *   which gives all three operations a weight of 1.
 * @param score_cutoff
 *   Optional argument for a score threshold as a float between 0 and 1.0.
 *   For ratio < score_cutoff 0 is returned instead. Default is 0,
 *   which deactivates this behaviour.
 *
 * @return Normalized weighted levenshtein distance between s1 and s2
 *   as a double between 0 and 1.0
 *
 * @see levenshtein()
 *
 * @remarks
 * @parblock
 * The normalization of the Levenshtein distance is performed in the following way:
 *
 * \f{align*}{
 *   ratio &= \frac{distance(s1, s2)}{max_dist}
 * \f}
 * @endparblock
 *
 *
 * @par Examples
 * @parblock
 * Find the normalized Levenshtein distance between two strings:
 * @code{.cpp}
 * // ratio is 81.81818181818181
 * double ratio = normalized_levenshtein("lewenstein", "levenshtein");
 * @endcode
 *
 * Setting a score_cutoff allows the implementation to select
 * a more efficient implementation:
 * @code{.cpp}
 * // ratio is 0.0
 * double ratio = normalized_levenshtein("lewenstein", "levenshtein", {1, 1, 1}, 85.0);
 * @endcode
 *
 * It is possible to select different weights by passing a `weight` struct
 * @code{.cpp}
 * // ratio is 85.71428571428571
 * double ratio = normalized_levenshtein("lewenstein", "levenshtein", {1, 1, 2});
 * @endcode
 * @endparblock
 */
template <typename InputIt1, typename InputIt2>
double levenshtein_normalized_similarity(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2,
                                         LevenshteinWeightTable weights = {1, 1, 1},
                                         double score_cutoff = 0.0, double score_hint = 0.0)
{
    return detail::Levenshtein::normalized_similarity(first1, last1, first2, last2, weights, score_cutoff,
                                                      score_hint);
}

template <typename Sentence1, typename Sentence2>
double levenshtein_normalized_similarity(const Sentence1& s1, const Sentence2& s2,
                                         LevenshteinWeightTable weights = {1, 1, 1},
                                         double score_cutoff = 0.0, double score_hint = 0.0)
{
    return detail::Levenshtein::normalized_similarity(s1, s2, weights, score_cutoff, score_hint);
}

/**
 * @brief Return list of EditOp describing how to turn s1 into s2.
 *
 * @tparam Sentence1 This is a string that can be converted to
 * basic_string_view<char_type>
 * @tparam Sentence2 This is a string that can be converted to
 * basic_string_view<char_type>
 *
 * @param s1
 *   string to compare with s2 (for type info check Template parameters above)
 * @param s2
 *   string to compare with s1 (for type info check Template parameters above)
 *
 * @return Edit operations required to turn s1 into s2
 */
template <typename InputIt1, typename InputIt2>
Editops levenshtein_editops(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2,
                            int64_t score_hint = std::numeric_limits<int64_t>::max())
{
    return detail::levenshtein_editops(detail::Range(first1, last1), detail::Range(first2, last2),
                                       score_hint);
}

template <typename Sentence1, typename Sentence2>
Editops levenshtein_editops(const Sentence1& s1, const Sentence2& s2,
                            int64_t score_hint = std::numeric_limits<int64_t>::max())
{
    return detail::levenshtein_editops(detail::Range(s1), detail::Range(s2), score_hint);
}

#ifdef RAPIDFUZZ_SIMD
namespace experimental {
template <int MaxLen>
struct MultiLevenshtein : public detail::MultiDistanceBase<MultiLevenshtein<MaxLen>, int64_t, 0,
                                                           std::numeric_limits<int64_t>::max()> {
private:
    friend detail::MultiDistanceBase<MultiLevenshtein<MaxLen>, int64_t, 0,
                                     std::numeric_limits<int64_t>::max()>;
    friend detail::MultiNormalizedMetricBase<MultiLevenshtein<MaxLen>>;

    constexpr static size_t get_vec_size()
    {
#    ifdef RAPIDFUZZ_AVX2
        using namespace detail::simd_avx2;
#    else
        using namespace detail::simd_sse2;
#    endif
        if constexpr (MaxLen <= 8)
            return native_simd<uint8_t>::size();
        else if constexpr (MaxLen <= 16)
            return native_simd<uint16_t>::size();
        else if constexpr (MaxLen <= 32)
            return native_simd<uint32_t>::size();
        else if constexpr (MaxLen <= 64)
            return native_simd<uint64_t>::size();

        static_assert(MaxLen <= 64);
    }

    constexpr static size_t find_block_count(size_t count)
    {
        size_t vec_size = get_vec_size();
        size_t simd_vec_count = detail::ceil_div(count, vec_size);
        return detail::ceil_div(simd_vec_count * vec_size * MaxLen, 64);
    }

public:
    MultiLevenshtein(size_t count, LevenshteinWeightTable aWeights = {1, 1, 1})
        : input_count(count), PM(find_block_count(count) * 64), weights(aWeights)
    {
        str_lens.resize(result_count());
        if (weights.delete_cost != 1 || weights.insert_cost != 1 || weights.replace_cost > 2)
            throw std::invalid_argument("unsupported weights");
    }

    /**
     * @brief get minimum size required for result vectors passed into
     * - distance
     * - similarity
     * - normalized_distance
     * - normalized_similarity
     *
     * @return minimum vector size
     */
    size_t result_count() const
    {
        size_t vec_size = get_vec_size();
        size_t simd_vec_count = detail::ceil_div(input_count, vec_size);
        return simd_vec_count * vec_size;
    }

    template <typename Sentence1>
    void insert(const Sentence1& s1_)
    {
        insert(detail::to_begin(s1_), detail::to_end(s1_));
    }

    template <typename InputIt1>
    void insert(InputIt1 first1, InputIt1 last1)
    {
        auto len = std::distance(first1, last1);
        int block_pos = static_cast<int>((pos * MaxLen) % 64);
        auto block = (pos * MaxLen) / 64;
        assert(len <= MaxLen);

        if (pos >= input_count) throw std::invalid_argument("out of bounds insert");

        str_lens[pos] = static_cast<size_t>(len);
        for (; first1 != last1; ++first1) {
            PM.insert(block, *first1, block_pos);
            block_pos++;
        }
        pos++;
    }

private:
    template <typename InputIt2>
    void _distance(int64_t* scores, size_t score_count, detail::Range<InputIt2> s2,
                   int64_t score_cutoff = std::numeric_limits<int64_t>::max()) const
    {
        if (score_count < result_count())
            throw std::invalid_argument("scores has to have >= result_count() elements");

        detail::Range scores_(scores, scores + score_count);
        if constexpr (MaxLen == 8)
            detail::levenshtein_hyrroe2003_simd<uint8_t>(scores_, PM, str_lens, s2, score_cutoff);
        else if constexpr (MaxLen == 16)
            detail::levenshtein_hyrroe2003_simd<uint16_t>(scores_, PM, str_lens, s2, score_cutoff);
        else if constexpr (MaxLen == 32)
            detail::levenshtein_hyrroe2003_simd<uint32_t>(scores_, PM, str_lens, s2, score_cutoff);
        else if constexpr (MaxLen == 64)
            detail::levenshtein_hyrroe2003_simd<uint64_t>(scores_, PM, str_lens, s2, score_cutoff);
    }

    template <typename InputIt2>
    int64_t maximum(size_t s1_idx, detail::Range<InputIt2> s2) const
    {
        return detail::levenshtein_maximum(static_cast<ptrdiff_t>(str_lens[s1_idx]), s2.size(), weights);
    }

    size_t get_input_count() const noexcept
    {
        return input_count;
    }

    size_t input_count;
    size_t pos = 0;
    detail::BlockPatternMatchVector PM;
    std::vector<size_t> str_lens;
    LevenshteinWeightTable weights;
};
} /* namespace experimental */
#endif

template <typename CharT1>
struct CachedLevenshtein : public detail::CachedDistanceBase<CachedLevenshtein<CharT1>, int64_t, 0,
                                                             std::numeric_limits<int64_t>::max()> {
    template <typename Sentence1>
    explicit CachedLevenshtein(const Sentence1& s1_, LevenshteinWeightTable aWeights = {1, 1, 1})
        : CachedLevenshtein(detail::to_begin(s1_), detail::to_end(s1_), aWeights)
    {}

    template <typename InputIt1>
    CachedLevenshtein(InputIt1 first1, InputIt1 last1, LevenshteinWeightTable aWeights = {1, 1, 1})
        : s1(first1, last1), PM(detail::Range(first1, last1)), weights(aWeights)
    {}

private:
    friend detail::CachedDistanceBase<CachedLevenshtein<CharT1>, int64_t, 0,
                                      std::numeric_limits<int64_t>::max()>;
    friend detail::CachedNormalizedMetricBase<CachedLevenshtein<CharT1>>;

    template <typename InputIt2>
    int64_t maximum(detail::Range<InputIt2> s2) const
    {
        return detail::levenshtein_maximum(static_cast<ptrdiff_t>(s1.size()), s2.size(), weights);
    }

    template <typename InputIt2>
    int64_t _distance(detail::Range<InputIt2> s2, int64_t score_cutoff, int64_t score_hint) const
    {
        if (weights.insert_cost == weights.delete_cost) {
            /* when insertions + deletions operations are free there can not be any edit distance */
            if (weights.insert_cost == 0) return 0;

            /* uniform Levenshtein multiplied with the common factor */
            if (weights.insert_cost == weights.replace_cost) {
                // max can make use of the common divisor of the three weights
                int64_t new_score_cutoff = detail::ceil_div(score_cutoff, weights.insert_cost);
                int64_t new_score_hint = detail::ceil_div(score_hint, weights.insert_cost);
                int64_t dist = detail::uniform_levenshtein_distance(PM, detail::Range(s1), s2,
                                                                    new_score_cutoff, new_score_hint);
                dist *= weights.insert_cost;

                return (dist <= score_cutoff) ? dist : score_cutoff + 1;
            }
            /*
             * when replace_cost >= insert_cost + delete_cost no substitutions are performed
             * therefore this can be implemented as InDel distance multiplied with the common factor
             */
            else if (weights.replace_cost >= weights.insert_cost + weights.delete_cost) {
                // max can make use of the common divisor of the three weights
                int64_t new_max = detail::ceil_div(score_cutoff, weights.insert_cost);
                int64_t dist = detail::indel_distance(PM, detail::Range(s1), s2, new_max);
                dist *= weights.insert_cost;
                return (dist <= score_cutoff) ? dist : score_cutoff + 1;
            }
        }

        return detail::generalized_levenshtein_distance(detail::Range(s1), s2, weights, score_cutoff);
    }

    std::basic_string<CharT1> s1;
    detail::BlockPatternMatchVector PM;
    LevenshteinWeightTable weights;
};

template <typename Sentence1>
explicit CachedLevenshtein(const Sentence1& s1_, LevenshteinWeightTable aWeights = {1, 1, 1})
    -> CachedLevenshtein<char_type<Sentence1>>;

template <typename InputIt1>
CachedLevenshtein(InputIt1 first1, InputIt1 last1, LevenshteinWeightTable aWeights = {1, 1, 1})
    -> CachedLevenshtein<iter_value_t<InputIt1>>;

} // namespace rapidfuzz

#include <cmath>
#include <numeric>

#include <stdexcept>

namespace rapidfuzz::detail {

/**
 * @brief Bitparallel implementation of the OSA distance.
 *
 * This implementation requires the first string to have a length <= 64.
 * The algorithm used is described @cite hyrro_2002 and has a time complexity
 * of O(N). Comments and variable names in the implementation follow the
 * paper. This implementation is used internally when the strings are short enough
 *
 * @tparam CharT1 This is the char type of the first sentence
 * @tparam CharT2 This is the char type of the second sentence
 *
 * @param s1
 *   string to compare with s2 (for type info check Template parameters above)
 * @param s2
 *   string to compare with s1 (for type info check Template parameters above)
 *
 * @return returns the OSA distance between s1 and s2
 */
template <typename PM_Vec, typename InputIt1, typename InputIt2>
int64_t osa_hyrroe2003(const PM_Vec& PM, Range<InputIt1> s1, Range<InputIt2> s2, int64_t max)
{
    /* VP is set to 1^m. Shifting by bitwidth would be undefined behavior */
    uint64_t VP = ~UINT64_C(0);
    uint64_t VN = 0;
    uint64_t D0 = 0;
    uint64_t PM_j_old = 0;
    int64_t currDist = s1.size();
    assert(s1.size() != 0);

    /* mask used when computing D[m,j] in the paper 10^(m-1) */
    uint64_t mask = UINT64_C(1) << (s1.size() - 1);

    /* Searching */
    for (const auto& ch : s2) {
        /* Step 1: Computing D0 */
        uint64_t PM_j = PM.get(0, ch);
        uint64_t TR = (((~D0) & PM_j) << 1) & PM_j_old;
        D0 = (((PM_j & VP) + VP) ^ VP) | PM_j | VN;
        D0 = D0 | TR;

        /* Step 2: Computing HP and HN */
        uint64_t HP = VN | ~(D0 | VP);
        uint64_t HN = D0 & VP;

        /* Step 3: Computing the value D[m,j] */
        currDist += bool(HP & mask);
        currDist -= bool(HN & mask);

        /* Step 4: Computing Vp and VN */
        HP = (HP << 1) | 1;
        HN = (HN << 1);

        VP = HN | ~(D0 | HP);
        VN = HP & D0;
        PM_j_old = PM_j;
    }

    return (currDist <= max) ? currDist : max + 1;
}

#ifdef RAPIDFUZZ_SIMD
template <typename VecType, typename InputIt, int _lto_hack = RAPIDFUZZ_LTO_HACK>
void osa_hyrroe2003_simd(Range<int64_t*> scores, const detail::BlockPatternMatchVector& block,
                         const std::vector<size_t>& s1_lengths, Range<InputIt> s2,
                         int64_t score_cutoff) noexcept
{
#    ifdef RAPIDFUZZ_AVX2
    using namespace simd_avx2;
#    else
    using namespace simd_sse2;
#    endif
    static constexpr size_t vec_width = native_simd<VecType>::size();
    static constexpr size_t vecs = static_cast<size_t>(native_simd<uint64_t>::size());
    assert(block.size() % vecs == 0);

    native_simd<VecType> zero(VecType(0));
    native_simd<VecType> one(1);
    size_t result_index = 0;

    for (size_t cur_vec = 0; cur_vec < block.size(); cur_vec += vecs) {
        /* VP is set to 1^m */
        native_simd<VecType> VP(static_cast<VecType>(-1));
        native_simd<VecType> VN(VecType(0));
        native_simd<VecType> D0(VecType(0));
        native_simd<VecType> PM_j_old(VecType(0));

        alignas(32) std::array<VecType, vec_width> currDist_;
        unroll<int, vec_width>(
            [&](auto i) { currDist_[i] = static_cast<VecType>(s1_lengths[result_index + i]); });
        native_simd<VecType> currDist(reinterpret_cast<uint64_t*>(currDist_.data()));
        /* mask used when computing D[m,j] in the paper 10^(m-1) */
        alignas(32) std::array<VecType, vec_width> mask_;
        unroll<int, vec_width>([&](auto i) {
            if (s1_lengths[result_index + i] == 0)
                mask_[i] = 0;
            else
                mask_[i] = static_cast<VecType>(UINT64_C(1) << (s1_lengths[result_index + i] - 1));
        });
        native_simd<VecType> mask(reinterpret_cast<uint64_t*>(mask_.data()));

        for (const auto& ch : s2) {
            /* Step 1: Computing D0 */
            alignas(32) std::array<uint64_t, vecs> stored;
            unroll<int, vecs>([&](auto i) { stored[i] = block.get(cur_vec + i, ch); });

            native_simd<VecType> PM_j(stored.data());
            auto TR = (andnot(PM_j, D0) << 1) & PM_j_old;
            D0 = (((PM_j & VP) + VP) ^ VP) | PM_j | VN;
            D0 = D0 | TR;

            /* Step 2: Computing HP and HN */
            auto HP = VN | ~(D0 | VP);
            auto HN = D0 & VP;

            /* Step 3: Computing the value D[m,j] */
            currDist += andnot(one, (HP & mask) == zero);
            currDist -= andnot(one, (HN & mask) == zero);

            /* Step 4: Computing Vp and VN */
            HP = (HP << 1) | one;
            HN = (HN << 1);

            VP = HN | ~(D0 | HP);
            VN = HP & D0;
            PM_j_old = PM_j;
        }

        alignas(32) std::array<VecType, vec_width> distances;
        currDist.store(distances.data());

        unroll<int, vec_width>([&](auto i) {
            int64_t score = 0;
            /* strings of length 0 are not handled correctly */
            if (s1_lengths[result_index] == 0) {
                score = s2.size();
            }
            /* calculate score under consideration of wraparounds in parallel counter */
            else {
                if constexpr (!std::is_same_v<VecType, uint64_t>) {
                    ptrdiff_t min_dist =
                        std::abs(static_cast<ptrdiff_t>(s1_lengths[result_index]) - s2.size());
                    int64_t wraparound_score = static_cast<int64_t>(std::numeric_limits<VecType>::max()) + 1;

                    score = (min_dist / wraparound_score) * wraparound_score;
                    VecType remainder = static_cast<VecType>(min_dist % wraparound_score);

                    if (distances[i] < remainder) score += wraparound_score;
                }

                score += static_cast<int64_t>(distances[i]);
            }
            scores[static_cast<int64_t>(result_index)] = (score <= score_cutoff) ? score : score_cutoff + 1;
            result_index++;
        });
    }
}
#endif

template <typename InputIt1, typename InputIt2>
int64_t osa_hyrroe2003_block(const BlockPatternMatchVector& PM, Range<InputIt1> s1, Range<InputIt2> s2,
                             int64_t max = std::numeric_limits<int64_t>::max())
{
    struct Row {
        uint64_t VP;
        uint64_t VN;
        uint64_t D0;
        uint64_t PM;

        Row() : VP(~UINT64_C(0)), VN(0), D0(0), PM(0)
        {}
    };

    ptrdiff_t word_size = sizeof(uint64_t) * 8;
    auto words = PM.size();
    uint64_t Last = UINT64_C(1) << ((s1.size() - 1) % word_size);

    int64_t currDist = s1.size();
    std::vector<Row> old_vecs(words + 1);
    std::vector<Row> new_vecs(words + 1);

    /* Searching */
    for (ptrdiff_t row = 0; row < s2.size(); ++row) {
        uint64_t HP_carry = 1;
        uint64_t HN_carry = 0;

        for (size_t word = 0; word < words; word++) {
            /* retrieve bit vectors from last iterations */
            uint64_t VN = old_vecs[word + 1].VN;
            uint64_t VP = old_vecs[word + 1].VP;
            uint64_t D0 = old_vecs[word + 1].D0;
            /* D0 last word */
            uint64_t D0_last = old_vecs[word].D0;

            /* PM of last char same word */
            uint64_t PM_j_old = old_vecs[word + 1].PM;
            /* PM of last word */
            uint64_t PM_last = new_vecs[word].PM;

            uint64_t PM_j = PM.get(word, s2[row]);
            uint64_t X = PM_j;
            uint64_t TR = ((((~D0) & X) << 1) | (((~D0_last) & PM_last) >> 63)) & PM_j_old;

            X |= HN_carry;
            D0 = (((X & VP) + VP) ^ VP) | X | VN | TR;

            uint64_t HP = VN | ~(D0 | VP);
            uint64_t HN = D0 & VP;

            if (word == words - 1) {
                currDist += bool(HP & Last);
                currDist -= bool(HN & Last);
            }

            uint64_t HP_carry_temp = HP_carry;
            HP_carry = HP >> 63;
            HP = (HP << 1) | HP_carry_temp;
            uint64_t HN_carry_temp = HN_carry;
            HN_carry = HN >> 63;
            HN = (HN << 1) | HN_carry_temp;

            new_vecs[word + 1].VP = HN | ~(D0 | HP);
            new_vecs[word + 1].VN = HP & D0;
            new_vecs[word + 1].D0 = D0;
            new_vecs[word + 1].PM = PM_j;
        }

        std::swap(new_vecs, old_vecs);
    }

    return (currDist <= max) ? currDist : max + 1;
}

class OSA : public DistanceBase<OSA, int64_t, 0, std::numeric_limits<int64_t>::max()> {
    friend DistanceBase<OSA, int64_t, 0, std::numeric_limits<int64_t>::max()>;
    friend NormalizedMetricBase<OSA>;

    template <typename InputIt1, typename InputIt2>
    static int64_t maximum(Range<InputIt1> s1, Range<InputIt2> s2)
    {
        return std::max(s1.size(), s2.size());
    }

    template <typename InputIt1, typename InputIt2>
    static int64_t _distance(Range<InputIt1> s1, Range<InputIt2> s2, int64_t score_cutoff, int64_t score_hint)
    {
        if (s2.size() < s1.size()) return _distance(s2, s1, score_cutoff, score_hint);

        remove_common_affix(s1, s2);
        if (s1.empty())
            return (s2.size() <= score_cutoff) ? s2.size() : score_cutoff + 1;
        else if (s1.size() < 64)
            return osa_hyrroe2003(PatternMatchVector(s1), s1, s2, score_cutoff);
        else
            return osa_hyrroe2003_block(BlockPatternMatchVector(s1), s1, s2, score_cutoff);
    }
};

} // namespace rapidfuzz::detail

namespace rapidfuzz {

/**
 * @brief Calculates the optimal string alignment (OSA) distance between two strings.
 *
 * @details
 * Both strings require a similar length
 *
 *
 * @tparam Sentence1 This is a string that can be converted to
 * basic_string_view<char_type>
 * @tparam Sentence2 This is a string that can be converted to
 * basic_string_view<char_type>
 *
 * @param s1
 *   string to compare with s2 (for type info check Template parameters above)
 * @param s2
 *   string to compare with s1 (for type info check Template parameters above)
 * @param max
 *   Maximum OSA distance between s1 and s2, that is
 *   considered as a result. If the distance is bigger than max,
 *   max + 1 is returned instead. Default is std::numeric_limits<size_t>::max(),
 *   which deactivates this behaviour.
 *
 * @return OSA distance between s1 and s2
 */
template <typename InputIt1, typename InputIt2>
int64_t osa_distance(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2,
                     int64_t score_cutoff = std::numeric_limits<int64_t>::max())
{
    return detail::OSA::distance(first1, last1, first2, last2, score_cutoff, score_cutoff);
}

template <typename Sentence1, typename Sentence2>
int64_t osa_distance(const Sentence1& s1, const Sentence2& s2,
                     int64_t score_cutoff = std::numeric_limits<int64_t>::max())
{
    return detail::OSA::distance(s1, s2, score_cutoff, score_cutoff);
}

template <typename InputIt1, typename InputIt2>
int64_t osa_similarity(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2,
                       int64_t score_cutoff = 0)
{
    return detail::OSA::similarity(first1, last1, first2, last2, score_cutoff, score_cutoff);
}

template <typename Sentence1, typename Sentence2>
int64_t osa_similarity(const Sentence1& s1, const Sentence2& s2, int64_t score_cutoff = 0)
{
    return detail::OSA::similarity(s1, s2, score_cutoff, score_cutoff);
}

template <typename InputIt1, typename InputIt2>
double osa_normalized_distance(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2,
                               double score_cutoff = 1.0)
{
    return detail::OSA::normalized_distance(first1, last1, first2, last2, score_cutoff, score_cutoff);
}

template <typename Sentence1, typename Sentence2>
double osa_normalized_distance(const Sentence1& s1, const Sentence2& s2, double score_cutoff = 1.0)
{
    return detail::OSA::normalized_distance(s1, s2, score_cutoff, score_cutoff);
}

/**
 * @brief Calculates a normalized hamming similarity
 *
 * @details
 * Both string require a similar length
 *
 *
 * @tparam Sentence1 This is a string that can be converted to
 * basic_string_view<char_type>
 * @tparam Sentence2 This is a string that can be converted to
 * basic_string_view<char_type>
 *
 * @param s1
 *   string to compare with s2 (for type info check Template parameters above)
 * @param s2
 *   string to compare with s1 (for type info check Template parameters above)
 * @param score_cutoff
 *   Optional argument for a score threshold as a float between 0 and 1.0.
 *   For ratio < score_cutoff 0 is returned instead. Default is 0,
 *   which deactivates this behaviour.
 *
 * @return Normalized hamming distance between s1 and s2
 *   as a float between 0 and 1.0
 */
template <typename InputIt1, typename InputIt2>
double osa_normalized_similarity(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2,
                                 double score_cutoff = 0.0)
{
    return detail::OSA::normalized_similarity(first1, last1, first2, last2, score_cutoff, score_cutoff);
}

template <typename Sentence1, typename Sentence2>
double osa_normalized_similarity(const Sentence1& s1, const Sentence2& s2, double score_cutoff = 0.0)
{
    return detail::OSA::normalized_similarity(s1, s2, score_cutoff, score_cutoff);
}

#ifdef RAPIDFUZZ_SIMD
namespace experimental {
template <int MaxLen>
struct MultiOSA
    : public detail::MultiDistanceBase<MultiOSA<MaxLen>, int64_t, 0, std::numeric_limits<int64_t>::max()> {
private:
    friend detail::MultiDistanceBase<MultiOSA<MaxLen>, int64_t, 0, std::numeric_limits<int64_t>::max()>;
    friend detail::MultiNormalizedMetricBase<MultiOSA<MaxLen>>;

    constexpr static size_t get_vec_size()
    {
#    ifdef RAPIDFUZZ_AVX2
        using namespace detail::simd_avx2;
#    else
        using namespace detail::simd_sse2;
#    endif
        if constexpr (MaxLen <= 8)
            return native_simd<uint8_t>::size();
        else if constexpr (MaxLen <= 16)
            return native_simd<uint16_t>::size();
        else if constexpr (MaxLen <= 32)
            return native_simd<uint32_t>::size();
        else if constexpr (MaxLen <= 64)
            return native_simd<uint64_t>::size();

        static_assert(MaxLen <= 64);
    }

    constexpr static size_t find_block_count(size_t count)
    {
        size_t vec_size = get_vec_size();
        size_t simd_vec_count = detail::ceil_div(count, vec_size);
        return detail::ceil_div(simd_vec_count * vec_size * MaxLen, 64);
    }

public:
    MultiOSA(size_t count) : input_count(count), PM(find_block_count(count) * 64)
    {
        str_lens.resize(result_count());
    }

    /**
     * @brief get minimum size required for result vectors passed into
     * - distance
     * - similarity
     * - normalized_distance
     * - normalized_similarity
     *
     * @return minimum vector size
     */
    size_t result_count() const
    {
        size_t vec_size = get_vec_size();
        size_t simd_vec_count = detail::ceil_div(input_count, vec_size);
        return simd_vec_count * vec_size;
    }

    template <typename Sentence1>
    void insert(const Sentence1& s1_)
    {
        insert(detail::to_begin(s1_), detail::to_end(s1_));
    }

    template <typename InputIt1>
    void insert(InputIt1 first1, InputIt1 last1)
    {
        auto len = std::distance(first1, last1);
        int block_pos = static_cast<int>((pos * MaxLen) % 64);
        auto block = (pos * MaxLen) / 64;
        assert(len <= MaxLen);

        if (pos >= input_count) throw std::invalid_argument("out of bounds insert");

        str_lens[pos] = static_cast<size_t>(len);
        for (; first1 != last1; ++first1) {
            PM.insert(block, *first1, block_pos);
            block_pos++;
        }
        pos++;
    }

private:
    template <typename InputIt2>
    void _distance(int64_t* scores, size_t score_count, detail::Range<InputIt2> s2,
                   int64_t score_cutoff = std::numeric_limits<int64_t>::max()) const
    {
        if (score_count < result_count())
            throw std::invalid_argument("scores has to have >= result_count() elements");

        detail::Range scores_(scores, scores + score_count);
        if constexpr (MaxLen == 8)
            detail::osa_hyrroe2003_simd<uint8_t>(scores_, PM, str_lens, s2, score_cutoff);
        else if constexpr (MaxLen == 16)
            detail::osa_hyrroe2003_simd<uint16_t>(scores_, PM, str_lens, s2, score_cutoff);
        else if constexpr (MaxLen == 32)
            detail::osa_hyrroe2003_simd<uint32_t>(scores_, PM, str_lens, s2, score_cutoff);
        else if constexpr (MaxLen == 64)
            detail::osa_hyrroe2003_simd<uint64_t>(scores_, PM, str_lens, s2, score_cutoff);
    }

    template <typename InputIt2>
    int64_t maximum(size_t s1_idx, detail::Range<InputIt2> s2) const
    {
        return std::max(static_cast<ptrdiff_t>(str_lens[s1_idx]), s2.size());
    }

    size_t get_input_count() const noexcept
    {
        return input_count;
    }

    size_t input_count;
    size_t pos = 0;
    detail::BlockPatternMatchVector PM;
    std::vector<size_t> str_lens;
};
} /* namespace experimental */
#endif

template <typename CharT1>
struct CachedOSA
    : public detail::CachedDistanceBase<CachedOSA<CharT1>, int64_t, 0, std::numeric_limits<int64_t>::max()> {
    template <typename Sentence1>
    explicit CachedOSA(const Sentence1& s1_) : CachedOSA(detail::to_begin(s1_), detail::to_end(s1_))
    {}

    template <typename InputIt1>
    CachedOSA(InputIt1 first1, InputIt1 last1) : s1(first1, last1), PM(detail::Range(first1, last1))
    {}

private:
    friend detail::CachedDistanceBase<CachedOSA<CharT1>, int64_t, 0, std::numeric_limits<int64_t>::max()>;
    friend detail::CachedNormalizedMetricBase<CachedOSA<CharT1>>;

    template <typename InputIt2>
    int64_t maximum(detail::Range<InputIt2> s2) const
    {
        return std::max(static_cast<ptrdiff_t>(s1.size()), s2.size());
    }

    template <typename InputIt2>
    int64_t _distance(detail::Range<InputIt2> s2, int64_t score_cutoff,
                      [[maybe_unused]] int64_t score_hint) const
    {
        int64_t res;
        if (s1.empty())
            res = s2.size();
        else if (s2.empty())
            res = static_cast<int64_t>(s1.size());
        else if (s1.size() < 64)
            res = detail::osa_hyrroe2003(PM, detail::Range(s1), s2, score_cutoff);
        else
            res = detail::osa_hyrroe2003_block(PM, detail::Range(s1), s2, score_cutoff);

        return (res <= score_cutoff) ? res : score_cutoff + 1;
    }

    std::basic_string<CharT1> s1;
    detail::BlockPatternMatchVector PM;
};

template <typename Sentence1>
CachedOSA(const Sentence1& s1_) -> CachedOSA<char_type<Sentence1>>;

template <typename InputIt1>
CachedOSA(InputIt1 first1, InputIt1 last1) -> CachedOSA<iter_value_t<InputIt1>>;
/**@}*/

} // namespace rapidfuzz

#include <cmath>
#include <numeric>


namespace rapidfuzz::detail {

class Postfix : public SimilarityBase<Postfix, int64_t, 0, std::numeric_limits<int64_t>::max()> {
    friend SimilarityBase<Postfix, int64_t, 0, std::numeric_limits<int64_t>::max()>;
    friend NormalizedMetricBase<Postfix>;

    template <typename InputIt1, typename InputIt2>
    static int64_t maximum(Range<InputIt1> s1, Range<InputIt2> s2)
    {
        return std::max(s1.size(), s2.size());
    }

    template <typename InputIt1, typename InputIt2>
    static int64_t _similarity(Range<InputIt1> s1, Range<InputIt2> s2, int64_t score_cutoff,
                               [[maybe_unused]] int64_t score_hint)
    {
        int64_t dist = static_cast<int64_t>(remove_common_suffix(s1, s2));
        return (dist >= score_cutoff) ? dist : 0;
    }
};

} // namespace rapidfuzz::detail

namespace rapidfuzz {

template <typename InputIt1, typename InputIt2>
int64_t postfix_distance(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2,
                         int64_t score_cutoff = std::numeric_limits<int64_t>::max())
{
    return detail::Postfix::distance(first1, last1, first2, last2, score_cutoff, score_cutoff);
}

template <typename Sentence1, typename Sentence2>
int64_t postfix_distance(const Sentence1& s1, const Sentence2& s2,
                         int64_t score_cutoff = std::numeric_limits<int64_t>::max())
{
    return detail::Postfix::distance(s1, s2, score_cutoff, score_cutoff);
}

template <typename InputIt1, typename InputIt2>
int64_t postfix_similarity(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2,
                           int64_t score_cutoff = 0)
{
    return detail::Postfix::similarity(first1, last1, first2, last2, score_cutoff, score_cutoff);
}

template <typename Sentence1, typename Sentence2>
int64_t postfix_similarity(const Sentence1& s1, const Sentence2& s2, int64_t score_cutoff = 0)
{
    return detail::Postfix::similarity(s1, s2, score_cutoff, score_cutoff);
}

template <typename InputIt1, typename InputIt2>
double postfix_normalized_distance(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2,
                                   double score_cutoff = 1.0)
{
    return detail::Postfix::normalized_distance(first1, last1, first2, last2, score_cutoff, score_cutoff);
}

template <typename Sentence1, typename Sentence2>
double postfix_normalized_distance(const Sentence1& s1, const Sentence2& s2, double score_cutoff = 1.0)
{
    return detail::Postfix::normalized_distance(s1, s2, score_cutoff, score_cutoff);
}

template <typename InputIt1, typename InputIt2>
double postfix_normalized_similarity(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2,
                                     double score_cutoff = 0.0)
{
    return detail::Postfix::normalized_similarity(first1, last1, first2, last2, score_cutoff, score_cutoff);
}

template <typename Sentence1, typename Sentence2>
double postfix_normalized_similarity(const Sentence1& s1, const Sentence2& s2, double score_cutoff = 0.0)
{
    return detail::Postfix::normalized_similarity(s1, s2, score_cutoff, score_cutoff);
}

template <typename CharT1>
struct CachedPostfix : public detail::CachedSimilarityBase<CachedPostfix<CharT1>, int64_t, 0,
                                                           std::numeric_limits<int64_t>::max()> {
    template <typename Sentence1>
    explicit CachedPostfix(const Sentence1& s1_) : CachedPostfix(detail::to_begin(s1_), detail::to_end(s1_))
    {}

    template <typename InputIt1>
    CachedPostfix(InputIt1 first1, InputIt1 last1) : s1(first1, last1)
    {}

private:
    friend detail::CachedSimilarityBase<CachedPostfix<CharT1>, int64_t, 0,
                                        std::numeric_limits<int64_t>::max()>;
    friend detail::CachedNormalizedMetricBase<CachedPostfix<CharT1>>;

    template <typename InputIt2>
    int64_t maximum(detail::Range<InputIt2> s2) const
    {
        return std::max(static_cast<ptrdiff_t>(s1.size()), s2.size());
    }

    template <typename InputIt2>
    int64_t _similarity(detail::Range<InputIt2> s2, int64_t score_cutoff,
                        [[maybe_unused]] int64_t score_hint) const
    {
        return detail::Postfix::similarity(s1, s2, score_cutoff, score_hint);
    }

    std::basic_string<CharT1> s1;
};

template <typename Sentence1>
explicit CachedPostfix(const Sentence1& s1_) -> CachedPostfix<char_type<Sentence1>>;

template <typename InputIt1>
CachedPostfix(InputIt1 first1, InputIt1 last1) -> CachedPostfix<iter_value_t<InputIt1>>;

/**@}*/

} // namespace rapidfuzz

#include <cmath>
#include <numeric>


namespace rapidfuzz::detail {

class Prefix : public SimilarityBase<Prefix, int64_t, 0, std::numeric_limits<int64_t>::max()> {
    friend SimilarityBase<Prefix, int64_t, 0, std::numeric_limits<int64_t>::max()>;
    friend NormalizedMetricBase<Prefix>;

    template <typename InputIt1, typename InputIt2>
    static int64_t maximum(Range<InputIt1> s1, Range<InputIt2> s2)
    {
        return std::max(s1.size(), s2.size());
    }

    template <typename InputIt1, typename InputIt2>
    static int64_t _similarity(Range<InputIt1> s1, Range<InputIt2> s2, int64_t score_cutoff,
                               [[maybe_unused]] int64_t score_hint)
    {
        int64_t dist = static_cast<int64_t>(remove_common_prefix(s1, s2));
        return (dist >= score_cutoff) ? dist : 0;
    }
};

} // namespace rapidfuzz::detail

namespace rapidfuzz {

template <typename InputIt1, typename InputIt2>
int64_t prefix_distance(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2,
                        int64_t score_cutoff = std::numeric_limits<int64_t>::max())
{
    return detail::Prefix::distance(first1, last1, first2, last2, score_cutoff, score_cutoff);
}

template <typename Sentence1, typename Sentence2>
int64_t prefix_distance(const Sentence1& s1, const Sentence2& s2,
                        int64_t score_cutoff = std::numeric_limits<int64_t>::max())
{
    return detail::Prefix::distance(s1, s2, score_cutoff, score_cutoff);
}

template <typename InputIt1, typename InputIt2>
int64_t prefix_similarity(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2,
                          int64_t score_cutoff = 0)
{
    return detail::Prefix::similarity(first1, last1, first2, last2, score_cutoff, score_cutoff);
}

template <typename Sentence1, typename Sentence2>
int64_t prefix_similarity(const Sentence1& s1, const Sentence2& s2, int64_t score_cutoff = 0)
{
    return detail::Prefix::similarity(s1, s2, score_cutoff, score_cutoff);
}

template <typename InputIt1, typename InputIt2>
double prefix_normalized_distance(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2,
                                  double score_cutoff = 1.0)
{
    return detail::Prefix::normalized_distance(first1, last1, first2, last2, score_cutoff, score_cutoff);
}

template <typename Sentence1, typename Sentence2>
double prefix_normalized_distance(const Sentence1& s1, const Sentence2& s2, double score_cutoff = 1.0)
{
    return detail::Prefix::normalized_distance(s1, s2, score_cutoff, score_cutoff);
}

template <typename InputIt1, typename InputIt2>
double prefix_normalized_similarity(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2,
                                    double score_cutoff = 0.0)
{
    return detail::Prefix::normalized_similarity(first1, last1, first2, last2, score_cutoff, score_cutoff);
}

template <typename Sentence1, typename Sentence2>
double prefix_normalized_similarity(const Sentence1& s1, const Sentence2& s2, double score_cutoff = 0.0)
{
    return detail::Prefix::normalized_similarity(s1, s2, score_cutoff, score_cutoff);
}

template <typename CharT1>
struct CachedPrefix : public detail::CachedSimilarityBase<CachedPrefix<CharT1>, int64_t, 0,
                                                          std::numeric_limits<int64_t>::max()> {
    template <typename Sentence1>
    explicit CachedPrefix(const Sentence1& s1_) : CachedPrefix(detail::to_begin(s1_), detail::to_end(s1_))
    {}

    template <typename InputIt1>
    CachedPrefix(InputIt1 first1, InputIt1 last1) : s1(first1, last1)
    {}

private:
    friend detail::CachedSimilarityBase<CachedPrefix<CharT1>, int64_t, 0,
                                        std::numeric_limits<int64_t>::max()>;
    friend detail::CachedNormalizedMetricBase<CachedPrefix<CharT1>>;

    template <typename InputIt2>
    int64_t maximum(detail::Range<InputIt2> s2) const
    {
        return std::max(static_cast<ptrdiff_t>(s1.size()), s2.size());
    }

    template <typename InputIt2>
    int64_t _similarity(detail::Range<InputIt2> s2, int64_t score_cutoff,
                        [[maybe_unused]] int64_t score_hint) const
    {
        return detail::Prefix::similarity(s1, s2, score_cutoff, score_cutoff);
    }

    std::basic_string<CharT1> s1;
};

template <typename Sentence1>
explicit CachedPrefix(const Sentence1& s1_) -> CachedPrefix<char_type<Sentence1>>;

template <typename InputIt1>
CachedPrefix(InputIt1 first1, InputIt1 last1) -> CachedPrefix<iter_value_t<InputIt1>>;

/**@}*/

} // namespace rapidfuzz

namespace rapidfuzz {

template <typename CharT, typename InputIt1, typename InputIt2>
std::basic_string<CharT> editops_apply(const Editops& ops, InputIt1 first1, InputIt1 last1, InputIt2 first2,
                                       InputIt2 last2)
{
    auto len1 = static_cast<size_t>(std::distance(first1, last1));
    auto len2 = static_cast<size_t>(std::distance(first2, last2));

    std::basic_string<CharT> res_str;
    res_str.resize(len1 + len2);
    size_t src_pos = 0;
    size_t dest_pos = 0;

    for (const auto& op : ops) {
        /* matches between last and current editop */
        while (src_pos < op.src_pos) {
            res_str[dest_pos] = static_cast<CharT>(first1[static_cast<ptrdiff_t>(src_pos)]);
            src_pos++;
            dest_pos++;
        }

        switch (op.type) {
        case EditType::None:
        case EditType::Replace:
            res_str[dest_pos] = static_cast<CharT>(first2[static_cast<ptrdiff_t>(op.dest_pos)]);
            src_pos++;
            dest_pos++;
            break;
        case EditType::Insert:
            res_str[dest_pos] = static_cast<CharT>(first2[static_cast<ptrdiff_t>(op.dest_pos)]);
            dest_pos++;
            break;
        case EditType::Delete: src_pos++; break;
        }
    }

    /* matches after the last editop */
    while (src_pos < len1) {
        res_str[dest_pos] = static_cast<CharT>(first1[static_cast<ptrdiff_t>(src_pos)]);
        src_pos++;
        dest_pos++;
    }

    res_str.resize(dest_pos);
    return res_str;
}

template <typename CharT, typename Sentence1, typename Sentence2>
std::basic_string<CharT> editops_apply(const Editops& ops, const Sentence1& s1, const Sentence2& s2)
{
    return editops_apply<CharT>(ops, detail::to_begin(s1), detail::to_end(s1), detail::to_begin(s2),
                                detail::to_end(s2));
}

template <typename CharT, typename InputIt1, typename InputIt2>
std::basic_string<CharT> opcodes_apply(const Opcodes& ops, InputIt1 first1, InputIt1 last1, InputIt2 first2,
                                       InputIt2 last2)
{
    auto len1 = static_cast<size_t>(std::distance(first1, last1));
    auto len2 = static_cast<size_t>(std::distance(first2, last2));

    std::basic_string<CharT> res_str;
    res_str.resize(len1 + len2);
    size_t dest_pos = 0;

    for (const auto& op : ops) {
        switch (op.type) {
        case EditType::None:
            for (auto i = op.src_begin; i < op.src_end; ++i) {
                res_str[dest_pos++] = static_cast<CharT>(first1[static_cast<ptrdiff_t>(i)]);
            }
            break;
        case EditType::Replace:
        case EditType::Insert:
            for (auto i = op.dest_begin; i < op.dest_end; ++i) {
                res_str[dest_pos++] = static_cast<CharT>(first2[static_cast<ptrdiff_t>(i)]);
            }
            break;
        case EditType::Delete: break;
        }
    }

    res_str.resize(dest_pos);
    return res_str;
}

template <typename CharT, typename Sentence1, typename Sentence2>
std::basic_string<CharT> opcodes_apply(const Opcodes& ops, const Sentence1& s1, const Sentence2& s2)
{
    return opcodes_apply<CharT>(ops, detail::to_begin(s1), detail::to_end(s1), detail::to_begin(s2),
                                detail::to_end(s2));
}

} // namespace rapidfuzz

#include <array>
#include <limits>
#include <stdint.h>
#include <stdio.h>
#include <type_traits>
#include <unordered_set>

namespace rapidfuzz::detail {

/*
 * taken from https://stackoverflow.com/a/17251989/11335032
 */
template <typename T, typename U>
bool CanTypeFitValue(const U value)
{
    const intmax_t botT = intmax_t(std::numeric_limits<T>::min());
    const intmax_t botU = intmax_t(std::numeric_limits<U>::min());
    const uintmax_t topT = uintmax_t(std::numeric_limits<T>::max());
    const uintmax_t topU = uintmax_t(std::numeric_limits<U>::max());
    return !((botT > botU && value < static_cast<U>(botT)) || (topT < topU && value > static_cast<U>(topT)));
}

template <typename CharT1, size_t size = sizeof(CharT1)>
struct CharSet;

template <typename CharT1>
struct CharSet<CharT1, 1> {
    using UCharT1 = typename std::make_unsigned<CharT1>::type;

    std::array<bool, std::numeric_limits<UCharT1>::max() + 1> m_val;

    CharSet() : m_val{}
    {}

    void insert(CharT1 ch)
    {
        m_val[UCharT1(ch)] = true;
    }

    template <typename CharT2>
    bool find(CharT2 ch) const
    {
        if (!CanTypeFitValue<CharT1>(ch)) return false;

        return m_val[UCharT1(ch)];
    }
};

template <typename CharT1, size_t size>
struct CharSet {
    std::unordered_set<CharT1> m_val;

    CharSet() : m_val{}
    {}

    void insert(CharT1 ch)
    {
        m_val.insert(ch);
    }

    template <typename CharT2>
    bool find(CharT2 ch) const
    {
        if (!CanTypeFitValue<CharT1>(ch)) return false;

        return m_val.find(CharT1(ch)) != m_val.end();
    }
};

} // namespace rapidfuzz::detail

#include <type_traits>

namespace rapidfuzz::fuzz {

/**
 * @defgroup Fuzz Fuzz
 * A collection of string matching algorithms from FuzzyWuzzy
 * @{
 */

/**
 * @brief calculates a simple ratio between two strings
 *
 * @details
 * @code{.cpp}
 * // score is 96.55
 * double score = ratio("this is a test", "this is a test!")
 * @endcode
 *
 * @tparam Sentence1 This is a string that can be converted to
 * basic_string_view<char_type>
 * @tparam Sentence2 This is a string that can be converted to
 * basic_string_view<char_type>
 *
 * @param s1 string to compare with s2 (for type info check Template parameters
 * above)
 * @param s2 string to compare with s1 (for type info check Template parameters
 * above)
 * @param score_cutoff Optional argument for a score threshold between 0% and
 * 100%. Matches with a lower score than this number will not be returned.
 * Defaults to 0.
 *
 * @return returns the ratio between s1 and s2 or 0 when ratio < score_cutoff
 */
template <typename InputIt1, typename InputIt2>
double ratio(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2, double score_cutoff = 0);

template <typename Sentence1, typename Sentence2>
double ratio(const Sentence1& s1, const Sentence2& s2, double score_cutoff = 0);

#ifdef RAPIDFUZZ_SIMD
namespace experimental {
template <int MaxLen>
struct MultiRatio {
public:
    MultiRatio(size_t count) : input_count(count), scorer(count)
    {}

    size_t result_count() const
    {
        return scorer.result_count();
    }

    template <typename Sentence1>
    void insert(const Sentence1& s1_)
    {
        insert(detail::to_begin(s1_), detail::to_end(s1_));
    }

    template <typename InputIt1>
    void insert(InputIt1 first1, InputIt1 last1)
    {
        scorer.insert(first1, last1);
    }

    template <typename InputIt2>
    void similarity(double* scores, size_t score_count, InputIt2 first2, InputIt2 last2,
                    double score_cutoff = 0.0) const
    {
        similarity(scores, score_count, detail::Range(first2, last2), score_cutoff);
    }

    template <typename Sentence2>
    void similarity(double* scores, size_t score_count, const Sentence2& s2, double score_cutoff = 0) const
    {
        scorer.normalized_similarity(scores, score_count, s2, score_cutoff / 100.0);

        for (size_t i = 0; i < input_count; ++i)
            scores[i] *= 100.0;
    }

private:
    size_t input_count;
    rapidfuzz::experimental::MultiIndel<MaxLen> scorer;
};
} /* namespace experimental */
#endif

// TODO documentation
template <typename CharT1>
struct CachedRatio {
    template <typename InputIt1>
    CachedRatio(InputIt1 first1, InputIt1 last1) : cached_indel(first1, last1)
    {}

    template <typename Sentence1>
    CachedRatio(const Sentence1& s1) : cached_indel(s1)
    {}

    template <typename InputIt2>
    double similarity(InputIt2 first2, InputIt2 last2, double score_cutoff = 0.0,
                      double score_hint = 0.0) const;

    template <typename Sentence2>
    double similarity(const Sentence2& s2, double score_cutoff = 0.0, double score_hint = 0.0) const;

    // private:
    CachedIndel<CharT1> cached_indel;
};

template <typename Sentence1>
CachedRatio(const Sentence1& s1) -> CachedRatio<char_type<Sentence1>>;

template <typename InputIt1>
CachedRatio(InputIt1 first1, InputIt1 last1) -> CachedRatio<iter_value_t<InputIt1>>;

template <typename InputIt1, typename InputIt2>
ScoreAlignment<double> partial_ratio_alignment(InputIt1 first1, InputIt1 last1, InputIt2 first2,
                                               InputIt2 last2, double score_cutoff = 0);

template <typename Sentence1, typename Sentence2>
ScoreAlignment<double> partial_ratio_alignment(const Sentence1& s1, const Sentence2& s2,
                                               double score_cutoff = 0);

/**
 * @brief calculates the fuzz::ratio of the optimal string alignment
 *
 * @details
 * test @cite hyrro_2004 @cite wagner_fischer_1974
 * @code{.cpp}
 * // score is 100
 * double score = partial_ratio("this is a test", "this is a test!")
 * @endcode
 *
 * @tparam Sentence1 This is a string that can be converted to
 * basic_string_view<char_type>
 * @tparam Sentence2 This is a string that can be converted to
 * basic_string_view<char_type>
 *
 * @param s1 string to compare with s2 (for type info check Template parameters
 * above)
 * @param s2 string to compare with s1 (for type info check Template parameters
 * above)
 * @param score_cutoff Optional argument for a score threshold between 0% and
 * 100%. Matches with a lower score than this number will not be returned.
 * Defaults to 0.
 *
 * @return returns the ratio between s1 and s2 or 0 when ratio < score_cutoff
 */
template <typename InputIt1, typename InputIt2>
double partial_ratio(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2,
                     double score_cutoff = 0);

template <typename Sentence1, typename Sentence2>
double partial_ratio(const Sentence1& s1, const Sentence2& s2, double score_cutoff = 0);

// todo add real implementation
template <typename CharT1>
struct CachedPartialRatio {
    template <typename>
    friend struct CachedWRatio;

    template <typename InputIt1>
    CachedPartialRatio(InputIt1 first1, InputIt1 last1);

    template <typename Sentence1>
    explicit CachedPartialRatio(const Sentence1& s1_)
        : CachedPartialRatio(detail::to_begin(s1_), detail::to_end(s1_))
    {}

    template <typename InputIt2>
    double similarity(InputIt2 first2, InputIt2 last2, double score_cutoff = 0.0,
                      double score_hint = 0.0) const;

    template <typename Sentence2>
    double similarity(const Sentence2& s2, double score_cutoff = 0.0, double score_hint = 0.0) const;

private:
    std::basic_string<CharT1> s1;
    rapidfuzz::detail::CharSet<CharT1> s1_char_set;
    CachedRatio<CharT1> cached_ratio;
};

template <typename Sentence1>
explicit CachedPartialRatio(const Sentence1& s1) -> CachedPartialRatio<char_type<Sentence1>>;

template <typename InputIt1>
CachedPartialRatio(InputIt1 first1, InputIt1 last1) -> CachedPartialRatio<iter_value_t<InputIt1>>;

/**
 * @brief Sorts the words in the strings and calculates the fuzz::ratio between
 * them
 *
 * @details
 * @code{.cpp}
 * // score is 100
 * double score = token_sort_ratio("fuzzy wuzzy was a bear", "wuzzy fuzzy was a
 * bear")
 * @endcode
 *
 * @tparam Sentence1 This is a string that can be converted to
 * basic_string_view<char_type>
 * @tparam Sentence2 This is a string that can be converted to
 * basic_string_view<char_type>
 *
 * @param s1 string to compare with s2 (for type info check Template parameters
 * above)
 * @param s2 string to compare with s1 (for type info check Template parameters
 * above)
 * @param score_cutoff Optional argument for a score threshold between 0% and
 * 100%. Matches with a lower score than this number will not be returned.
 * Defaults to 0.
 *
 * @return returns the ratio between s1 and s2 or 0 when ratio < score_cutoff
 */
template <typename InputIt1, typename InputIt2>
double token_sort_ratio(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2,
                        double score_cutoff = 0);

template <typename Sentence1, typename Sentence2>
double token_sort_ratio(const Sentence1& s1, const Sentence2& s2, double score_cutoff = 0);

#ifdef RAPIDFUZZ_SIMD
namespace experimental {
template <int MaxLen>
struct MultiTokenSortRatio {
public:
    MultiTokenSortRatio(size_t count) : scorer(count)
    {}

    size_t result_count() const
    {
        return scorer.result_count();
    }

    template <typename Sentence1>
    void insert(const Sentence1& s1_)
    {
        insert(detail::to_begin(s1_), detail::to_end(s1_));
    }

    template <typename InputIt1>
    void insert(InputIt1 first1, InputIt1 last1)
    {
        scorer.insert(detail::sorted_split(first1, last1).join());
    }

    template <typename InputIt2>
    void similarity(double* scores, size_t score_count, InputIt2 first2, InputIt2 last2,
                    double score_cutoff = 0.0) const
    {
        scorer.similarity(scores, score_count, detail::sorted_split(first2, last2).join(), score_cutoff);
    }

    template <typename Sentence2>
    void similarity(double* scores, size_t score_count, const Sentence2& s2, double score_cutoff = 0) const
    {
        similarity(scores, score_count, detail::to_begin(s2), detail::to_end(s2), score_cutoff);
    }

private:
    MultiRatio<MaxLen> scorer;
};
} /* namespace experimental */
#endif

// todo CachedRatio speed for equal strings vs original implementation
// TODO documentation
template <typename CharT1>
struct CachedTokenSortRatio {
    template <typename InputIt1>
    CachedTokenSortRatio(InputIt1 first1, InputIt1 last1)
        : s1_sorted(detail::sorted_split(first1, last1).join()), cached_ratio(s1_sorted)
    {}

    template <typename Sentence1>
    explicit CachedTokenSortRatio(const Sentence1& s1)
        : CachedTokenSortRatio(detail::to_begin(s1), detail::to_end(s1))
    {}

    template <typename InputIt2>
    double similarity(InputIt2 first2, InputIt2 last2, double score_cutoff = 0.0,
                      double score_hint = 0.0) const;

    template <typename Sentence2>
    double similarity(const Sentence2& s2, double score_cutoff = 0.0, double score_hint = 0.0) const;

private:
    std::basic_string<CharT1> s1_sorted;
    CachedRatio<CharT1> cached_ratio;
};

template <typename Sentence1>
explicit CachedTokenSortRatio(const Sentence1& s1) -> CachedTokenSortRatio<char_type<Sentence1>>;

template <typename InputIt1>
CachedTokenSortRatio(InputIt1 first1, InputIt1 last1) -> CachedTokenSortRatio<iter_value_t<InputIt1>>;

/**
 * @brief Sorts the words in the strings and calculates the fuzz::partial_ratio
 * between them
 *
 *
 * @tparam Sentence1 This is a string that can be converted to
 * basic_string_view<char_type>
 * @tparam Sentence2 This is a string that can be converted to
 * basic_string_view<char_type>
 *
 * @param s1 string to compare with s2 (for type info check Template parameters
 * above)
 * @param s2 string to compare with s1 (for type info check Template parameters
 * above)
 * @param score_cutoff Optional argument for a score threshold between 0% and
 * 100%. Matches with a lower score than this number will not be returned.
 * Defaults to 0.
 *
 * @return returns the ratio between s1 and s2 or 0 when ratio < score_cutoff
 */
template <typename InputIt1, typename InputIt2>
double partial_token_sort_ratio(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2,
                                double score_cutoff = 0);

template <typename Sentence1, typename Sentence2>
double partial_token_sort_ratio(const Sentence1& s1, const Sentence2& s2, double score_cutoff = 0);

// TODO documentation
template <typename CharT1>
struct CachedPartialTokenSortRatio {
    template <typename InputIt1>
    CachedPartialTokenSortRatio(InputIt1 first1, InputIt1 last1)
        : s1_sorted(detail::sorted_split(first1, last1).join()), cached_partial_ratio(s1_sorted)
    {}

    template <typename Sentence1>
    explicit CachedPartialTokenSortRatio(const Sentence1& s1)
        : CachedPartialTokenSortRatio(detail::to_begin(s1), detail::to_end(s1))
    {}

    template <typename InputIt2>
    double similarity(InputIt2 first2, InputIt2 last2, double score_cutoff = 0.0,
                      double score_hint = 0.0) const;

    template <typename Sentence2>
    double similarity(const Sentence2& s2, double score_cutoff = 0.0, double score_hint = 0.0) const;

private:
    std::basic_string<CharT1> s1_sorted;
    CachedPartialRatio<CharT1> cached_partial_ratio;
};

template <typename Sentence1>
explicit CachedPartialTokenSortRatio(const Sentence1& s1)
    -> CachedPartialTokenSortRatio<char_type<Sentence1>>;

template <typename InputIt1>
CachedPartialTokenSortRatio(InputIt1 first1, InputIt1 last1)
    -> CachedPartialTokenSortRatio<iter_value_t<InputIt1>>;

/**
 * @brief Compares the words in the strings based on unique and common words
 * between them using fuzz::ratio
 *
 * @details
 * @code{.cpp}
 * // score1 is 83.87
 * double score1 = token_sort_ratio("fuzzy was a bear", "fuzzy fuzzy was a
 * bear")
 * // score2 is 100
 * double score2 = token_set_ratio("fuzzy was a bear", "fuzzy fuzzy was a bear")
 * @endcode
 *
 * @tparam Sentence1 This is a string that can be converted to
 * basic_string_view<char_type>
 * @tparam Sentence2 This is a string that can be converted to
 * basic_string_view<char_type>
 *
 * @param s1 string to compare with s2 (for type info check Template parameters
 * above)
 * @param s2 string to compare with s1 (for type info check Template parameters
 * above)
 * @param score_cutoff Optional argument for a score threshold between 0% and
 * 100%. Matches with a lower score than this number will not be returned.
 * Defaults to 0.
 *
 * @return returns the ratio between s1 and s2 or 0 when ratio < score_cutoff
 */
template <typename InputIt1, typename InputIt2>
double token_set_ratio(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2,
                       double score_cutoff = 0);

template <typename Sentence1, typename Sentence2>
double token_set_ratio(const Sentence1& s1, const Sentence2& s2, double score_cutoff = 0);

// TODO documentation
template <typename CharT1>
struct CachedTokenSetRatio {
    template <typename InputIt1>
    CachedTokenSetRatio(InputIt1 first1, InputIt1 last1)
        : s1(first1, last1), tokens_s1(detail::sorted_split(std::begin(s1), std::end(s1)))
    {}

    template <typename Sentence1>
    explicit CachedTokenSetRatio(const Sentence1& s1_)
        : CachedTokenSetRatio(detail::to_begin(s1_), detail::to_end(s1_))
    {}

    template <typename InputIt2>
    double similarity(InputIt2 first2, InputIt2 last2, double score_cutoff = 0.0,
                      double score_hint = 0.0) const;

    template <typename Sentence2>
    double similarity(const Sentence2& s2, double score_cutoff = 0.0, double score_hint = 0.0) const;

private:
    std::basic_string<CharT1> s1;
    detail::SplittedSentenceView<typename std::basic_string<CharT1>::iterator> tokens_s1;
};

template <typename Sentence1>
explicit CachedTokenSetRatio(const Sentence1& s1) -> CachedTokenSetRatio<char_type<Sentence1>>;

template <typename InputIt1>
CachedTokenSetRatio(InputIt1 first1, InputIt1 last1) -> CachedTokenSetRatio<iter_value_t<InputIt1>>;

/**
 * @brief Compares the words in the strings based on unique and common words
 * between them using fuzz::partial_ratio
 *
 * @tparam Sentence1 This is a string that can be converted to
 * basic_string_view<char_type>
 * @tparam Sentence2 This is a string that can be converted to
 * basic_string_view<char_type>
 *
 * @param s1 string to compare with s2 (for type info check Template parameters
 * above)
 * @param s2 string to compare with s1 (for type info check Template parameters
 * above)
 * @param score_cutoff Optional argument for a score threshold between 0% and
 * 100%. Matches with a lower score than this number will not be returned.
 * Defaults to 0.
 *
 * @return returns the ratio between s1 and s2 or 0 when ratio < score_cutoff
 */
template <typename InputIt1, typename InputIt2>
double partial_token_set_ratio(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2,
                               double score_cutoff = 0);

template <typename Sentence1, typename Sentence2>
double partial_token_set_ratio(const Sentence1& s1, const Sentence2& s2, double score_cutoff = 0);

// TODO documentation
template <typename CharT1>
struct CachedPartialTokenSetRatio {
    template <typename InputIt1>
    CachedPartialTokenSetRatio(InputIt1 first1, InputIt1 last1)
        : s1(first1, last1), tokens_s1(detail::sorted_split(std::begin(s1), std::end(s1)))
    {}

    template <typename Sentence1>
    explicit CachedPartialTokenSetRatio(const Sentence1& s1_)
        : CachedPartialTokenSetRatio(detail::to_begin(s1_), detail::to_end(s1_))
    {}

    template <typename InputIt2>
    double similarity(InputIt2 first2, InputIt2 last2, double score_cutoff = 0.0,
                      double score_hint = 0.0) const;

    template <typename Sentence2>
    double similarity(const Sentence2& s2, double score_cutoff = 0.0, double score_hint = 0.0) const;

private:
    std::basic_string<CharT1> s1;
    detail::SplittedSentenceView<typename std::basic_string<CharT1>::iterator> tokens_s1;
};

template <typename Sentence1>
explicit CachedPartialTokenSetRatio(const Sentence1& s1) -> CachedPartialTokenSetRatio<char_type<Sentence1>>;

template <typename InputIt1>
CachedPartialTokenSetRatio(InputIt1 first1, InputIt1 last1)
    -> CachedPartialTokenSetRatio<iter_value_t<InputIt1>>;

/**
 * @brief Helper method that returns the maximum of fuzz::token_set_ratio and
 * fuzz::token_sort_ratio (faster than manually executing the two functions)
 *
 * @tparam Sentence1 This is a string that can be converted to
 * basic_string_view<char_type>
 * @tparam Sentence2 This is a string that can be converted to
 * basic_string_view<char_type>
 *
 * @param s1 string to compare with s2 (for type info check Template parameters
 * above)
 * @param s2 string to compare with s1 (for type info check Template parameters
 * above)
 * @param score_cutoff Optional argument for a score threshold between 0% and
 * 100%. Matches with a lower score than this number will not be returned.
 * Defaults to 0.
 *
 * @return returns the ratio between s1 and s2 or 0 when ratio < score_cutoff
 */
template <typename InputIt1, typename InputIt2>
double token_ratio(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2, double score_cutoff = 0);

template <typename Sentence1, typename Sentence2>
double token_ratio(const Sentence1& s1, const Sentence2& s2, double score_cutoff = 0);

// todo add real implementation
template <typename CharT1>
struct CachedTokenRatio {
    template <typename InputIt1>
    CachedTokenRatio(InputIt1 first1, InputIt1 last1)
        : s1(first1, last1),
          s1_tokens(detail::sorted_split(std::begin(s1), std::end(s1))),
          s1_sorted(s1_tokens.join()),
          cached_ratio_s1_sorted(s1_sorted)
    {}

    template <typename Sentence1>
    explicit CachedTokenRatio(const Sentence1& s1_)
        : CachedTokenRatio(detail::to_begin(s1_), detail::to_end(s1_))
    {}

    template <typename InputIt2>
    double similarity(InputIt2 first2, InputIt2 last2, double score_cutoff = 0.0,
                      double score_hint = 0.0) const;

    template <typename Sentence2>
    double similarity(const Sentence2& s2, double score_cutoff = 0.0, double score_hint = 0.0) const;

private:
    std::basic_string<CharT1> s1;
    detail::SplittedSentenceView<typename std::basic_string<CharT1>::iterator> s1_tokens;
    std::basic_string<CharT1> s1_sorted;
    CachedRatio<CharT1> cached_ratio_s1_sorted;
};

template <typename Sentence1>
explicit CachedTokenRatio(const Sentence1& s1) -> CachedTokenRatio<char_type<Sentence1>>;

template <typename InputIt1>
CachedTokenRatio(InputIt1 first1, InputIt1 last1) -> CachedTokenRatio<iter_value_t<InputIt1>>;

/**
 * @brief Helper method that returns the maximum of
 * fuzz::partial_token_set_ratio and fuzz::partial_token_sort_ratio (faster than
 * manually executing the two functions)
 *
 * @tparam Sentence1 This is a string that can be converted to
 * basic_string_view<char_type>
 * @tparam Sentence2 This is a string that can be converted to
 * basic_string_view<char_type>
 *
 * @param s1 string to compare with s2 (for type info check Template parameters
 * above)
 * @param s2 string to compare with s1 (for type info check Template parameters
 * above)
 * @param score_cutoff Optional argument for a score threshold between 0% and
 * 100%. Matches with a lower score than this number will not be returned.
 * Defaults to 0.
 *
 * @return returns the ratio between s1 and s2 or 0 when ratio < score_cutoff
 */
template <typename InputIt1, typename InputIt2>
double partial_token_ratio(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2,
                           double score_cutoff = 0);

template <typename Sentence1, typename Sentence2>
double partial_token_ratio(const Sentence1& s1, const Sentence2& s2, double score_cutoff = 0);

// todo add real implementation
template <typename CharT1>
struct CachedPartialTokenRatio {
    template <typename InputIt1>
    CachedPartialTokenRatio(InputIt1 first1, InputIt1 last1)
        : s1(first1, last1),
          tokens_s1(detail::sorted_split(std::begin(s1), std::end(s1))),
          s1_sorted(tokens_s1.join())
    {}

    template <typename Sentence1>
    explicit CachedPartialTokenRatio(const Sentence1& s1_)
        : CachedPartialTokenRatio(detail::to_begin(s1_), detail::to_end(s1_))
    {}

    template <typename InputIt2>
    double similarity(InputIt2 first2, InputIt2 last2, double score_cutoff = 0.0,
                      double score_hint = 0.0) const;

    template <typename Sentence2>
    double similarity(const Sentence2& s2, double score_cutoff = 0.0, double score_hint = 0.0) const;

private:
    std::basic_string<CharT1> s1;
    detail::SplittedSentenceView<typename std::basic_string<CharT1>::iterator> tokens_s1;
    std::basic_string<CharT1> s1_sorted;
};

template <typename Sentence1>
explicit CachedPartialTokenRatio(const Sentence1& s1) -> CachedPartialTokenRatio<char_type<Sentence1>>;

template <typename InputIt1>
CachedPartialTokenRatio(InputIt1 first1, InputIt1 last1) -> CachedPartialTokenRatio<iter_value_t<InputIt1>>;

/**
 * @brief Calculates a weighted ratio based on the other ratio algorithms
 *
 * @details
 * @todo add a detailed description
 *
 * @tparam Sentence1 This is a string that can be converted to
 * basic_string_view<char_type>
 * @tparam Sentence2 This is a string that can be converted to
 * basic_string_view<char_type>
 *
 * @param s1 string to compare with s2 (for type info check Template parameters
 * above)
 * @param s2 string to compare with s1 (for type info check Template parameters
 * above)
 * @param score_cutoff Optional argument for a score threshold between 0% and
 * 100%. Matches with a lower score than this number will not be returned.
 * Defaults to 0.
 *
 * @return returns the ratio between s1 and s2 or 0 when ratio < score_cutoff
 */
template <typename InputIt1, typename InputIt2>
double WRatio(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2, double score_cutoff = 0);

template <typename Sentence1, typename Sentence2>
double WRatio(const Sentence1& s1, const Sentence2& s2, double score_cutoff = 0);

// todo add real implementation
template <typename CharT1>
struct CachedWRatio {
    template <typename InputIt1>
    explicit CachedWRatio(InputIt1 first1, InputIt1 last1);

    template <typename Sentence1>
    CachedWRatio(const Sentence1& s1_) : CachedWRatio(detail::to_begin(s1_), detail::to_end(s1_))
    {}

    template <typename InputIt2>
    double similarity(InputIt2 first2, InputIt2 last2, double score_cutoff = 0.0,
                      double score_hint = 0.0) const;

    template <typename Sentence2>
    double similarity(const Sentence2& s2, double score_cutoff = 0.0, double score_hint = 0.0) const;

private:
    // todo somehow implement this using other ratios with creating PatternMatchVector
    // multiple times
    std::basic_string<CharT1> s1;
    CachedPartialRatio<CharT1> cached_partial_ratio;
    detail::SplittedSentenceView<typename std::basic_string<CharT1>::iterator> tokens_s1;
    std::basic_string<CharT1> s1_sorted;
    rapidfuzz::detail::BlockPatternMatchVector blockmap_s1_sorted;
};

template <typename Sentence1>
explicit CachedWRatio(const Sentence1& s1) -> CachedWRatio<char_type<Sentence1>>;

template <typename InputIt1>
CachedWRatio(InputIt1 first1, InputIt1 last1) -> CachedWRatio<iter_value_t<InputIt1>>;

/**
 * @brief Calculates a quick ratio between two strings using fuzz.ratio
 *
 * @details
 * @todo add a detailed description
 *
 * @tparam Sentence1 This is a string that can be converted to
 * basic_string_view<char_type>
 * @tparam Sentence2 This is a string that can be converted to
 * basic_string_view<char_type>
 *
 * @param s1 string to compare with s2 (for type info check Template parameters
 * above)
 * @param s2 string to compare with s1 (for type info check Template parameters
 * above)
 * @param score_cutoff Optional argument for a score threshold between 0% and
 * 100%. Matches with a lower score than this number will not be returned.
 * Defaults to 0.
 *
 * @return returns the ratio between s1 and s2 or 0 when ratio < score_cutoff
 */
template <typename InputIt1, typename InputIt2>
double QRatio(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2, double score_cutoff = 0);

template <typename Sentence1, typename Sentence2>
double QRatio(const Sentence1& s1, const Sentence2& s2, double score_cutoff = 0);

#ifdef RAPIDFUZZ_SIMD
namespace experimental {
template <int MaxLen>
struct MultiQRatio {
public:
    MultiQRatio(size_t count) : scorer(count)
    {}

    size_t result_count() const
    {
        return scorer.result_count();
    }

    template <typename Sentence1>
    void insert(const Sentence1& s1_)
    {
        insert(detail::to_begin(s1_), detail::to_end(s1_));
    }

    template <typename InputIt1>
    void insert(InputIt1 first1, InputIt1 last1)
    {
        scorer.insert(first1, last1);
        str_lens.push_back(static_cast<size_t>(std::distance(first1, last1)));
    }

    template <typename InputIt2>
    void similarity(double* scores, size_t score_count, InputIt2 first2, InputIt2 last2,
                    double score_cutoff = 0.0) const
    {
        similarity(scores, score_count, detail::Range(first2, last2), score_cutoff);
    }

    template <typename Sentence2>
    void similarity(double* scores, size_t score_count, const Sentence2& s2, double score_cutoff = 0) const
    {
        rapidfuzz::detail::Range s2_(s2);
        if (s2_.empty()) {
            for (size_t i = 0; i < str_lens.size(); ++i)
                scores[i] = 0;

            return;
        }

        scorer.similarity(scores, score_count, s2, score_cutoff);

        for (size_t i = 0; i < str_lens.size(); ++i)
            if (str_lens[i] == 0) scores[i] = 0;
    }

private:
    std::vector<size_t> str_lens;
    MultiRatio<MaxLen> scorer;
};
} /* namespace experimental */
#endif

template <typename CharT1>
struct CachedQRatio {
    template <typename InputIt1>
    CachedQRatio(InputIt1 first1, InputIt1 last1) : s1(first1, last1), cached_ratio(first1, last1)
    {}

    template <typename Sentence1>
    explicit CachedQRatio(const Sentence1& s1_) : CachedQRatio(detail::to_begin(s1_), detail::to_end(s1_))
    {}

    template <typename InputIt2>
    double similarity(InputIt2 first2, InputIt2 last2, double score_cutoff = 0.0,
                      double score_hint = 0.0) const;

    template <typename Sentence2>
    double similarity(const Sentence2& s2, double score_cutoff = 0.0, double score_hint = 0.0) const;

private:
    std::basic_string<CharT1> s1;
    CachedRatio<CharT1> cached_ratio;
};

template <typename Sentence1>
explicit CachedQRatio(const Sentence1& s1) -> CachedQRatio<char_type<Sentence1>>;

template <typename InputIt1>
CachedQRatio(InputIt1 first1, InputIt1 last1) -> CachedQRatio<iter_value_t<InputIt1>>;

/**@}*/

} // namespace rapidfuzz::fuzz

#include <limits>

#include <algorithm>
#include <cmath>
#include <iterator>
#include <unordered_map>
#include <vector>

namespace rapidfuzz::fuzz {

/**********************************************
 *                  ratio
 *********************************************/

template <typename InputIt1, typename InputIt2>
double ratio(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2, double score_cutoff)
{
    return ratio(detail::Range(first1, last1), detail::Range(first2, last2), score_cutoff);
}

template <typename Sentence1, typename Sentence2>
double ratio(const Sentence1& s1, const Sentence2& s2, const double score_cutoff)
{
    return indel_normalized_similarity(s1, s2, score_cutoff / 100) * 100;
}

template <typename CharT1>
template <typename InputIt2>
double CachedRatio<CharT1>::similarity(InputIt2 first2, InputIt2 last2, double score_cutoff,
                                       double score_hint) const
{
    return similarity(detail::Range(first2, last2), score_cutoff, score_hint);
}

template <typename CharT1>
template <typename Sentence2>
double CachedRatio<CharT1>::similarity(const Sentence2& s2, double score_cutoff, double score_hint) const
{
    return cached_indel.normalized_similarity(s2, score_cutoff / 100, score_hint / 100) * 100;
}

/**********************************************
 *              partial_ratio
 *********************************************/

namespace fuzz_detail {

template <typename InputIt1, typename InputIt2, typename CachedCharT1>
ScoreAlignment<double>
partial_ratio_impl(rapidfuzz::detail::Range<InputIt1> s1, rapidfuzz::detail::Range<InputIt2> s2,
                   const CachedRatio<CachedCharT1>& cached_ratio,
                   const detail::CharSet<iter_value_t<InputIt1>>& s1_char_set, double score_cutoff)
{
    ScoreAlignment<double> res;
    auto len1 = static_cast<size_t>(s1.size());
    auto len2 = static_cast<size_t>(s2.size());
    res.src_start = 0;
    res.src_end = len1;
    res.dest_start = 0;
    res.dest_end = len1;

    if (len2 > len1) {
        int64_t maximum = static_cast<int64_t>(len1) * 2;
        double norm_cutoff_sim = rapidfuzz::detail::NormSim_to_NormDist(score_cutoff / 100);
        int64_t cutoff_dist = static_cast<int64_t>(std::ceil(static_cast<double>(maximum) * norm_cutoff_sim));
        int64_t best_dist = std::numeric_limits<int64_t>::max();
        std::vector<int64_t> scores(len2 - len1, -1);
        std::vector<std::pair<size_t, size_t>> windows = {{0, len2 - len1 - 1}};
        std::vector<std::pair<size_t, size_t>> new_windows;

        while (!windows.empty()) {
            for (const auto& window : windows) {
                auto subseq1_first = s2.begin() + static_cast<ptrdiff_t>(window.first);
                auto subseq2_first = s2.begin() + static_cast<ptrdiff_t>(window.second);
                rapidfuzz::detail::Range subseq1(subseq1_first, subseq1_first + static_cast<ptrdiff_t>(len1));
                rapidfuzz::detail::Range subseq2(subseq2_first, subseq2_first + static_cast<ptrdiff_t>(len1));

                if (scores[window.first] == -1) {
                    scores[window.first] = cached_ratio.cached_indel.distance(subseq1);
                    if (scores[window.first] < cutoff_dist) {
                        cutoff_dist = best_dist = scores[window.first];
                        res.dest_start = window.first;
                        res.dest_end = window.first + len1;
                        if (best_dist == 0) {
                            res.score = 100;
                            return res;
                        }
                    }
                }
                if (scores[window.second] == -1) {
                    scores[window.second] = cached_ratio.cached_indel.distance(subseq2);
                    if (scores[window.second] < cutoff_dist) {
                        cutoff_dist = best_dist = scores[window.second];
                        res.dest_start = window.second;
                        res.dest_end = window.second + len1;
                        if (best_dist == 0) {
                            res.score = 100;
                            return res;
                        }
                    }
                }

                size_t cell_diff = window.second - window.first;
                if (cell_diff == 1) continue;

                /* find the minimum score possible in the range first <-> last */
                int64_t known_edits = std::abs(scores[window.first] - scores[window.second]);
                /* half of the cells that are not needed for known_edits can lead to a better score */
                int64_t min_score = std::min(scores[window.first], scores[window.second]) -
                                    (static_cast<int64_t>(cell_diff) + known_edits / 2);
                if (min_score < cutoff_dist) {
                    size_t center = cell_diff / 2;
                    new_windows.emplace_back(window.first, window.first + center);
                    new_windows.emplace_back(window.first + center, window.second);
                }
            }

            std::swap(windows, new_windows);
            new_windows.clear();
        }

        double score = 1.0 - (static_cast<double>(best_dist) / static_cast<double>(maximum));
        score *= 100;
        if (score >= score_cutoff) score_cutoff = res.score = score;
    }

    for (size_t i = 1; i < len1; ++i) {
        rapidfuzz::detail::Range subseq(s2.begin(), s2.begin() + static_cast<ptrdiff_t>(i));
        if (!s1_char_set.find(subseq.back())) continue;

        double ls_ratio = cached_ratio.similarity(subseq, score_cutoff);
        if (ls_ratio > res.score) {
            score_cutoff = res.score = ls_ratio;
            res.dest_start = 0;
            res.dest_end = i;
            if (res.score == 100.0) return res;
        }
    }

    for (size_t i = len2 - len1; i < len2; ++i) {
        rapidfuzz::detail::Range subseq(s2.begin() + static_cast<ptrdiff_t>(i), s2.end());
        if (!s1_char_set.find(subseq.front())) continue;

        double ls_ratio = cached_ratio.similarity(subseq, score_cutoff);
        if (ls_ratio > res.score) {
            score_cutoff = res.score = ls_ratio;
            res.dest_start = i;
            res.dest_end = len2;
            if (res.score == 100.0) return res;
        }
    }

    return res;
}

template <typename InputIt1, typename InputIt2, typename CharT1 = iter_value_t<InputIt1>>
ScoreAlignment<double> partial_ratio_impl(rapidfuzz::detail::Range<InputIt1> s1,
                                          rapidfuzz::detail::Range<InputIt2> s2, double score_cutoff)
{
    CachedRatio<CharT1> cached_ratio(s1);

    detail::CharSet<CharT1> s1_char_set;
    for (auto ch : s1)
        s1_char_set.insert(ch);

    return partial_ratio_impl(s1, s2, cached_ratio, s1_char_set, score_cutoff);
}

} // namespace fuzz_detail

template <typename InputIt1, typename InputIt2>
ScoreAlignment<double> partial_ratio_alignment(InputIt1 first1, InputIt1 last1, InputIt2 first2,
                                               InputIt2 last2, double score_cutoff)
{
    auto len1 = static_cast<size_t>(std::distance(first1, last1));
    auto len2 = static_cast<size_t>(std::distance(first2, last2));

    if (len1 > len2) {
        ScoreAlignment<double> result = partial_ratio_alignment(first2, last2, first1, last1, score_cutoff);
        std::swap(result.src_start, result.dest_start);
        std::swap(result.src_end, result.dest_end);
        return result;
    }

    if (score_cutoff > 100) return ScoreAlignment<double>(0, 0, len1, 0, len1);

    if (!len1 || !len2)
        return ScoreAlignment<double>(static_cast<double>(len1 == len2) * 100.0, 0, len1, 0, len1);

    auto s1 = detail::Range(first1, last1);
    auto s2 = detail::Range(first2, last2);

    auto alignment = fuzz_detail::partial_ratio_impl(s1, s2, score_cutoff);
    if (alignment.score != 100 && s1.size() == s2.size()) {
        score_cutoff = std::max(score_cutoff, alignment.score);
        auto alignment2 = fuzz_detail::partial_ratio_impl(s2, s1, score_cutoff);
        if (alignment2.score > alignment.score) {
            std::swap(alignment2.src_start, alignment2.dest_start);
            std::swap(alignment2.src_end, alignment2.dest_end);
            return alignment2;
        }
    }

    return alignment;
}

template <typename Sentence1, typename Sentence2>
ScoreAlignment<double> partial_ratio_alignment(const Sentence1& s1, const Sentence2& s2, double score_cutoff)
{
    return partial_ratio_alignment(detail::to_begin(s1), detail::to_end(s1), detail::to_begin(s2),
                                   detail::to_end(s2), score_cutoff);
}

template <typename InputIt1, typename InputIt2>
double partial_ratio(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2, double score_cutoff)
{
    return partial_ratio_alignment(first1, last1, first2, last2, score_cutoff).score;
}

template <typename Sentence1, typename Sentence2>
double partial_ratio(const Sentence1& s1, const Sentence2& s2, double score_cutoff)
{
    return partial_ratio_alignment(s1, s2, score_cutoff).score;
}

template <typename CharT1>
template <typename InputIt1>
CachedPartialRatio<CharT1>::CachedPartialRatio(InputIt1 first1, InputIt1 last1)
    : s1(first1, last1), cached_ratio(first1, last1)
{
    for (const auto& ch : s1)
        s1_char_set.insert(ch);
}

template <typename CharT1>
template <typename InputIt2>
double CachedPartialRatio<CharT1>::similarity(InputIt2 first2, InputIt2 last2, double score_cutoff,
                                              [[maybe_unused]] double score_hint) const
{
    size_t len1 = s1.size();
    size_t len2 = static_cast<size_t>(std::distance(first2, last2));

    if (len1 > len2)
        return partial_ratio(detail::to_begin(s1), detail::to_end(s1), first2, last2, score_cutoff);

    if (score_cutoff > 100) return 0;

    if (!len1 || !len2) return static_cast<double>(len1 == len2) * 100.0;

    auto s1_ = detail::Range(s1);
    auto s2 = detail::Range(first2, last2);

    double score = fuzz_detail::partial_ratio_impl(s1_, s2, cached_ratio, s1_char_set, score_cutoff).score;
    if (score != 100 && s1_.size() == s2.size()) {
        score_cutoff = std::max(score_cutoff, score);
        double score2 = fuzz_detail::partial_ratio_impl(s2, s1_, score_cutoff).score;
        if (score2 > score) return score2;
    }

    return score;
}

template <typename CharT1>
template <typename Sentence2>
double CachedPartialRatio<CharT1>::similarity(const Sentence2& s2, double score_cutoff,
                                              [[maybe_unused]] double score_hint) const
{
    return similarity(detail::to_begin(s2), detail::to_end(s2), score_cutoff);
}

/**********************************************
 *             token_sort_ratio
 *********************************************/
template <typename InputIt1, typename InputIt2>
double token_sort_ratio(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2, double score_cutoff)
{
    if (score_cutoff > 100) return 0;

    return ratio(detail::sorted_split(first1, last1).join(), detail::sorted_split(first2, last2).join(),
                 score_cutoff);
}

template <typename Sentence1, typename Sentence2>
double token_sort_ratio(const Sentence1& s1, const Sentence2& s2, double score_cutoff)
{
    return token_sort_ratio(detail::to_begin(s1), detail::to_end(s1), detail::to_begin(s2),
                            detail::to_end(s2), score_cutoff);
}

template <typename CharT1>
template <typename InputIt2>
double CachedTokenSortRatio<CharT1>::similarity(InputIt2 first2, InputIt2 last2, double score_cutoff,
                                                [[maybe_unused]] double score_hint) const
{
    if (score_cutoff > 100) return 0;

    return cached_ratio.similarity(detail::sorted_split(first2, last2).join(), score_cutoff);
}

template <typename CharT1>
template <typename Sentence2>
double CachedTokenSortRatio<CharT1>::similarity(const Sentence2& s2, double score_cutoff,
                                                [[maybe_unused]] double score_hint) const
{
    return similarity(detail::to_begin(s2), detail::to_end(s2), score_cutoff);
}

/**********************************************
 *          partial_token_sort_ratio
 *********************************************/

template <typename InputIt1, typename InputIt2>
double partial_token_sort_ratio(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2,
                                double score_cutoff)
{
    if (score_cutoff > 100) return 0;

    return partial_ratio(detail::sorted_split(first1, last1).join(),
                         detail::sorted_split(first2, last2).join(), score_cutoff);
}

template <typename Sentence1, typename Sentence2>
double partial_token_sort_ratio(const Sentence1& s1, const Sentence2& s2, double score_cutoff)
{
    return partial_token_sort_ratio(detail::to_begin(s1), detail::to_end(s1), detail::to_begin(s2),
                                    detail::to_end(s2), score_cutoff);
}

template <typename CharT1>
template <typename InputIt2>
double CachedPartialTokenSortRatio<CharT1>::similarity(InputIt2 first2, InputIt2 last2, double score_cutoff,
                                                       [[maybe_unused]] double score_hint) const
{
    if (score_cutoff > 100) return 0;

    return cached_partial_ratio.similarity(detail::sorted_split(first2, last2).join(), score_cutoff);
}

template <typename CharT1>
template <typename Sentence2>
double CachedPartialTokenSortRatio<CharT1>::similarity(const Sentence2& s2, double score_cutoff,
                                                       [[maybe_unused]] double score_hint) const
{
    return similarity(detail::to_begin(s2), detail::to_end(s2), score_cutoff);
}

/**********************************************
 *               token_set_ratio
 *********************************************/

namespace fuzz_detail {
template <typename InputIt1, typename InputIt2>
double token_set_ratio(const rapidfuzz::detail::SplittedSentenceView<InputIt1>& tokens_a,
                       const rapidfuzz::detail::SplittedSentenceView<InputIt2>& tokens_b,
                       const double score_cutoff)
{
    /* in FuzzyWuzzy this returns 0. For sake of compatibility return 0 here as well
     * see https://github.com/maxbachmann/RapidFuzz/issues/110 */
    if (tokens_a.empty() || tokens_b.empty()) return 0;

    auto decomposition = detail::set_decomposition(tokens_a, tokens_b);
    auto intersect = decomposition.intersection;
    auto diff_ab = decomposition.difference_ab;
    auto diff_ba = decomposition.difference_ba;

    // one sentence is part of the other one
    if (!intersect.empty() && (diff_ab.empty() || diff_ba.empty())) return 100;

    auto diff_ab_joined = diff_ab.join();
    auto diff_ba_joined = diff_ba.join();

    size_t ab_len = diff_ab_joined.length();
    size_t ba_len = diff_ba_joined.length();
    size_t sect_len = intersect.length();

    // string length sect+ab <-> sect and sect+ba <-> sect
    int64_t sect_ab_len = static_cast<int64_t>(sect_len + bool(sect_len) + ab_len);
    int64_t sect_ba_len = static_cast<int64_t>(sect_len + bool(sect_len) + ba_len);

    double result = 0;
    auto cutoff_distance = detail::score_cutoff_to_distance<100>(score_cutoff, sect_ab_len + sect_ba_len);
    int64_t dist = indel_distance(diff_ab_joined, diff_ba_joined, cutoff_distance);

    if (dist <= cutoff_distance)
        result = detail::norm_distance<100>(dist, sect_ab_len + sect_ba_len, score_cutoff);

    // exit early since the other ratios are 0
    if (!sect_len) return result;

    // levenshtein distance sect+ab <-> sect and sect+ba <-> sect
    // since only sect is similar in them the distance can be calculated based on
    // the length difference
    int64_t sect_ab_dist = static_cast<int64_t>(bool(sect_len) + ab_len);
    double sect_ab_ratio =
        detail::norm_distance<100>(sect_ab_dist, static_cast<int64_t>(sect_len) + sect_ab_len, score_cutoff);

    int64_t sect_ba_dist = static_cast<int64_t>(bool(sect_len) + ba_len);
    double sect_ba_ratio =
        detail::norm_distance<100>(sect_ba_dist, static_cast<int64_t>(sect_len) + sect_ba_len, score_cutoff);

    return std::max({result, sect_ab_ratio, sect_ba_ratio});
}
} // namespace fuzz_detail

template <typename InputIt1, typename InputIt2>
double token_set_ratio(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2, double score_cutoff)
{
    if (score_cutoff > 100) return 0;

    return fuzz_detail::token_set_ratio(detail::sorted_split(first1, last1),
                                        detail::sorted_split(first2, last2), score_cutoff);
}

template <typename Sentence1, typename Sentence2>
double token_set_ratio(const Sentence1& s1, const Sentence2& s2, double score_cutoff)
{
    return token_set_ratio(detail::to_begin(s1), detail::to_end(s1), detail::to_begin(s2), detail::to_end(s2),
                           score_cutoff);
}

template <typename CharT1>
template <typename InputIt2>
double CachedTokenSetRatio<CharT1>::similarity(InputIt2 first2, InputIt2 last2, double score_cutoff,
                                               [[maybe_unused]] double score_hint) const
{
    if (score_cutoff > 100) return 0;

    return fuzz_detail::token_set_ratio(tokens_s1, detail::sorted_split(first2, last2), score_cutoff);
}

template <typename CharT1>
template <typename Sentence2>
double CachedTokenSetRatio<CharT1>::similarity(const Sentence2& s2, double score_cutoff,
                                               [[maybe_unused]] double score_hint) const
{
    return similarity(detail::to_begin(s2), detail::to_end(s2), score_cutoff);
}

/**********************************************
 *          partial_token_set_ratio
 *********************************************/

namespace fuzz_detail {
template <typename InputIt1, typename InputIt2>
double partial_token_set_ratio(const rapidfuzz::detail::SplittedSentenceView<InputIt1>& tokens_a,
                               const rapidfuzz::detail::SplittedSentenceView<InputIt2>& tokens_b,
                               const double score_cutoff)
{
    /* in FuzzyWuzzy this returns 0. For sake of compatibility return 0 here as well
     * see https://github.com/maxbachmann/RapidFuzz/issues/110 */
    if (tokens_a.empty() || tokens_b.empty()) return 0;

    auto decomposition = detail::set_decomposition(tokens_a, tokens_b);

    // exit early when there is a common word in both sequences
    if (!decomposition.intersection.empty()) return 100;

    return partial_ratio(decomposition.difference_ab.join(), decomposition.difference_ba.join(),
                         score_cutoff);
}
} // namespace fuzz_detail

template <typename InputIt1, typename InputIt2>
double partial_token_set_ratio(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2,
                               double score_cutoff)
{
    if (score_cutoff > 100) return 0;

    return fuzz_detail::partial_token_set_ratio(detail::sorted_split(first1, last1),
                                                detail::sorted_split(first2, last2), score_cutoff);
}

template <typename Sentence1, typename Sentence2>
double partial_token_set_ratio(const Sentence1& s1, const Sentence2& s2, double score_cutoff)
{
    return partial_token_set_ratio(detail::to_begin(s1), detail::to_end(s1), detail::to_begin(s2),
                                   detail::to_end(s2), score_cutoff);
}

template <typename CharT1>
template <typename InputIt2>
double CachedPartialTokenSetRatio<CharT1>::similarity(InputIt2 first2, InputIt2 last2, double score_cutoff,
                                                      [[maybe_unused]] double score_hint) const
{
    if (score_cutoff > 100) return 0;

    return fuzz_detail::partial_token_set_ratio(tokens_s1, detail::sorted_split(first2, last2), score_cutoff);
}

template <typename CharT1>
template <typename Sentence2>
double CachedPartialTokenSetRatio<CharT1>::similarity(const Sentence2& s2, double score_cutoff,
                                                      [[maybe_unused]] double score_hint) const
{
    return similarity(detail::to_begin(s2), detail::to_end(s2), score_cutoff);
}

/**********************************************
 *                token_ratio
 *********************************************/

template <typename InputIt1, typename InputIt2>
double token_ratio(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2, double score_cutoff)
{
    if (score_cutoff > 100) return 0;

    auto tokens_a = detail::sorted_split(first1, last1);
    auto tokens_b = detail::sorted_split(first2, last2);

    auto decomposition = detail::set_decomposition(tokens_a, tokens_b);
    auto intersect = decomposition.intersection;
    auto diff_ab = decomposition.difference_ab;
    auto diff_ba = decomposition.difference_ba;

    if (!intersect.empty() && (diff_ab.empty() || diff_ba.empty())) return 100;

    auto diff_ab_joined = diff_ab.join();
    auto diff_ba_joined = diff_ba.join();

    size_t ab_len = diff_ab_joined.length();
    size_t ba_len = diff_ba_joined.length();
    size_t sect_len = intersect.length();

    double result = ratio(tokens_a.join(), tokens_b.join(), score_cutoff);

    // string length sect+ab <-> sect and sect+ba <-> sect
    int64_t sect_ab_len = static_cast<int64_t>(sect_len + bool(sect_len) + ab_len);
    int64_t sect_ba_len = static_cast<int64_t>(sect_len + bool(sect_len) + ba_len);

    auto cutoff_distance = detail::score_cutoff_to_distance<100>(score_cutoff, sect_ab_len + sect_ba_len);
    int64_t dist = indel_distance(diff_ab_joined, diff_ba_joined, cutoff_distance);
    if (dist <= cutoff_distance)
        result = std::max(result, detail::norm_distance<100>(dist, sect_ab_len + sect_ba_len, score_cutoff));

    // exit early since the other ratios are 0
    if (!sect_len) return result;

    // levenshtein distance sect+ab <-> sect and sect+ba <-> sect
    // since only sect is similar in them the distance can be calculated based on
    // the length difference
    int64_t sect_ab_dist = static_cast<int64_t>(bool(sect_len) + ab_len);
    double sect_ab_ratio =
        detail::norm_distance<100>(sect_ab_dist, static_cast<int64_t>(sect_len) + sect_ab_len, score_cutoff);

    int64_t sect_ba_dist = static_cast<int64_t>(bool(sect_len) + ba_len);
    double sect_ba_ratio =
        detail::norm_distance<100>(sect_ba_dist, static_cast<int64_t>(sect_len) + sect_ba_len, score_cutoff);

    return std::max({result, sect_ab_ratio, sect_ba_ratio});
}

template <typename Sentence1, typename Sentence2>
double token_ratio(const Sentence1& s1, const Sentence2& s2, double score_cutoff)
{
    return token_ratio(detail::to_begin(s1), detail::to_end(s1), detail::to_begin(s2), detail::to_end(s2),
                       score_cutoff);
}

namespace fuzz_detail {
template <typename CharT1, typename CachedCharT1, typename InputIt2>
double token_ratio(const rapidfuzz::detail::SplittedSentenceView<CharT1>& s1_tokens,
                   const CachedRatio<CachedCharT1>& cached_ratio_s1_sorted, InputIt2 first2, InputIt2 last2,
                   double score_cutoff)
{
    if (score_cutoff > 100) return 0;

    auto s2_tokens = detail::sorted_split(first2, last2);

    auto decomposition = detail::set_decomposition(s1_tokens, s2_tokens);
    auto intersect = decomposition.intersection;
    auto diff_ab = decomposition.difference_ab;
    auto diff_ba = decomposition.difference_ba;

    if (!intersect.empty() && (diff_ab.empty() || diff_ba.empty())) return 100;

    auto diff_ab_joined = diff_ab.join();
    auto diff_ba_joined = diff_ba.join();

    int64_t ab_len = diff_ab_joined.length();
    int64_t ba_len = diff_ba_joined.length();
    int64_t sect_len = intersect.length();

    double result = cached_ratio_s1_sorted.similarity(s2_tokens.join(), score_cutoff);

    // string length sect+ab <-> sect and sect+ba <-> sect
    int64_t sect_ab_len = sect_len + bool(sect_len) + ab_len;
    int64_t sect_ba_len = sect_len + bool(sect_len) + ba_len;

    auto cutoff_distance = detail::score_cutoff_to_distance<100>(score_cutoff, sect_ab_len + sect_ba_len);
    int64_t dist = indel_distance(diff_ab_joined, diff_ba_joined, cutoff_distance);
    if (dist <= cutoff_distance)
        result = std::max(result, detail::norm_distance<100>(dist, sect_ab_len + sect_ba_len, score_cutoff));

    // exit early since the other ratios are 0
    if (!sect_len) return result;

    // levenshtein distance sect+ab <-> sect and sect+ba <-> sect
    // since only sect is similar in them the distance can be calculated based on
    // the length difference
    int64_t sect_ab_dist = bool(sect_len) + ab_len;
    double sect_ab_ratio = detail::norm_distance<100>(sect_ab_dist, sect_len + sect_ab_len, score_cutoff);

    int64_t sect_ba_dist = bool(sect_len) + ba_len;
    double sect_ba_ratio = detail::norm_distance<100>(sect_ba_dist, sect_len + sect_ba_len, score_cutoff);

    return std::max({result, sect_ab_ratio, sect_ba_ratio});
}

// todo this is a temporary solution until WRatio is properly implemented using other scorers
template <typename CharT1, typename InputIt1, typename InputIt2>
double token_ratio(const std::basic_string<CharT1>& s1_sorted,
                   const rapidfuzz::detail::SplittedSentenceView<InputIt1>& tokens_s1,
                   const detail::BlockPatternMatchVector& blockmap_s1_sorted, InputIt2 first2, InputIt2 last2,
                   double score_cutoff)
{
    if (score_cutoff > 100) return 0;

    auto tokens_b = detail::sorted_split(first2, last2);

    auto decomposition = detail::set_decomposition(tokens_s1, tokens_b);
    auto intersect = decomposition.intersection;
    auto diff_ab = decomposition.difference_ab;
    auto diff_ba = decomposition.difference_ba;

    if (!intersect.empty() && (diff_ab.empty() || diff_ba.empty())) return 100;

    auto diff_ab_joined = diff_ab.join();
    auto diff_ba_joined = diff_ba.join();

    int64_t ab_len = diff_ab_joined.length();
    int64_t ba_len = diff_ba_joined.length();
    int64_t sect_len = intersect.length();

    double result = 0;
    auto s2_sorted = tokens_b.join();
    if (s1_sorted.size() < 65) {
        double norm_sim = detail::indel_normalized_similarity(blockmap_s1_sorted, detail::Range(s1_sorted),
                                                              detail::Range(s2_sorted), score_cutoff / 100);
        result = norm_sim * 100;
    }
    else {
        result = fuzz::ratio(s1_sorted, s2_sorted, score_cutoff);
    }

    // string length sect+ab <-> sect and sect+ba <-> sect
    int64_t sect_ab_len = sect_len + bool(sect_len) + ab_len;
    int64_t sect_ba_len = sect_len + bool(sect_len) + ba_len;

    auto cutoff_distance = detail::score_cutoff_to_distance<100>(score_cutoff, sect_ab_len + sect_ba_len);
    int64_t dist = indel_distance(diff_ab_joined, diff_ba_joined, cutoff_distance);
    if (dist <= cutoff_distance)
        result = std::max(result, detail::norm_distance<100>(dist, sect_ab_len + sect_ba_len, score_cutoff));

    // exit early since the other ratios are 0
    if (!sect_len) return result;

    // levenshtein distance sect+ab <-> sect and sect+ba <-> sect
    // since only sect is similar in them the distance can be calculated based on
    // the length difference
    int64_t sect_ab_dist = bool(sect_len) + ab_len;
    double sect_ab_ratio = detail::norm_distance<100>(sect_ab_dist, sect_len + sect_ab_len, score_cutoff);

    int64_t sect_ba_dist = bool(sect_len) + ba_len;
    double sect_ba_ratio = detail::norm_distance<100>(sect_ba_dist, sect_len + sect_ba_len, score_cutoff);

    return std::max({result, sect_ab_ratio, sect_ba_ratio});
}
} // namespace fuzz_detail

template <typename CharT1>
template <typename InputIt2>
double CachedTokenRatio<CharT1>::similarity(InputIt2 first2, InputIt2 last2, double score_cutoff,
                                            [[maybe_unused]] double score_hint) const
{
    return fuzz_detail::token_ratio(s1_tokens, cached_ratio_s1_sorted, first2, last2, score_cutoff);
}

template <typename CharT1>
template <typename Sentence2>
double CachedTokenRatio<CharT1>::similarity(const Sentence2& s2, double score_cutoff,
                                            [[maybe_unused]] double score_hint) const
{
    return similarity(detail::to_begin(s2), detail::to_end(s2), score_cutoff);
}

/**********************************************
 *            partial_token_ratio
 *********************************************/

template <typename InputIt1, typename InputIt2>
double partial_token_ratio(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2,
                           double score_cutoff)
{
    if (score_cutoff > 100) return 0;

    auto tokens_a = detail::sorted_split(first1, last1);
    auto tokens_b = detail::sorted_split(first2, last2);

    auto decomposition = detail::set_decomposition(tokens_a, tokens_b);

    // exit early when there is a common word in both sequences
    if (!decomposition.intersection.empty()) return 100;

    auto diff_ab = decomposition.difference_ab;
    auto diff_ba = decomposition.difference_ba;

    double result = partial_ratio(tokens_a.join(), tokens_b.join(), score_cutoff);

    // do not calculate the same partial_ratio twice
    if (tokens_a.word_count() == diff_ab.word_count() && tokens_b.word_count() == diff_ba.word_count()) {
        return result;
    }

    score_cutoff = std::max(score_cutoff, result);
    return std::max(result, partial_ratio(diff_ab.join(), diff_ba.join(), score_cutoff));
}

template <typename Sentence1, typename Sentence2>
double partial_token_ratio(const Sentence1& s1, const Sentence2& s2, double score_cutoff)
{
    return partial_token_ratio(detail::to_begin(s1), detail::to_end(s1), detail::to_begin(s2),
                               detail::to_end(s2), score_cutoff);
}

namespace fuzz_detail {
template <typename CharT1, typename InputIt1, typename InputIt2>
double partial_token_ratio(const std::basic_string<CharT1>& s1_sorted,
                           const rapidfuzz::detail::SplittedSentenceView<InputIt1>& tokens_s1,
                           InputIt2 first2, InputIt2 last2, double score_cutoff)
{
    if (score_cutoff > 100) return 0;

    auto tokens_b = detail::sorted_split(first2, last2);

    auto decomposition = detail::set_decomposition(tokens_s1, tokens_b);

    // exit early when there is a common word in both sequences
    if (!decomposition.intersection.empty()) return 100;

    auto diff_ab = decomposition.difference_ab;
    auto diff_ba = decomposition.difference_ba;

    double result = partial_ratio(s1_sorted, tokens_b.join(), score_cutoff);

    // do not calculate the same partial_ratio twice
    if (tokens_s1.word_count() == diff_ab.word_count() && tokens_b.word_count() == diff_ba.word_count()) {
        return result;
    }

    score_cutoff = std::max(score_cutoff, result);
    return std::max(result, partial_ratio(diff_ab.join(), diff_ba.join(), score_cutoff));
}

} // namespace fuzz_detail

template <typename CharT1>
template <typename InputIt2>
double CachedPartialTokenRatio<CharT1>::similarity(InputIt2 first2, InputIt2 last2, double score_cutoff,
                                                   [[maybe_unused]] double score_hint) const
{
    return fuzz_detail::partial_token_ratio(s1_sorted, tokens_s1, first2, last2, score_cutoff);
}

template <typename CharT1>
template <typename Sentence2>
double CachedPartialTokenRatio<CharT1>::similarity(const Sentence2& s2, double score_cutoff,
                                                   [[maybe_unused]] double score_hint) const
{
    return similarity(detail::to_begin(s2), detail::to_end(s2), score_cutoff);
}

/**********************************************
 *                  WRatio
 *********************************************/

template <typename InputIt1, typename InputIt2>
double WRatio(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2, double score_cutoff)
{
    if (score_cutoff > 100) return 0;

    constexpr double UNBASE_SCALE = 0.95;

    auto len1 = std::distance(first1, last1);
    auto len2 = std::distance(first2, last2);

    /* in FuzzyWuzzy this returns 0. For sake of compatibility return 0 here as well
     * see https://github.com/maxbachmann/RapidFuzz/issues/110 */
    if (!len1 || !len2) return 0;

    double len_ratio = (len1 > len2) ? static_cast<double>(len1) / static_cast<double>(len2)
                                     : static_cast<double>(len2) / static_cast<double>(len1);

    double end_ratio = ratio(first1, last1, first2, last2, score_cutoff);

    if (len_ratio < 1.5) {
        score_cutoff = std::max(score_cutoff, end_ratio) / UNBASE_SCALE;
        return std::max(end_ratio, token_ratio(first1, last1, first2, last2, score_cutoff) * UNBASE_SCALE);
    }

    const double PARTIAL_SCALE = (len_ratio < 8.0) ? 0.9 : 0.6;

    score_cutoff = std::max(score_cutoff, end_ratio) / PARTIAL_SCALE;
    end_ratio =
        std::max(end_ratio, partial_ratio(first1, last1, first2, last2, score_cutoff) * PARTIAL_SCALE);

    score_cutoff = std::max(score_cutoff, end_ratio) / UNBASE_SCALE;
    return std::max(end_ratio, partial_token_ratio(first1, last1, first2, last2, score_cutoff) *
                                   UNBASE_SCALE * PARTIAL_SCALE);
}

template <typename Sentence1, typename Sentence2>
double WRatio(const Sentence1& s1, const Sentence2& s2, double score_cutoff)
{
    return WRatio(detail::to_begin(s1), detail::to_end(s1), detail::to_begin(s2), detail::to_end(s2),
                  score_cutoff);
}

template <typename Sentence1>
template <typename InputIt1>
CachedWRatio<Sentence1>::CachedWRatio(InputIt1 first1, InputIt1 last1)
    : s1(first1, last1),
      cached_partial_ratio(first1, last1),
      tokens_s1(detail::sorted_split(std::begin(s1), std::end(s1))),
      s1_sorted(tokens_s1.join()),
      blockmap_s1_sorted(detail::Range(s1_sorted))
{}

template <typename CharT1>
template <typename InputIt2>
double CachedWRatio<CharT1>::similarity(InputIt2 first2, InputIt2 last2, double score_cutoff,
                                        [[maybe_unused]] double score_hint) const
{
    if (score_cutoff > 100) return 0;

    constexpr double UNBASE_SCALE = 0.95;

    ptrdiff_t len1 = s1.size();
    ptrdiff_t len2 = std::distance(first2, last2);

    /* in FuzzyWuzzy this returns 0. For sake of compatibility return 0 here as well
     * see https://github.com/maxbachmann/RapidFuzz/issues/110 */
    if (!len1 || !len2) return 0;

    double len_ratio = (len1 > len2) ? static_cast<double>(len1) / static_cast<double>(len2)
                                     : static_cast<double>(len2) / static_cast<double>(len1);

    double end_ratio = cached_partial_ratio.cached_ratio.similarity(first2, last2, score_cutoff);

    if (len_ratio < 1.5) {
        score_cutoff = std::max(score_cutoff, end_ratio) / UNBASE_SCALE;
        // use pre calculated values
        auto r =
            fuzz_detail::token_ratio(s1_sorted, tokens_s1, blockmap_s1_sorted, first2, last2, score_cutoff);
        return std::max(end_ratio, r * UNBASE_SCALE);
    }

    const double PARTIAL_SCALE = (len_ratio < 8.0) ? 0.9 : 0.6;

    score_cutoff = std::max(score_cutoff, end_ratio) / PARTIAL_SCALE;
    end_ratio =
        std::max(end_ratio, cached_partial_ratio.similarity(first2, last2, score_cutoff) * PARTIAL_SCALE);

    score_cutoff = std::max(score_cutoff, end_ratio) / UNBASE_SCALE;
    auto r = fuzz_detail::partial_token_ratio(s1_sorted, tokens_s1, first2, last2, score_cutoff);
    return std::max(end_ratio, r * UNBASE_SCALE * PARTIAL_SCALE);
}

template <typename CharT1>
template <typename Sentence2>
double CachedWRatio<CharT1>::similarity(const Sentence2& s2, double score_cutoff,
                                        [[maybe_unused]] double score_hint) const
{
    return similarity(detail::to_begin(s2), detail::to_end(s2), score_cutoff);
}

/**********************************************
 *                QRatio
 *********************************************/

template <typename InputIt1, typename InputIt2>
double QRatio(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2, double score_cutoff)
{
    auto len1 = std::distance(first1, last1);
    auto len2 = std::distance(first2, last2);

    /* in FuzzyWuzzy this returns 0. For sake of compatibility return 0 here as well
     * see https://github.com/maxbachmann/RapidFuzz/issues/110 */
    if (!len1 || !len2) return 0;

    return ratio(first1, last1, first2, last2, score_cutoff);
}

template <typename Sentence1, typename Sentence2>
double QRatio(const Sentence1& s1, const Sentence2& s2, double score_cutoff)
{
    return QRatio(detail::to_begin(s1), detail::to_end(s1), detail::to_begin(s2), detail::to_end(s2),
                  score_cutoff);
}

template <typename CharT1>
template <typename InputIt2>
double CachedQRatio<CharT1>::similarity(InputIt2 first2, InputIt2 last2, double score_cutoff,
                                        [[maybe_unused]] double score_hint) const
{
    auto len2 = std::distance(first2, last2);

    /* in FuzzyWuzzy this returns 0. For sake of compatibility return 0 here as well
     * see https://github.com/maxbachmann/RapidFuzz/issues/110 */
    if (s1.empty() || !len2) return 0;

    return cached_ratio.similarity(first2, last2, score_cutoff);
}

template <typename CharT1>
template <typename Sentence2>
double CachedQRatio<CharT1>::similarity(const Sentence2& s2, double score_cutoff,
                                        [[maybe_unused]] double score_hint) const
{
    return similarity(detail::to_begin(s2), detail::to_end(s2), score_cutoff);
}

} // namespace rapidfuzz::fuzz

#endif // RAPIDFUZZ_AMALGAMATED_HPP_INCLUDED