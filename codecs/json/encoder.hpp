#pragma once
#include <sstream>
#include "preferences.hpp"
#include "../../assert.hpp"
#include "../../writer.hpp"

namespace reflect::codecs::json {

    class encoder {

        writer* const _writer = writer::null;

        enum scope { root, array, object, property } _scope = root;

        unsigned _scope_depth = 0;

        unsigned _scope_size = 0;

        const preferences _prefs;

    public: // structors

        encoder() = default;

        encoder(writer& writer, preferences prefs={})
        :_writer(&writer)
        ,_prefs(prefs) {}

    public: // encoding

        template<typename T>
        void operator()(const T& in) {
            return write_value(in);
        }

        template<typename T>
        void operator()(substring key, const T& in) {
            return write_property(key, in);
        }

    private: // writing

        void write_null() {
            _writer->write("null");
        }

        template<typename T>
        void write_boolean(const T& in) {
            _writer->write(bool(in)?"true":"false");
        }

        template<typename T>
        void write_number(const T& in) {
            enum { size = 32 };
            char buffer[32] {0};
            _writer->write(format_number(buffer,in));
        }

        template<typename T>
        void write_string(const T& in) {
            _writer->write('\"');
            for (const char c : in) {
                if (auto escaped = escape(c)) {
                    _writer->write(escaped);
                } else {
                    _writer->write(c);
                }
            }
            _writer->write('\"');
        }

        template<typename T>
        void write_property(substring key, const T& in) {
            reflect_assert(_scope == object);
            write_separator();
            write_string(key);
            write_colon();
            const auto previous_scope = _scope;
            const auto previous_scope_size = _scope_size;
            _scope = property;
            _scope_size = 0;
            write_value(in);
            _scope = previous_scope;
            _scope_size = previous_scope_size;
        }

        template<typename T>
        void write_value(const T& in) {
            reflect_assert(_scope != object);
            write_separator();
            if constexpr(is_boolean_v<T>) {
                return write_boolean(in);
            }
            if constexpr(is_number_v<T>) {
                return write_number(in);
            }
            if constexpr(is_string_v<T>) {
                return write_string(in);
            }
            if constexpr(is_array_v<T>) {
                return write_array(in);
            }
            if constexpr(is_object_v<T>) {
                return write_object(in);
            }
        }

        template<typename T>
        void write_array(const T& in) {
            write_aggregate<array,'[',']'>(in);
        }

        template<typename T>
        void write_object(const T& in) {
            write_aggregate<object,'{','}'>(in);
        }

        template<scope Scope, char Head, char Tail, typename T>
        void write_aggregate(const T& in) {
            const auto previous_scope = _scope;
            const auto previous_scope_size = _scope_size;
            _scope = Scope;
            _scope_size = 0;
            _writer->write(Head);
            _scope_depth += 1;
            encode<T>(*this,in);
            _scope_depth -= 1;
            if (_scope_size) {
                if (_prefs.trailing_comma) {
                    write_comma();
                }
                write_newline();
                write_indent();
            }
            _writer->write(Tail);
            _scope = previous_scope;
            _scope_size = previous_scope_size;
            if (_scope_depth == 0 and _prefs.newline_at_eof) {
                write_newline();
            }
        }

    private: // layout

        void write_colon() {
            const auto colon = _prefs.colon;
            if (colon and colon[0]) {
                _writer->write(colon);
            }
        }

        void write_comma() {
            const auto comma = _prefs.comma;
            if (comma and comma[0]) {
                _writer->write(comma);
            }
        }

        void write_indent() {
            const auto indent = _prefs.indent;
            if (indent and indent[0]) {
                auto d = _scope_depth;
                while (d-->0) _writer->write(indent);
            }
        }

        void write_newline() {
            const auto newline = _prefs.newline;
            if (newline and newline[0]) {
                _writer->write(newline);
            }
        }

        void write_separator() {
            if (_scope == object or _scope == array) {
                if (_scope_size++) {
                    write_comma();
                }
                write_newline();
                write_indent();
            }
        }

    private: // conversion

        template<typename T, size_t Size>
        const char*
        format_number(char (&buffer)[Size], const T& in) {
            if constexpr(std::is_same_v<T,unsigned short>) {
                snprintf(buffer,sizeof(buffer),"%hu",in);
            }
            if constexpr(std::is_same_v<T,unsigned int>) {
                snprintf(buffer,sizeof(buffer),"%u",in);
            }
            if constexpr(std::is_same_v<T,unsigned long>) {
                snprintf(buffer,sizeof(buffer),"%lu",in);
            }
            if constexpr(std::is_same_v<T,unsigned long long>) {
                snprintf(buffer,sizeof(buffer),"%llu",in);
            }
            if constexpr(std::is_same_v<T,signed short>) {
                snprintf(buffer,sizeof(buffer),"%hi",in);
            }
            if constexpr(std::is_same_v<T,signed int>) {
                snprintf(buffer,sizeof(buffer),"%i",in);
            }
            if constexpr(std::is_same_v<T,signed long>) {
                snprintf(buffer,sizeof(buffer),"%li",in);
            }
            if constexpr(std::is_same_v<T,signed long long>) {
                snprintf(buffer,sizeof(buffer),"%lli",in);
            }
            if constexpr(std::is_same_v<T,float>) {
                switch (_prefs.float_format) {
                    default:
                    case float_format::concise:
                        snprintf(buffer,sizeof(buffer),"%g",in);
                        break;
                    case float_format::precise:
                        snprintf(buffer,sizeof(buffer),"%.9g",in);
                        break;
                }
            }
            if constexpr(std::is_same_v<T,double>) {
                switch (_prefs.float_format) {
                    default:
                    case float_format::concise:
                        snprintf(buffer,sizeof(buffer),"%g",in);
                        break;
                    case float_format::precise:
                        snprintf(buffer,sizeof(buffer),"%.17g",in);
                        break;
                }
            }
            if constexpr(std::is_same_v<T,long double>) {
                switch (_prefs.float_format) {
                    default:
                    case float_format::concise:
                        snprintf(buffer,sizeof(buffer),"%Lg",in);
                        break;
                    case float_format::precise:
                        snprintf(buffer,sizeof(buffer),"%.17Lg",in);
                        break;
                }
            }
            return buffer;
        }

        static const char* escape(const char c) {
            switch (c) {
                case'\x00': return R"(\u0000)";
                case'\x01': return R"(\u0001)";
                case'\x02': return R"(\u0002)";
                case'\x03': return R"(\u0003)";
                case'\x04': return R"(\u0004)";
                case'\x05': return R"(\u0005)";
                case'\x06': return R"(\u0006)";
                case'\x07': return R"(\u0007)";
                case'\x08': return R"(\b)";
                case'\x09': return R"(\t)";
                case'\x0A': return R"(\n)";
                case'\x0B': return R"(\u000B)";
                case'\x0C': return R"(\f)";
                case'\x0D': return R"(\r)";
                case'\x0E': return R"(\u000E)";
                case'\x0F': return R"(\u000F)";
                case'\x10': return R"(\u0010)";
                case'\x11': return R"(\u0011)";
                case'\x12': return R"(\u0012)";
                case'\x13': return R"(\u0013)";
                case'\x14': return R"(\u0014)";
                case'\x15': return R"(\u0015)";
                case'\x16': return R"(\u0016)";
                case'\x17': return R"(\u0017)";
                case'\x18': return R"(\u0018)";
                case'\x19': return R"(\u0019)";
                case'\x1A': return R"(\u001A)";
                case'\x1B': return R"(\u001B)";
                case'\x1C': return R"(\u001C)";
                case'\x1D': return R"(\u001D)";
                case'\x1E': return R"(\u001E)";
                case'\x1F': return R"(\u001F)";
                case'\x22': return R"(\")";
                case'\x5C': return R"(\\)";
                case'\x7F': return R"(\u007F)";
                default   : return nullptr;
            }
        }

    private: // predicates

        static int is_control(const int c) {
            return ((c <= 0x1F)|(c == 0x7F));
        }

    };

} // namespace reflect::codecs::json
