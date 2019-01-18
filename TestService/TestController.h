#pragma once

#include "Module.h"
#include "ITestSuite.h"
#include <list>

namespace WPEFramework {
namespace TestCore {

class TestController {
    private:

        class Metadata : public Core::JSON::Container {
            private:
                Metadata(const Metadata&) = delete;
                Metadata& operator=(const Metadata&) = delete;

            public:
                Metadata()
                    : Core::JSON::Container()
                    , TestSuites()
                {
                    Add(_T("testSuites"), &TestSuites);
                }
                ~Metadata() {}

            public:
                Core::JSON::ArrayType<Core::JSON::String> TestSuites;
        };

        TestController(const TestController&) = delete;
        TestController& operator=(const TestController&) = delete;
        TestController()
            : _adminLock()
            , _testSuites()
        {
        }

    public:
        static TestController& Instance();

        virtual ~TestController()
        {
        }

        // TestControllerImplementation methods
        typedef Core::IteratorMapType<std::map<string, TestCore::ITestSuite*>, TestCore::ITestSuite*, string> Iterator;
        inline Iterator TestSuites()
        {
            return (Iterator(_testSuites));
        }

        string /*JSON*/ GetTestSuites(void)
        {
            string response;
            Metadata testSuites;

            for (auto& testStuite : _testSuites)
            {
                Core::JSON::String name;
                name = testStuite.first;
                testSuites.TestSuites.Add(name);
            }
            testSuites.ToString(response);

            return response;
        }

        void AnnounceTestSuite(TestCore::ITestSuite& testsArea, const string& testSuiteName)
        {
            _adminLock.Lock();

            auto index = _testSuites.find(testSuiteName);

            // Announce a tests area only once
            ASSERT(index == _testSuites.end());

            if (index == _testSuites.end())
            {
                _testSuites.insert(std::pair<string, TestCore::ITestSuite*>(testSuiteName, &testsArea));
            }

            _adminLock.Unlock();
        }
        void RevokeTestSuite(const string& testSuiteName)
        {
            _adminLock.Lock();

            auto index = _testSuites.find(testSuiteName);

            // Only revoke test areas you subscribed !!!!
            ASSERT(index != _testSuites.end());

            if (index != _testSuites.end())
            {
                _testSuites.erase(index);
            }

            _adminLock.Unlock();
        }

        void RevokeAllTestSuite()
        {
            _adminLock.Lock();

            _testSuites.clear();

            _adminLock.Unlock();
        }

    private:
        Core::CriticalSection _adminLock;
        std::map<string, TestCore::ITestSuite*> _testSuites;
};

} // namespace TestCore
} // namespace WPEFramework
