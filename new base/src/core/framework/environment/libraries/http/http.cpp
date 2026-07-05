#include "http.h"

namespace module::core::environment
{
    using namespace module::rbx;

    int c_http::http_get(lua_State* L)
    {
        std::string url;

        if (lua_isstring(L, 1))
            url = lua_tostring(L, 1);
        else if (lua_isstring(L, 2))
            url = lua_tostring(L, 2);
        else
        {
            lua_pushnil(L);
            lua_pushstring(L, "expected string");
            return 2;
        }

        if (url.find("http://") != 0 && url.find("https://") != 0)
        {
            lua_pushnil(L);
            lua_pushstring(L, "invalid protocol (expected 'http://' or 'https://')");
            return 2;
        }

        if (is_url_blocked(url))
        {
            lua_pushnil(L);
            lua_pushstring(L, "blocked domain");
            return 2;
        }

        cpr::Header headers = build_default_headers();

        return yielder::execution(L, [url, headers]() -> yielded
        {
            cpr::Response result;
            std::string error_msg;

            try
            {
                result = cpr::Get(
                    cpr::Url{ url },
                    headers,
                    cpr::Timeout{ 10000 },
                    cpr::VerifySsl{ false }
                );
            }
            catch (const std::exception& ex)
            {
                error_msg = std::string("http_get failed: ") + ex.what();
            }
            catch (...)
            {
                error_msg = "http_get failed: unknown exception";
            }

            if (error_msg.empty() && result.error)
            {
                switch (result.error.code)
                {
                    case cpr::ErrorCode::CONNECTION_FAILURE:
                        error_msg = "http_get failed: connection refused or unreachable";
                        break;
                    case cpr::ErrorCode::OPERATION_TIMEDOUT:
                        error_msg = "http_get failed: request timed out";
                        break;
                    case cpr::ErrorCode::HOST_RESOLUTION_FAILURE:
                        error_msg = "http_get failed: could not resolve host";
                        break;
                    case cpr::ErrorCode::SSL_CONNECT_ERROR:
                        error_msg = "http_get failed: SSL connection error";
                        break;
                    default:
                        error_msg = "http_get failed: " + result.error.message;
                        break;
                }
            }

            return [result, error_msg](lua_State* L) -> int
            {
                if (!error_msg.empty())
                {
                    lua_pushnil(L);
                    lua_pushstring(L, error_msg.c_str());
                    return 2;
                }

                lua_pushlstring(L, result.text.c_str(), result.text.size());
                return 1;
            };
        });
    }

    int c_http::http_post(lua_State* L)
    {
        std::string url;
        std::string body;
        std::string content_type = "application/x-www-form-urlencoded";

        if (lua_isstring(L, 1))
            url = lua_tostring(L, 1);
        else if (lua_isstring(L, 2))
            url = lua_tostring(L, 2);
        else
        {
            lua_pushnil(L);
            lua_pushstring(L, "expected string");
            return 2;
        }

        if (lua_isstring(L, 2))
            body = lua_tostring(L, 2);
        else if (lua_isstring(L, 3))
            body = lua_tostring(L, 3);

        if (lua_isstring(L, 3))
            content_type = lua_tostring(L, 3);
        else if (lua_isstring(L, 4))
            content_type = lua_tostring(L, 4);

        if (url.find("http://") != 0 && url.find("https://") != 0)
        {
            lua_pushnil(L);
            lua_pushstring(L, "invalid protocol (expected 'http://' or 'https://')");
            return 2;
        }

        if (is_url_blocked(url))
        {
            lua_pushnil(L);
            lua_pushstring(L, "blocked domain");
            return 2;
        }

        cpr::Header headers = build_default_headers();
        headers["Content-Type"] = content_type;

        return yielder::execution(L, [url, body, headers]() -> yielded
        {
            cpr::Response result;
            std::string error_msg;

            try
            {
                result = cpr::Post(
                    cpr::Url{ url },
                    cpr::Body{ body },
                    headers,
                    cpr::Timeout{ 10000 },
                    cpr::VerifySsl{ false }
                );
            }
            catch (const std::exception& ex)
            {
                error_msg = std::string("http_post failed: ") + ex.what();
            }
            catch (...)
            {
                error_msg = "http_post failed: unknown exception";
            }

            if (error_msg.empty() && result.error)
            {
                switch (result.error.code)
                {
                    case cpr::ErrorCode::CONNECTION_FAILURE:
                        error_msg = "http_post failed: connection refused or unreachable";
                        break;
                    case cpr::ErrorCode::OPERATION_TIMEDOUT:
                        error_msg = "http_post failed: request timed out";
                        break;
                    case cpr::ErrorCode::HOST_RESOLUTION_FAILURE:
                        error_msg = "http_post failed: could not resolve host";
                        break;
                    case cpr::ErrorCode::SSL_CONNECT_ERROR:
                        error_msg = "http_post failed: SSL connection error";
                        break;
                    default:
                        error_msg = "http_post failed: " + result.error.message;
                        break;
                }
            }

            return [result, error_msg](lua_State* L) -> int
            {
                if (!error_msg.empty())
                {
                    lua_pushnil(L);
                    lua_pushstring(L, error_msg.c_str());
                    return 2;
                }

                lua_pushlstring(L, result.text.c_str(), result.text.size());
                return 1;
            };
        });
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

        if (url.find("http://") != 0 && url.find("https://") != 0)
        {
            lua_pushnil(L);
            lua_pushstring(L, "invalid protocol (expected 'http://' or 'https://')");
            return 2;
        }

        if (is_url_blocked(url))
        {
            lua_pushnil(L);
            lua_pushstring(L, "blocked domain");
            return 2;
        }

        request_methods method = h_get;
        lua_getfield(L, 1, "Method");
        if (lua_isstring(L, -1))
        {
            std::string method_str = lua_tostring(L, -1);
            std::transform(method_str.begin(), method_str.end(), method_str.begin(), ::tolower);

            if (request_method_map.count(method_str))
                method = request_method_map[method_str];
        }
        lua_pop(L, 1);

        cpr::Header headers = build_default_headers();
        lua_getfield(L, 1, "Headers");
        if (lua_istable(L, -1))
        {
            lua_pushnil(L);
            while (lua_next(L, -2))
            {
                if (lua_isstring(L, -2) && lua_isstring(L, -1))
                    headers[lua_tostring(L, -2)] = lua_tostring(L, -1);
                lua_pop(L, 1);
            }
        }
        lua_pop(L, 1);

        cpr::Cookies cookies;
        lua_getfield(L, 1, "Cookies");
        if (lua_istable(L, -1))
        {
            lua_pushnil(L);
            while (lua_next(L, -2))
            {
                if (lua_isstring(L, -2) && lua_isstring(L, -1))
                    cookies[lua_tostring(L, -2)] = lua_tostring(L, -1);
                lua_pop(L, 1);
            }
        }
        lua_pop(L, 1);

        long timeout = 10000;
        lua_getfield(L, 1, "Timeout");
        if (lua_isnumber(L, -1))
            timeout = static_cast<long>(lua_tonumber(L, -1) * 1000.0);
        lua_pop(L, 1);

        std::string body;
        lua_getfield(L, 1, "Body");
        if (lua_isstring(L, -1))
            body = lua_tostring(L, -1);
        lua_pop(L, 1);

        return yielder::execution(L, [url, method, headers, cookies, body, timeout]() -> yielded
        {
            cpr::Response response;
            std::string error_msg;

            try
            {
                auto cpr_timeout = cpr::Timeout{ timeout };
                auto cpr_ssl = cpr::VerifySsl{ false };

                switch (method)
                {
                    case h_get:
                        response = cpr::Get(cpr::Url{ url }, cookies, headers, cpr_timeout, cpr_ssl);
                        break;
                    case h_head:
                        response = cpr::Head(cpr::Url{ url }, cookies, headers, cpr_timeout, cpr_ssl);
                        break;
                    case h_post:
                        response = cpr::Post(cpr::Url{ url }, cpr::Body{ body }, cookies, headers, cpr_timeout, cpr_ssl);
                        break;
                    case h_put:
                        response = cpr::Put(cpr::Url{ url }, cpr::Body{ body }, cookies, headers, cpr_timeout, cpr_ssl);
                        break;
                    case h_delete:
                        response = cpr::Delete(cpr::Url{ url }, cpr::Body{ body }, cookies, headers, cpr_timeout, cpr_ssl);
                        break;
                    case h_options:
                        response = cpr::Options(cpr::Url{ url }, cpr::Body{ body }, cookies, headers, cpr_timeout, cpr_ssl);
                        break;
                }
            }
            catch (const std::exception& ex)
            {
                error_msg = std::string("request failed: ") + ex.what();
            }
            catch (...)
            {
                error_msg = "request failed: unknown exception";
            }

            if (error_msg.empty() && response.error)
            {
                switch (response.error.code)
                {
                    case cpr::ErrorCode::CONNECTION_FAILURE:
                        error_msg = "request failed: connection refused or unreachable";
                        break;
                    case cpr::ErrorCode::OPERATION_TIMEDOUT:
                        error_msg = "request failed: request timed out";
                        break;
                    case cpr::ErrorCode::HOST_RESOLUTION_FAILURE:
                        error_msg = "request failed: could not resolve host";
                        break;
                    case cpr::ErrorCode::SSL_CONNECT_ERROR:
                        error_msg = "request failed: SSL connection error";
                        break;
                    default:
                        error_msg = "request failed: " + response.error.message;
                        break;
                }
            }

            return [response, error_msg](lua_State* L) -> int
            {
                if (!error_msg.empty())
                {
                    lua_pushnil(L);
                    lua_pushstring(L, error_msg.c_str());
                    return 2;
                }

                push_response(L, response);
                return 1;
            };
        });
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