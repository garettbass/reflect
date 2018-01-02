#ifndef reflect_vector_t
#warning "reflect_vector_t undefined"
#else

    reflect_is_array_template((typename T,class A),(reflect_vector_t<T,A>));

    reflect_decode_template((typename T,class A),(reflect_vector_t<T,A>)) {
        for (T t; reflect(t);) {
            value.emplace_back(std::move(t));
        }
    }

    reflect_encode_template((typename T,class A),(reflect_vector_t<T,A>)) {
        if constexpr(std::is_fundamental_v<T>) {
            for (const T t : value) {
                reflect(t);
            }
        } else {
            for (const T& t : value) {
                reflect(t);
            }
        }
    }

#endif