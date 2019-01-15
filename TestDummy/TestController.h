#pragma once

#include "Module.h"

#include "ITestController.h"
#include "TestData.h"

#include <vector>

namespace WPEFramework {
namespace Plugin {
namespace TestCore {

class TestController : public Exchange::ITestController {
private:
    class TestMethod {
    private:
        TestMethod& operator=(const TestMethod&) = delete;

    public:
        TestMethod(string name, string desciption, const std::map<int, std::vector<string>>& input,
            const std::map<int, std::vector<string>>& output,
            const std::function<Core::ProxyType<Web::Response>(const Web::Request&)>& callback)
            : _name(name)
            , _description(desciption)
            , _input(input)
            , _output(output)
            , _callback(callback)
        {
        }

        virtual ~TestMethod() {}

    public:
        string _name;
        string _description;
        std::map<int, std::vector<string>> _input;
        std::map<int, std::vector<string>> _output;
        std::function<Core::ProxyType<Web::Response>(const Web::Request&)> _callback;
    };

private:
    TestController(const TestController&) = delete;
    TestController& operator=(const TestController&) = delete;

public:
    TestController()
    {
        _tests.insert(std::pair<Web::Request::type, std::list<TestMethod>>(Web::Request::type::HTTP_POST, {}));
        _tests.insert(std::pair<Web::Request::type, std::list<TestMethod>>(Web::Request::type::HTTP_GET, {}));
    }

    virtual ~TestController()
    {
        for (auto& test : _tests) {
            test.second.clear();
        }
        _tests.clear();
    }

    // ITestController methods
    bool Reqister(const string& name, const string& desciption, const std::map<int, std::vector<string>>& input,
        const std::map<int, std::vector<string>>& output, Web::Request::type requestType,
        const std::function<Core::ProxyType<Web::Response>(const Web::Request&)>& processRequest) override;

    bool Unregister(const string& name) override;
    Core::ProxyType<Web::Response> Process(const Web::Request& request, uint8_t skipURL) override;

private:
    Core::ProxyType<Web::Response> GetMethods(void) override;
    Core::ProxyType<Web::Response> GetMethodDescription(string methodName) override;
    Core::ProxyType<Web::Response> GetMethodParameters(string methodName) override;

private:
    std::map<Web::Request::type, std::list<TestMethod>> _tests;
};

} // namespace TestCore
} // namespace Plugin
} // namespace WPEFramework
