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
    _skipURL = static_cast<uint8_t>(_shell->WebPrefix().length());

    return message;
}

/* virtual */ void CrashDummy::Deinitialize(PluginHost::IShell* shell)
{
    TRACE(Trace::Information, ("%s", __FUNCTION__));

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

}
}