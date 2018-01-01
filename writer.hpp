#pragma once
#include <ostream>
#include <vector>
#include "interface.hpp"
#include "substring.hpp"

namespace reflect {

    class writer : interface {

        struct null_writer;

    public:

        static writer* const null;

        virtual explicit operator bool() const = 0;

        virtual size_t offset() const = 0;

        void write(char c) { write(&c,1); }

        void write(substring s) { write(s.begin(),s.size()); }

        virtual void write(const char*,size_t) = 0;

    };

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    struct writer::null_writer final : writer {

        static writer* instance() {
            static null_writer _instance;
            return &_instance;
        }

        explicit operator bool() const override { return false; }

        size_t offset() const override { return 0; }

        void write(const char*, size_t) override {}
    };

    inline writer* const writer::null = writer::null_writer::instance();

    //--------------------------------------------------------------------------

    class stream_writer final : public writer {
        using pos_type = std::istream::pos_type;
        using off_type = std::istream::off_type;
        std::ostream* const _stream = nullptr;
        const pos_type _head {};

    public: // structors

        stream_writer() = default;

        stream_writer(std::ostream& stream)
        :_stream(&stream)
        ,_head(stream.tellp()) {}

    public: // overrides

        explicit operator bool() const override {
            return _stream and _stream->good();
        }

        size_t offset() const override {
            return operator bool() ? size_t(_stream->tellp()-_head) : 0;
        }

        void write(const char* s, size_t n) override {
            if (operator bool()) _stream->write(s,n);
        }

    };

    //--------------------------------------------------------------------------

    template<class Allocator = std::allocator<char>>
    class vector_writer final : public writer {
        std::vector<char,Allocator>* const _vector = nullptr;
        const size_t _head = 0;
        const size_t _offset = 0;

    public: // types

        using vector_type = decltype(_vector);

    public: // structors

        vector_writer() = default;

        vector_writer(vector_type& vector)
        :_vector(&vector)
        ,_head(vector.size()) {}

    public: // overrides

        explicit operator bool() const override {
            return _vector and _vector->size() < _vector->max_size();
        }

        size_t offset() const override {
            return _offset;
        }

        void write(const char* s, size_t n) override {
            if (operator bool()) {
                const auto start = _vector->size();
                const auto size = start + n;
                _vector->resize(size+1,0);
                char* itr = _vector->data() + start;
                const char* const end = itr + size;
                for (;itr < end; *itr++=*s++);
                reflect_assert(_vector->back()=='\0');
                _vector->resize(size);
            }
        }

    };

} // namespace reflect