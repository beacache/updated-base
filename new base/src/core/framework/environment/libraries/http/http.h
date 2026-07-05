#pragma once

#include <string>
#include <map>
#include <unordered_set>
#include <algorithm>

#include <lstate.h>
#include <lualib.h>
#include <cpr/cpr.h>
#include <json/json.h>
#include <Windows.h>

#include <globals.h>
#include <roblox.h>
#include <yielder/yield.h>

namespace module::core::environment
{
    enum request_methods
    {
        h_get, h_head, h_post, h_put, h_delete, h_options
    };

    inline std::map<std::string, request_methods> request_method_map = {
        { "get", h_get }, { "head", h_head }, { "post", h_post },
        { "put", h_put }, { "delete", h_delete }, { "options", h_options }
    };

    inline std::unordered_set<std::string> blocked_domains = {
        "accountinformation.roblox.com", "accountsettings.roblox.com",
        "twostepverification.roblox.com", "trades.roblox.com",
        "billing.roblox.com", "economy.roblox.com", "auth.roblox.com",
        "accountinformation.roproxy.com", "accountsettings.roproxy.com",
        "twostepverification.roproxy.com", "trades.roproxy.com",
        "billing.roproxy.com", "economy.roproxy.com", "auth.roproxy.com",
        "api.ipify.org",
    };

    struct c_http
    {
        static bool is_url_blocked(const std::string& url)
        {
            std::string lower = url;
            std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
            for (const auto& domain : blocked_domains)
                if (lower.find(domain) != std::string::npos)
                    return true;
            return false;
        }

        static std::string get_hwid()
        {
            HW_PROFILE_INFO hw_info;
            if (GetCurrentHwProfile(&hw_info))
            {
                std::wstring w_guid(hw_info.szHwProfileGuid);
                return std::string(w_guid.begin(), w_guid.end());
            }
            return "Unknown";
        }

        static bool read_game_info_safe(uintptr_t real_dm, int64_t& place_id, int64_t& game_id)
        {
            __try
            {
                place_id = module::update::data_model::place_id.get(real_dm);
                game_id = module::update::data_model::game_id.get(real_dm);
                return true;
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                return false;
            }
        }

        static void get_game_info(std::string& game_id_out, std::string& place_id_out)
        {
            game_id_out = "0";
            place_id_out = "0";

            uintptr_t fake_dm = *reinterpret_cast<uintptr_t*>(module::update::data_model::fake_dm_pointer);
            if (!fake_dm) return;

            uintptr_t real_dm = module::update::data_model::fake_dm_to_real_dm.get(fake_dm);
            if (!real_dm) return;

            int64_t place_id = 0;
            int64_t game_id = 0;

            if (read_game_info_safe(real_dm, place_id, game_id))
            {
                if (place_id > 0) place_id_out = std::to_string(place_id);
                if (game_id > 0) game_id_out = std::to_string(game_id);
            }
        }

        static std::string get_status_phrase(int code)
        {
            switch (code)
            {
                case 200: return "OK";
                case 201: return "Created";
                case 204: return "No Content";
                case 301: return "Moved Permanently";
                case 302: return "Found";
                case 304: return "Not Modified";
                case 400: return "Bad Request";
                case 401: return "Unauthorized";
                case 403: return "Forbidden";
                case 404: return "Not Found";
                case 405: return "Method Not Allowed";
                case 408: return "Request Timeout";
                case 429: return "Too Many Requests";
                case 500: return "Internal Server Error";
                case 502: return "Bad Gateway";
                case 503: return "Service Unavailable";
                case 504: return "Gateway Timeout";
                default: return "Unknown";
            }
        }

        static void push_response(lua_State* L, const cpr::Response& response)
        {
            lua_newtable(L);

            lua_pushboolean(L, response.status_code >= 200 && response.status_code < 300);
            lua_setfield(L, -2, "Success");

            lua_pushinteger(L, static_cast<int>(response.status_code));
            lua_setfield(L, -2, "StatusCode");

            lua_pushstring(L, get_status_phrase(response.status_code).c_str());
            lua_setfield(L, -2, "StatusMessage");

            lua_newtable(L);
            for (const auto& header : response.header)
            {
                lua_pushstring(L, header.first.c_str());
                lua_pushstring(L, header.second.c_str());
                lua_settable(L, -3);
            }
            lua_setfield(L, -2, "Headers");

            lua_newtable(L);
            for (const auto& cookie : response.cookies.map_)
            {
                lua_pushstring(L, cookie.first.c_str());
                lua_pushstring(L, cookie.second.c_str());
                lua_settable(L, -3);
            }
            lua_setfield(L, -2, "Cookies");

            lua_pushlstring(L, response.text.c_str(), response.text.size());
            lua_setfield(L, -2, "Body");
        }

        static cpr::Header build_default_headers()
        {
            std::string game_id, place_id;
            get_game_info(game_id, place_id);
            std::string hwid = get_hwid();

            nlohmann::json session_json;
            session_json["GameId"] = game_id;
            session_json["PlaceId"] = place_id;

            return {
                { "User-Agent", "Roblox/WinInet" },
                { "Roblox-Session-Id", session_json.dump() },
                { "Roblox-Place-Id", place_id },
                { "Roblox-Game-Id", game_id },
                { "Exploit-Identifier", "Roblox/WinInet" },
                { "Exploit-Guid", hwid },
                { "Exploit-Fingerprint", hwid },
                { "Accept", "*/*" }
            };
        }

        static int http_get(lua_State* L);
        static int http_post(lua_State* L);
        static int request(lua_State* L);
        void register_library(lua_State* L);
    };

    inline c_http http{};
}