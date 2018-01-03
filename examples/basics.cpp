////usr/bin/env $(dirname $0)/cxx -c++17 -I $(dirname $0)/../.. -r $0; exit $?

//------------------------------------------------------------------------------
// Declare a struct or class with some reflected fields:
#include <reflect/reflect.hpp>

struct example_struct {

    // declare reflected member variables
    reflect_fields(
        ((int),i),
        ((float),f),
        ((std::string),s) // NOTE: no trailing comma here
    )

    double d; // NOTE: this field will not be reflected
};

class example_class {

    // declare reflected private member variables
    reflect_fields(
        ((int),_i),
        ((float),_f),
        ((std::string),_s) // NOTE: no trailing comma here
    )

    double _d; // NOTE: this field will not be reflected

public:

    example_class() = default;

    example_class(
        int i,
        float f,
        std::string s,
        double d)
    : _i(i), _f(f), _s(s), _d(d) {}
};

//------------------------------------------------------------------------------
// Define reflection for existing types or templates:

struct person { std::string name, address; };

reflect_type((person)) {
    reflect("name",value.name);
    reflect("address",value.address);
}

template<typename T>
struct vector3 { T x, y, z; };

reflect_template((typename T),(vector3<T>)) {
    reflect("x",value.x);
    reflect("y",value.y);
    reflect("z",value.z);
}

//------------------------------------------------------------------------------
// Additional headers provide reflection for standard library types,
// std::vector, std::map, and std::unordered_map:

#include <reflect/reflect.std.map.hpp>
#include <reflect/reflect.std.unordered_map.hpp>
#include <reflect/reflect.std.vector.hpp>

struct example_struct_2 {
    reflect_fields(
        ((std::map<int,float>),numbers),
        ((std::unordered_map<std::string,person>),people),
        ((std::vector<vector3<float>>),vector),
        ((std::vector<bool>),bitmap)
    );
};

//------------------------------------------------------------------------------
// Serialize to/from JSON:

#include <reflect/codecs/json/decoder.hpp>
#include <reflect/codecs/json/encoder.hpp>

int main(int,char**) {
    reflect::codecs::json::preferences prefs;
    prefs.indent = "    ";
    prefs.newline = "\n";
    prefs.newline_at_eof = true;

    reflect::stream_writer writer(std::cout);
    reflect::codecs::json::encoder encode(writer,prefs);

    example_struct s {1,2.3f,"hello",4.5};
    encode(s);
    // {
    //     "i":1,
    //     "f":2.3,
    //     "s":"hello"
    // }

    reflect::string_reader reader(R"({ "i":2, "f":3.4, "s":"world" })");
    reflect::codecs::json::decoder decode(reader);
    decode(s);
    encode(s);
    // {
    //     "i":2,
    //     "f":3.5,
    //     "s":"world"
    // }

    example_struct_2 s2 {
        /*numbers*/{{1,2.3f},{4,5.6f}},
        /*people*/{
            {"me",{"Me","123 My St."}},
            {"you",{"You","456 Your St."}},
        },
        /*vector*/{
            {1,2,3},
            {4,5,6},
        },
        /*bitmap*/{
            false,
            true,
        },
    };
    encode(s2);
    // {
    //     "numbers":{
    //         "1":2.3,
    //         "4":5.6
    //     },
    //     "people":{
    //         "you":{
    //             "name":"You",
    //             "address":"456 Your St."
    //         },
    //         "me":{
    //             "name":"Me",
    //             "address":"123 My St."
    //         }
    //     },
    //     "vector":[
    //         {
    //             "x":1,
    //             "y":2,
    //             "z":3
    //         },
    //         {
    //             "x":4,
    //             "y":5,
    //             "z":6
    //         }
    //     ],
    //     "bitmap":[
    //         false,
    //         true
    //     ]
    // }

    return 0;
}
