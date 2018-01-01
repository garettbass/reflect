#pragma once
#include <sstream>
#include <type_traits>
#include "map.h"
#include "substring.hpp"

namespace reflect {

    template<typename T>
    struct is_boolean : std::is_same<T,bool> {};

    template<typename T>
    static inline constexpr bool is_boolean_v { is_boolean<T>::value };

    //--------------------------------------------------------------------------

    template<typename T>
    struct is_number : std::is_arithmetic<T> {};

    template<typename T>
    static inline constexpr bool is_number_v { is_number<T>::value };

    //--------------------------------------------------------------------------

    template<typename>
    struct is_string : std::false_type {};

    template<>
    struct is_string<std::string> : std::true_type {};

    template<typename T>
    static inline constexpr bool is_string_v { is_string<T>::value };

    //--------------------------------------------------------------------------

    template<typename T>
    static inline constexpr bool is_terminal_v {
        is_boolean_v<T>
        or
        is_number_v<T>
        or
        is_string_v<T>
    };

    //--------------------------------------------------------------------------

    template<typename>
    struct is_array : std::false_type {};

    template<typename T>
    static inline constexpr bool is_array_v { is_array<T>::value };

    //--------------------------------------------------------------------------

    template<typename T>
    struct is_object : std::bool_constant<
        not is_terminal_v<T>
        and
        not is_array_v<T>
    > {};

    template<typename T>
    static inline constexpr bool is_object_v { is_object<T>::value };

    //--------------------------------------------------------------------------

    template<typename T>
    static inline constexpr bool is_aggregate_v {
        is_array_v<T>
        or
        is_object_v<T>
    };

    //--------------------------------------------------------------------------

    template<typename T>
    substring nameof() {
        return
            substring(__PRETTY_FUNCTION__)
            .seek_end("T = ")
            .truncate(1);
    }

    //--------------------------------------------------------------------------

    template<typename T>
    struct decode {
        template<class Decoder>
        decode(Decoder& reflect, T& out) {
            out.reflect_fields(reflect);
        }
    };

    //--------------------------------------------------------------------------

    template<typename T>
    struct encode {
        template<class Encoder>
        encode(Encoder& reflect, const T& in) {
            in.reflect_fields(reflect);
        }
    };

    //--------------------------------------------------------------------------

    template<typename T>
    struct transcode {
        template<class Decoder>
        transcode(Decoder& reflect, T& out) {
            ::reflect::decode<T>(reflect, out);
        }
        template<class Encoder>
        transcode(Encoder& reflect, const T& in) {
            ::reflect::encode<T>(reflect, in);
        }
    };

} // namespace reflect


#define _reflect_unpack(va_args) _reflect_unpack_ va_args
#define _reflect_unpack_(...) __VA_ARGS__


//------------------------------------------------------------------------------
//  reflect_is_array_type(Name)
//
//  Declares a specialization of reflect::is_array for a concrete type.
//
//  EXAMPLE:
//
//      reflect_is_array_type((std::vector<int>));
//
#define reflect_is_array_type(Name) \
    template<> \
    struct ::reflect::is_array<_reflect_unpack(Name)>:std::true_type {};


//------------------------------------------------------------------------------
//  reflect_is_array_template(Parameters,Name)
//
//  Declares a specialization of reflect::is_array for a template type.
//
//  EXAMPLE:
//
//      reflect_is_array_template((typename T),(std::vector<T>));
//
#define reflect_is_array_template(Parameters,Name) \
    template<_reflect_unpack(Parameters)> \
    struct ::reflect::is_array<_reflect_unpack(Name)>:std::true_type {};


//------------------------------------------------------------------------------
//  reflect_decode_type(Name)
//
//  Declares a specialization of reflect::decode for a class type,
//  ready for the function body to be defined.
//
//  EXAMPLE:
//
//      reflect_decode_type((std::vector<int>)) {
//          for (int t; reflect(t);) {
//              value.emplace_back(std::move(t));
//          }
//      }
//
#define reflect_decode_type(Name) \
    template<> \
    struct ::reflect::decode<_reflect_unpack(Name)> { \
        template<class Decoder> \
        decode(Decoder& reflect, _reflect_unpack(Name)& value); \
    }; \
    template<class Decoder> \
    ::reflect::decode<_reflect_unpack(Name)>:: \
    decode(Decoder& reflect, _reflect_unpack(Name)& value)


//------------------------------------------------------------------------------
//  reflect_decode_template(Parameters,Name)
//
//  Declares a specialization of reflect::decode for a class template,
//  ready for the function body to be defined.
//
//  EXAMPLE:
//
//      reflect_decode_template((typename T),(std::vector<T>)) {
//          for (T t; reflect(t);) {
//              value.emplace_back(std::move(t));
//          }
//      }
//
#define reflect_decode_template(Parameters,Name) \
    template<_reflect_unpack(Parameters)> \
    struct ::reflect::decode<_reflect_unpack(Name)> { \
        template<class Decoder> \
        decode(Decoder& reflect, _reflect_unpack(Name)& value); \
    }; \
    template<_reflect_unpack(Parameters)> \
    template<class Decoder> \
    ::reflect::decode<_reflect_unpack(Name)>:: \
    decode(Decoder& reflect, _reflect_unpack(Name)& value)


//------------------------------------------------------------------------------
//  reflect_encode_type(Name)
//
//  Declares a specialization of reflect::encode for a class type,
//  ready for the function body to be defined.
//
//  EXAMPLE:
//
//      reflect_encode_type((std::vector<int>)) {
//          for (int i : value) {
//              reflect(i);
//          }
//      }
//
#define reflect_encode_type(Name) \
    template<> \
    struct ::reflect::encode<_reflect_unpack(Name)> { \
        template<class Encoder> \
        encode(Encoder& reflect, const _reflect_unpack(Name)& value); \
    }; \
    template<class Encoder> \
    ::reflect::encode<_reflect_unpack(Name)>:: \
    encode(Encoder& reflect, const _reflect_unpack(Name)& value)


//------------------------------------------------------------------------------
//  reflect_encode_template(Parameters,Name)
//
//  Declares a specialization of reflect::encode for a collection type, ready
//  for the function body to be defined.
//
//  EXAMPLE:
//
//      reflect_encode_template((typename T),(std::vector<T>)) {
//          for (const T& t : value) {
//              reflect(t);
//          }
//      }
//
#define reflect_encode_template(Parameters,Name) \
    template<_reflect_unpack(Parameters)> \
    struct ::reflect::encode<_reflect_unpack(Name)> { \
        template<class Encoder> \
        encode(Encoder& reflect, const _reflect_unpack(Name)& value); \
    }; \
    template<_reflect_unpack(Parameters)> \
    template<class Encoder> \
    ::reflect::encode<_reflect_unpack(Name)>:: \
    encode(Encoder& reflect, const _reflect_unpack(Name)& value)


//------------------------------------------------------------------------------
//  reflect_type(Name)
//
//  Reflect the fields of a structure without modifying its declaration.
//  This macro is particularly useful when providing reflection for POD types
//  from a third-party library.
//
//  EXAMPLE:
//
//      struct vec3 { float x,y,z; };
//
//      reflect_type((vec3)) {
//          reflect("x",value.x);
//          reflect("y",value.y);
//          reflect("z",value.z);
//      }
//
#define reflect_type(Name) \
    template<> \
    struct ::reflect::transcode<_reflect_unpack(Name)> { \
        template<class Codec, typename Const_Or_NonConst> \
        transcode(Codec& reflect, Const_Or_NonConst& value); \
    }; \
    reflect_decode_type(Name) { \
        ::reflect::transcode<_reflect_unpack(Name)>(reflect, value); \
    } \
    reflect_encode_type(Name) { \
        ::reflect::transcode<_reflect_unpack(Name)>(reflect, value); \
    } \
    template<class Codec, typename Const_Or_NonConst> \
    ::reflect::transcode<_reflect_unpack(Name)>:: \
    transcode(Codec& reflect, Const_Or_NonConst& value)


//------------------------------------------------------------------------------
//  reflect_template(Parameters,Name)
//
//  Reflect the fields of a structure without modifying its declaration.
//  This macro is particularly useful when providing reflection for POD types
//  from a third-party library.
//
//  EXAMPLE:
//
//      template<typename T>
//      struct vector3 { T x,y,z; };
//
//      reflect_struct_template((typename T),(vector3<T>)) {
//          reflect("x",value.x);
//          reflect("y",value.y);
//          reflect("z",value.z);
//      }
//
#define reflect_template(Parameters,Name) \
    template<_reflect_unpack(Parameters)> \
    struct ::reflect::transcode<_reflect_unpack(Name)> { \
        template<class Codec, typename Const_Or_NonConst> \
        transcode(Codec& reflect, Const_Or_NonConst& value); \
    }; \
    reflect_decode_template(Parameters,Name) { \
        ::reflect::transcode<_reflect_unpack(Name)>(reflect, value); \
    } \
    reflect_encode_template(Parameters,Name) { \
        ::reflect::transcode<_reflect_unpack(Name)>(reflect, value); \
    } \
    template<_reflect_unpack(Parameters)> \
    template<class Codec, typename Const_Or_NonConst> \
    ::reflect::transcode<_reflect_unpack(Name)>:: \
    transcode(Codec& reflect, Const_Or_NonConst& value)


//------------------------------------------------------------------------------
//  reflect_fields(...)
//
//  Define member variables and their reflection simultaneously within a struct.
//  This macro defines the listed member variables and introduces two inline
//  member functions to faciliate reflection:
//
//      template<typename Decoder> void reflect_fields(Decoder& reflect);
//      template<typename Encoder> void reflect_fields(Encoder& reflect) const;
//
//  EXAMPLE:
//
//      struct foo {
//          reflect_fields(
//              ((int),i),
//              ((float),f)
//          )
//          double d; // this field is not reflected
//      };
//
#define reflect_fields(...) \
    MAP(reflect_field_definition, __VA_ARGS__) \
    template<typename> friend struct ::reflect::decode; \
    template<typename Decoder> void reflect_fields(Decoder& reflect) { \
        MAP(reflect_field_to_decoder, __VA_ARGS__) \
    } \
    template<typename> friend struct ::reflect::encode; \
    template<typename Encoder> void reflect_fields(Encoder& reflect) const { \
        MAP(reflect_field_to_encoder, __VA_ARGS__) \
    }

#define reflect_field_definition(params) reflect_field_definition_ params
#define reflect_field_definition_(T, name) _reflect_unpack(T) name;

#define reflect_field_to_decoder(params) reflect_field_to_decoder_ params
#define reflect_field_to_decoder_(T, name) reflect(#name,name);

#define reflect_field_to_encoder(params) reflect_field_to_encoder_ params
#define reflect_field_to_encoder_(T, name) reflect(#name,name);