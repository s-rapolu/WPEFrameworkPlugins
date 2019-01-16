#pragma once

#include "Module.h"

#include <interfaces/ITest.h>

#include "TestController.h"

namespace WPEFramework {

class MemoryAllocation : public Exchange::ITest {
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

        TestCore::TestController::Instance().Announce(*this);
    }

    virtual ~MemoryAllocation() { Free(); }

    BEGIN_INTERFACE_MAP(MemoryAllocation)
    INTERFACE_ENTRY(Exchange::ITest)
    END_INTERFACE_MAP

public:
    // ITest methods
    virtual void Setup(const string& body) override;
    virtual const string& /*JSON*/ Execute(const string& method) override;
    virtual void Cleanup(void) override;

    virtual const std::list<string> GetMethods(void) override;
    virtual const string& /*JSON*/ GetMethodDes(const string& method) override;
    virtual const string& /*JSON*/ GetMethodParam(const string& method) override;

private:
    uint32_t Malloc(uint32_t size);
    void Statm(uint32_t& allocated, uint32_t& size, uint32_t& resident);
    void Free(void);

    void DisableOOMKill(void);
    void LogMemoryUsage(void);

private:
    uint32_t _startSize;
    uint32_t _startResident;
    Core::CriticalSection _lock;
    Core::ProcessInfo _process;
    std::list<void*> _memory;
    uint32_t _currentMemoryAllocation; // size in Kb
};

static MemoryAllocation* _singleton(Core::Service<MemoryAllocation>::Create<MemoryAllocation>());

} // namespace WPEFramework
