#pragma once
#include <iostream>
#include <string>

namespace reflect {

    class substring {
        const char* const _head = "";
        const size_t      _size = 0;

    public: // types

        using this_t = substring;
        using const_t = const substring;

    public: // structors

        substring() = default;

        substring(decltype(nullptr)) {}

        substring(const char* s, size_t len)
        : _head(s?s:""), _size(s?len:0) {}

        template<size_t SIZE>
        substring(const char (&s)[SIZE])
        : substring(s, s ? strlen(s) : 0) {}

        substring(const char* s)
        : substring(s, s ? strlen(s) : 0) {}

        substring(const std::string& s)
        : substring(s.c_str(), s.length()) {}

        substring(const_t& src) = default;
        substring& operator =(const_t& s) {
            return (this == &s) ? *this : *new(this) this_t(s);
        }

    public: // operators

        bool operator ==(const_t& s) const { return compare(s) == 0; }
        bool operator !=(const_t& s) const { return compare(s) != 0; }
        bool operator  <(const_t& s) const { return compare(s)  < 0; }
        bool operator <=(const_t& s) const { return compare(s) <= 0; }
        bool operator  >(const_t& s) const { return compare(s)  > 0; }
        bool operator >=(const_t& s) const { return compare(s) >= 0; }

        char operator [](size_t index) const {
            return (index < _size) ? _head[index] : '\0';
        }

        template<typename T> explicit operator T() const;

    public: // properties
        const char* data() const { return _head; }
        bool       empty() const { return _size == 0; }
        size_t      size() const { return _size; }

    public: // iterators
        const char* begin() const { return _head; }
        const char*   end() const { return _head + _size; }

    public: // queries
        int compare(const_t&) const;

        size_t hash(uint32_t seed = 0) const;

        bool has_prefix(const_t&) const;
        bool has_suffix(const_t&) const;

    public: // transforms

        const substring prefix(size_t length) const;
        const substring suffix(size_t length) const;

        const substring seek(substring) const;
        const substring seek_end(substring) const;

        const substring skip(substring) const;
        const substring skip(size_t length) const;

        const substring truncate(substring) const;
        const substring truncate(size_t length) const;
    };


    template<>
    substring::operator std::string() const { return {data(),size()}; }


    //--------------------------------------------------------------------------

    inline
    std::string
    operator +(const std::string& a, const substring& b) {
        std::string c = a; c.append(b.begin(), b.size());
        return c;
    }

    //--------------------------------------------------------------------------

   std::ostream& operator << (std::ostream& o, const substring& s) {
       o.write(s.begin(), s.size());
       return o;
   }

    //--------------------------------------------------------------------------

    // inline
    // size_t
    // substring::hash(uint32_t seed) const {
    //     return _size ? size_t(hash64(_head, _size, seed)) : 0;
    // }

    //--------------------------------------------------------------------------

    inline
    int
    substring::compare(const_t& s) const {
        if (this == &s)
            return 0;

        const size_t a_len =   _size;
        const size_t b_len = s._size;
        if (a_len == 0 and b_len == 0)
            return 0;

        // skip equivalent chars up to last char
        const char* a =   _head;
        const char* b = s._head;
        const char* const a_last = (a + a_len) - 1;
        const char* const b_last = (b + b_len) - 1;
        while (a < a_last and b < b_last and *a == *b) { ++a; ++b; }

        if (const int d_char = int(*a) - int(*b))
            return (d_char < 0) ? -1 : +1;

        const ptrdiff_t d_len = ptrdiff_t(a_len) - ptrdiff_t(b_len);
        return (d_len < 0) ? -1 : (d_len > 0);
    }

    inline
    bool
    substring::has_prefix(const_t& p) const {
        return p == prefix(p._size);
    }

    inline
    bool
    substring::has_suffix(const_t& s) const {
        return s == suffix(s._size);
    }

    //--------------------------------------------------------------------------

    inline
    const substring
    substring::prefix(size_t length) const {
        length = std::min(_size, length);
        return substring(_head, length);
    }

    inline
    const substring
    substring::suffix(size_t length) const {
        length = std::min(_size, length);
        return substring(_head + _size - length, length);
    }

    inline
    const substring
    substring::seek(substring p) const {
        for (this_t s = *this; s.size(); s = s.skip(1)) {
            if (s.has_prefix(p)) return s;
        }
        return {};
    }

    inline
    const substring
    substring::seek_end(substring p) const {
        return seek(p).skip(p);
    }

    inline
    const substring
    substring::skip(substring p) const {
        return has_prefix(p) ? skip(p._size) : *this;
    }

    inline
    const substring
    substring::skip(size_t length) const {
        if (length >= _size) return {};
        return substring(_head + length, _size - length);
    }

    inline
    const substring
    substring::truncate(substring s) const {
        return has_suffix(s) ? truncate(s._size) : *this;
    }

    inline
    const substring
    substring::truncate(size_t length) const {
        if (length >= _size) return {};
        return substring(_head, _size - length);
    }

} // namespace reflect