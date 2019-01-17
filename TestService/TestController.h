#pragma once

#include "Module.h"
#include "ITestSuite.h"
#include <list>

namespace WPEFramework {
namespace TestCore {

class TestController {
    private:
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

        void AnnounceTestSuite(TestCore::ITestSuite& testsArea, const string& groupName)
        {
            _adminLock.Lock();
            SYSLOG(Trace::Fatal, (_T("*** TestController::AnnounceTestSuite ***")))

            auto index = _testSuites.find(groupName);

            // Announce a tests area only once
            ASSERT(index == _testSuites.end());

            if (index == _testSuites.end())
            {
                _testSuites.insert(std::pair<string, TestCore::ITestSuite*>(groupName, &testsArea));
            }

            _adminLock.Unlock();
        }
        void RevokeTestSuite(const string& groupName)
        {
            _adminLock.Lock();
            SYSLOG(Trace::Fatal, (_T("*** TestController::RevokeTestSuite ***")))

            auto index = _testSuites.find(groupName);

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
            SYSLOG(Trace::Fatal, (_T("*** TestController::RevokeAllTestSuite ***")))
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
