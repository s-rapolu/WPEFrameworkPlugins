#include "MemoryAllocation.h"

namespace WPEFramework {
SERVICE_REGISTRATION(MemoryAllocation, 1, 0);

// ITest methods
void MemoryAllocation::Setup(const string& body){}
const string& /*JSON*/ MemoryAllocation::Execute(const string& method){ return ""; }
void MemoryAllocation::Cleanup(void){}

const std::list<string> MemoryAllocation::GetMethods(void){ return {}; }
const string& /*JSON*/ MemoryAllocation::GetMethodDes(const string& method){ return ""; }
const string& /*JSON*/ MemoryAllocation::GetMethodParam(const string& method){ return ""; }

// Memory Allocation methods
uint32_t MemoryAllocation::Malloc(uint32_t size) // size in Kb
{
    TRACE(Trace::Information, (_T("*** Allocate %lu Kb ***"), size))
    uint32_t noOfBlocks = 0;
    uint32_t blockSize = (32 * (getpagesize() >> 10)); // 128kB block size
    uint32_t runs = (uint32_t)size / blockSize;

    _lock.Lock();
    for (noOfBlocks = 0; noOfBlocks < runs; ++noOfBlocks)
    {
        _memory.push_back(malloc(static_cast<size_t>(blockSize << 10)));
        if (!_memory.back())
        {
            TRACE(Trace::Fatal, (_T("*** Failed allocation !!! ***")))
            break;
        }

        for (uint32_t index = 0; index < (blockSize << 10); index++)
        {
            static_cast<unsigned char*>(_memory.back())[index] = static_cast<unsigned char>(rand() & 0xFF);
        }
    }

    _currentMemoryAllocation += (noOfBlocks * blockSize);
    _lock.Unlock();

    return _currentMemoryAllocation;
}

void MemoryAllocation::Statm(uint32_t& allocated, uint32_t& size, uint32_t& resident)
{
    TRACE(Trace::Information, (_T("*** TestServiceImplementation::Statm ***")))

    _lock.Lock();
    allocated = _currentMemoryAllocation;
    _lock.Unlock();

    size = static_cast<uint32_t>(_process.Allocated() >> 10);
    resident = static_cast<uint32_t>(_process.Resident() >> 10);

    LogMemoryUsage();
}

void MemoryAllocation::Free(void)
{
    TRACE(Trace::Information, (_T("*** TestServiceImplementation::Free ***")))

    if (!_memory.empty())
    {
        for (auto const& memoryBlock : _memory)
        {
            free(memoryBlock);
        }
        _memory.clear();
    }

    _lock.Lock();
    _currentMemoryAllocation = 0;
    _lock.Unlock();

    LogMemoryUsage();
}

void MemoryAllocation::DisableOOMKill()
{
    int8_t oomNo = -17;
    _process.OOMAdjust(oomNo);
}

void MemoryAllocation::LogMemoryUsage(void)
{
    TRACE(Trace::Information, (_T("*** Current allocated: %lu Kb ***"), _currentMemoryAllocation))
    TRACE(Trace::Information, (_T("*** Initial Size:     %lu Kb ***"), _startSize))
    TRACE(Trace::Information, (_T("*** Initial Resident: %lu Kb ***"), _startResident))
    TRACE(Trace::Information, (_T("*** Size:     %lu Kb ***"), static_cast<uint32_t>(_process.Allocated() >> 10)))
    TRACE(Trace::Information, (_T("*** Resident: %lu Kb ***"), static_cast<uint32_t>(_process.Resident() >> 10)))
}
} // namespace WPEFramework
