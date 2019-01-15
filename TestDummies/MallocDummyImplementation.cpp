#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>

#include "Module.h"
#include <interfaces/IMallocDummy.h>

namespace WPEFramework
{
    class MallocDummyImplementation : public Exchange::IMallocDummy
    {
        private:
            MallocDummyImplementation(const MallocDummyImplementation&) = delete;
            MallocDummyImplementation& operator=(const MallocDummyImplementation&) = delete;

        public:
            MallocDummyImplementation()
                : _currentMemoryAllocation(0)
                , _lock()
                , _process()
            {
                DisableOOMKill();
                _startSize = static_cast<uint32_t>(_process.Allocated() >> 10);
                _startResident = static_cast<uint32_t>(_process.Resident() >>10);
            }

            virtual ~MallocDummyImplementation()
            {
                Free();
            }

            BEGIN_INTERFACE_MAP(MallocDummyImplementation)
                INTERFACE_ENTRY(Exchange::IMallocDummy)
            END_INTERFACE_MAP

            // IMallocDummy methods
            uint32_t Malloc(uint32_t size) //size in Kb
            {
                _lock.Lock();

                SYSLOG(Trace::Information, (_T("*** Allocate %lu Kb ***"), size))
                uint32_t noOfBlocks = 0;
                uint32_t blockSize = (32 * (getpagesize() >> 10)); //128kB block size
                uint32_t runs = (uint32_t)size / blockSize;

                for (noOfBlocks = 0; noOfBlocks < runs; ++noOfBlocks)
                {
                    _memory.push_back(malloc(static_cast<size_t>(blockSize << 10)));
                    if (!_memory.back())
                    {
                        SYSLOG(Trace::Fatal, (_T("*** Failed allocation !!! ***")))
                        break;
                    }

                    for (uint32_t index = 0; index < (blockSize << 10); index++)
                    {
                        static_cast<unsigned char*>(_memory.back())[index] = static_cast<unsigned char>(rand() & 0xFF);
                    }
                }

                _currentMemoryAllocation += (noOfBlocks * blockSize);

                _lock.Unlock();

                return _currentMemoryAllocation;
            }

            void Statm(uint32_t &allocated, uint32_t &size, uint32_t &resident)
            {
                _lock.Lock();

                SYSLOG(Trace::Information, (_T("*** MallocDummyImplementation::Statm ***")))

                allocated = _currentMemoryAllocation;
                size = static_cast<uint32_t>(_process.Allocated() >> 10);
                resident = static_cast<uint32_t>(_process.Resident() >> 10);

                LogMemoryUsage();
                _lock.Unlock();
            }

            void Free(void)
            {
                _lock.Lock();

                SYSLOG(Trace::Information, (_T("*** MallocDummyImplementation::Free ***")))

                if (!_memory.empty())
                {
                    for (auto const& memoryBlock : _memory)
                    {
                        free(memoryBlock);
                    }
                    _memory.clear();
                }

                _currentMemoryAllocation = 0;

                LogMemoryUsage();
                _lock.Unlock();
            }

        private:
            void DisableOOMKill(void);
            void LogMemoryUsage(void);

            uint32_t _startSize;
            uint32_t _startResident;
            Core::CriticalSection _lock;
            Core::ProcessInfo _process;
            std::list<void *> _memory;
            uint32_t _currentMemoryAllocation; //size in Kb
    };

    void MallocDummyImplementation::DisableOOMKill()
    {
        int8_t oomNo = -17;
        _process.OOMAdjust(oomNo);
    }

    void MallocDummyImplementation::LogMemoryUsage(void)
    {
        SYSLOG(Trace::Information, (_T("*** Current allocated: %lu Kb ***"), _currentMemoryAllocation))
        SYSLOG(Trace::Information, (_T("*** Initial Size:     %lu Kb ***"), _startSize))
        SYSLOG(Trace::Information, (_T("*** Initial Resident: %lu Kb ***"), _startResident))
        SYSLOG(Trace::Information, (_T("*** Size:     %lu Kb ***"), static_cast<uint32_t>(_process.Allocated() >> 10)))
        SYSLOG(Trace::Information, (_T("*** Resident: %lu Kb ***"), static_cast<uint32_t>(_process.Resident() >>10 )))
    }

SERVICE_REGISTRATION(MallocDummyImplementation, 1, 0);
}
