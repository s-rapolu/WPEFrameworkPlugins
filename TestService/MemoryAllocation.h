#pragma once

#include "Module.h"
#include "ITestSuite.h"
#include "TestController.h"
#include "TestCaseMetadata.h"

#include <functional>
using namespace std::placeholders;

namespace WPEFramework {

class MemoryAllocation : public TestCore::ITestSuite {
    private:

        enum class ResponseType { TEST_CASE_RESULT, TEST_CASES_LIST, TEST_CASE_DESCRIPTION, TEST_CASE_PARAMETERS};

        class MemoryOutputMetadata : public Core::JSON::Container {
            private:
                MemoryOutputMetadata(const MemoryOutputMetadata&) = delete;
                MemoryOutputMetadata& operator=(const MemoryOutputMetadata&) = delete;

            public:
                MemoryOutputMetadata()
                    : Core::JSON::Container()
                    , Allocated(0)
                    , Size(0)
                    , Resident(0)
                {
                    Add(_T("allocated"), &Allocated);
                    Add(_T("size"), &Size);
                    Add(_T("resident"), &Resident);
                }
                ~MemoryOutputMetadata()
                {
                }

            public:
                Core::JSON::DecSInt32 Allocated;
                Core::JSON::DecSInt32 Size;
                Core::JSON::DecSInt32 Resident;
        };

        class MallocInputMetadata : public Core::JSON::Container {
            private:
                MallocInputMetadata(const MallocInputMetadata&) = delete;
                MallocInputMetadata& operator=(const MallocInputMetadata&) = delete;

            public:
                MallocInputMetadata()
                    : Core::JSON::Container()
                    , Size(0)
                {
                    Add(_T("size"), &Size);
                }
                ~MallocInputMetadata()
                {
                }

            public:
                Core::JSON::DecSInt32 Size;
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
                TestCaseInfo(string name, MemoryAllocation::ResponseType type, const std::function<string(MemoryAllocation::ResponseType, string)> &testCaseInfo)
                        : _name(name)
                        , _type(type)
                        , _exec(testCaseInfo)
                    {
                    }

                virtual ~TestCaseInfo() {}

            public:
                string _name;
                MemoryAllocation::ResponseType _type;
                std::function<string(MemoryAllocation::ResponseType, string)> _exec;
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

            ReqisterHelpers("TestCases", MemoryAllocation::ResponseType::TEST_CASES_LIST, std::bind(&MemoryAllocation::CreateResponse, this, _1, _2));
            ReqisterHelpers("Statm/Description", MemoryAllocation::ResponseType::TEST_CASE_DESCRIPTION, std::bind(&MemoryAllocation::CreateResponse, this, _1, _2));
            ReqisterHelpers("Statm/Parameters", MemoryAllocation::ResponseType::TEST_CASE_PARAMETERS, std::bind(&MemoryAllocation::CreateResponse, this, _1, _2));
            ReqisterHelpers("Free/Description", MemoryAllocation::ResponseType::TEST_CASE_DESCRIPTION, std::bind(&MemoryAllocation::CreateResponse, this, _1, _2));
            ReqisterHelpers("Free/Parameters", MemoryAllocation::ResponseType::TEST_CASE_PARAMETERS, std::bind(&MemoryAllocation::CreateResponse, this, _1, _2));
            ReqisterHelpers("Malloc/Description", MemoryAllocation::ResponseType::TEST_CASE_DESCRIPTION, std::bind(&MemoryAllocation::CreateResponse, this, _1, _2));
            ReqisterHelpers("Malloc/Parameters", MemoryAllocation::ResponseType::TEST_CASE_PARAMETERS, std::bind(&MemoryAllocation::CreateResponse, this, _1, _2));

            TestCore::TestController::Instance().AnnounceTestSuite(*this, "Memory");
        }

        virtual ~MemoryAllocation() { Free(); }

    public:
        // ITest methods
        virtual void Setup(const string& body) override;
        virtual string /*JSON*/ Execute(const string& testCase) override;
        virtual void Cleanup(void) override;

    private:
        //Test Suite Memory Allocation Method
        string /*JSON*/ Malloc(void);
        string /*JSON*/ Statm(void);
        string /*JSON*/ Free(void);
        void DisableOOMKill(void);
        void LogMemoryUsage(void);

        //Common methods for each TestSuite, consider to create separate object
        void Reqister(const string &name, const string &desciption, const std::function<string(void)> &testCaseCallback);
        void ReqisterHelpers(const string &name, MemoryAllocation::ResponseType type, const std::function<string(MemoryAllocation::ResponseType, string)> &testCaseInfoCallback);
        string /*JSON*/ ExecuteTestCase(const string& testCase);
        string /*JSON*/ ExecuteTestCaseInfo(const string& testCase);
        //string /*JSON*/ GetTestCasesInfo(const string& testCase);
        string /*JSON*/ GetBody(void);
        string /*JSON*/ CreateResponse(const MemoryAllocation::ResponseType type, const string& method);
        //string /*JSON*/ MemoryAllocation::CreateDescriptionResponse(const string& testCase);
        //string /*JSON*/ MemoryAllocation::CreateParametersResponse(const string& testCase);
        //string /*JSON*/ MemoryAllocation::CreateTestCasesResponse(void);

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
