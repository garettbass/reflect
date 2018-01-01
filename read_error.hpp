#pragma once
#include <iostream>
#include "reader.hpp"

namespace reflect {

    class read_error final {
        reader* _reader = reader::null;
        const char* _message = nullptr;
        size_t _offset = 0;
        size_t _size = 0;

    public: // structors

        read_error() = default;

        read_error(
            reader& reader,
            const char* message,
            size_t offset,
            size_t size)
        :_reader(&reader)
        ,_message(message)
        ,_offset(offset)
        ,_size(size) {}

    public: // operators

        explicit operator bool() const { return _message != nullptr; }

    public: // properties

        reader& reader() const { return *_reader; }

        const char* message() const { return _message; }

        size_t offset() const { return _offset; }

        size_t size() const { return _size; }
    };

    std::ostream& operator<<(std::ostream& o, const read_error& e) {
        if (e) {
            auto& reader = e.reader();
            const size_t offset = e.offset();
            size_t line = 1, column = 0;
            reader.seek(0);
            while (reader.offset() < offset) {
                const char c = reader.read();
                const bool is_newline = c == '\n';
                column = (column + 1) * not is_newline;
                line += is_newline;
            }
            o << line << ":" << column << ": " << e.message();
            const size_t size = e.size();
            if (size > 0) {
                o << ": \"" << reader.peek(offset,size) << "\"";
            }
        } else {
            o << "OK";
        }
        return o;
    }

} // namespace reflect