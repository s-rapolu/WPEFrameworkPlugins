#include "TestService.h"

namespace WPEFramework {
namespace TestService {

Exchange::IMemory* MemoryObserver(const uint32_t pid)
{
    class MemoryObserverImpl : public Exchange::IMemory {
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
        virtual uint64_t Resident() const { return (_observable == false ? 0 : _main.Resident()); }

        virtual uint64_t Allocated() const { return (_observable == false ? 0 : _main.Allocated()); }

        virtual uint64_t Shared() const { return (_observable == false ? 0 : _main.Shared()); }

        virtual uint8_t Processes() const { return (IsOperational() ? 1 : 0); }

        virtual const bool IsOperational() const { return (_observable == false) || (_main.IsActive()); }

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

namespace Plugin {
SERVICE_REGISTRATION(TestService, 1, 0);

/* virtual */ const string TestService::Initialize(PluginHost::IShell* service)
{
    /*Assume that everything is OK*/
    SYSLOG(Trace::Fatal, (_T("*** TestService::Deinitialize 1 ***")))
    string message = EMPTY_STRING;
    Config config;

    ASSERT(service != nullptr);
    ASSERT(_service == nullptr);
    ASSERT(_testsControllerImp == nullptr);
    ASSERT(_memory == nullptr);

    _service = service;
    _skipURL = static_cast<uint8_t>(_service->WebPrefix().length());
    _service->Register(&_notification);

    _testsControllerImp = Core::ServiceAdministrator::Instance().Instantiate<Exchange::ITestController>(Core::Library(), _T("TestControllerImplementation"), static_cast<uint32_t>(~0));
    //_testsControllerImp = _service->Root<Exchange::ITestController>(_pid, ImplWaitTime, _T("TestControllerImplementation"));
    SYSLOG(Trace::Fatal, (_T("*** TestService::Deinitialize 2***")))

    if ((_testsControllerImp != nullptr) && (_service != nullptr))
    {
        _memory = WPEFramework::TestService::MemoryObserver(_pid);
        ASSERT(_memory != nullptr);
        _memory->Observe(_pid);
        SYSLOG(Trace::Fatal, (_T("*** TestService::Deinitialize 3***")))
    }
    else
    {
        ProcessTermination(_pid);
        _service = nullptr;
        _testsControllerImp = nullptr;
        _service->Unregister(&_notification);

        TRACE(Trace::Fatal, (_T("*** TestService could not be instantiated ***")))
        message = _T("TestService could not be instantiated.");
        SYSLOG(Trace::Fatal, (_T("*** TestService::Deinitialize 4***")))
    }

    return message;
}

/* virtual */ void TestService::Deinitialize(PluginHost::IShell* service)
{
    ASSERT(_service == service);
    ASSERT(_testsControllerImp != nullptr);
    ASSERT(_memory != nullptr);
    ASSERT(_pid);

    SYSLOG(Trace::Fatal, (_T("*** TestService::Deinitialize ***")))
    TRACE(Trace::Information, (_T("*** OutOfProcess Plugin is properly destructed. PID: %d ***"), _pid))

    ProcessTermination(_pid);
    _testsControllerImp = nullptr;
    _memory->Release();
    _memory = nullptr;
    _service->Unregister(&_notification);
    _service = nullptr;
}

/* virtual */ string TestService::Information() const
{
    // No additional info to report.
    return ((_T("The purpose of [%s] plugin is proivde ability to execute ffunctional tests."), _pluginName.c_str()));
}

/* virtual */ void TestService::Inbound(Web::Request& request)
{
    if (request.Verb == Web::Request::HTTP_POST)
    {
        //ToDo: Handle body if it is needed
    }
}

/* virtual */ Core::ProxyType<Web::Response> TestService::Process(const Web::Request& request)
{
    string responseBody;
    ASSERT(_skipURL <= request.Path.length());
    Core::ProxyType<Web::Response> result(PluginHost::Factories::Instance().Response());

    SYSLOG(Trace::Fatal, (_T("*** TestService::Process path: %s ***"), request.Path.c_str()))
    responseBody = _testsControllerImp->Process(request.Path, _skipURL, "");
    //ToDo: Convert responseBody to valid format

    result->ErrorCode = Web::STATUS_OK;
    result->Message = (_T("Handle Methods Description request"));
    result->ContentType = Web::MIMETypes::MIME_JSON;

    return result;
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
} // namespace Plugin
} // namespace WPEFramework
