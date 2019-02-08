#include "SystemCommands.h"

namespace WPEFramework{
namespace Plugin {

        SERVICE_REGISTRATION(SystemCommands, 1, 0);
        static Core::ProxyPoolType<Web::JSONBodyType<Core::JSON::ArrayType<SystemCommands::Param>>> jsonBodyResponseFactory(2);

        const string SystemCommands::Initialize(PluginHost::IShell* service)
        {
            string result;
            _skipURL = static_cast<uint8_t>(service->WebPrefix().length());
            uint32_t pid = 0;
            _implementation = service->Root<Exchange::ISystemCommands>(pid, 2000, _T("ISystemCommandsImplementation"));
            if (!_implementation)
                result = "Couln't create ISystemCommands instance.";
            return result;
        }

        void SystemCommands::Deinitialize(PluginHost::IShell* service)
        {
        }

        string SystemCommands::Information() const
        {
            return string();
        }

        void SystemCommands::Inbound(Web::Request& request)
        {
            if (request.Verb == Web::Request::HTTP_PUT || request.Verb == Web::Request::HTTP_POST)
                request.Body(jsonBodyResponseFactory.Element());
        }

        Core::ProxyType<Web::Response> SystemCommands::Process(const Web::Request& request)
        {
            ASSERT(_skipURL <= request.Path.length());

            Core::ProxyType<Web::Response> result(PluginHost::Factories::Instance().Response());
            Core::TextSegmentIterator index(Core::TextFragment(request.Path, _skipURL, request.Path.length() - _skipURL), false, '/');
            index.Next();

            result->ErrorCode = Web::STATUS_BAD_REQUEST;
            result->Message = _T("Unsupported request for the [SystemCommands] service.");

            if ((request.Verb == Web::Request::HTTP_PUT ||
                request.Verb == Web::Request::HTTP_POST) &&
                request.HasBody() && index.Next()) {

                if (index.Current().Text() == "USBReset") {
                    auto requestParams = request.Body<Web::JSONBodyType<Core::JSON::ArrayType<Param>>>();
                    Core::JSON::ArrayType<Param>::ConstIterator paramsIter = requestParams->Elements();
                    string device;
                    while (paramsIter.Next()) {
                        auto& param = paramsIter.Current();
                        if (param.Name == "device") {
                            device = param.Value;
                            break;
                        }
                    }

                    if (device.empty()) {
                        result->ErrorCode = Web::STATUS_BAD_REQUEST;
                        result->Message = _T("No device name given to USBReset system command.");
                    } else {
                        uint32_t status = _implementation->USBReset(device);
                        if (status != Core::ERROR_NONE) {
                            result->ErrorCode = Web::STATUS_INTERNAL_SERVER_ERROR;
                            result->Message = _T("ResetUSB failed for ") + device;
                        }
                    }
                }
            }

            return result;
        }

} // namespace Plugin
} // namespace WPEFramework
