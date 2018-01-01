#pragma once
#include <istream>
#include "interface.hpp"
#include "substring.hpp"

namespace reflect {

    class reader : interface {

        struct null_reader;

    public:

        static reader* const null;

        virtual explicit operator bool() const = 0;

        virtual size_t offset() const = 0;

        std::string peek(size_t offset, size_t) const;

        void peek(std::vector<char>&, size_t offset, size_t) const;

        virtual char peek() const = 0;

        virtual char read() = 0;

        virtual void seek(size_t offset) = 0;

        virtual size_t size() const = 0;
    };

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    inline std::string
    reader::peek(size_t offset, size_t size) const {
        std::string s;
        reader& r = *const_cast<reader*>(this);
        const auto start = this->offset();
        r.seek(offset);
        s.assign(size+1,0);
        char* itr = s.data();
        const char* const end = itr + size;
        for (;itr < end; *itr++=r.read());
        reflect_assert(s.back()=='\0');
        s.resize(size);
        r.seek(start);
        reflect_assert(r.offset()==start);
        return s;
    }

    inline void
    reader::peek(std::vector<char>& v, size_t offset, size_t size) const {
        reader& r = *const_cast<reader*>(this);
        const auto start = this->offset();
        r.seek(offset);
        v.clear();
        v.resize(size+1,0);
        char* itr = v.data();
        const char* const end = itr + size;
        for (;itr < end; *itr++=r.read());
        reflect_assert(v.back()=='\0');
        v.resize(size);
        r.seek(start);
        reflect_assert(r.offset()==start);
    }

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    struct reader::null_reader final : reader {

        static reader* instance() {
            static null_reader _instance;
            return &_instance;
        }

        explicit operator bool() const override { return false; }

        size_t offset() const override { return 0; }

        char peek() const override { return 0; }

        char read() override { return 0; }

        void seek(size_t offset) override {}

        size_t size() const override { return 0; }

    };

    inline reader* const reader::null = reader::null_reader::instance();

    //--------------------------------------------------------------------------

    class stream_reader final : public reader {
        using pos_type = std::istream::pos_type;
        using off_type = std::istream::off_type;
        std::istream* const _stream = nullptr;
        const pos_type _head;

    public: // structors

        stream_reader() = default;

        stream_reader(std::istream& stream)
        :_stream(&stream)
        ,_head(stream.tellg()) {}

    public: // overrides

        explicit operator bool() const override {
            return _stream and _stream->good();
        }

        size_t offset() const override {
            return operator bool() ? size_t(_stream->tellg()-_head) : 0;
        }

        char peek() const override {
            return operator bool() ? _stream->peek() : 0;
        }

        char read() override {
            return operator bool() ? _stream->get() : 0;
        }

        void seek(size_t offset) override {
            if (operator bool()) _stream->seekg(_head + off_type(offset));
        }

        size_t size() const override {
            if (operator bool()) {
                const auto start = _stream->tellg();
                _stream->seekg(0,std::ios::end);
                const auto end = _stream->tellg();
                _stream->seekg(start);
                return end - _head;
            }
            return 0;
        }

    };

    //--------------------------------------------------------------------------

    class string_reader final : public reader {
        substring _string;
        const char* _itr = nullptr;

    public: // structors

        string_reader() = default;

        string_reader(substring s)
        :_string(s)
        ,_itr(_string.begin()) {}

        template<size_t SIZE>
        string_reader(const char (&s)[SIZE])
        :string_reader(substring(s)) {}

        string_reader(const char* s)
        :string_reader(substring(s)) {}

        string_reader(const std::string& s)
        :string_reader(substring(s)) {}

    public: // overrides

        explicit operator bool() const override {
            return _itr < _string.end();
        }

        size_t offset() const override {
            return size_t(_itr) - size_t(_string.begin());
        }

        char peek() const override {
            return operator bool() ? *_itr : 0;
        }

        char read() override {
            const char c = peek();
            _itr = std::min(_itr+1,_string.end());
            return c;
        }

        void seek(size_t offset) override {
            _itr = std::min(_string.begin()+offset,_string.end());
        }

        size_t size() const override {
            return _string.size();
        }

    };

} // namespace reflect
