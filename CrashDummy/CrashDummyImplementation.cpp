#include "CrashDummyImplementation.h"

#include <fstream>

namespace WPEFramework {
namespace Plugin {

#define PENDING_CRASH_FILEPATH "/tmp/CrashDummy.pending"

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

bool CrashDummyImplementation::CrashNTimes(uint8_t n)
{
    bool status = true;
    uint8_t pendingCrashCount = PendingCrashCount();

    if (pendingCrashCount != 0) {
        status = false;
        TRACE(Trace::Information, (_T("Pending crash already in progress")));
    } else {
        if (!SetPendingCrashCount(n)) {
            TRACE(Trace::Fatal, (_T("Failed to set new pending crash count")));
            status = false;
        } else {
            ExecPendingCrash();
        }
    }

    return status;
}

void CrashDummyImplementation::ExecPendingCrash()
{
    uint8_t pendingCrashCount = PendingCrashCount();
    if (pendingCrashCount > 0) {
        pendingCrashCount--;
        if (SetPendingCrashCount(pendingCrashCount)) {
            Crash();
        } else {
            TRACE(Trace::Fatal, (_T("Failed to set new pending crash count")));
        }
    } else {
        TRACE(Trace::Information, (_T("No pending crash")));
    }
}

uint8_t CrashDummyImplementation::PendingCrashCount()
{
    uint8_t pendingCrashCount = 0;

    std::ifstream pendingCrashFile;
    pendingCrashFile.open(PENDING_CRASH_FILEPATH, std::fstream::binary);

    if (pendingCrashFile.is_open()) {
        uint8_t readVal = 0;

        pendingCrashFile >> readVal;
        if (pendingCrashFile.good()) {
            pendingCrashCount = readVal;
        } else {
            TRACE(Trace::Information, (_T("Failed to read value from pendingCrashFile")));
        }
    }

    return pendingCrashCount;
}

bool CrashDummyImplementation::SetPendingCrashCount(uint8_t newCrashCount)
{
    bool status = false;

    std::ofstream pendingCrashFile;
    pendingCrashFile.open(PENDING_CRASH_FILEPATH, std::fstream::binary | std::fstream::trunc);

    if (pendingCrashFile.is_open()) {

        pendingCrashFile << newCrashCount;

        if (pendingCrashFile.good()) {
            status = true;
        } else {
            TRACE(Trace::Information, (_T("Failed to write value to pendingCrashFile ")));
        }
        pendingCrashFile.close();
    }

    return status;
}
} // namespace Plugin
} // namespace WPEFramework
