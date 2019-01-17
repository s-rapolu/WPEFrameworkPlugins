#pragma once

#include "Module.h"

#include "TestController.h"

#include <functional>
using namespace std::placeholders;

namespace WPEFramework {

class MemoryAllocation : public WPEFramework::TestCore::ITestSuite {
private:
    class TestCase
    {
        private:
        TestCase& operator=(const TestCase&) = delete;

        public:
        TestCase(string name, string desciption, const std::map<int, std::vector<string>> &input, const std::map<int, std::vector<string>> &output, const std::function<string(void)> &testCase)
                : _name(name)
                , _description(desciption)
                , _input(input)
                , _output(output)
                , _exec(testCase)
            {
            }

            virtual ~TestCase() {}

        public:
            string _name;
            string _description;
            std::map<int, std::vector<string>> _input; //ToDo: Replace by class type
            std::map<int, std::vector<string>> _output; //ToDo: Replace by class type
            std::function<string(void)> _exec;
    };

    class TestCaseInfo
    {
        private:
        TestCaseInfo& operator=(const TestCaseInfo&) = delete;

        public:
        TestCaseInfo(string name, const std::function<string(string)> &testCaseInfo)
                : _name(name)
                , _exec(testCaseInfo)
            {
            }

            virtual ~TestCaseInfo() {}

        public:
            string _name;
            std::function<string(string)> _exec;
    };

private:
    MemoryAllocation(const MemoryAllocation&) = delete;
    MemoryAllocation& operator=(const MemoryAllocation&) = delete;

public:
    MemoryAllocation()
        : _currentMemoryAllocation(0)
        , _lock()
        , _process()
    {
        DisableOOMKill();
        _startSize = static_cast<uint32_t>(_process.Allocated() >> 10);
        _startResident = static_cast<uint32_t>(_process.Resident() >> 10);

        auto statm = std::bind(&MemoryAllocation::Statm, this);
        auto malloc = std::bind(&MemoryAllocation::Malloc, this);
        auto free = std::bind(&MemoryAllocation::Free, this);
        auto testCases = std::bind(&MemoryAllocation::GetTestCases, this);

        auto testCaseDescription = std::bind(&MemoryAllocation::GetTestCaseDescription, this, _1);
        auto testCaseParameters = std::bind(&MemoryAllocation::GetTestCaseParameters, this, _1);

        Reqister("Statm", "Get memory allocation statistics", {}, {}, statm);
        Reqister("Malloc", "Allocate memory", {}, {}, malloc);
        Reqister("Free", "Free memory", {}, {}, free);
        Reqister("TestCases", "", {}, {}, testCases);//ToDo: Change this implementation

        ReqisterHelpers("Statm/Description", testCaseDescription);
        ReqisterHelpers("Statm/Parameters", testCaseParameters);
        ReqisterHelpers("Free/Description", testCaseDescription);
        ReqisterHelpers("Free/Parameters", testCaseParameters);
        ReqisterHelpers("Malloc/Description", testCaseDescription);
        ReqisterHelpers("Malloc/Parameters", testCaseParameters);

        TestCore::TestController::Instance().AnnounceTestSuite(*this, "Memory");
    }

    virtual ~MemoryAllocation() { Free(); }

public:
    // ITest methods
    virtual void Setup(const string& body) override;
    virtual string /*JSON*/ Execute(const string& testCase) override;
    virtual void Cleanup(void) override;

    //ToDo: Consider to change this interface
    string /*JSON*/ GetTestCases(void) override;
    string /*JSON*/ GetTestCaseDescription(const string& testCase) override;
    string /*JSON*/ GetTestCaseParameters(const string& testCase) override;

private:
    string /*JSON*/ Malloc(void);
    string /*JSON*/ Statm(void);
    string /*JSON*/ Free(void);

    void Reqister(const string &name, const string &desciption, const std::map<int, std::vector<string>> &input, const std::map<int, std::vector<string>> &output, const std::function<string(void)> &testCaseCallback);
    void ReqisterHelpers(const string &name, const std::function<string(string)> &testCaseInfoCallback);
    string /*JSON*/ GetBody(void);
    void DisableOOMKill(void);
    void LogMemoryUsage(void);

    BEGIN_INTERFACE_MAP(MemoryAllocation)
        INTERFACE_ENTRY(TestCore::ITestSuite)
    END_INTERFACE_MAP

private:
    uint32_t _startSize;
    uint32_t _startResident;
    Core::CriticalSection _lock;
    Core::ProcessInfo _process;
    std::list<void*> _memory;
    uint32_t _currentMemoryAllocation; // size in Kb
    std::list<TestCase> _testCases;
    std::list<TestCaseInfo> _testCasesInfo;
};

static MemoryAllocation* _singleton(Core::Service<MemoryAllocation>::Create<MemoryAllocation>());

} // namespace WPEFramework
