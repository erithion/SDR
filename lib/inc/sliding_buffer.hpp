#pragma once

#include <cstdint>
#include <format>
#include <vector>
#include <expected>
#include <algorithm>

namespace utils
{
    template <typename T>
    struct sliding_buffer
    {
        explicit sliding_buffer(size_t size)
            : data_(size, 0)
            , cur_{0} {}
        
        const T* data() const 
        {
            return data_.data();
        }

        std::expected<T, std::string> operator[](typename std::vector<T>::size_type pos) const
        {
            if (pos >= data_.size())
                return std::unexpected(std::format("The pos={} exceeds size={}", pos, data_.size()));
    
            return data_[(cur_ + pos) % data_.size()];
        }

        template <typename It>
        std::expected<void, std::string> push_back(It begin, It end)
        {
            using num_t = typename std::iterator_traits<It>::difference_type;
            num_t left = data_.size() - cur_;
            num_t n = std::distance(begin, end);
            auto r = std::copy_n(begin, std::min(left, n), data_.begin() + cur_);
            cur_ = std::distance(data_.begin(), r);
            if (r == data_.end())
                r = std::copy(begin + std::min(left, n), end, data_.begin());
            cur_ = std::distance(data_.begin(), r);
            return {};
        }

        std::expected<void, std::string> push_back(const T& val)
        {
            std::initializer_list<T> rg = {val};
            return push_back(rg.begin(), rg.end());
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