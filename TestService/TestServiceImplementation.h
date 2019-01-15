#pragma once

#include "Module.h"

#include <interfaces/ITestService.h>

namespace WPEFramework {
class TestServiceImplementation : public Exchange::ITestService {
private:
    TestServiceImplementation(const TestServiceImplementation&) = delete;
    TestServiceImplementation& operator=(const TestServiceImplementation&) = delete;

public:
    TestServiceImplementation()
        : _currentMemoryAllocation(0)
        , _lock()
        , _process()
    {
        DisableOOMKill();
        _startSize = static_cast<uint32_t>(_process.Allocated() >> 10);
        _startResident = static_cast<uint32_t>(_process.Resident() >> 10);
    }

    virtual ~TestServiceImplementation() { Free(); }

    BEGIN_INTERFACE_MAP(TestServiceImplementation)
    INTERFACE_ENTRY(Exchange::ITestService)
    END_INTERFACE_MAP

public:
    // ITestService methods
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
