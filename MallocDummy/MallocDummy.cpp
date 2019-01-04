#include "MallocDummy.h"

namespace WPEFramework
{
    namespace Plugin {

        SERVICE_REGISTRATION(MallocDummy, 1, 0);

        /* virtual */ const string MallocDummy::Initialize(PluginHost::IShell* service)
        {
            ASSERT (_service == nullptr);
            ASSERT (service != nullptr);

            MallocDummyData configData;
            configData.FromString(service->ConfigLine());

            SYSLOG(Trace::Fatal, (_T("*** DAD Plugin start, name: [%s] ***"), _pluginName.c_str()))
            _service = service;

            DisplayAppConfigConfig(configData);

            return (string());
        }

        /* virtual */ void MallocDummy::Deinitialize(PluginHost::IShell* service)
        {
            ASSERT (_service == service);
            SYSLOG(Trace::Fatal, (_T("*** DAD Plugin end, name: [%s] ***"), _pluginName.c_str()))
            _service = nullptr;
        }

        /* virtual */ string MallocDummy::Information() const
        {
            // No additional info to report.
            return (string());
        }

        string MallocDummy::GetPluginName() const
        {
            return (string());
        }
        void MallocDummy::DisplayAppConfigConfig(MallocDummyData & config)
        {
            string data = config.SampleConfigData.Value();
            SYSLOG(Trace::Fatal, (_T("Config data :%s"), data.c_str()))
        }
    } // namespace Plugin
} // namespace WPEFramework
