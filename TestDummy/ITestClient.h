#pragma once

#include "Module.h"

#include <functional>

namespace WPEFramework {
// ToDo: Potentially move this Interface to WPEFramework
namespace Exchange {

struct ITestClient {

    virtual ~ITestClient() {}
    virtual bool Reqister(const string& name, const string& desciption, const std::map<int, std::vector<string>>& input,
        const std::map<int, std::vector<string>>& output, Web::Request::type requestType,
        const std::function<Core::ProxyType<Web::Response>(const Web::Request&)>& processRequest)
        = 0;
    virtual bool Unregister(const string& name) = 0;
    virtual Core::ProxyType<Web::Response> Process(const Web::Request& request, uint8_t skipURL) = 0;

    virtual Core::ProxyType<Web::Response> GetMethods(void) = 0;
    virtual Core::ProxyType<Web::Response> GetMethodDescription(string methodName) = 0;
    virtual Core::ProxyType<Web::Response> GetMethodParameters(string methodName) = 0;
};

} // namespace Exchange
} // namespace WPEFramework
