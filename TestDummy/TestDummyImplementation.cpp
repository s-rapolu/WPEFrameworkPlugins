#include "TestDummyImplementation.h"

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace WPEFramework {
SERVICE_REGISTRATION(TestDummyImplementation, 1, 0);

// ITestDummy methods
uint32_t TestDummyImplementation::Malloc(uint32_t size) // size in Kb
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

void TestDummyImplementation::Statm(uint32_t& allocated, uint32_t& size, uint32_t& resident)
{
    _lock.Lock();

    SYSLOG(Trace::Information, (_T("*** TestDummyImplementation::Statm ***")))

    allocated = _currentMemoryAllocation;
    size = static_cast<uint32_t>(_process.Allocated() >> 10);
    resident = static_cast<uint32_t>(_process.Resident() >> 10);

    LogMemoryUsage();
    _lock.Unlock();
}

void TestDummyImplementation::Free(void)
{
    _lock.Lock();

    SYSLOG(Trace::Information, (_T("*** TestDummyImplementation::Free ***")))

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

void TestDummyImplementation::DisableOOMKill()
{
    int8_t oomNo = -17;
    _process.OOMAdjust(oomNo);
}

void TestDummyImplementation::LogMemoryUsage(void)
{
    SYSLOG(Trace::Information, (_T("*** Current allocated: %lu Kb ***"), _currentMemoryAllocation))
    SYSLOG(Trace::Information, (_T("*** Initial Size:     %lu Kb ***"), _startSize))
    SYSLOG(Trace::Information, (_T("*** Initial Resident: %lu Kb ***"), _startResident))
    SYSLOG(Trace::Information, (_T("*** Size:     %lu Kb ***"), static_cast<uint32_t>(_process.Allocated() >> 10)))
    SYSLOG(Trace::Information, (_T("*** Resident: %lu Kb ***"), static_cast<uint32_t>(_process.Resident() >> 10)))
}

bool Configure(PluginHost::IShell* shell)
{
    // ToDo: add implementation
    return true;
}

void Crash()
{
    // ToDo: add implementation
    return;
}

bool CrashNTimes(uint8_t n)
{
    // ToDo: add implementation
    return true;
}

void ExecPendingCrash()
{
    // ToDo: add implementation
    return;
}

uint8_t PendingCrashCount()
{
    // ToDo: add implementation
    return 0;
}

} // namespace WPEFramework
