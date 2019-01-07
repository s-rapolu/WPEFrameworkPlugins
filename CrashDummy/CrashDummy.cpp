#include "CrashDummy.h"

namespace WPEFramework {
namespace Plugin {

SERVICE_REGISTRATION(CrashDummy, 1, 0);

// IPlugin overrides
/* virtual */ const string CrashDummy::Initialize(PluginHost::IShell* shell)
{
    TRACE(Trace::Information, ("%s", __FUNCTION__));

    string message;

    ASSERT(_shell == nullptr);
    _shell = shell;

    ASSERT(_implementation == nullptr);
    _pid = 0;

    // Register the Process::Notification stuff. The Remote process might die before we get a
    // change to "register" the sink for these events !!! So do it ahead of instantiation.
    _shell->Register(&_notification);
    _implementation = _shell->Root<Exchange::ICrashDummy>(_pid, 1000, _T("CrashDummyImplementation"));
    if (_implementation == nullptr){
        message = _T("CrashDummy could not be instantiated.");
        _shell->Unregister(&_notification);
        _shell = nullptr;
    }
    else {
        TRACE(Trace::Information, ("CrashDummy Plugin initialized %p", _implementation));
    }

    _skipURL = static_cast<uint8_t>(_shell->WebPrefix().length());

    return message;
}

/* virtual */ void CrashDummy::Deinitialize(PluginHost::IShell* shell)
{
    TRACE(Trace::Information, ("%s", __FUNCTION__));

    ASSERT(_shell == shell);
    ASSERT(_implementation != nullptr);

    _shell->Unregister(&_notification);

    if (_implementation->Release() != Core::ERROR_DESTRUCTION_SUCCEEDED) {
        ASSERT(_pid != 0);

        TRACE(Trace::Information, ("CrashDummy Plugin is not properly destructed. %d", _pid));
        RPC::IRemoteProcess* process(_shell->RemoteProcess(_pid));

        // The process can disappear in the meantime...
        if (process != nullptr) {
            // But if it did not dissapear in the meantime, forcefully terminate it. Shoot to kill :-)
            process->Terminate();
            process->Release();
        }
    }

    // Deinitialize what we initialized..
    _shell = nullptr;
    _implementation = nullptr;

    return;
}

/* virtual */ string CrashDummy::Information() const
{
    TRACE(Trace::Information, ("%s", __FUNCTION__));

    return "";
}

// IWeb overrides
/* virtual */ void CrashDummy::Inbound(Web::Request& request)
{
    TRACE(Trace::Information, ("%s", __FUNCTION__));

    return;
}

/* virtual */ Core::ProxyType<Web::Response> CrashDummy::Process(const Web::Request& request)
{
    TRACE(Trace::Information, ("%s", __FUNCTION__));

    // Proxy object for response type.
    Core::ProxyType<Web::Response> response(PluginHost::Factories::Instance().Response());

    // Default is not allowed.
    response->Message = _T("Method not allowed");
    response->ErrorCode = Web::STATUS_METHOD_NOT_ALLOWED;

    return (response);
}

void CrashDummy::Deactivated(RPC::IRemoteProcess* process)
{
    if (process->Id() == _pid) {

        ASSERT(_shell != nullptr);

        PluginHost::WorkerPool::Instance().Submit(PluginHost::IShell::Job::Create(
            _shell,
            PluginHost::IShell::DEACTIVATED,
            PluginHost::IShell::FAILURE));
    }
}

}
}