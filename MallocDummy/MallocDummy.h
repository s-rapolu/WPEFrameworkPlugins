#ifndef _MALLOCDUMMY_H
#define _MALLOCDUMMY_H

#include "Module.h"

namespace WPEFramework {
namespace Plugin {

    class MallocDummy : public PluginHost::IPlugin, public PluginHost::IWeb {
    public:
        class MallocDummyData : public Core::JSON::Container {

        private:
            MallocDummyData(const MallocDummyData&) = delete;
            MallocDummyData& operator=(const MallocDummyData&) = delete;

        public:
            MallocDummyData()
                : Core::JSON::Container()
                , SampleConfigData("Default data")
            {
                Add(_T("sampleData"), &SampleConfigData);
            }

            virtual ~MallocDummyData()
            {
            }

        public:
            Core::JSON::String SampleConfigData;
        };

    private:
        MallocDummy(const MallocDummy&) = delete;
        MallocDummy& operator=(const MallocDummy&) = delete;

    public:
        MallocDummy()
            : _service(nullptr)
            , _pluginName("MallocDummy")
        {
        }

        virtual ~MallocDummy()
        {
        }

        BEGIN_INTERFACE_MAP(MallocDummy)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(PluginHost::IWeb)
        END_INTERFACE_MAP

    public:
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
        string GetPluginName() const;
        void DisplayAppConfigConfig(MallocDummyData & config);

    private:
        PluginHost::IShell* _service;
        string _pluginName;
    };

} // namespace Plugin
} // namespace WPEFramework

#endif // _MALLOCDUMMY_H
