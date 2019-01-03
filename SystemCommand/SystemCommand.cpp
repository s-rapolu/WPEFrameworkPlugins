#include "Module.h"
#include "SystemCommand.h"


namespace WPEFramework
{

    using Exchange::ISystemCommand;
    namespace Plugin {

        SERVICE_REGISTRATION(SystemCommand, 1, 0);
        static Core::ProxyPoolType<Web::JSONBodyType<Core::JSON::ArrayType<Core::JSON::String>>> jsonBodyResponseFactory(4);
        static Core::ProxyPoolType<Web::JSONBodyType<Core::JSON::ArrayType<SystemCommand::Param>>>
                jsonBodyRequestFactory(4);

        SystemCommand::SystemCommand()
            : _skipURL(0)
            , _executor(nullptr)
        {
        }

        SystemCommand::~SystemCommand()
        {
        }

        void SystemCommand::Deinitialize(PluginHost::IShell* service)
        {
        }

        const string SystemCommand::Initialize(PluginHost::IShell* service)
        {
            ASSERT (service != nullptr);

            _config.FromString(service->ConfigLine());
            _skipURL = static_cast<uint8_t>(service->WebPrefix().length());
            uint32_t pid = 0;

            _executor = service->Root<Exchange::ISystemCommand>(pid, 2000, _T("SystemCommandImplementation"));

            return EMPTY_STRING;
        }

        string SystemCommand::Information() const
        {
            return string();
        }

        void SystemCommand::Inbound(Web::Request& request)
        {
            if (request.Verb == Web::Request::HTTP_POST || request.Verb == Web::Request::HTTP_PUT)
                request.Body(jsonBodyRequestFactory.Element());
        }

        Core::ProxyType<Web::Response> SystemCommand::Process(const Web::Request& request)
        {
            ASSERT(_skipURL <= request.Path.length());

            Core::ProxyType<Web::Response> result(PluginHost::Factories::Instance().Response());
            Core::TextSegmentIterator index(Core::TextFragment(request.Path, _skipURL, request.Path.length() - _skipURL), false, '/');
            result->ErrorCode = Web::STATUS_OK;
            result->Message = "OK";

            // Always skip the first one, it is an empty part because we start with a '/' if there are more parameters.
            index.Next();

            if (request.Verb == Web::Request::HTTP_GET) {
                if (!index.Next()) {
                    auto data = jsonBodyResponseFactory.Element();
                    const auto& commands = _executor->SupportedCommands();
                    for (auto command : commands) {
                        Core::JSON::String newValue(command);
                        data->Add(newValue);
                    }
                    result->Body(Core::proxy_cast<Web::IBody>(data));
                    result->ContentType = Web::MIME_JSON;
                } else {
                    result->ErrorCode = Web::STATUS_BAD_REQUEST;
                    result->Message = _T("Unsupported request for the [SystemCommand] service.");
                }
            } else if ((request.Verb == Web::Request::HTTP_POST || request.Verb == Web::Request::HTTP_PUT) &&
                        index.Next()) {
                ISystemCommand::CommandParams commandParams;
                Core::JSON::ArrayType<Param>::ConstIterator paramsIter;
                if (!request.HasBody()) {
                    auto defaultParamsIter = _config.DefaultParams.Elements();
                    while (defaultParamsIter.Next()) {
                        if (index.Current() == defaultParamsIter.Current().CommandName.Value()) {
                            const auto& constParams = defaultParamsIter.Current().Params;
                            paramsIter = constParams.Elements();
                            break;
                        }
                    }
                } else {
                   auto params = request.Body<Web::JSONBodyType<Core::JSON::ArrayType<Param>>>();
                   paramsIter = params->Elements();
                }

                while (paramsIter.Next()) {
                     auto& param = paramsIter.Current();
                     ISystemCommand::CommandParam commandParam;
                     commandParam.name = param.Name;
                     commandParam.value = param.Value;
                     commandParams.push_back(commandParam);
                }
                if (!_executor->Execute(index.Current().Text(), commandParams)) {
                    result->ErrorCode = Web::STATUS_BAD_REQUEST;
                    result->Message = _T("Execution of ") + index.Current().Text() + _T(" has failed.");
                }
            } else {
                result->ErrorCode = Web::STATUS_BAD_REQUEST;
                result->Message = _T("Unsupported request for the [SystemCommand] service.");
            }

            return result;
        }
    } // namespace Plugin
} // namespace WPEFramework
