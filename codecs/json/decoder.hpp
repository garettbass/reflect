#pragma once
#include <vector>
#include <sstream>
#include "token.hpp"
#include "../../assert.hpp"
#include "../../read_error.hpp"

namespace reflect::codecs::json {

    class decoder {

        reader* const _reader = reader::null;

        read_error _error;

        std::vector<char> _utf8;

        std::vector<uint16_t> _utf16;

        std::string _property_key;

    public: // structors

        decoder() = default;

        decoder(reader& reader):_reader(&reader) {}

    public: // validation

        read_error error() const { return _error; }

        read_error validate();

    public: // decoding

        template<typename T>
        bool operator()(T& out) {
            return parse_value(out);
        }

        template<typename T>
        bool operator()(substring key, T& out) {
            return parse_property(key, out);
        }

        template<typename T>
        bool operator()(substring* key, T& out) {
            return parse_property(key, out);
        }

    public: // parsing

        template<typename T>
        bool parse_null(T& out) {
            if (consume_null(no_consumer)) {
                out = nullptr;
                return true;
            }
            if (peek_token() != token::undefined) {
                error("expected null",offset());
            }
            return false;
        }

        template<typename T>
        bool parse_boolean(T& out) {
            if (consume_false(no_consumer)) {
                out = false;
                return true;
            }
            if (consume_true(no_consumer)) {
                out = true;
                return true;
            }
            if (peek_token() != token::undefined) {
                error("expected boolean",offset());
            }
            return false;
        }

        template<typename T>
        bool parse_number(T& out) {
            auto consumer = [&](auto, auto i, auto n){
                const char* const itr = read_string(i,n);
                char* end = nullptr;
                if (count(_utf8,'.')) {
                    out = clamp<T>(strtod(itr,&end));
                    reflect_assert(end > itr);
                    return;
                } else {
                    out = clamp<T>(strtoll(itr,&end,10));
                    reflect_assert(end > itr);
                    return;
                }
            };
            if (consume_number(consumer)) {
                return true;
            }
            if (peek_token() != token::undefined) {
                error("expected number",offset());
            }
            return false;
        }

        template<typename T>
        bool parse_string(T& out) {
            auto consumer = [&](token t, size_t i, size_t n){
                if (t == token::string) {
                    const auto str = unescape_string(i,n);
                    out = T(str);
                    return;
                }
                error("unexpected property",i,n);
            };
            if (consume_string(consumer)) {
                return true;
            }
            if (peek_token() != token::undefined) {
                error("expected string",offset());
            }
            return false;
        }

        template<typename T>
        bool parse_property(substring key, T& out) {
            const auto start = offset();
            bool found = false;
            auto consumer = [&](token t, size_t i, size_t n){
                if (t == token::property) {
                    const auto str = unescape_string(i,n);
                    found = (key == str);
                } else error("expected property",i,n);
            };
            while (consume_string(consumer) and not found) {
                skip_value();
            }
            if (found) {
                parse_value(out);
            }
            seek(start);
            return false;
        }

        template<typename T>
        bool parse_property(substring* key, T& out) {
            std::string property_key;
            auto consumer = [&](token t, size_t i, size_t n){
                if (t == token::property) {
                    const auto str = unescape_string(i,n);
                    property_key = str;
                } else error("expected property",i,n);
            };
            if (consume_string(consumer) and parse_value(out)) {
                *key = _property_key = property_key;
                return true;
            }
            *key = nullptr;
            return false;
        }

        template<typename T>
        bool parse_value(T& out) {
            if constexpr(is_boolean_v<T>) {
                return parse_boolean(out);
            }
            if constexpr(is_number_v<T>) {
                return parse_number(out);
            }
            if constexpr(is_string_v<T>) {
                return parse_string(out);
            }
            if constexpr(is_array_v<T>) {
                return parse_array(out);
            }
            if constexpr(is_object_v<T>) {
                return parse_object(out);
            }
        }

        bool parse_array_head() {
            return consume_array_head(no_consumer);
        }

        bool parse_array_tail() {
            while (peek_token() != token::array_tail and skip_value());
            return consume_array_tail(no_consumer);
        }

        template<typename T>
        bool parse_array(T& out) {
            if (parse_array_head()) {
                decode<T>(*this,out);
                if (parse_array_tail()) {
                    return true;
                }
                error("expected ']'",offset());
            }
            return false;
        }

        bool parse_object_head() {
            return consume_object_head(no_consumer);
        }

        bool parse_object_tail() {
            while (peek_token() != token::object_tail and skip_value());
            return consume_object_tail(no_consumer);
        }

        template<typename T>
        bool parse_object(T& out) {
            if (parse_object_head()) {
                decode<T>(*this,out);
                if (parse_object_tail()) {
                    return true;
                }
                error("expected '}'",offset());
            }
            return false;
        }

    private: // parsing

        static void no_consumer(token,size_t,size_t) {}

        template<typename Consumer>
        void consume(Consumer&& consumer, token t, size_t i) {
            const size_t n = decoder::offset() - i;
            consume(consumer,t,i,n);
        }

        template<typename Consumer>
        void consume(Consumer&& consumer, token t, size_t i, size_t n) {
            const auto start = decoder::offset();
            consumer(t,i,n);
            seek(start);
        }

        void error(const char* message, size_t offset, size_t size = 0) {
            if (_error) return;
            seek(offset);
            _error = read_error{*_reader, message, offset, size};
        }

        size_t offset() const {
            return _reader->offset();
        }

        char peek() const {
            if (_error) return 0;
            return _reader->peek();
        }

        bool peek(const char c) const {
            if (_error) return false;
            return peek() == c;
        }

        bool peek(substring s) {
            if (_error) return false;
            const auto start = offset();
            for (const char c : s) {
                if (not peek(c)) {
                    seek(start);
                    return false;
                }
                _reader->read();
            }
            seek(start);
            return true;
        }

        bool peek(int(*p)(int)) const {
            if (_error) return false;
            const char c = peek();
            return c and p(c);
        }

        char read() {
            if (_error) return 0;
            return _reader->read();
        }

        void seek(size_t offset) {
            if (_error) return;
            return _reader->seek(offset);
        }

        bool skip(const char c) {
            if (_error) return false;
            if (peek(c)) {
                read();
                return true;
            }
            return false;
        }

        bool skip(substring s) {
            if (_error) return false;
            const auto start = offset();
            for (const char c : s) {
                if (not skip(c)) {
                    seek(start);
                    return false;
                }
            }
            return true;
        }

        unsigned skip_while(int(*p)(int)) {
            if (_error) return 0;
            unsigned count = 0;
            while (peek(p)) {
                read();
                count += 1;
            }
            return count;
        }

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        bool skip_comment() {
            return bool{
                skip_comment_block() or
                skip_comment_line()
            };
        }

        bool skip_comment_block() {
            const auto start = offset();
            if (skip("/*")) {
                while (not peek("*/") and read());
                if (skip("*/")) {
                    return true;
                }
            }
            seek(start);
            return false;
        }

        bool skip_comment_line() {
            const auto start = offset();
            if (skip("//")) {
                while (not peek('\n') and read());
                return true;
            }
            seek(start);
            return false;
        }

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        template<typename Consumer>
        bool consume_number(Consumer&& consumer) {
            const auto start = offset();
            if (skip_number_integer()) {
                skip_number_fraction();
                skip_number_exponent();
                consume(consumer,token::number,start);
                skip_comma();
                return true;
            }
            seek(start);
            return false;
        }

        bool skip_number_digits() {
            return skip_while(is_digit) > 0;
        }

        bool skip_number_exponent() {
            const auto start = offset();
            if (skip('e') or skip('E')) {
                skip('-') or skip('+');
                if (skip_number_digits()) {
                    return true;
                }
                const auto size = offset() - start;
                error("invalid exponent",start,size);
                return false;
            }
            seek(start);
            return false;
        }

        bool skip_number_fraction() {
            const auto start = offset();
            if (skip('.')) {
                if (skip_number_digits()) {
                    return true;
                }
                error("invalid fraction",start,1);
                return false;
            }
            seek(start);
            return false;
        }

        bool skip_number_integer() {
            const auto start = offset();
            if (skip_number_digits()) {
                return true;
            }
            if (skip('-')) {
                if (skip_number_digits()) {
                    return true;
                }
                error("invalid integer",start,1);
                return false;
            }
            seek(start);
            return false;
        }

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        template<typename Consumer>
        bool consume_string(Consumer&& consumer) {
            const auto start = offset();
            if (skip('"')) {
                char c;
                while ((c = read()) != '"') {
                    if (is_control(c)) {
                        error("invalid character",start);
                        return false;
                    }
                    if (c == '\\') {
                        const char e = read();
                        switch (e) {
                            case '"': continue;
                            case'\\': continue;
                            case '/': continue;
                            case 'b': continue;
                            case 'f': continue;
                            case 'n': continue;
                            case 'r': continue;
                            case 't': continue;
                            case 'u': {
                                if (skip_string_hex_quad())
                                    continue;
                            }
                        }
                        error("invalid escape sequence",offset()-2,2);
                        return false;
                    }
                }
                const auto size = offset() - start;
                skip_whitespace();
                if (skip(':')) {
                    consume(consumer,token::property,start,size);
                } else {
                    consume(consumer,token::string,start,size);
                    skip_comma();
                }
                return true;
            }
            seek(start);
            return false;
        }

        void unescape_string() {
            reflect_assert(_utf8.front()=='"');
            reflect_assert(_utf8.back()=='"');
            _utf8.pop_back();
            _utf8.erase(_utf8.begin());
            for (size_t i = 0; i < _utf8.size(); ++i) {
                const auto itr = _utf8.begin()+1;
                if (*itr != '\\') continue;
                _utf8.erase(itr);
                switch (*(itr+1)) {
                    case '"':
                    case'\\':
                    case '/': continue;
                    case 'b': *itr='\b'; continue;
                    case 'f': *itr='\f'; continue;
                    case 'n': *itr='\n'; continue;
                    case 'r': *itr='\r'; continue;
                    case 't': *itr='\t'; continue;
                    case 'u': continue; // TODO: utf16 to utf8
                }
            }
            _utf8.push_back(0);
            _utf8.pop_back();
        }

        const char* unescape_string(size_t offset, size_t size) {
            read_string(offset,size);
            unescape_string();
            return _utf8.data();
        }

        const char* read_string(size_t offset, size_t size) {
            _reader->peek(_utf8,offset,size);
            return _utf8.data();
        }

        bool skip_string_hex_quad() {
            const auto start = offset();
            if (skip_while(is_hex) == 4) {
                return true;
            }
            seek(start);
            return false;
        }

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        template<typename Consumer>
        bool consume_array_head(Consumer&& consumer) {
            const auto start = offset();
            if (skip('[')) {
                consume(consumer,token::array_head,start);
                skip_whitespace();
                return true;
            }
            seek(start);
            return false;
        }

        template<typename Consumer>
        bool consume_array_tail(Consumer&& consumer) {
            const auto start = offset();
            if (skip(']')) {
                consume(consumer,token::array_tail,start);
                skip_comma();
                return true;
            }
            return false;
        }

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        template<typename Consumer>
        bool consume_object_head(Consumer&& consumer) {
            const auto start = offset();
            if (skip('{')) {
                consume(consumer,token::object_head,start);
                skip_whitespace();
                return true;
            }
            seek(start);
            return false;
        }

        template<typename Consumer>
        bool consume_object_tail(Consumer&& consumer) {
            const auto start = offset();
            if (skip('}')) {
                consume(consumer,token::object_tail,start);
                skip_comma();
                return true;
            }
            return false;
        }

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        template<typename Consumer>
        bool consume_false(Consumer&& consumer) {
            const auto start = offset();
            if (skip("false")) {
                consume(consumer,token::boolean,start);
                skip_comma();
                return true;
            }
            error("expected 'false'",start);
            return false;
        }

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        template<typename Consumer>
        bool consume_true(Consumer&& consumer) {
            const auto start = offset();
            if (skip("true")) {
                consume(consumer,token::boolean,start);
                skip_comma();
                return true;
            }
            error("expected 'true'",start);
            return false;
        }

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        template<typename Consumer>
        bool consume_null(Consumer&& consumer) {
            const auto start = offset();
            if (skip("null")) {
                consume(consumer,token::null,start);
                skip_comma();
                return true;
            }
            error("expected 'null'",start);
            return false;
        }

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        bool skip_value() {
            const auto start = offset();
            int depth = 0;
            auto consumer = [&](token t, size_t i, size_t n) {
                depth += (t==token::array_head)|(t==token::object_head);
                depth -= (t==token::array_tail)|(t==token::object_tail);
            };
            if (consume_token(consumer)) {
                while (depth > 0 and consume_token(consumer));
                return true;
            }
            seek(start);
            return false;
        }

        token peek_token() {
            skip_whitespace();
            const char c = peek();
            switch (c) {
                case 0 : return token::undefined;
                case'n': return token::null;
                case'f': return token::boolean;
                case't': return token::boolean;
                case'0':
                case'1':
                case'2':
                case'3':
                case'4':
                case'5':
                case'6':
                case'7':
                case'8':
                case'9':
                case'-': return token::number;
                case'"': return token::string;
                case'[': return token::array_head;
                case']': return token::array_tail;
                case'{': return token::object_head;
                case'}': return token::object_tail;
                default: {
                    error("invalid character",offset(),1);
                    return token::undefined;
                }
            }
        }

        template<typename Consumer>
        bool consume_token(Consumer&& consumer) {
            const auto start = offset();
            skip_whitespace();
            const char c = peek();
            switch (c) {
                case 0 : return false;
                case'n': return consume_null(consumer);
                case'f': return consume_false(consumer);
                case't': return consume_true(consumer);
                case'0':
                case'1':
                case'2':
                case'3':
                case'4':
                case'5':
                case'6':
                case'7':
                case'8':
                case'9':
                case'-': return consume_number(consumer);
                case'"': return consume_string(consumer);
                case'[': return consume_array_head(consumer);
                case']': return consume_array_tail(consumer);
                case'{': return consume_object_head(consumer);
                case'}': return consume_object_tail(consumer);
                default: {
                    error("invalid character",offset(),1);
                    return false;
                }
            }
            seek(start);
            return false;
        }

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        bool skip_comma() {
            skip_whitespace();
            if (skip(',')) {
                skip_whitespace();
                return true;
            }
            switch(peek()) {
                case 0 :
                case']':
                case'}': return true;
                case'n':
                case'f':
                case't':
                case'0':
                case'1':
                case'2':
                case'3':
                case'4':
                case'5':
                case'6':
                case'7':
                case'8':
                case'9':
                case'-':
                case'"':
                case'[':
                case'{': {
                    error("missing ','",offset());
                    return false;
                }
                default: {
                    error("invalid character",offset(),1);
                    return false;
                }
            }
        }

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        void skip_whitespace() {
            while (skip_comment() or skip_while(is_space));
        }

    private: // conversion

        static constexpr char16_t _hex_to_int(uint8_t u) {
            return ((u >> 6) * 9) + (u & 0xF);
        }

        static char16_t hex_to_int(uint8_t u) {
            static_assert(_hex_to_int('0') == 0);
            static_assert(_hex_to_int('1') == 1);
            static_assert(_hex_to_int('2') == 2);
            static_assert(_hex_to_int('3') == 3);
            static_assert(_hex_to_int('4') == 4);
            static_assert(_hex_to_int('5') == 5);
            static_assert(_hex_to_int('6') == 6);
            static_assert(_hex_to_int('7') == 7);
            static_assert(_hex_to_int('8') == 8);
            static_assert(_hex_to_int('9') == 9);

            static_assert(_hex_to_int('A') == 10);
            static_assert(_hex_to_int('B') == 11);
            static_assert(_hex_to_int('C') == 12);
            static_assert(_hex_to_int('D') == 13);
            static_assert(_hex_to_int('E') == 14);
            static_assert(_hex_to_int('F') == 15);

            static_assert(_hex_to_int('a') == 10);
            static_assert(_hex_to_int('b') == 11);
            static_assert(_hex_to_int('c') == 12);
            static_assert(_hex_to_int('d') == 13);
            static_assert(_hex_to_int('e') == 14);
            static_assert(_hex_to_int('f') == 15);

            reflect_assert(is_hex(u));
            return _hex_to_int(u);
        };

    private: // predicates

        static int is_digit(const int c) {
            return ((c >= int('0'))&(c <= int('9')));
        }

        static int is_control(const int c) {
            return ((c <= 0x1F)|(c == 0x7F));
        }

        static int is_hex(const int c) {
            return ((c >= int('0'))&(c <= int('9')))
                 | ((c >= int('A'))&(c <= int('F')))
                 | ((c >= int('a'))&(c <= int('f')));
        }

        static int is_space(const int c) {
            enum:int {N='\n',R='\r',S=' ',T='\t'};
            return ((c == N)|(c == R)|(c == S)|(c == T));
        }

    private: // utility

        template<typename T>
        static size_t count(const std::vector<T>& v, T t) {
            size_t count = 0;
            for (auto& e : v) count += (e == t);
            return count;
        }

        template<typename T, typename U>
        static T clamp(U in, U min, U max) {
            if (in < min) return T(min);
            if (in > max) return T(max);
            return T(in);
        }

        template<typename T, typename U>
        static T clamp(U in) {
            const U min = U(std::numeric_limits<T>::min());
            const U max = U(std::numeric_limits<T>::max());
            return clamp<T,U>(in,min,max);
        }

    };

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    #ifndef reflect_codecs_json_decoder_validate_debug
    #define reflect_codecs_json_decoder_validate_debug 0
    #endif

    inline read_error decoder::validate() {
        const auto start = offset();
        enum scope { root, array, object, property };
        std::vector<scope> stack {root};

        #if reflect_codecs_json_decoder_validate_debug
        unsigned depth = 0;
        #endif // reflect_codecs_json_decoder_validate_debug

        auto consumer = [&](token t, size_t i, size_t n){
            const bool indent(stack.back() and stack.back()!=property);
            switch (t) {
                case token::array_head: {
                    if (stack.back() == object)
                        return error("expected property",i);
                    stack.push_back(array);
                } break;
                case token::array_tail: {
                    if (stack.back() != array)
                        return error("invalid character",i,1);
                    stack.pop_back();
                    if (stack.back() == property)
                        stack.pop_back();
                } break;
                case token::object_head: {
                    if (stack.back() == object)
                        return error("expected property",i);
                    stack.push_back(object);
                } break;
                case token::object_tail: {
                    if (stack.back() != object)
                        return error("invalid character",i,1);
                    stack.pop_back();
                    if (stack.back() == property)
                        stack.pop_back();
                } break;
                case token::property: {
                    if (stack.back() != object)
                        return error("unexpected property",i,n);
                    stack.push_back(property);
                } break;
                default: {
                    if (stack.back() == object)
                        return error("expected property name",i);
                    if (stack.back() == property)
                        stack.pop_back();
                } break;
            }

            #if reflect_codecs_json_decoder_validate_debug
            depth += (t==token::array_head)|(t==token::object_head);
            depth -= (t==token::array_tail)|(t==token::object_tail);
            if (indent) {
                for (auto i=0u; i < depth; ++i) std::cout << "    ";
            }
            std::cout << read_string(i,n);
            if (t == token::property) {
                std::cout<<":";
            } else {
                std::cout<<"\n";
            }
            #endif // reflect_codecs_json_decoder_validate_debug
        };
        while (consume_token(consumer));
        seek(start);
        return error();
    }


} // namespace reflect::codecs::json
