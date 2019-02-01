#pragma once

#include <interfaces/ISystemCommands.h>
#include <core/JSON.h>

namespace WPEFramework {
namespace Plugin {

    class SystemCommands
        : public PluginHost::IPlugin,
          public PluginHost::IWeb {
    public:
        struct Data : public Core::JSON::Container {
            Data()
            {
                Add(_T("command"), &Command);
                Add(_T("description"), &Description);
            }
            Data(const Data& other)
                : Command(other.Command)
                , Description(other.Description)
            {
                Add(_T("command"), &Command);
                Add(_T("description"), &Description);
            }
            Data operator=(const Data&) = delete;

            Core::JSON::String Command;
            Core::JSON::String Description;
        };

        SystemCommands();
        ~SystemCommands() override;

        BEGIN_INTERFACE_MAP(SystemCommands)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        INTERFACE_ENTRY(PluginHost::IWeb)
        INTERFACE_AGGREGATE(Exchange::ISystemCommands, _commands)
        END_INTERFACE_MAP

        //   IPlugin methods
        // -------------------------------------------------------------------------------------------------------
        const string Initialize(PluginHost::IShell* service) override;
        void Deinitialize(PluginHost::IShell* service) override;
        string Information() const override;

        //   IWeb methods
        // -------------------------------------------------------------------------------------------------------
        void Inbound(Web::Request& request) override;
        Core::ProxyType<Web::Response> Process(const Web::Request& request) override;

    private:
        SystemCommands(const SystemCommands&) = delete;
        SystemCommands& operator=(const SystemCommands&) = delete;

        uint8_t _skipURL = 0;
        Exchange::ISystemCommands* _commands = nullptr;
    };

#undef NONCOPYABLEl

} // namespace Plugin
} // namespace WPEFramework
