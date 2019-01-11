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

            enum
            {
                PAGE_SZ = ((1 << 12) >> 10)
            };

            struct MemoryStats
            {
                uint32_t size;
                uint32_t resident;
                uint32_t share;
                uint32_t text;
                uint32_t lib;
                uint32_t data;
                uint32_t dt;
            };

            MallocDummyImplementation(const MallocDummyImplementation&) = delete;
            MallocDummyImplementation& operator=(const MallocDummyImplementation&) = delete;

        public:
            MallocDummyImplementation()
                : _currentMemoryAllocation(0)
                , _lock()
            {
                DisableOOMKill();
                if(!ReadMemoryStatus(_currentMemoryInfo))
                {
                    SYSLOG(Trace::Fatal, (_T("*** MallocDummyImplementation:: Not able to read memory status  ***")))
                    _status = false;
                }
                else
                {
                    _startSize = _currentMemoryInfo.size;
                    _startResident = _currentMemoryInfo.resident;
                    _status = true;
                }
            }

            virtual ~MallocDummyImplementation()
            {
                _status = false;
                printf("*** MallocDummyImplementation::Destruct() ***\n");
            }

            BEGIN_INTERFACE_MAP(MallocDummyImplementation)
                INTERFACE_ENTRY(Exchange::IMallocDummy)
            END_INTERFACE_MAP

            // IMallocDummy methods
            uint32_t Malloc(uint32_t size) //size in Kb
            {
                ASSERT(_status);

                _lock.Lock();

                SYSLOG(Trace::Information, (_T("*** Allocate %lu Kb ***"), size))
                uint32_t noOfBlocks = 0;
                uint32_t blockSize = (8 * PAGE_SZ); //32kB block size
                uint32_t runs = (uint32_t)size / blockSize; //in Kb

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
                ASSERT(_status);

                _lock.Lock();

                SYSLOG(Trace::Fatal, (_T("*** MallocDummyImplementation::Statm ***")))

                TestMemoryUsage();
                allocated = _currentMemoryAllocation;
                size = (_currentMemoryInfo.size - _startSize) * PAGE_SZ;
                resident = (_currentMemoryInfo.resident - _startResident) * PAGE_SZ;

                _lock.Unlock();
            }

            void Free(void)
            {
                ASSERT(_status);

                _lock.Lock();

                SYSLOG(Trace::Information, (_T("*** MallocDummyImplementation::Free ***")))

                for (auto const& memoryBlock : _memory)
                {
                    free(memoryBlock);
                }
                _memory.clear();

                _currentMemoryAllocation = 0;
                TestMemoryUsage();

                _lock.Unlock();
            }

        private:
            void DisableOOMKill(void);
            bool ReadMemoryStatus(MemoryStats& info);
            void TestMemoryUsage(void);

            MemoryStats _currentMemoryInfo;
            uint32_t _startSize;
            uint32_t _startResident;
            Core::CriticalSection _lock;
            bool _status;

            std::list<void *> _memory;
            uint32_t _currentMemoryAllocation; //size in Kb
    };

    void MallocDummyImplementation::DisableOOMKill()
    {
        int no_oom = -17;

        FILE* fp = fopen("/proc/self/oom_adj", "w");
        if (fp != NULL)
        {
          fprintf(fp, "%d", no_oom);
          fclose(fp);
        }
    }

    bool MallocDummyImplementation::ReadMemoryStatus(MemoryStats& info)
    {
        const char* statm_path = "/proc/self/statm";
        bool status = false;

        FILE* fp = fopen(statm_path, "r");
        if (fp)
        {
            if (7 == fscanf(fp, "%ld %ld %ld %ld %ld %ld %ld",
                                 &info.size, &info.resident, &info.share, &info.text, &info.lib, &info.data, &info.dt))
            {
                status = true;
            }
        }
        fclose(fp);
        return status;
    }

    void MallocDummyImplementation::TestMemoryUsage(void)
    {
        if(!ReadMemoryStatus(_currentMemoryInfo))
        {
            SYSLOG(Trace::Fatal, (_T("*** MallocDummyImplementation:: Not able to read memory status  ***")))
        }
        else
        {
            uint32_t runSize = _currentMemoryInfo.size - _startSize;
            uint32_t runResident = _currentMemoryInfo.resident - _startResident;

            SYSLOG(Trace::Information, (_T("*** Current allocated: %lu Kb ***"), _currentMemoryAllocation))
            SYSLOG(Trace::Information, (_T("*** Run Size:     %lu Kb ***"), runSize * PAGE_SZ))
            SYSLOG(Trace::Information, (_T("*** Run Resident: %lu Kb ***"), runResident * PAGE_SZ))
        }
    }

SERVICE_REGISTRATION(MallocDummyImplementation, 1, 0);
}
