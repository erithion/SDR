#pragma once

#include <cstdint>
#include <format>
#include <vector>
#include <expected>
#include <algorithm>

namespace utils
{
    /**
     * Wraps around data that does not fit into the buffer
     */
    template <typename T>
    struct sliding_buffer
    {
        struct iterator
        {
            using value_type = T;
            using difference_type = std::ptrdiff_t;
            using pointer = value_type*;
            using reference = value_type&;
            using iterator_category = std::forward_iterator_tag;

            iterator(sliding_buffer* p, size_t idx)
                : parent_(p), idx_(idx) {}

            reference operator*() const { return parent_->operator[](idx_); }
            pointer operator->() const { return &parent_->operator[](idx_); }

            iterator& operator++() // Pre-increment operator
            {
                idx_++;
                return *this;
            }
        
            iterator operator++(int) // Post-increment operator
            {
                iterator temp = *this;
                operator++();
                return temp;
            }
        
            bool operator==(const iterator& other) const { return idx_ == other.idx_; }
            bool operator!=(const iterator& other) const { return !(*this == other); }
        
        private:
            sliding_buffer* parent_;
            size_t          idx_;
        };

        explicit sliding_buffer(size_t size)
            : data_(size, 0)
            , cur_{0} {}

        iterator begin() { return iterator(this, 0); }
        iterator end() { return iterator(this, data_.size()); }
        
        std::expected<T, std::string> at(typename std::vector<T>::size_type pos) const
        {
            if (pos >= data_.size())
                return std::unexpected(std::format("The pos={} exceeds size={}", pos, data_.size()));
    
            return operator[](pos);
        }

        T& operator[](typename std::vector<T>::size_type pos)
        {
            const auto& r = static_cast<const sliding_buffer&>(*this)[pos];
            return const_cast<T&>(r); // cast away constness
        }

        const T& operator[](typename std::vector<T>::size_type pos) const
        {
            return data_[(cur_ + pos) % data_.size()];
        }

        template <typename It>
        void push_back(It begin, It end)
        {
            using num_t = typename std::iterator_traits<It>::difference_type;
            num_t free = data_.size() - cur_;
            num_t len = std::distance(begin, end);
            auto  it = std::copy_n(begin, std::min(free, len), data_.begin() + cur_);
            if (it == data_.end())
                it = std::copy(begin + std::min(free, len), end, data_.begin());
            cur_ = std::distance(data_.begin(), it);
        }

        void push_back(const T& val)
        {
            std::initializer_list<T> rg = {val};
            push_back(rg.begin(), rg.end());
        }

        size_t size() const
        {
            return data_.size();
        }
    private:
        
        std::vector<T>  data_;
        size_t          cur_;
    };
}