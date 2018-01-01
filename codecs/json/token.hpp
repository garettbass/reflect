#pragma once
#include <iostream>

namespace reflect::codecs::json {

    enum class token : unsigned {
        undefined,
        null,
        boolean,
        number,
        string,
        array_head,
        array_tail,
        object_head,
        object_tail,
        property,
    };

    std::ostream& operator<<(std::ostream& o, token t) {
        switch (t) {
            case token::undefined:   return o << "undefined";
            case token::null:        return o << "null";
            case token::boolean:     return o << "boolean";
            case token::number:      return o << "number";
            case token::string:      return o << "string";
            case token::array_head:  return o << "array_head";
            case token::array_tail:  return o << "array_tail";
            case token::object_head: return o << "object_head";
            case token::object_tail: return o << "object_tail";
            case token::property:    return o << "property";
        }
        return o << "<invalid token " << unsigned(t) << ">";
    }

} // namespace reflect::codecs::json
