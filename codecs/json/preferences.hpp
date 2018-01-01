#pragma once

namespace reflect::codecs::json {

    enum class float_format : char { concise, precise };

    struct preferences {
        const char* colon = ":";
        const char* comma = ",";
        const char* indent = "";
        const char* newline = "";
        bool trailing_comma = false;
        bool newline_at_eof = false;
        float_format float_format = float_format::concise;

        preferences(
            const char* colon = ":",
            const char* comma = ",",
            const char* indent = "",
            const char* newline = "",
            bool trailing_comma = false,
            bool newline_at_eof = false,
            enum float_format float_format = float_format::concise)
        :colon(colon)
        ,comma(comma)
        ,indent(indent)
        ,newline(newline)
        ,trailing_comma(trailing_comma)
        ,newline_at_eof(newline_at_eof)
        ,float_format(float_format) {}
    };

} // namespace reflect::codecs::json
