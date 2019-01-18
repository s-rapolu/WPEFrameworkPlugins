#include "MemoryAllocation.h"

namespace WPEFramework {

// ITest methods
void MemoryAllocation::Setup(const string& body)
{
    //Store body locally
    _body = body;
}

string /*JSON*/ MemoryAllocation::Execute(const string& testCase)
{
    string response/*JSON*/ = EMPTY_STRING;

    response = ExecuteTestCase(testCase);

    if (response == EMPTY_STRING)
    {
        response = ExecuteTestCaseInfo(testCase);
    }
    return response;
}

string /*JSON*/ MemoryAllocation::ExecuteTestCase(const string& testCase)
{
    string response/*JSON*/ = EMPTY_STRING;

    for (auto const& test : _testCases)
    {
        if (test._name == testCase)
        {
            response = test._exec();
            break;
        }
    }
    return response;
}

string /*JSON*/ MemoryAllocation::ExecuteTestCaseInfo(const string& testCase)
{
    string response/*JSON*/ = EMPTY_STRING;

    for (auto const& testInfo : _testCasesInfo)
    {
        if (testInfo._name == testCase)
        {
            Core::TextSegmentIterator index(Core::TextFragment(testCase), false, '/');
            index.Next();
            response = testInfo._exec(testInfo._type, index.Current().Text());
            break;
        }
    }
    return response;
}

void MemoryAllocation::Cleanup(void)
{
    Free();
}

string /*JSON*/ MemoryAllocation::GetBody()
{
    return _body;
}

string /*JSON*/ MemoryAllocation::CreateResponse(const MemoryAllocation::ResponseType type, const string& method)
{
    string response = EMPTY_STRING;

    if (type == MemoryAllocation::ResponseType::TEST_CASE_RESULT)
    {
        MemoryOutputMetadata exeResponse;
        uint32_t allocated, size, resident;

        _lock.Lock();
        allocated = _currentMemoryAllocation;
        size = static_cast<uint32_t>(_process.Allocated() >> 10);
        resident = static_cast<uint32_t>(_process.Resident() >> 10);
        _lock.Unlock();

        exeResponse.Allocated = allocated;
        exeResponse.Resident = resident;
        exeResponse.Size = size;
        exeResponse.ToString(response);
    }
    else if (type == MemoryAllocation::ResponseType::TEST_CASES_LIST)
    {
        TestCore::TestCases testCasesListResponse;
        for (auto& testCase : _testCases)
        {
            Core::JSON::String name;
            name = testCase._name;
            testCasesListResponse.TestCaseNames.Add(name);
        }
        testCasesListResponse.ToString(response);
    }
    else if (type == MemoryAllocation::ResponseType::TEST_CASE_DESCRIPTION)
    {
        TestCore::TestCaseDescription testCaseDesResponse;
        for (auto& testCase : _testCases)
        {
            if(testCase._name == _T(method))
            {
                string description;
                description = testCase._description;
                testCaseDesResponse.Description = description;
                testCaseDesResponse.ToString(response);
                break;
            }
        }
    }
    else if (type == MemoryAllocation::ResponseType::TEST_CASE_PARAMETERS)
    {
    }

    return response;
}

// Memory Allocation methods
string /*JSON*/ MemoryAllocation::Malloc(void) // size in Kb
{
    uint32_t size;
    //Get body metadata
    string body = GetBody();
    MallocInputMetadata input;

    if (input.FromString(body))
    {
        size = input.Size;
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
    }
    else
    {
        TRACE(Trace::Fatal, (_T("*** Invalid POST Body, Memory is not allocated !!! ***")))
    }

    return CreateResponse(MemoryAllocation::ResponseType::TEST_CASE_RESULT, "");
}

string /*JSON*/ MemoryAllocation::Statm(void)
{
    TRACE(Trace::Information, (_T("*** TestServiceImplementation::Statm ***")))

    uint32_t allocated;
    uint32_t size;
    uint32_t resident;

    _lock.Lock();
    allocated = _currentMemoryAllocation;
    _lock.Unlock();

    size = static_cast<uint32_t>(_process.Allocated() >> 10);
    resident = static_cast<uint32_t>(_process.Resident() >> 10);

    return CreateResponse(MemoryAllocation::ResponseType::TEST_CASE_RESULT, "");
}

string /*JSON*/ MemoryAllocation::Free(void)
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

    return CreateResponse(MemoryAllocation::ResponseType::TEST_CASE_RESULT, "");
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

// ITest methods
void MemoryAllocation::Reqister(const string &name, const string &desciption, const std::function<string(void)> &testCaseCallback)
{
    TestCase newTestCase(name, desciption, testCaseCallback);
   _testCases.push_back(newTestCase);
}

void MemoryAllocation::ReqisterHelpers(const string &name, MemoryAllocation::ResponseType type, const std::function<string(MemoryAllocation::ResponseType, string)> &testCaseInfoCallback)
{
    TestCaseInfo newTestCaseInfo(name, type, testCaseInfoCallback);
    _testCasesInfo.push_back(newTestCaseInfo);
}
} // namespace WPEFramework
