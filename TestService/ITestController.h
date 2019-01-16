#pragma once

#include "Module.h"

#include <functional>

namespace WPEFramework {
namespace Exchange {

struct ITestController {

    virtual ~ITestController() {}
    virtual bool Reqister(const string& name,
                          const string& desciption,
                          const std::map<int,
                          std::vector<string>>& input,
                          const std::map<int,
                          std::vector<string>>& output,
                          const Web::Request::type requestType,
                          const std::function<Core::ProxyType<Web::Response>(const Web::Request&)>& processRequest) = 0;
    virtual bool Unregister(const string& name) = 0;
    virtual Core::ProxyType<Web::Response> Process(const Web::Request& request, const uint8_t skipURL) = 0;

    virtual Core::ProxyType<Web::Response> GetMethods() = 0;
    virtual Core::ProxyType<Web::Response> GetMethodDescription(const string& methodName) = 0;
    virtual Core::ProxyType<Web::Response> GetMethodParameters(const string& methodName) = 0;
};

} // namespace Exchange
} // namespace WPEFramework
