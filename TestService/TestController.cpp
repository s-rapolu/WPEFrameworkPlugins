#include "TestController.h"

#include "TestData.h"

#include <list>

namespace WPEFramework {
namespace Plugin {
namespace TestCore {

static Core::ProxyPoolType<Web::JSONBodyType<TestData::Methods>> methodsFactory(2);
static Core::ProxyPoolType<Web::JSONBodyType<TestData::MethodDescription>> methodDescriptionFactory(2);
static Core::ProxyPoolType<Web::JSONBodyType<TestData::Parameters>> parametersFactory(2);

// ITestController methods
bool TestController::Reqister(const string& name, const string& desciption,
    const std::map<int, std::vector<string>>& input, const std::map<int, std::vector<string>>& output,
    Web::Request::type requestType,
    const std::function<Core::ProxyType<Web::Response>(const Web::Request&)>& processRequest)
{
    bool status = false;
    TestMethod newTestMethod(name, desciption, input, output, processRequest);

    for (auto& test : _tests)
    {
        if (test.first == requestType)
        {
            test.second.push_back(newTestMethod);
            status = true;
        }
    }

    if (!status)
    {
        SYSLOG(Trace::Fatal, (_T("Request type : %d is not supported"), requestType))
    }

    return status;
}

bool TestController::Unregister(const string& name)
{
    // ToDo: Missing implementation
    return false;
}

Core::ProxyType<Web::Response> TestController::Process(const Web::Request& request, uint8_t skipURL)
{
    bool exit = false;
    Core::TextSegmentIterator index(
        Core::TextFragment(request.Path, skipURL, request.Path.length() - skipURL), false, '/');
    Core::ProxyType<Web::Response> result(PluginHost::Factories::Instance().Response());
    result->Message = _T("Method not supported");
    result->ErrorCode = Web::STATUS_BAD_REQUEST;

    index.Next();
    index.Next();

    if (index.Current().Text() != _T("Methods"))
    {
        for (auto const& test : _tests)
        {
            if (test.first == request.Verb)
            {
                for (auto const& method : test.second)
                {
                    if (method._name == index.Current().Text())
                    {
                        result = method._callback(request);
                        exit = true;
                        break;
                    }
                }
            }
            if (exit)
                break;
        }
    }
    else // Handle default /Methods/... request separately
    {
        if (!index.Next())
        {
            result = GetMethods();
        }
        else
        {
            string methodName = index.Current().Text();
            index.Next();
            string callType = index.Current().Text();
            if (callType == _T("Description"))
            {
                result = GetMethodDescription(methodName);
            }
            else if (callType == _T("Parameters"))
            {
                result = GetMethodParameters(methodName);
            }
        }
    }

    return result;
}

Core::ProxyType<Web::Response> TestController::GetMethods(void)
{
    Core::ProxyType<Web::Response> result(PluginHost::Factories::Instance().Response());
    result->ErrorCode = Web::STATUS_OK;
    result->Message = (_T("Handle Methods request"));

    Core::ProxyType<Web::JSONBodyType<TestData::Methods>> response(methodsFactory.Element());
    for (auto const& test : _tests)
    {
        for (auto const& method : test.second)
        {
            Core::JSON::String newMethod;
            newMethod = method._name;
            response->MethodNames.Add(newMethod);
        }
    }

    result->ContentType = Web::MIMETypes::MIME_JSON;
    result->Body(Core::proxy_cast<Web::IBody>(response));

    return result;
}

Core::ProxyType<Web::Response> TestController::GetMethodDescription(string methodName)
{
    bool exit = false;
    Core::ProxyType<Web::Response> result(PluginHost::Factories::Instance().Response());
    result->ErrorCode = Web::STATUS_OK;
    result->Message = (_T("Handle Methods Description request"));

    Core::ProxyType<Web::JSONBodyType<TestData::MethodDescription>> response(methodDescriptionFactory.Element());
    for (auto const& test : _tests)
    {
        for (auto const& method : test.second)
        {
            if (method._name == methodName)
            {
                response->Description = method._description;
                exit = true;
                break;
            }
        }
        if (exit)
            break;
    }

    if (!exit)
    {
        result->Message = _T("Method not supported");
        result->ErrorCode = Web::STATUS_BAD_REQUEST;
    }

    result->ContentType = Web::MIMETypes::MIME_JSON;
    result->Body(Core::proxy_cast<Web::IBody>(response));

    return result;
}

Core::ProxyType<Web::Response> TestController::GetMethodParameters(string methodName)
{
    bool exit = false;
    Core::ProxyType<Web::Response> result(PluginHost::Factories::Instance().Response());
    result->ErrorCode = Web::STATUS_OK;
    result->Message = (_T("Handle Methods Parameters request"));

    Core::ProxyType<Web::JSONBodyType<TestData::Parameters>> response(parametersFactory.Element());
    for (auto const& test : _tests)
    {
        for (auto const& method : test.second)
        {
            if (method._name == methodName)
            {
                // ToDo: Fill Parameters
                for (auto const& in : method._input)
                {
                    TestData::Parameters::Parameter newInParam;
                    newInParam.Type = in.second[0];
                    newInParam.Name = in.second[1];
                    newInParam.Comment = in.second[2];
                    response->Input.Add(newInParam);
                }

                for (auto const& out : method._output)
                {
                    TestData::Parameters::Parameter newOutParam;
                    newOutParam.Type = out.second[0];
                    newOutParam.Name = out.second[1];
                    newOutParam.Comment = out.second[2];
                    response->Output.Add(newOutParam);
                }
                exit = true;
                break;
            }
        }
        if (exit)
            break;
    }

    if (!exit)
    {
        result->Message = _T("Method not supported");
        result->ErrorCode = Web::STATUS_BAD_REQUEST;
    }

    result->ContentType = Web::MIMETypes::MIME_JSON;
    result->Body(Core::proxy_cast<Web::IBody>(response));

    return result;
}

} // namespace TestCore
} // namespace Plugin
} // namespace WPEFramework
