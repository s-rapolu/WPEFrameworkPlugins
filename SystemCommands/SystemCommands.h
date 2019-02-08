#pragma once

#include "Module.h"
#include <interfaces/ISystemCommands.h>

namespace WPEFramework {
namespace Plugin {

    class SystemCommands : public PluginHost::IPlugin, public PluginHost::IWeb {
    public:
        struct Param : public Core::JSON::Container {
            Param()
            {
                Add(_T("name"), &Name);
                Add(_T("value"), &Value);
            }

            Param(const Param& other)
                : Name(other.Name)
                , Value(other.Value)
            {
                Add(_T("name"), &Name);
                Add(_T("value"), &Value);
            }

            Param& operator=(const Param&) = delete;

            Core::JSON::String Name;
            Core::JSON::String Value;
        };

        SystemCommands() = default;
        ~SystemCommands() override = default;

        BEGIN_INTERFACE_MAP(SystemCommands)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(PluginHost::IWeb)
            INTERFACE_AGGREGATE(Exchange::ISystemCommands, _implementation)
        END_INTERFACE_MAP

        //   IPlugin methods
        const string Initialize(PluginHost::IShell* service) final;
        void Deinitialize(PluginHost::IShell* service) final;
        string Information() const final;

        //   IWeb methods
        void Inbound(Web::Request& request) final;
         Core::ProxyType<Web::Response> Process(const Web::Request& request) final;

    private:
        SystemCommands(const SystemCommands&) = delete;
        SystemCommands& operator=(const SystemCommands&) = delete;

        uint8_t _skipURL = 0;
        Exchange::ISystemCommands* _implementation = nullptr;
    };

} // namespace Plugin
} // namespace WPEFramework
