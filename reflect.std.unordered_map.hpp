#pragma once
#include <unordered_map>
#include "reflect.hpp"
#define   reflect_map_t ::std::unordered_map
#include "reflect_map_t.inl"
#undef    reflect_map_t