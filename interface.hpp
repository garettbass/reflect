#pragma once

namespace reflect {

    class interface {
        interface(interface&&)                   = delete;
        interface(const interface&)              = delete;
        interface& operator = (interface&&)      = delete;
        interface& operator = (const interface&) = delete;
    protected:
        interface() = default;
        virtual ~interface() = default;
    };

} // namespace reflect