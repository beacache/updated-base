#pragma once

#include <map>
#include <string>
#include <sstream>
#include <optional>
#include <algorithm>
#include <Windows.h>

#include <lstate.h>
#include <lualib.h>
#include <cpr/cpr.h>
#include <json/json.h>

#include <globals.h>
#include <roblox.h>
#include <miscellaneous/utility/yielder/yielder.h>

namespace module::core::environment
{
    enum request_method_t
    {
        H_GET,
        H_HEAD,
        H_POST,
        H_PUT,
        H_DELETE,
        H_OPTIONS
    };

    inline std::map<std::string, request_method_t> request_method_map = {
        { "get",     H_GET     },
        { "head",    H_HEAD    },
        { "post",    H_POST    },
        { "put",     H_PUT     },
        { "delete",  H_DELETE  },
        { "options", H_OPTIONS }
    };

    struct c_http
    {
        static int http_get(lua_State* L);
        static int request(lua_State* L);
        void register_library(lua_State* L);
    };

    inline c_http http{};
}
