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
                PAGE_SZ = 1 << 12
            };

            struct MemoryStats
            {
                uint64_t size;
                uint64_t resident;
                uint64_t share;
                uint64_t text;
                uint64_t lib;
                uint64_t data;
                uint64_t dt;
            };

            MallocDummyImplementation(const MallocDummyImplementation&) = delete;
            MallocDummyImplementation& operator=(const MallocDummyImplementation&) = delete;

        public:
            MallocDummyImplementation()
                : _currentMemoryAllocation(0)
            {
                PreventOOMKill();
                if(!ReadMemoryStatus(_currentMemoryInfo))
                {
                    printf("*** MallocDummyImplementation:: Not able to read memory status  ***\n");
                    _status = false;
                }
                else
                {
                    _startSize = static_cast<uint64_t>(_currentMemoryInfo.size);
                    _startResident = static_cast<uint64_t>(_currentMemoryInfo.resident);
                    _status = true;
                }

                printf("*** ##  Memory size  %8lu Kb  resident %8lu Kb ***\n", (_currentMemoryInfo.size * PAGE_SZ) >> 10, (_currentMemoryInfo.resident * PAGE_SZ) >> 10);
                printf("*** Status %d ***\n", _status);
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
            uint64_t Malloc(uint64_t size)
            {
                ASSERT(_status);
                SYSLOG(Trace::Fatal, (_T("*** MallocDummyImplementation::Malloc() ***")))

                uint32_t noOfBlocks = 0;
                uint64_t blockSize = (8 * PAGE_SZ); //32kB block size
                uint64_t runs = (size * 1024) / (blockSize / 1024);

                for (noOfBlocks = 0; noOfBlocks < runs; ++noOfBlocks)
                {
                    void* _memory = malloc(static_cast<size_t>(blockSize));
                    if (!_memory)
                    {
                        SYSLOG(Trace::Fatal, (_T("*** Failed allocation !!! ***")))
                        break;
                    }
                    for (uint32_t index = 0; index < blockSize; index++)
                    {
                        static_cast<unsigned char*>(_memory)[index] = static_cast<unsigned char>(rand() & 0xFF);
                    }
                }

                _currentMemoryAllocation = (noOfBlocks * blockSize) >> 10;
                LogMemoryUsage();

                return _currentMemoryAllocation;
            }

            uint64_t GetAllocatedMemory(void)
            {
                ASSERT(_status);

                LogMemoryUsage();

                return _currentMemoryAllocation;
            }

        private:
            void PreventOOMKill(void);
            bool ReadMemoryStatus(MemoryStats& info);
            void LogMemoryUsage(void);

            MemoryStats _currentMemoryInfo;
            uint64_t _startSize;
            uint64_t _startResident;
            bool _status;

            uint64_t _currentMemoryAllocation; //size in MB
    };

    void MallocDummyImplementation::PreventOOMKill()
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

    void MallocDummyImplementation::LogMemoryUsage(void)
    {
        uint64_t runSize = static_cast<uint64_t>(_currentMemoryInfo.size - _startSize);
        uint64_t runResident = static_cast<uint64_t>(_currentMemoryInfo.resident - _startResident);

        SYSLOG(Trace::Fatal, (_T("*** Current allocated: %lu KB ***"), (uint64_t)_currentMemoryAllocation << 10))
        SYSLOG(Trace::Fatal, (_T("*** Current Size:     %lu KB ***"), (uint64_t)runSize * PAGE_SZ >> 10))
        SYSLOG(Trace::Fatal, (_T("*** Current Resident: %lu KB ***"), (uint64_t)runResident * PAGE_SZ >> 10))
    }

SERVICE_REGISTRATION(MallocDummyImplementation, 1, 0);
}
