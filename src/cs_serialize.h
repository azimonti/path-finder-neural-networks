#ifndef CS_SERIALIZE_H
#define CS_SERIALIZE_H

#define JSON_NO_IO
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4365)
#endif
#include "nlohmann/json.hpp"
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#define CS_SERIALIZE_VAL(X) {#X, v.X}
#define CS_DESERIALIZE_VAL(X)                                                                                          \
    {                                                                                                                  \
        if (auto it = j.find(#X); it != j.end()) it->get_to(v.X);                                                      \
    }

#endif
