#pragma once

#include <interfaces/ISystemCommand.h>

namespace WPEFramework {
namespace Plugin {

    class SystemCommandImplementation : public Exchange::ISystemCommand {
    public:
        SystemCommandImplementation(SystemCommandImplementation&) = delete;
        SystemCommandImplementation& operator=(const SystemCommandImplementation&) = default;
        SystemCommandImplementation();
        ~SystemCommandImplementation() override;

        //   ISystemCommand methods
        // -------------------------------------------------------------------------------------------------------
        void Register(const Exchange::ISystemCommand::CommandId& id,
                      Exchange::ISystemCommand::IExecutor* delegate) override;
        void Unregister(Exchange::ISystemCommand::IExecutor* delegate) override;
        bool Execute(const Exchange::ISystemCommand::CommandId& id,
                     const Exchange::ISystemCommand::CommandParams& params) override;
        std::vector<Exchange::ISystemCommand::CommandId> SupportedCommands() const override;

        BEGIN_INTERFACE_MAP(SystemCommandImplementation)
        INTERFACE_ENTRY(Exchange::ISystemCommand)
        END_INTERFACE_MAP
    private:
        void Register(const Exchange::ISystemCommand::CommandId& id,
                      Exchange::ISystemCommand::IExecutor* delegate,
                      bool builtIn);
        using DelegeteList = std::map<Exchange::ISystemCommand::CommandId,
                                      Exchange::ISystemCommand::IExecutor*> ;

        DelegeteList _delegates;
        mutable Core::CriticalSection _adminLock;
    };

}
}
