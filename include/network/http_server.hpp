#pragma once

#include "httplib.h"
#include <nlohmann/json_fwd.hpp>
#include <unistd.h>

enum class RequestType{
    get,
    post
};


class HTTPServer{
    private:
        httplib::Server svr;
    public:
        void start_server(int port);
        void add_handler(RequestType rt, std::string pattern, std::function<void (const httplib::Request &, httplib::Response &)>);
};

void json_ok(httplib::Response& res, nlohmann::json body);
void json_error(httplib::Response& res, std::string err, nlohmann::json body);
