#ifndef reflect_map_t
#warning "reflect_map_t undefined"
#else

    reflect_decode_template((typename K,typename T),(reflect_map_t<K,T>)) {
        reflect::substring s;
        if constexpr(is_string_v<K>) {
            T v;
            while (reflect(&s,v)) {
                value[K(s)]=v;
            }
        } else {
            K k; T v;
            std::stringstream ss;
            while (reflect(&s,v)) {
                ss.seekg(0);
                ss.seekp(0);
                ss << s;
                ss >> k;
                value[k]=v;
            }
        }
    }

    reflect_encode_template((typename K,typename T),(reflect_map_t<K,T>)) {
        if constexpr(is_string_v<K>) {
            for (auto& pair : value) {
                reflect(pair.first,pair.second);
            }
        } else {
            std::stringstream ss;
            for (auto& pair : value) {
                ss.seekg(0);
                ss.seekp(0);
                ss << pair.first;
                reflect(ss.str(),pair.second);
            }
        }
    }

#endif