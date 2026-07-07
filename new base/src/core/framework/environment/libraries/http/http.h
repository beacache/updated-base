#pragma once

#include <string>
#include <map>
#include <unordered_set>
#include <algorithm>
#include <vector>
#include <mutex>

#include <lstate.h>
#include <lualib.h>
#include <json/json.h>
#include <Windows.h>

#include <globals.h>
#include <roblox.h>

namespace module::core::environment
{
    inline std::unordered_set<std::string> blocked_domains = {
        "accountinformation.roblox.com", "accountsettings.roblox.com",
        "twostepverification.roblox.com", "trades.roblox.com",
        "billing.roblox.com", "economy.roblox.com", "auth.roblox.com",
        "accountinformation.roproxy.com", "accountsettings.roproxy.com",
        "twostepverification.roproxy.com", "trades.roproxy.com",
        "billing.roproxy.com", "economy.roproxy.com", "auth.roproxy.com",
        "api.ipify.org", "api4.ipify.org", "api6.ipify.org", "api64.ipify.org",
        "api.ipapi.is", "api.ipapi.co", "api.ipapi.com",
        "api.ipinfo.io", "api.ipgeolocation.io", "api.freegeoip.app",
        "api.myip.com", "api.ip.sb", "api.ipwhois.io",
        "api.ipdata.co", "api.ipstack.com", "api.db-ip.com",
        "api.seeip.org", "api.ipregistry.co", "api.abstractapi.com",
        "api.ipbase.com", "api.iplocation.net", "api.ip.nf",
        "api.bigdatacloud.net", "api.techniknews.net", "api.country.is",
        "api.2ip.me", "api.reallyfreegeoip.org",
    };

    struct parsed_url_t
    {
        bool secure = false;
        std::string host;
        int port = 80;
        std::string path;
    };

    struct http_result_t
    {
        int status = 0;
        std::string body;
        std::string error;
        bool success = false;
        std::vector<std::pair<std::string, std::string>> headers;
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

        static std::map<std::string, std::string> build_default_headers();
        static std::map<std::string, std::string> build_minimal_headers();
        static std::map<std::string, std::string> get_headers_for_url(const std::string& url);
        static bool parse_url(const std::string& url, parsed_url_t& out);
        static http_result_t make_request(const std::string& url, const std::string& method, const std::string& body, const std::map<std::string, std::string>& headers, const std::string& content_type, int timeout_sec);
        static int get_url_idx(lua_State* L);

        static int http_get(lua_State* L);
        static int http_post(lua_State* L);
        static int request(lua_State* L);
        void register_library(lua_State* L);
    };

    inline c_http http{};
}
