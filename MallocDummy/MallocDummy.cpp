#include "MallocDummy.h"

namespace WPEFramework
{
namespace MallocDummy
{
    Exchange::IMemory* MemoryObserver(const uint32_t pid)
    {
        class MemoryObserverImpl : public Exchange::IMemory
        {
            private:
                MemoryObserverImpl();
                MemoryObserverImpl(const MemoryObserverImpl&);
                MemoryObserverImpl& operator=(const MemoryObserverImpl&);

            public:
                MemoryObserverImpl(const uint32_t id)
                    : _main(id == 0 ? Core::ProcessInfo().Id() : id)
                    , _observable(false)
                {
                }
                ~MemoryObserverImpl() {}

            public:
                virtual void Observe(const uint32_t pid)
                {
                    if (pid == 0)
                    {
                        _observable = false;
                    }
                    else
                    {
                        _observable = false;
                        _main = Core::ProcessInfo(pid);
                    }
                }
                virtual uint64_t Resident() const
                {
                    return (_observable == false ? 0 : _main.Resident());
                }

                virtual uint64_t Allocated() const
                {
                    return (_observable == false ? 0 : _main.Allocated());
                }

                virtual uint64_t Shared() const
                {
                    return (_observable == false ? 0 : _main.Shared());
                }

                virtual uint8_t Processes() const
                {
                    return (IsOperational() ? 1 : 0);
                }

                virtual const bool IsOperational() const
                {
                    return (_observable == false) || (_main.IsActive());
                }

                BEGIN_INTERFACE_MAP(MemoryObserverImpl)
                    INTERFACE_ENTRY(Exchange::IMemory)
                END_INTERFACE_MAP

            private:
                Core::ProcessInfo _main;
                bool _observable;
        };

        return (Core::Service<MemoryObserverImpl>::Create<Exchange::IMemory>(pid));
    }
} // namespace MallocDummy

namespace Plugin
{
    SERVICE_REGISTRATION(MallocDummy, 1, 0);

    /*QA: What is a purpose of the jsonResponseFactory argument */
    static Core::ProxyPoolType<Web::JSONBodyType<MallocDummy::Data> > jsonBodyDataFactory(4);
    static Core::ProxyPoolType<Web::JSONBodyType<MallocDummy::Data> > jsonResponseFactory(4);

    /* virtual */ const string MallocDummy::Initialize(PluginHost::IShell* service)
    {
        /*Assume that everything is OK*/
        string message = EMPTY_STRING;
        Config config;

        ASSERT(service != nullptr);
        ASSERT(_service == nullptr);
        ASSERT(_mallocDummy == nullptr);
        ASSERT(_memory == nullptr);

        _service = service;
        _skipURL = static_cast<uint8_t>(_service->WebPrefix().length());
        _service->Register(&_notification);

        config.FromString(_service->ConfigLine());
        if (config.OutOfProcess.Value())
        {
            SYSLOG(Trace::Fatal, (_T("*** Out of process implementation ***")))
            _mallocDummy = _service->Root<Exchange::IMallocDummy>(_pid, 2000, _T("MallocDummyImplementation"));
        }
        else
        {
            SYSLOG(Trace::Fatal, (_T("*** In process implementation ***")))
            _mallocDummy = Core::ServiceAdministrator::Instance().Instantiate<Exchange::IMallocDummy>(Core::Library(), _T("MallocDummyImplementation"), static_cast<uint32_t>(~0));
        }

        if ((_mallocDummy != nullptr) && (_service != nullptr))
        {
            _memory = WPEFramework::MallocDummy::MemoryObserver(_pid);
            ASSERT(_memory != nullptr);
            _memory->Observe(true);
        }
        else
        {
            ProcessTermination(_pid);
            _service = nullptr;
            _service->Unregister(&_notification);

            SYSLOG(Trace::Fatal, (_T("*** MallocDummy could not be instantiated ***")))
            message = _T("MallocDummy could not be instantiated.");
        }
        return message;
    }

    /* virtual */ void MallocDummy::Deinitialize(PluginHost::IShell* service)
    {
        ASSERT(_service == service);
        ASSERT(_mallocDummy != nullptr);
        ASSERT(_memory != nullptr);
        ASSERT(_pid);

        SYSLOG(Trace::Fatal, (_T("*** OutOfProcess Plugin is not properly destructed. PID: %d ***"), _pid))

        ProcessTermination(_pid);
        _mallocDummy = nullptr;
        _memory->Release();
        _memory = nullptr;
        _service->Unregister(&_notification);
        _service = nullptr;
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

        bool status = false;
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
                status = true;
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
                status = true;
            }
        }

        if (status == false)
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

    void MallocDummy::ProcessTermination(uint32_t pid)
    {
        RPC::IRemoteProcess* process(_service->RemoteProcess(pid));
        if (process != nullptr)
        {
            process->Terminate();
            process->Release();
        }
    }

    void MallocDummy::Deactivated(RPC::IRemoteProcess* process)
    {
        if (_pid == process->Id())
        {
            ASSERT(_service != nullptr);
            PluginHost::WorkerPool::Instance().Submit(PluginHost::IShell::Job::Create(_service, PluginHost::IShell::DEACTIVATED, PluginHost::IShell::FAILURE));
        }
    }

} // namespace Plugin
} // namespace WPEFramework
