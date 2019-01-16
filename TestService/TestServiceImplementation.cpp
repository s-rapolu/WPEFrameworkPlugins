#include "TestServiceImplementation.h"

#include <fstream>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace WPEFramework {
SERVICE_REGISTRATION(TestServiceImplementation, 1, 0);

#define PENDING_CRASH_FILEPATH "/tmp/TestService.pending"

// ITestService methods
uint32_t TestServiceImplementation::Malloc(uint32_t size) // size in Kb
{
    _lock.Lock();

    SYSLOG(Trace::Information, (_T("*** Allocate %lu Kb ***"), size))
    uint32_t noOfBlocks = 0;
    uint32_t blockSize = (32 * (getpagesize() >> 10)); // 128kB block size
    uint32_t runs = (uint32_t)size / blockSize;

    for (noOfBlocks = 0; noOfBlocks < runs; ++noOfBlocks) {
        _memory.push_back(malloc(static_cast<size_t>(blockSize << 10)));
        if (!_memory.back()) {
            SYSLOG(Trace::Fatal, (_T("*** Failed allocation !!! ***")))
            break;
        }

        for (uint32_t index = 0; index < (blockSize << 10); index++) {
            static_cast<unsigned char*>(_memory.back())[index] = static_cast<unsigned char>(rand() & 0xFF);
        }
    }

    _currentMemoryAllocation += (noOfBlocks * blockSize);

    _lock.Unlock();

    return _currentMemoryAllocation;
}

void TestServiceImplementation::Statm(uint32_t& allocated, uint32_t& size, uint32_t& resident)
{
    _lock.Lock();

    SYSLOG(Trace::Information, (_T("*** TestServiceImplementation::Statm ***")))

    allocated = _currentMemoryAllocation;
    size = static_cast<uint32_t>(_process.Allocated() >> 10);
    resident = static_cast<uint32_t>(_process.Resident() >> 10);

    LogMemoryUsage();
    _lock.Unlock();
}

void TestServiceImplementation::Free(void)
{
    _lock.Lock();

    SYSLOG(Trace::Information, (_T("*** TestServiceImplementation::Free ***")))

    if (!_memory.empty()) {
        for (auto const& memoryBlock : _memory) {
            free(memoryBlock);
        }
        _memory.clear();
    }

    _currentMemoryAllocation = 0;

    LogMemoryUsage();
    _lock.Unlock();
}

void TestServiceImplementation::DisableOOMKill()
{
    int8_t oomNo = -17;
    _process.OOMAdjust(oomNo);
}

void TestServiceImplementation::LogMemoryUsage(void)
{
    SYSLOG(Trace::Information, (_T("*** Current allocated: %lu Kb ***"), _currentMemoryAllocation))
    SYSLOG(Trace::Information, (_T("*** Initial Size:     %lu Kb ***"), _startSize))
    SYSLOG(Trace::Information, (_T("*** Initial Resident: %lu Kb ***"), _startResident))
    SYSLOG(Trace::Information, (_T("*** Size:     %lu Kb ***"), static_cast<uint32_t>(_process.Allocated() >> 10)))
    SYSLOG(Trace::Information, (_T("*** Resident: %lu Kb ***"), static_cast<uint32_t>(_process.Resident() >> 10)))
}

bool TestServiceImplementation::Configure(PluginHost::IShell* shell)
{
    ASSERT(shell != nullptr);
    _lock.Lock();

    bool status = _config.FromString(shell->ConfigLine());
    if (status) {
        _crashDelay = _config.CrashDelay.Value();
        TRACE(Trace::Information, (_T("Set crash delay = %d"), _crashDelay));
    } else {
        TRACE(Trace::Information, (_T("Set default crash delay = %d"), _crashDelay));
    }

    _lock.Unlock();

    return status;
}

void TestServiceImplementation::Crash()
{
    TRACE(Trace::Information, (_T("Crash in %d..."), _crashDelay));
    sleep(_crashDelay);
    TRACE(Trace::Information, (_T("Executing crash!")));

    uint8_t* tmp = nullptr;
    *tmp = 3; // segmentaion fault

    return;
}

bool TestServiceImplementation::CrashNTimes(uint8_t n)
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

void TestServiceImplementation::ExecPendingCrash()
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

    return;
}

uint8_t TestServiceImplementation::PendingCrashCount()
{
    uint8_t pendingCrashCount = 0;
    std::ifstream pendingCrashFile;

    _lock.Lock();

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

    _lock.Unlock();

    return pendingCrashCount;
}

bool TestServiceImplementation::SetPendingCrashCount(uint8_t newCrashCount)
{
    bool status = false;
    _lock.Lock();

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

    _lock.Unlock();

    return status;
}

} // namespace WPEFramework
