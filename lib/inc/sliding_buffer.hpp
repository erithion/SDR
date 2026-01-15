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
        explicit sliding_buffer(size_t size)
            : data_(size, 0)
            , cur_{0} {}
        
        std::expected<T, std::string> operator[](typename std::vector<T>::size_type pos) const
        {
            if (pos >= data_.size())
                return std::unexpected(std::format("The pos={} exceeds size={}", pos, data_.size()));
    
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