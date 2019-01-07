#ifndef _MALLOCDUMMY_H
#define _MALLOCDUMMY_H

#include "Module.h"
#include <interfaces/IMallocDummy.h>

namespace WPEFramework {
namespace Plugin {

    class MallocDummy : public PluginHost::IPlugin, public PluginHost::IWeb {
    public:
        class Data : public Core::JSON::Container {
        public:
            class MemoryInfo : public Core::JSON::Container {
            private:
                MemoryInfo(const MemoryInfo&) = delete;
                MemoryInfo& operator=(const MemoryInfo&) = delete;

            public:
                MemoryInfo()
                    : Core::JSON::Container()
                    , CurrentAllocation(0)
                {
                    Add(_T("memoryAllocation"), &CurrentAllocation);
                }
                ~MemoryInfo()
                {
                }

            public:
                Core::JSON::DecUInt64 CurrentAllocation;
            };


        private:
            Data(const Data&) = delete;
            Data& operator=(const Data&) = delete;

        public:
            Data()
                : Core::JSON::Container()
                , Memory()
            {
                Add(_T("memoryInfo"), &Memory);
            }

            virtual ~Data()
            {
            }

        public:
            MemoryInfo Memory;
        };

        MallocDummy()
            : _service(nullptr)
            , _mallocDummy(nullptr)
            , _pluginName("MallocDummy")
            , _skipURL(0) { }

        virtual ~MallocDummy() { }

        BEGIN_INTERFACE_MAP(MallocDummy)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(PluginHost::IWeb)
            INTERFACE_AGGREGATE(Exchange::IMallocDummy, _mallocDummy)
        END_INTERFACE_MAP

        //   IPlugin methods
        // -------------------------------------------------------------------------------------------------------
        virtual const string Initialize(PluginHost::IShell* service) override;
        virtual void Deinitialize(PluginHost::IShell* service) override;
        virtual string Information() const override;

        //  IWeb methods
        // -------------------------------------------------------------------------------------------------------
        virtual void Inbound(Web::Request& request);
        virtual Core::ProxyType<Web::Response> Process(const Web::Request& request);

    private:
        MallocDummy(const MallocDummy&) = delete;
        MallocDummy& operator=(const MallocDummy&) = delete;

        void GetMemoryInfo(Data::MemoryInfo& memoryInfo);

        PluginHost::IShell* _service;
        Exchange::IMallocDummy* _mallocDummy;
        string _pluginName;
        uint8_t _skipURL;
    };

} // namespace Plugin
} // namespace WPEFramework

#endif // _MALLOCDUMMY_H
