#include "Module.h"
#include "SystemCommands.h"


namespace WPEFramework
{

    using Exchange::ISystemCommands;
    namespace Plugin {

        SERVICE_REGISTRATION(SystemCommands, 1, 0);
        static Core::ProxyPoolType<Web::JSONBodyType<Core::JSON::ArrayType<SystemCommands::Data>>> jsonBodyResponseFactory(4);
        static Core::ProxyPoolType<Web::TextBody> jsonBodyRequestFactory(4);

        SystemCommands::SystemCommands()
            : _skipURL(0)
            , _commands(nullptr)
        {
        }

        SystemCommands::~SystemCommands()
        {
        }

        void SystemCommands::Deinitialize(PluginHost::IShell* service)
        {
        }

        const string SystemCommands::Initialize(PluginHost::IShell* service)
        {
            ASSERT (service != nullptr);

            _skipURL = static_cast<uint8_t>(service->WebPrefix().length());
            uint32_t pid = 0;
            string result = EMPTY_STRING;

            _commands = service->Root<Exchange::ISystemCommands>(pid, 2000, _T("SystemCommandsImplementation"));
            if (!_commands)
                result = _T("Couldn't create System Commands service");
            return result;
        }

        string SystemCommands::Information() const
        {
            return string();
        }

        void SystemCommands::Inbound(Web::Request& request)
        {
            if (request.Verb == Web::Request::HTTP_POST || request.Verb == Web::Request::HTTP_PUT)
                request.Body(jsonBodyRequestFactory.Element());
        }

        Core::ProxyType<Web::Response> SystemCommands::Process(const Web::Request& request)
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
                    auto response = jsonBodyResponseFactory.Element();
                    RPC::IStringIterator* iter = _commands->SupportedCommands();
                    while (iter->Next()) {
                        ISystemCommands::ICommand* command = _commands->Command(iter->Current());
                        Data commandData;
                        commandData.Command = command->Id();
                        commandData.Description = command->Description();
                        response->Add(commandData);
                    }
                    result->Body(Core::proxy_cast<Web::IBody>(response));
                    result->ContentType = Web::MIME_JSON;
                } else {
                    result->ErrorCode = Web::STATUS_BAD_REQUEST;
                    result->Message = _T("Unsupported request for the [SystemCommand] service.");
                }
            } else if ((request.Verb == Web::Request::HTTP_POST || request.Verb == Web::Request::HTTP_PUT) &&
                        index.Next()) {
                std::string params;
                if (request.HasBody()) {
                   params = static_cast<std::string>(*request.Body<Web::TextBody>());
                }
                if (_commands->Execute(index.Current().Text(), params)) {
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
