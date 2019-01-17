#pragma once

#include "Module.h"
#include "ITestSuite.h"
#include "TestController.h"

#include <functional>
using namespace std::placeholders;

namespace WPEFramework {

class MemoryAllocation : public TestCore::ITestSuite {
private:
    class MemoryMetadata : public Core::JSON::Container
    {
        public:
            class StatmMetadata : public Core::JSON::Container
            {
                private:
                    StatmMetadata(const StatmMetadata&) = delete;
                    StatmMetadata& operator=(const StatmMetadata&) = delete;

                public:
                    StatmMetadata()
                        : Core::JSON::Container()
                        , Allocated(0)
                        , Size(0)
                        , Resident(0)
                    {
                        Add(_T("allocated"), &Allocated);
                        Add(_T("size"), &Size);
                        Add(_T("resident"), &Resident);
                    }
                    ~StatmMetadata()
                    {
                    }

                public:
                    Core::JSON::DecSInt32 Allocated;
                    Core::JSON::DecSInt32 Size;
                    Core::JSON::DecSInt32 Resident;
            };

            class MallocMetadata : public Core::JSON::Container
            {
                private:
                    MallocMetadata(const MallocMetadata&) = delete;
                    MallocMetadata& operator=(const MallocMetadata&) = delete;

                public:
                    MallocMetadata()
                        : Core::JSON::Container()
                        , Size(0)
                    {
                        Add(_T("size"), &Size);
                    }
                    ~MallocMetadata()
                    {
                    }

                public:
                    Core::JSON::DecSInt32 Size;
            };

        private:
            MemoryMetadata(const MemoryMetadata&) = delete;
            MemoryMetadata& operator=(const MemoryMetadata&) = delete;

        public:
            MemoryMetadata()
                : Core::JSON::Container()
                , Statm()
                , Malloc()
            {
                Add(_T("statm"), &Statm);
                Add(_T("malloc"), &Malloc);
            }

            virtual ~MemoryMetadata()
            {
            }

        public:
            StatmMetadata Statm;
            MallocMetadata Malloc;
    };

    class TestCase
    {
        private:
        TestCase& operator=(const TestCase&) = delete;

        public:
        TestCase(string name, string desciption, const std::function<string(void)> &testCase)
                : _name(name)
                , _description(desciption)
                , _exec(testCase)
            {
            }

            virtual ~TestCase() {}

        public:
            string _name;
            string _description;
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
        , _body()
    {
        DisableOOMKill();
        _startSize = static_cast<uint32_t>(_process.Allocated() >> 10);
        _startResident = static_cast<uint32_t>(_process.Resident() >> 10);

        Reqister("Statm", "Get memory allocation statistics", std::bind(&MemoryAllocation::Statm, this));
        Reqister("Malloc", "Allocate memory", std::bind(&MemoryAllocation::Malloc, this));
        Reqister("Free", "Free memory", std::bind(&MemoryAllocation::Free, this));
        Reqister("TestCases", "", std::bind(&MemoryAllocation::GetTestCases, this));//ToDo: Consider to change this registration

        ReqisterHelpers("Statm/Description", std::bind(&MemoryAllocation::GetTestCaseDescription, this, _1));
        ReqisterHelpers("Statm/Parameters", std::bind(&MemoryAllocation::GetTestCaseParameters, this, _1));
        ReqisterHelpers("Free/Description", std::bind(&MemoryAllocation::GetTestCaseDescription, this, _1));
        ReqisterHelpers("Free/Parameters", std::bind(&MemoryAllocation::GetTestCaseParameters, this, _1));
        ReqisterHelpers("Malloc/Description", std::bind(&MemoryAllocation::GetTestCaseDescription, this, _1));
        ReqisterHelpers("Malloc/Parameters", std::bind(&MemoryAllocation::GetTestCaseParameters, this, _1));

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

    void Reqister(const string &name, const string &desciption, const std::function<string(void)> &testCaseCallback);
    void ReqisterHelpers(const string &name, const std::function<string(string)> &testCaseInfoCallback);
    string /*JSON*/ GetBody(const string& testCase);

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

    string _body;
    std::list<TestCase> _testCases;
    std::list<TestCaseInfo> _testCasesInfo;
};

static MemoryAllocation* _singleton(Core::Service<MemoryAllocation>::Create<MemoryAllocation>());

} // namespace WPEFramework
