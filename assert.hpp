#pragma once
#include <cstdio>
#include <sstream>

#ifndef reflect_assert_enabled
    #if defined(DEBUG) || defined(_DEBUG)
        #define reflect_assert_enabled 1
    #elif defined(__GNUC__) && !defined(__OPTIMIZE)
        #define reflect_assert_enabled 1
    #elif defined(assert_enabled)
        #define reflect_assert_enabled assert_enabled
    #endif
#endif

namespace reflect::assert {

    struct location {
        const char* file; int line;
        const char* function;
        const char* parameters;
    };

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    template<typename... Args>
    std::string
    to_string(Args&&... args) {
        std::stringstream s;
        struct local { static void unpack(...) {} };
        local::unpack(((s << args),0)...);
        return s.str();
    }

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    void
    log(const char* msg) { fputs(msg,stderr); }

    template<typename... Args>
    void
    log(Args&&... args) { log(to_string(args...).c_str()); }

    template<typename... Args>
    void
    log(const location& loc, Args&&... args) {
        log(loc.file,":",loc.line,": ",loc.function,loc.parameters);
        log(args...);
    }

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    void
    fail(const location& loc) {
        log(loc," failed\n");
        abort();
    }

    template<typename... Args>
    void
    fail(const location& loc, Args&&... args) {
        log(loc," failed: ",args...,"\n");
        abort();
    }

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    void
    todo(const location& loc) {
        log(loc," todo\n");
    }

    template<typename... Args>
    void
    todo(const location& loc, Args&&... args) {
        log(loc," todo: ",args...,"\n");
    }

} // namespace reflect::assert

#define _reflect_stringify_(...) #__VA_ARGS__
#define _reflect_stringify(x) _reflect_stringify_(x)

#define _reflect_location ::reflect::assert::location{ \
    __FILE__, __LINE__, __func__, "()" \
}

#if reflect_assert_enabled

    #define reflect_assert(expr) ((void)( \
        bool(expr) \
        or \
        (reflect_fail("reflect_assert",_reflect_stringify((expr))),false) \
    ))

#else

    #define reflect_assert(expr) ((void)0)

#endif

#define reflect_fail(...) ( \
    ::reflect::assert::fail(_reflect_location, ##__VA_ARGS__) \
)

#define reflect_fail_if(expr) ((void)( \
    bool(expr) \
    and \
    (reflect_fail("reflect_fail_if",_reflect_stringify((expr))),false) \
))

#define reflect_todo(...) ( \
    ::reflect::assert::todo(_reflect_location, ##__VA_ARGS__) \
)