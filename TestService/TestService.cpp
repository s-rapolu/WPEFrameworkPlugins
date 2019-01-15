#include "TestService.h"

using namespace std::placeholders;

namespace WPEFramework
{
namespace TestService
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
} // namespace TestService

namespace Plugin
{
    SERVICE_REGISTRATION(TestService, 1, 0);

    static Core::ProxyPoolType<Web::JSONBodyType<TestService::Data> > jsonDataFactory(2);

    /* virtual */ const string TestService::Initialize(PluginHost::IShell* service)
    {
        /*Assume that everything is OK*/
        string message = EMPTY_STRING;
        Config config;

        ASSERT(service != nullptr);
        ASSERT(_service == nullptr);
        ASSERT(_implementation == nullptr);
        ASSERT(_memory == nullptr);

        _service = service;
        _skipURL = static_cast<uint8_t>(_service->WebPrefix().length());
        _service->Register(&_notification);

        _implementation = _service->Root<Exchange::ITestService>(_pid, ImplWaitTime, _T("TestServiceImplementation"));

        if ((_implementation != nullptr) && (_service != nullptr))
        {
            _memory = WPEFramework::TestService::MemoryObserver(_pid);
            ASSERT(_memory != nullptr);
            _memory->Observe(_pid);

            ///////////////////// Start - Test Methods Definition: Memory ///////////////////////
            auto statm = std::bind(&TestService::Statm, this, _1);
            auto malloc = std::bind(&TestService::Malloc, this, _1);
            auto free = std::bind(&TestService::Free, this, _1);

            //ToDo: Try to simplify this initialization of functions parameters
            std::vector<std::string> statmIn_0({ "void", "", "no input argument required" });
            std::map<int, std::vector<string>> statmIn = {{0, statmIn_0}};

            std::vector<std::string> mallocIn_0({ "uint32_t", "size", "allocate block of memory [Kb]" });
            std::map<int, std::vector<string>> mallocIn = {{0, mallocIn_0}};

            std::vector<std::string> freeIn_0({ "void", "", "no input argument required" });
            std::map<int, std::vector<string>> freeIn = {{0, freeIn_0}};
            std::vector<std::string> freeOut_0({ "void", "", "no output argument returned" });
            std::map<int, std::vector<string>> freeOut = {{0, freeOut_0}};

            std::vector<std::string> memOut_0({ "uint32_t", "allocated", "current allocated memory [Kb]" });
            std::vector<std::string> memOut_1({ "uint32_t", "size", "size value from /proc/self/statm [Kb]" });
            std::vector<std::string> memOut_2({ "uint32_t", "resident", "resident value from /proc/self/statm [Kb]" });
            std::map<int, std::vector<string>> memOut = {{0, memOut_0}, {1, memOut_1}, {2, memOut_2}};

            _controller.Reqister("Statm", "Get memory allocation statistics", statmIn, memOut, Web::Request::type::HTTP_GET, statm);
            _controller.Reqister("Malloc", "Allocate memory", mallocIn, memOut, Web::Request::type::HTTP_POST, malloc);
            _controller.Reqister("Free", "Free memory", freeIn, freeOut, Web::Request::type::HTTP_POST, free);
            ///////////////////// End - Test Methods Definition: Memory ///////////////////////

            ///////////////////// Start - Test Methods Definition: Crash ///////////////////////
            auto crash = std::bind(&TestService::Crash, this, _1);
            auto crashNTimes = std::bind(&TestService::CrashNTimes, this, _1);

            //ToDo: Try to simplify this initialization of functions parameters
            std::vector<std::string> crashIn_0({ "void", "", "no input argument required" });
            std::map<int, std::vector<string>> crashIn = {{0, crashIn_0}};
            std::vector<std::string> crashOut_0({ "void", "", "no output argument returned" });
            std::map<int, std::vector<string>> crashOut = {{0, crashOut_0}};

            std::vector<std::string> crashNTimesIn_0({ "uint8_t", "crashCount", "desired consequtive crash count" });
            std::map<int, std::vector<string>> crashNTimesIn = {{0, crashNTimesIn_0}};
            std::vector<std::string> crashNTimesOut_0({ "void", "", "no output argument returned" });
            std::map<int, std::vector<string>> crashNTimesOut = {{0, crashNTimesOut_0}};

            _controller.Reqister("Crash", "Causes plugin to crash", crashIn, crashOut, Web::Request::type::HTTP_POST, crash);
            _controller.Reqister("CrashNTimes", "Causes plugin to crash N times in  row", crashNTimesIn, crashNTimesOut, Web::Request::type::HTTP_POST, crashNTimes);
            ///////////////////// End - Test Methods Definition: Crash ///////////////////////

            _implementation->Configure(_service);
            _implementation->ExecPendingCrash();
        }
        else
        {
            ProcessTermination(_pid);
            _service = nullptr;
            _service->Unregister(&_notification);

            SYSLOG(Trace::Fatal, (_T("*** TestService could not be instantiated ***")))
            message = _T("TestService could not be instantiated.");
        }
        return message;
    }

    /* virtual */ void TestService::Deinitialize(PluginHost::IShell* service)
    {
        ASSERT(_service == service);
        ASSERT(_implementation != nullptr);
        ASSERT(_memory != nullptr);
        ASSERT(_pid);

        SYSLOG(Trace::Information, (_T("*** OutOfProcess Plugin is properly destructed. PID: %d ***"), _pid))

        //ToDo: Unregister tests are missing

        ProcessTermination(_pid);
        _implementation = nullptr;
        _memory->Release();
        _memory = nullptr;
        _service->Unregister(&_notification);
        _service = nullptr;
    }

    /* virtual */ string TestService::Information() const
    {
        // No additional info to report.
        return ((_T("The purpose of [%s] plugin is enabling expanding of the memory consumption."), _pluginName.c_str()));
    }

    /* virtual */ void TestService::Inbound(Web::Request& request)
    {
        if (request.Verb == Web::Request::HTTP_POST)
        {
            request.Body(jsonDataFactory.Element());
        }
    }

    /* virtual */ Core::ProxyType<Web::Response> TestService::Process(const Web::Request& request)
    {
        ASSERT(_skipURL <= request.Path.length());

        return _controller.Process(request, _skipURL);
    }

    void TestService::ProcessTermination(uint32_t pid)
    {
        RPC::IRemoteProcess* process(_service->RemoteProcess(pid));
        if (process != nullptr)
        {
            process->Terminate();
            process->Release();
        }
    }

    void TestService::Activated(RPC::IRemoteProcess* /*process*/)
    {
        return;
    }

    void TestService::Deactivated(RPC::IRemoteProcess* process)
    {
        if (_pid == process->Id())
        {
            ASSERT(_service != nullptr);
            PluginHost::WorkerPool::Instance().Submit(PluginHost::IShell::Job::Create(_service, PluginHost::IShell::DEACTIVATED, PluginHost::IShell::FAILURE));
        }
    }

    // Tests API
    Core::ProxyType<Web::Response> TestService::Statm(const Web::Request& request)
    {
        Core::ProxyType<Web::Response> result(PluginHost::Factories::Instance().Response());
        result->ErrorCode = Web::STATUS_OK;
        result->Message = (_T("Handle Statm request for the [%s] service"), _pluginName.c_str());

        Core::ProxyType<Web::JSONBodyType<Data>> response(jsonDataFactory.Element());
        GetStatm(response->Memory);

        result->ContentType = Web::MIMETypes::MIME_JSON;
        result->Body(Core::proxy_cast<Web::IBody>(response));

        return result;
    }

    Core::ProxyType<Web::Response> TestService::Malloc(const Web::Request& request)
    {
        Core::ProxyType<Web::Response> result(PluginHost::Factories::Instance().Response());
        result->ErrorCode = Web::STATUS_OK;
        result->Message = (_T("Handle Malloc request for the [%s] service"), _pluginName.c_str());

        uint32_t memAllocation = request.Body<const Data>()->Malloc.Size.Value();

        ASSERT(_implementation != nullptr)
        _implementation->Malloc(memAllocation);

        Core::ProxyType<Web::JSONBodyType<Data>> response(jsonDataFactory.Element());
        GetStatm(response->Memory);

        result->ContentType = Web::MIMETypes::MIME_JSON;
        result->Body(Core::proxy_cast<Web::IBody>(response));

        return result;
    }

    Core::ProxyType<Web::Response> TestService::Free(const Web::Request& request)
    {
        Core::ProxyType<Web::Response> result(PluginHost::Factories::Instance().Response());
        result->ErrorCode = Web::STATUS_OK;
        result->Message = (_T("Handle Free request for the [%s] service"), _pluginName.c_str());

        ASSERT(_implementation != nullptr)
        _implementation->Free();

        return result;
    }

    Core::ProxyType<Web::Response> TestService::Crash(const Web::Request& request)
    {
        Core::ProxyType<Web::Response> result(PluginHost::Factories::Instance().Response());
        result->ErrorCode = Web::STATUS_OK;
        result->Message = (_T("Handle Crash request for the [%s] service"), _pluginName.c_str());

        ASSERT(_implementation != nullptr)
        _implementation->Crash();

        return result;
    }

    Core::ProxyType<Web::Response> TestService::CrashNTimes(const Web::Request& request)
    {
        Core::ProxyType<Web::Response> result(PluginHost::Factories::Instance().Response());
        result->ErrorCode = Web::STATUS_OK;
        result->Message = (_T("Handle CrashNTimes request for the [%s] service"), _pluginName.c_str());

        if(request.HasBody()){
            uint8_t crashCount = request.Body<const Data>()->Crash.CrashCount.Value();
            ASSERT(_implementation != nullptr)
            _implementation->CrashNTimes(crashCount);
        }
        else
        {
            result->ErrorCode = Web::STATUS_BAD_REQUEST;
            result->Message = (_T("Missing body for CrashNTimes request"));
        }

        return result;
    }

    void TestService::GetStatm(Data::Statm& statm)
    {
        ASSERT(_implementation != nullptr)
        uint32_t allocated, size, resident;

        _implementation->Statm(allocated, size, resident);

        statm.Allocated = allocated;
        statm.Size = size;
        statm.Resident = resident;
    }
} // namespace Plugin
} // namespace WPEFramework
