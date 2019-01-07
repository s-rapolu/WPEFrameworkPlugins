#include "MallocDummy.h"

namespace WPEFramework
{
    namespace Plugin {

        SERVICE_REGISTRATION(MallocDummy, 1, 0);

        /*QA: What is a purpose of the jsonResponseFactory argument */
        static Core::ProxyPoolType<Web::JSONBodyType<MallocDummy::Data> > jsonBodyDataFactory(4);
        static Core::ProxyPoolType<Web::JSONBodyType<MallocDummy::Data> > jsonResponseFactory(4);

        /* virtual */ const string MallocDummy::Initialize(PluginHost::IShell* service)
        {
            string message;

            ASSERT (_service == nullptr);
            ASSERT(_mallocDummy == nullptr);
            ASSERT (service != nullptr);

            _service = service;
            _skipURL = static_cast<uint8_t>(_service->WebPrefix().length());

            //ToDo: Based on configuration run it in process or out of process (see TVControl for implementation)
            _mallocDummy = Core::ServiceAdministrator::Instance().Instantiate<Exchange::IMallocDummy>(Core::Library(), _T("MallocDummyImplementation"), static_cast<uint32_t>(~0));
            if (_mallocDummy == nullptr)
            {
                SYSLOG(Trace::Fatal, (_T("*** MallocDummy could not be instantiated ***")))
                message = _T("MallocDummy could not be instantiated.");
            }
            else
            {
                message = (_service != nullptr) ?  EMPTY_STRING : (_T("Could not initialize plugin: [%s]."), _pluginName.c_str());
            }

            SYSLOG(Trace::Fatal, (_T("*** Plugin: [%s] has been started ***"), _pluginName.c_str()))

            // On success return empty, to indicate there is no error text.
            return message;
        }

        /* virtual */ void MallocDummy::Deinitialize(PluginHost::IShell* service)
        {
            ASSERT (_service == service);
            ASSERT(_mallocDummy != nullptr);

            SYSLOG(Trace::Fatal, (_T("*** Plugin: [%s] has been finished ***"), _pluginName.c_str()))
            _service = nullptr;
            _mallocDummy = nullptr;
        }

        /* virtual */ string MallocDummy::Information() const
        {
            // No additional info to report.
            return ((_T("The purpose of [%s] plugin is enabling expanding of the memory consumption."), _pluginName.c_str()));
        }

        /* virtual */ void MallocDummy::Inbound(Web::Request& request)
        {
            if (request.Verb == Web::Request::HTTP_POST)
            {
                request.Body(jsonBodyDataFactory.Element());
            }
        }

        /* virtual */ Core::ProxyType<Web::Response> MallocDummy::Process(const Web::Request& request)
        {
            ASSERT(_skipURL <= request.Path.length());

            Core::ProxyType<Web::Response> result(PluginHost::Factories::Instance().Response());
            result->ErrorCode = Web::STATUS_OK;
            result->Message = "OK";

            Core::TextSegmentIterator index(Core::TextFragment(request.Path, _skipURL, request.Path.length() - _skipURL), false, '/');
            // Common response for GET and POST, all the time return memoryInfo
            Core::ProxyType<Web::JSONBodyType<Data>> response(jsonResponseFactory.Element());

            // <GET> - returning current memory allocation
            if ((request.Verb == Web::Request::HTTP_GET) && ((index.Next() == true) && (index.Next() == true)))
            {
                SYSLOG(Trace::Fatal, (_T("*** Plugin: [%s]: Handle GET request ***"), _pluginName.c_str()))

                if (index.Current() == _T("MemoryInfo"))
                {
                    GetMemoryInfo(response->Memory);

                    result->ContentType = Web::MIMETypes::MIME_JSON;
                    result->Body(Core::proxy_cast<Web::IBody>(response));
                    result->Message = (_T("Handle GET request for the [%s] service"), _pluginName.c_str());
                }
                else
                {
                    result->ErrorCode = Web::STATUS_BAD_REQUEST;
                    result->Message = (_T("Unsupported request for the [%s] service."), _pluginName.c_str());
                }
            }
            // <POST> - set memory allocation value
            else if ((request.Verb == Web::Request::HTTP_POST) && ((index.Next() == true) && (index.Next() == true)))
            {
                SYSLOG(Trace::Fatal, (_T("*** Plugin: [%s]: Handle POST request ***"), _pluginName.c_str()))

                if ((index.Current() == _T("SetMemoryAllocation")) && (request.HasBody()))
                {
                    uint64_t memAllocation = request.Body<const Data>()->Memory.CurrentAllocation.Value();
                    SYSLOG(Trace::Fatal, (_T("*** Set memory allocation: [%d] ***"), memAllocation))

                    ASSERT(_mallocDummy != nullptr)
                    _mallocDummy->Malloc(memAllocation);

                    GetMemoryInfo(response->Memory);
                    result->ContentType = Web::MIMETypes::MIME_JSON;
                    result->Body(Core::proxy_cast<Web::IBody>(response));
                    result->Message = (_T("Handle POST request for the [%s] service"), _pluginName.c_str());
                }
                else
                {
                    result->ErrorCode = Web::STATUS_BAD_REQUEST;
                    result->Message = (_T("Unsupported request for the [%s] service."), _pluginName.c_str());
                }
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
            ASSERT(_mallocDummy != nullptr)

            memoryInfo.CurrentAllocation = _mallocDummy->GetAllocatedMemory();
        }

    } // namespace Plugin
} // namespace WPEFramework
