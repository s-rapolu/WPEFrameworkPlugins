#include "Module.h"
#include <interfaces/ISystemCommands.h>
#include "SystemCommandsImplementation.h"

#include <linux/usbdevice_fs.h>

namespace WPEFramework {
namespace Plugin {

    using Exchange::ISystemCommands;

    class USBResetCommand : public ISystemCommands::ICommand {
    public:
        USBResetCommand();
        ~USBResetCommand() override;
        uint32_t Execute(const std::string& params) final;
        std::string Description() const final;
        std::string Id() const final;

        BEGIN_INTERFACE_MAP(USBResetCommand)
            INTERFACE_ENTRY(ISystemCommands::ICommand)
        END_INTERFACE_MAP
    private:
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

        USBResetCommand(const USBResetCommand&) = delete;
        USBResetCommand& operator=(const USBResetCommand&) = delete;
    };

    USBResetCommand::USBResetCommand()
    {
        SystemCommandsImplementation::Register(this);
    }

    USBResetCommand::~USBResetCommand()
    {
        SystemCommandsImplementation::Unregister(this);
    }

    std::string USBResetCommand::Description() const
    {
        return "A command resetting an USB device";
    }

    std::string USBResetCommand::Id() const
    {
        return "USBReset";
    }

    uint32_t USBResetCommand::Execute(const std::string& params)
    {
        uint32_t result = Core::ERROR_NONE;
        std::string device;
        Core::JSON::ArrayType<Param> parsedParams;
        Core::JSON::IElement::FromString(params, parsedParams);
        auto paramsIter = parsedParams.Elements();
        while (paramsIter.Next()) {
            auto& param = paramsIter.Current();
            if (param.Name == "device")
             device = param.Value;
        }

        int fd = open(device.c_str(), O_WRONLY);
        if (fd < 0) {
            TRACE(Trace::Error, (_T("Opening of %s failed."), device.c_str()));
            result = Core::ERROR_GENERAL;
        } else {
            int rc = ioctl(fd, USBDEVFS_RESET, 0);
            if (rc < 0) {
                TRACE(Trace::Error, (_T("ioctl(USBDEVFS_RESET) failed with %d. Errno: %d"), rc, errno));
                result = Core::ERROR_GENERAL;
            }
            close(fd);
        }

        return result;
    }

    static USBResetCommand* _registrar(Core::Service<USBResetCommand>::Create<USBResetCommand>());
}
}
