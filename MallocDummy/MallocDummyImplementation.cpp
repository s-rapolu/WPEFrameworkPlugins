#include "Module.h"
#include <interfaces/IMallocDummy.h>

namespace WPEFramework {

class MallocDummyImplementation : public Exchange::IMallocDummy {
private:
    MallocDummyImplementation(const MallocDummyImplementation&) = delete;
    MallocDummyImplementation& operator=(const MallocDummyImplementation&) = delete;

public:
    MallocDummyImplementation()
        : _currentMemoryAllocation(0)
    {
        SYSLOG(Trace::Fatal, (_T("*** MallocDummyImplementation::Construct() ***")))
    }

    virtual ~MallocDummyImplementation()
    {
        SYSLOG(Trace::Fatal, (_T("*** MallocDummyImplementation::Destruct() ***")))
    }

    BEGIN_INTERFACE_MAP(MallocDummyImplementation)
        INTERFACE_ENTRY(Exchange::IMallocDummy)
    END_INTERFACE_MAP

    // IMallocDummy methods
    uint64_t Malloc(uint64_t size)
    {
        SYSLOG(Trace::Fatal, (_T("*** MallocDummyImplementation::Malloc() ***")))
        _currentMemoryAllocation = size;
        return _currentMemoryAllocation;
    }

    uint64_t GetAllocatedMemory(void)
    {
        SYSLOG(Trace::Fatal, (_T("*** MallocDummyImplementation::GetAllocatedMemory() ***")))
        return _currentMemoryAllocation;
    }

private:
    uint64_t _currentMemoryAllocation;
};

SERVICE_REGISTRATION(MallocDummyImplementation, 1, 0);
}
