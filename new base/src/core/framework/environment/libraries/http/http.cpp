#include "http.h"

#define CPPHTTPLIB_MBEDTLS_SUPPORT
#include <cpp-httplib/httplib.h>
#include <thread>

namespace module::core::environment
{
    using namespace module::rbx;

    bool c_http::parse_url(const std::string& url, parsed_url_t& out)
    {
        out.secure = false;
        out.port = 80;

        std::string u = url;
        if (u.rfind("https://", 0) == 0)
        {
            out.secure = true;
            out.port = 443;
            u = u.substr(8);
        }
        else if (u.rfind("http://", 0) == 0)
        {
            u = u.substr(7);
        }
        else
        {
            return false;
        }

        auto slash = u.find('/');
        if (slash == std::string::npos)
        {
            out.host = u;
            out.path = "/";
        }
        else
        {
            out.host = u.substr(0, slash);
            out.path = u.substr(slash);
        }

        auto colon = out.host.find(':');
        if (colon != std::string::npos)
        {
            try
            {
                out.port = std::stoi(out.host.substr(colon + 1));
            }
            catch (...)
            {
                return false;
            }
            out.host = out.host.substr(0, colon);
        }

        return !out.host.empty();
    }

    http_result_t c_http::make_request(const std::string& url, const std::string& method, const std::string& body, const std::map<std::string, std::string>& headers, const std::string& content_type, int timeout_sec)
    {
        http_result_t resp;

        parsed_url_t parsed;
        if (!parse_url(url, parsed))
        {
            resp.error = "failed to parse url";
            return resp;
        }

        try
        {
            std::string scheme = parsed.secure ? "https://" : "http://";
            std::string base = scheme + parsed.host + ":" + std::to_string(parsed.port);

            httplib::Client cli(base);
            cli.set_connection_timeout(timeout_sec, 0);
            cli.set_read_timeout(timeout_sec, 0);
            cli.set_write_timeout(timeout_sec, 0);
            cli.set_follow_location(true);
            cli.enable_server_certificate_verification(false);

            httplib::Headers hdrs;
            for (const auto& h : headers)
                hdrs.emplace(h.first, h.second);

            httplib::Result result;

            if (method == "GET")
                result = cli.Get(parsed.path, hdrs);
            else if (method == "HEAD")
                result = cli.Head(parsed.path, hdrs);
            else if (method == "POST")
                result = cli.Post(parsed.path, hdrs, body, content_type);
            else if (method == "PUT")
                result = cli.Put(parsed.path, hdrs, body, content_type);
            else if (method == "DELETE")
                result = cli.Delete(parsed.path, hdrs, body, content_type);
            else if (method == "OPTIONS")
                result = cli.Options(parsed.path, hdrs);
            else if (method == "PATCH")
                result = cli.Patch(parsed.path, hdrs, body, content_type);
            else
                result = cli.Get(parsed.path, hdrs);

            if (!result)
            {
                auto err = result.error();
                switch (err)
                {
                    case httplib::Error::Connection:
                        resp.error = "connection failed";
                        break;
                    case httplib::Error::Read:
                        resp.error = "read error";
                        break;
                    case httplib::Error::Write:
                        resp.error = "write error";
                        break;
                    case httplib::Error::SSLConnection:
                        resp.error = "ssl connection error";
                        break;
                    case httplib::Error::SSLServerVerification:
                        resp.error = "ssl verification error";
                        break;
                    case httplib::Error::ConnectionTimeout:
                        resp.error = "connection timed out";
                        break;
                    default:
                        resp.error = "unknown error";
                        break;
                }
                return resp;
            }

            resp.status = result->status;
            resp.body = result->body;
            resp.success = resp.status >= 200 && resp.status < 300;

            for (const auto& h : result->headers)
                resp.headers.emplace_back(h.first, h.second);
        }
        catch (const std::exception& e)
        {
            resp.error = std::string("exception: ") + e.what();
        }
        catch (...)
        {
            resp.error = "unknown exception";
        }

        return resp;
    }

    int c_http::get_url_idx(lua_State* L)
    {
        int top = lua_gettop(L);
        for (int i = 1; i <= top; i++)
        {
            if (lua_isstring(L, i))
            {
                std::string s = lua_tostring(L, i);
                if (s.rfind("http://", 0) == 0 || s.rfind("https://", 0) == 0)
                    return i;
            }
        }
        return -1;
    }

    static bool is_roblox_url(const std::string& url)
    {
        std::string lower = url;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        return lower.find("roblox.com") != std::string::npos || lower.find("roproxy.com") != std::string::npos;
    }

    std::map<std::string, std::string> c_http::build_default_headers()
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

    std::map<std::string, std::string> c_http::build_minimal_headers()
    {
        return {
            { "User-Agent", "Roblox/WinInet" },
            { "Accept", "*/*" }
        };
    }

    std::map<std::string, std::string> c_http::get_headers_for_url(const std::string& url)
    {
        if (is_roblox_url(url))
            return build_default_headers();
        return build_minimal_headers();
    }

    int c_http::http_get(lua_State* L)
    {
        int idx = get_url_idx(L);
        if (idx == -1)
        {
            lua_pushnil(L);
            lua_pushstring(L, "expected url string");
            return 2;
        }

        std::string url = lua_tostring(L, idx);

        if (is_url_blocked(url))
        {
            lua_pushnil(L);
            lua_pushstring(L, "blocked domain");
            return 2;
        }

        auto hdrs = get_headers_for_url(url);
        http_result_t resp = make_request(url, "GET", "", hdrs, "", 15);

        if (!resp.error.empty())
        {
            lua_pushnil(L);
            lua_pushstring(L, resp.error.c_str());
            return 2;
        }

        lua_pushlstring(L, resp.body.c_str(), resp.body.size());
        return 1;
    }

    int c_http::http_post(lua_State* L)
    {
        int idx = get_url_idx(L);
        if (idx == -1)
        {
            lua_pushnil(L);
            lua_pushstring(L, "expected url string");
            return 2;
        }

        std::string url = lua_tostring(L, idx);

        if (is_url_blocked(url))
        {
            lua_pushnil(L);
            lua_pushstring(L, "blocked domain");
            return 2;
        }

        std::string body;
        if (lua_isstring(L, idx + 1))
            body = lua_tostring(L, idx + 1);

        std::string content_type = "application/x-www-form-urlencoded";
        if (lua_isstring(L, idx + 2))
            content_type = lua_tostring(L, idx + 2);

        auto hdrs = get_headers_for_url(url);
        http_result_t resp = make_request(url, "POST", body, hdrs, content_type, 15);

        if (!resp.error.empty())
        {
            lua_pushnil(L);
            lua_pushstring(L, resp.error.c_str());
            return 2;
        }

        lua_pushlstring(L, resp.body.c_str(), resp.body.size());
        return 1;
    }

    int c_http::request(lua_State* L)
    {
        if (!lua_istable(L, 1))
        {
            lua_pushnil(L);
            lua_pushstring(L, "expected table");
            return 2;
        }

        lua_getfield(L, 1, "Url");
        if (!lua_isstring(L, -1))
        {
            lua_pop(L, 1);
            lua_pushnil(L);
            lua_pushstring(L, "missing Url");
            return 2;
        }
        std::string url = lua_tostring(L, -1);
        lua_pop(L, 1);

        if (url.rfind("http://", 0) != 0 && url.rfind("https://", 0) != 0)
        {
            lua_pushnil(L);
            lua_pushstring(L, "invalid protocol");
            return 2;
        }

        if (is_url_blocked(url))
        {
            lua_pushnil(L);
            lua_pushstring(L, "blocked domain");
            return 2;
        }

        std::string method = "GET";
        lua_getfield(L, 1, "Method");
        if (lua_isstring(L, -1))
        {
            method = lua_tostring(L, -1);
            std::transform(method.begin(), method.end(), method.begin(), ::toupper);
        }
        lua_pop(L, 1);

        std::string body;
        lua_getfield(L, 1, "Body");
        if (lua_isstring(L, -1))
            body = lua_tostring(L, -1);
        lua_pop(L, 1);

        std::string content_type = "application/json";
        auto hdrs = get_headers_for_url(url);

        lua_getfield(L, 1, "Headers");
        if (lua_istable(L, -1))
        {
            lua_pushnil(L);
            while (lua_next(L, -2))
            {
                if (lua_isstring(L, -2) && lua_isstring(L, -1))
                {
                    std::string key = lua_tostring(L, -2);
                    std::string val = lua_tostring(L, -1);

                    std::string key_lower = key;
                    std::transform(key_lower.begin(), key_lower.end(), key_lower.begin(), ::tolower);
                    if (key_lower == "content-type")
                        content_type = val;
                    else
                        hdrs[key] = val;
                }
                lua_pop(L, 1);
            }
        }
        lua_pop(L, 1);

        int timeout_sec = 15;
        lua_getfield(L, 1, "Timeout");
        if (lua_isnumber(L, -1))
            timeout_sec = (int)lua_tonumber(L, -1);
        lua_pop(L, 1);

        http_result_t resp = make_request(url, method, body, hdrs, content_type, timeout_sec);

        if (!resp.error.empty())
        {
            lua_pushnil(L);
            lua_pushstring(L, resp.error.c_str());
            return 2;
        }

        lua_newtable(L);

        lua_pushboolean(L, resp.success);
        lua_setfield(L, -2, "Success");

        lua_pushinteger(L, resp.status);
        lua_setfield(L, -2, "StatusCode");

        lua_pushstring(L, get_status_phrase(resp.status).c_str());
        lua_setfield(L, -2, "StatusMessage");

        lua_newtable(L);
        for (const auto& h : resp.headers)
        {
            lua_pushstring(L, h.first.c_str());
            lua_pushstring(L, h.second.c_str());
            lua_settable(L, -3);
        }
        lua_setfield(L, -2, "Headers");

        lua_newtable(L);
        lua_setfield(L, -2, "Cookies");

        lua_pushlstring(L, resp.body.c_str(), resp.body.size());
        lua_setfield(L, -2, "Body");

        return 1;
    }

    void c_http::register_library(lua_State* L)
    {
        utils.add_function(L, "HttpGet", c_http::http_get);
        utils.add_function(L, "HttpGetAsync", c_http::http_get);
        utils.add_function(L, "HttpPost", c_http::http_post);
        utils.add_function(L, "HttpPostAsync", c_http::http_post);
        utils.add_function(L, "request", c_http::request);
        utils.add_function(L, "http_request", c_http::request);

        lua_newtable(L);
        utils.add_table_function(L, "request", c_http::request);
        lua_setreadonly(L, -1, true);
        lua_setglobal(L, "http");
    }
}
