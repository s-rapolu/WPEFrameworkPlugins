#include "MallocDummy.h"

namespace WPEFramework
{
    namespace Plugin {

        SERVICE_REGISTRATION(MallocDummy, 1, 0);

        static Core::ProxyPoolType<Web::JSONBodyType<MallocDummy::Data> > jsonResponseFactory(4);

        /* virtual */ const string MallocDummy::Initialize(PluginHost::IShell* service)
        {
            ASSERT (_service == nullptr);
            ASSERT (service != nullptr);

            _service = service;
            _skipURL = static_cast<uint8_t>(_service->WebPrefix().length());

            SYSLOG(Trace::Fatal, (_T("*** Plugin: [%s] has been started ***"), _pluginName.c_str()))
            // On success return empty, to indicate there is no error text.
            return (_service != nullptr) ?  EMPTY_STRING : (_T("Could not initialize plugin: [%s]."), _pluginName.c_str());
        }

        /* virtual */ void MallocDummy::Deinitialize(PluginHost::IShell* service)
        {
            ASSERT (_service == service);
            SYSLOG(Trace::Fatal,  (_T("*** Plugin: [%s] has been finished ***"), _pluginName.c_str()))
            _service = nullptr;
        }

        /* virtual */ string MallocDummy::Information() const
        {
            // No additional info to report.
            return ((_T("The purpose of [%s] plugin is enabling expanding of the memory consumption."), _pluginName.c_str()));
        }

        /* virtual */ void MallocDummy::Inbound(Web::Request& /* request */)
        {
        }

        /* virtual */ Core::ProxyType<Web::Response> MallocDummy::Process(const Web::Request& request)
        {
            ASSERT(_skipURL <= request.Path.length());

            Core::ProxyType<Web::Response> result(PluginHost::Factories::Instance().Response());

            // By default, we assume everything works..
            result->ErrorCode = Web::STATUS_OK;
            result->Message = "OK";

            // <GET> - currently, only the GET command is supported, returning system info
            if (request.Verb == Web::Request::HTTP_GET)
            {

                SYSLOG(Trace::Fatal,  (_T("*** Plugin: [%s]: Handle GET request ***"), _pluginName.c_str()))

                Core::ProxyType<Web::JSONBodyType<Data>> response(jsonResponseFactory.Element());
                Core::TextSegmentIterator index(Core::TextFragment(request.Path, _skipURL, request.Path.length() - _skipURL), false, '/');

                // Always skip the first one, it is an empty part because we start with a '/' if there are more parameters.
                index.Next();

                if (index.Next() == false)
                {
                    result->ErrorCode = Web::STATUS_BAD_REQUEST;
                    result->Message = (_T("Unsupported request for the [%s] service."), _pluginName.c_str());
                }
                else if (index.Current() == "MemoryInfo")
                {
                    GetMemoryInfo(response->Memory);
                }

                result->ContentType = Web::MIMETypes::MIME_JSON;
                result->Body(Core::proxy_cast<Web::IBody>(response));

                result->Message = (_T("Handle request for the [%s] service, no data for now."), _pluginName.c_str());
            }
            else
            {
                result->ErrorCode = Web::STATUS_BAD_REQUEST;
                result->Message = (_T("Unsupported request for the [%s] service."), _pluginName.c_str());
            }

            return result;
        }

        void MallocDummy::GetMemoryInfo(Data::MemoryInfo& memoryInfo)
        {
            memoryInfo.CurrentAllocation = _currentMemoryAllocation;
        }

    } // namespace Plugin
} // namespace WPEFramework
