#include "MemoryAllocation.h"

namespace WPEFramework {

// ITest methods
void MemoryAllocation::Setup(const string& body)
{
    SYSLOG(Trace::Fatal, (_T("*** Call MemoryAllocation::Setup ***")))
    //Store body locally
    _body = body;
}

string /*JSON*/ MemoryAllocation::Execute(const string& testCase)
{
    SYSLOG(Trace::Fatal, (_T("*** Call MemoryAllocation::ExecuteTest ***")))
    string result = "";
    bool extecuted = false;

    for (auto const& test : _testCases)
    {
        if (test._name == testCase)
        {
            result = test._exec();
            extecuted = true;
            break;
        }
    }

    if (extecuted == false) //Check helper request list
    {
        for (auto const& testInfo : _testCasesInfo)
        {
            if (testInfo._name == testCase)
            {
                result = testInfo._exec(testCase);
                extecuted = true;
                break;
            }
        }
    }

    return result;
}

void MemoryAllocation::Cleanup(void)
{
    SYSLOG(Trace::Fatal, (_T("*** Call MemoryAllocation::Cleanup ***")))
}

string /*JSON*/ MemoryAllocation::GetTestCases(void)
{
    SYSLOG(Trace::Fatal, (_T("*** Call MemoryAllocation::GetTestCases ***")))
    return "";
}

string /*JSON*/ MemoryAllocation::GetTestCaseDescription(const string& testCase)
{
    SYSLOG(Trace::Fatal, (_T("*** Call MemoryAllocation::GetTestCaseDescription %s ***"), testCase.c_str()))
    return "";
}

string /*JSON*/ MemoryAllocation::GetTestCaseParameters(const string& testCase)
{
    SYSLOG(Trace::Fatal, (_T("*** Call MemoryAllocation::GetTestCaseParameters %s ***"), testCase.c_str()))
    return "";
}

string /*JSON*/ MemoryAllocation::GetBody(const string& testCase)
{
    return "";
}

// Memory Allocation methods
string MemoryAllocation::Malloc(void) // size in Kb
{
    uint32_t size = 100;//ToDo: Get size from Body using string /*JSON*/ GetBody(void);
    SYSLOG(Trace::Fatal, (_T("*** Call MemoryAllocation::Malloc ***")))
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

    return "";//ToDo: Create full JSON response in string format
}

string MemoryAllocation::Statm(void)
{
    SYSLOG(Trace::Fatal, (_T("*** Call MemoryAllocation::Statm ***")))
    TRACE(Trace::Information, (_T("*** TestServiceImplementation::Statm ***")))

    uint32_t allocated;
    uint32_t size;
    uint32_t resident;

    _lock.Lock();
    allocated = _currentMemoryAllocation;
    _lock.Unlock();

    size = static_cast<uint32_t>(_process.Allocated() >> 10);
    resident = static_cast<uint32_t>(_process.Resident() >> 10);

    LogMemoryUsage();
    return "";//ToDo: Create full JSON response in string format
}

string MemoryAllocation::Free(void)
{
    SYSLOG(Trace::Fatal, (_T("*** Call MemoryAllocation::Free ***")))
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
    return "";//ToDo: Create full JSON response in string format
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

    SYSLOG(Trace::Fatal, (_T("*** Current allocated: %lu Kb ***"), _currentMemoryAllocation))
    SYSLOG(Trace::Fatal, (_T("*** Initial Size:     %lu Kb ***"), _startSize))
    SYSLOG(Trace::Fatal, (_T("*** Initial Resident: %lu Kb ***"), _startResident))
    SYSLOG(Trace::Fatal, (_T("*** Size:     %lu Kb ***"), static_cast<uint32_t>(_process.Allocated() >> 10)))
    SYSLOG(Trace::Fatal, (_T("*** Resident: %lu Kb ***"), static_cast<uint32_t>(_process.Resident() >> 10)))
}

// ITest methods
void MemoryAllocation::Reqister(const string &name, const string &desciption, const std::function<string(void)> &testCaseCallback)
{
    TestCase newTestCase(name, desciption, testCaseCallback);
   _testCases.push_back(newTestCase);
}

void MemoryAllocation::ReqisterHelpers(const string &name, const std::function<string(string)> &testCaseInfoCallback)
{
    TestCaseInfo newTestCaseInfo(name, testCaseInfoCallback);
    _testCasesInfo.push_back(newTestCaseInfo);
}
} // namespace WPEFramework
