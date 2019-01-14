#include "MallocDummy.h"

using namespace std::placeholders;

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
                        _observable = true;
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

    static Core::ProxyPoolType<Web::JSONBodyType<MallocDummy::Data> > jsonDataFactory(2);

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

        _mallocDummy = _service->Root<Exchange::IMallocDummy>(_pid, 2000, _T("MallocDummyImplementation"));

        if ((_mallocDummy != nullptr) && (_service != nullptr))
        {
            _memory = WPEFramework::MallocDummy::MemoryObserver(_pid);
            ASSERT(_memory != nullptr);
            _memory->Observe(_pid);

            ///////////////////// Start - Test Methods Definition ///////////////////////
            auto statm = std::bind(&MallocDummy::Statm, this, _1);
            auto malloc = std::bind(&MallocDummy::Malloc, this, _1);
            auto free = std::bind(&MallocDummy::Free, this, _1);

            _client.Reqister("Statm", Web::Request::type::HTTP_GET, statm);
            _client.Reqister("Malloc", Web::Request::type::HTTP_POST, malloc);
            _client.Reqister("Free", Web::Request::type::HTTP_POST, free);
            ///////////////////// End - Test Methods Definition ///////////////////////
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

        SYSLOG(Trace::Information, (_T("*** OutOfProcess Plugin is properly destructed. PID: %d ***"), _pid))

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
            request.Body(jsonDataFactory.Element());
        }
    }

    /* virtual */ Core::ProxyType<Web::Response> MallocDummy::Process(const Web::Request& request)
    {
        ASSERT(_skipURL <= request.Path.length());

        return _client.Process(request, _skipURL);
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

    // Tests API
    Core::ProxyType<Web::Response> MallocDummy::Statm(const Web::Request& request)
    {
        Core::ProxyType<Web::Response> result(PluginHost::Factories::Instance().Response());
        result->ErrorCode = Web::STATUS_OK;
        result->Message = (_T("Handle Malloc request for the [%s] service"), _pluginName.c_str());

        Core::ProxyType<Web::JSONBodyType<Data>> response(jsonDataFactory.Element());
        GetStatm(response->Memory);

        result->ContentType = Web::MIMETypes::MIME_JSON;
        result->Body(Core::proxy_cast<Web::IBody>(response));

        return result;
    }

    Core::ProxyType<Web::Response> MallocDummy::Malloc(const Web::Request& request)
    {
        Core::ProxyType<Web::Response> result(PluginHost::Factories::Instance().Response());
        result->ErrorCode = Web::STATUS_OK;
        result->Message = (_T("Handle POST request for the [%s] service"), _pluginName.c_str());

        uint32_t memAllocation = request.Body<const Data>()->Malloc.Size.Value();

        ASSERT(_mallocDummy != nullptr)
        _mallocDummy->Malloc(memAllocation);

        Core::ProxyType<Web::JSONBodyType<Data>> response(jsonDataFactory.Element());
        GetStatm(response->Memory);

        result->ContentType = Web::MIMETypes::MIME_JSON;
        result->Body(Core::proxy_cast<Web::IBody>(response));

        return result;
    }

    Core::ProxyType<Web::Response> MallocDummy::Free(const Web::Request& request)
    {
        Core::ProxyType<Web::Response> result(PluginHost::Factories::Instance().Response());
        result->ErrorCode = Web::STATUS_OK;
        result->Message = (_T("Handle Free request for the [%s] service"), _pluginName.c_str());

        ASSERT(_mallocDummy != nullptr)
        _mallocDummy->Free();

        return result;
    }

    void MallocDummy::GetStatm(Data::Statm& statm)
    {
        ASSERT(_mallocDummy != nullptr)
        uint32_t allocated, size, resident;

        _mallocDummy->Statm(allocated, size, resident);

        statm.Allocated = allocated;
        statm.Size = size;
        statm.Resident = resident;
    }
} // namespace Plugin
} // namespace WPEFramework
