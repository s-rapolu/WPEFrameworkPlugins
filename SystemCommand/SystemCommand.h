#pragma once

#include <interfaces/ISystemCommand.h>
#include <core/JSON.h>

namespace WPEFramework {
namespace Plugin {

    class SystemCommand
        : public PluginHost::IPlugin,
          public PluginHost::IWeb {
    public:
        struct Param : public Core::JSON::Container {
            Param(const Param&) : Param() {}
            Param& operator=(const Param&) = delete;
            Param()
            {
                Add(_T("name"), &Name);
                Add(_T("value"), &Value);
            }
            ~Param() = default;
            Core::JSON::String Name;
            Core::JSON::String Value;
        };
        SystemCommand();
        ~SystemCommand() override;

        BEGIN_INTERFACE_MAP(SystemCommand)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        INTERFACE_ENTRY(PluginHost::IWeb)
        INTERFACE_AGGREGATE(Exchange::ISystemCommand, _executor)
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
//        "defaultparams": [
//             {
//               "command": "usbreset",
//               "params": [
//                 {
//                   "name":"device",
//                   "value": "/dev/null"
//                 }
//               ]
//             }
//          ],
        struct Config : public Core::JSON::Container {
            struct Command : public Core::JSON::Container {
                Command(const Command&) : Command() {}
                Command& operator=(const Command&) = delete;
                Command()
                {
                    Add(_T("command"), &CommandName);
                    Add(_T("params"), &Params);
                }
                ~Command() = default;
                Core::JSON::String CommandName;
                Core::JSON::ArrayType<Param> Params;
            };

            Config(const Config&) : Config() {}
            Config& operator=(const Config&) = delete;
            Config()
            {
                Add(_T("defaultparams"), &DefaultParams);
            }
            ~Config() = default;

            Core::JSON::ArrayType<Command> DefaultParams;
        };
        SystemCommand(const SystemCommand&) = delete;
        SystemCommand& operator=(const SystemCommand&) = delete;

        uint8_t _skipURL = 0;
        Exchange::ISystemCommand* _executor = nullptr;
        Config _config;
        Core::CriticalSection _adminLock;
    };

} // namespace Plugin
} // namespace WPEFramework
