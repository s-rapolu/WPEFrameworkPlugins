#include "Module.h"
#include <interfaces/ISystemCommands.h>
#include "SystemCommandsImplementation.h"

#include <linux/usbdevice_fs.h>

namespace WPEFramework {
namespace Plugin {

    using Exchange::ISystemCommands;

    SystemCommandsImplementation::SystemCommandsImplementation()
    {
    }

    SystemCommandsImplementation::~SystemCommandsImplementation()
    {
    }

    uint32_t SystemCommandsImplementation::Execute(const CommandId& id, const string& params)
    {
        ISystemCommands::ICommand* command = Command(id);
        return command ? command->Execute(params) : Core::ERROR_UNAVAILABLE;
    }

    RPC::IStringIterator* SystemCommandsImplementation::SupportedCommands() const
    {
        return Core::Service<RPC::StringIterator>::Create<RPC::IStringIterator>(Store::Instance().Commands());
    }

     ISystemCommands::ICommand* SystemCommandsImplementation::Command(const CommandId& id) const
     {
        return Store::Instance().Command(id);
     }

    SERVICE_REGISTRATION(SystemCommandsImplementation, 1, 0);

}
}
