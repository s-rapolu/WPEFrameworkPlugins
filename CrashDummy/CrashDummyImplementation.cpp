#include "CrashDummyImplementation.h"

namespace WPEFramework {
namespace Plugin {

SERVICE_REGISTRATION(CrashDummyImplementation, 1, 0);

bool CrashDummyImplementation::Configure(PluginHost::IShell* shell)
{
    ASSERT(shell != nullptr);
    bool status = _config.FromString(shell->ConfigLine());
    if (status) {
        _crashDelay = _config.CrashDelay.Value();
        TRACE(Trace::Information, ("crash delay set to %d", _crashDelay));
    } else {
        TRACE(Trace::Information, ("crash delay default %d", _crashDelay));
    }

    return status;
}

void CrashDummyImplementation::Crash()
{
    TRACE(Trace::Information, (_T("Preparing for crash...")));
    sleep(_crashDelay);

    TRACE(Trace::Information, (_T("Executing crash!")));
    uint8_t* tmp = nullptr;
    *tmp = 3; // segmentaion fault

    return;
}

} // namespace Plugin
} // namespace WPEFramework
