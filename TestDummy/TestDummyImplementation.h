#pragma once

#include "Module.h"

#include <interfaces/ITestDummy.h>

namespace WPEFramework {
class TestDummyImplementation : public Exchange::ITestDummy {
private:
    TestDummyImplementation(const TestDummyImplementation&) = delete;
    TestDummyImplementation& operator=(const TestDummyImplementation&) = delete;

public:
    TestDummyImplementation()
        : _currentMemoryAllocation(0)
        , _lock()
        , _process()
    {
        DisableOOMKill();
        _startSize = static_cast<uint32_t>(_process.Allocated() >> 10);
        _startResident = static_cast<uint32_t>(_process.Resident() >> 10);
    }

    virtual ~TestDummyImplementation() { Free(); }

    BEGIN_INTERFACE_MAP(TestDummyImplementation)
    INTERFACE_ENTRY(Exchange::ITestDummy)
    END_INTERFACE_MAP

public:
    // ITestDummy methods
    uint32_t Malloc(uint32_t size) override;
    void Statm(uint32_t& allocated, uint32_t& size, uint32_t& resident) override;
    void Free(void) override;

    bool Configure(PluginHost::IShell* shell) override;
    void Crash() override;
    bool CrashNTimes(uint8_t n) override;
    void ExecPendingCrash() override;
    uint8_t PendingCrashCount() override;

private:
    class Config : public Core::JSON::Container {
    private:
        Config(const Config&) = delete;
        Config& operator=(const Config&) = delete;

    public:
        Config()
            : Core::JSON::Container()
            , CrashDelay(0)
        {
            Add(_T("crashDelay"), &CrashDelay);
        }
        ~Config() {}

    public:
        Core::JSON::DecUInt8 CrashDelay;
    };

private:
    void DisableOOMKill(void);
    void LogMemoryUsage(void);
    bool SetPendingCrashCount(uint8_t newCrashCount);

private:
    uint32_t _startSize;
    uint32_t _startResident;
    Core::CriticalSection _lock;
    Core::ProcessInfo _process;
    std::list<void*> _memory;
    uint32_t _currentMemoryAllocation; // size in Kb

    Config _config;
    uint8_t _crashDelay;
};
} // namespace WPEFramework
