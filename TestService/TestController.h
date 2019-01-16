#pragma once

#include "Module.h"

#include <interfaces/ITest.h>

#include <list>

namespace WPEFramework {
namespace TestCore {

class TestController {
private:
    TestController(const TestController&) = delete;
    TestController& operator=(const TestController&) = delete;
    TestController()
        : _adminLock()
        , _testAreas()
    {
    }

public:
    static TestController& Instance();

    virtual ~TestController()
    {
    }

    // TestControllerImplementation methods
    typedef Core::IteratorType<std::list<Exchange::ITest*>, Exchange::ITest*> Iterator;
    inline Iterator Producers() { return (Iterator(_testAreas)); } //ToDo: Not sure whether it is needed

    void Announce(Exchange::ITest& testsArea)
    {
        _adminLock.Lock();

        auto index(std::find(_testAreas.begin(), _testAreas.end(), &testsArea));

        // Announce a tests area only once
        ASSERT(index == _testAreas.end());

        if (index == _testAreas.end())
        {
            _testAreas.push_back(&testsArea);
        }

        _adminLock.Unlock();
    }
    void Revoke(Exchange::ITest& testsArea)
    {
        _adminLock.Lock();

        auto index(std::find(_testAreas.begin(), _testAreas.end(), &testsArea));

        // Only revoke test areas you subscribed !!!!
        ASSERT(index != _testAreas.end());

        if (index != _testAreas.end())
        {
            _testAreas.erase(index);
        }

        _adminLock.Unlock();
    }

    void RevokeAll()
    {
        _adminLock.Lock();

        _testAreas.clear();

        _adminLock.Unlock();
    }

private:
    Core::CriticalSection _adminLock;
    std::list<Exchange::ITest*> _testAreas;
};

} // namespace TestCore
} // namespace WPEFramework
