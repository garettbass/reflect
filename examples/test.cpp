////usr/bin/env $(dirname $0)/cxx -c++17 -r $0; exit $?
#include <cstdio>
#include <iostream>
#include <vector>
#include <map>
#include "echo.hpp"
#include "reflect.hpp"
#include "reflect.std.map.hpp"
#include "reflect.std.unordered_map.hpp"
#include "reflect.std.vector.hpp"
#include "codecs/json/decoder.hpp"
#include "codecs/json/encoder.hpp"

template<typename T>
struct vector3 { T x, y, z; };

reflect_template((typename T),(vector3<T>)) {
    reflect("x",value.x);
    reflect("y",value.y);
    reflect("z",value.z);
}

struct foo {
    reflect_fields(
        ((std::string),string),
        ((vector3<float>),vector),
        ((std::vector<vector3<float>>),array),
        ((std::map<int,int>),intmap),
        ((std::unordered_map<std::string,std::string>),strmap)
    )
};

int main(const int argc, const char* argv[]) {
    reflect::stream_writer writer(std::cout);
    reflect::codecs::json::preferences prefs;
    prefs.indent = "    ";
    prefs.newline = "\n";
    prefs.trailing_comma = true;
    prefs.newline_at_eof = true;

    vector3<float> v {1,2,3};
    reflect::codecs::json::encoder(writer,prefs)(v);

    foo f {
        /*string*/ "hello",
        /*vector*/ v,
        /*array*/  {{1,2,3},{4,5,6}},
        /*intmap*/ {{1,1},{2,2}},
        /*strmap*/ {{"apple","pear"},{"orang","utan"}},
    };
    reflect::codecs::json::encoder(writer,prefs)(f);

    {
        const char json[] = R"({
            "null":null,
            "false":false,
            "true":true,
            "int":123,
            "double":123.456,
            "string":"string",
            "array":[
                null,
                false,
                true,
                123,
                123.456,
                "string", // optional trailing comma
            ],
        })";
        reflect::string_reader r(json);
        echo(r.size());
        reflect::codecs::json::decoder d(r);
        d.validate();
        std::cout << d.error() << "\n";
    }

    {
        const char vector_json[] = R"({
            "x":4,
            "y":5,
            "z":6
        })";
        reflect::string_reader vector_reader(vector_json);
        echo(vector_reader.size());
        reflect::codecs::json::decoder vector_decoder(vector_reader);
        std::cout << vector_decoder.validate() << "\n";
        echo(vector_decoder(v));
        reflect::codecs::json::encoder(writer,prefs)(v);
    }

    {
        const char foo_json[] = R"({
            "string":"hello",
            "vector":{
                "w":0, // ignored
                "y":8, // out of order, ok!
                "x":7, // out of order, ok!
                "z":9,
            },
            "array":[
                {
                    "x":11,
                    "y":12,
                    "z":13,
                },
                {
                    "x":14,
                    "y":15,
                    "z":16,
                },
            ],
            "intmap":{
                "3":3,
                "4":4,
            },
            "strmap":{
                "fizzle":"whizzle",
                "baz":"que",
            },
        })";
        reflect::string_reader foo_reader(foo_json);
        echo(foo_reader.size());
        reflect::codecs::json::decoder foo_decoder(foo_reader);
        std::cout <<"foo_json:" << foo_decoder.validate() << "\n";
        f = foo{};
        echo(foo_decoder(f));
        reflect::codecs::json::encoder(writer,prefs)(f);
    }

    return 0;
}
