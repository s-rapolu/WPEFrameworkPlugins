#include "Module.h"
#include <interfaces/ISystemCommand.h>
#include "SystemCommandImplementation.h"

#include <linux/usbdevice_fs.h>

namespace WPEFramework {
namespace Plugin {

    using Exchange::ISystemCommand;

    namespace {
        class USBReset : public Exchange::ISystemCommand::IExecutor {
        public:
            USBReset() {}
            ~USBReset() override;
            bool Execute(const Exchange::ISystemCommand::CommandParams& params) final;

            BEGIN_INTERFACE_MAP(USBReset)
            INTERFACE_ENTRY(Exchange::ISystemCommand::IExecutor)
            END_INTERFACE_MAP
        };

        USBReset::~USBReset()
        {
        }

        bool USBReset::Execute(const ISystemCommand::CommandParams& params)
        {
            TRACE(Trace::Information, (_T("USBReset with:\n")));
            for (auto& param : params)
                TRACE(Trace::Information, (_T("param %s=%s"), param.name, param.value));
            bool result = true;
            std::string device;
            for (auto& param : params) {
                if (param.name == "device")
                    device = param.value;
            }
            int fd = open(device.c_str(), O_WRONLY);
            if (fd < 0) {
                TRACE(Trace::Error, (_T("Opening of %s failed."), device.c_str()));
                result = false;
            } else {
                int rc = ioctl(fd, USBDEVFS_RESET, 0);
                if (rc < 0) {
                    TRACE(Trace::Error, (_T("ioctl(USBDEVFS_RESET) failed with %d. Errno: %d"), rc, errno));
                    result = false;
                }
                close(fd);
            }

            return result;
        }
    }  // namespace

    SystemCommandImplementation::SystemCommandImplementation()
    {
        auto* usbReset = Core::Service<USBReset>::Create<Exchange::ISystemCommand::IExecutor>();
        Register("USBReset", usbReset, true);
    }

    SystemCommandImplementation::~SystemCommandImplementation()
    {
    }

    void SystemCommandImplementation::Register(const Exchange::ISystemCommand::CommandId& id,
                                                        Exchange::ISystemCommand::IExecutor* delegate)
    {
        Register(id, delegate, false);
    }

    void SystemCommandImplementation::Register(const Exchange::ISystemCommand::CommandId& id,
                                                        Exchange::ISystemCommand::IExecutor* delegate,
                                                        bool builtIn)
    {
        _adminLock.Lock();
        if (!builtIn) {
            auto old = _delegates.find(id);
            if (old !=  _delegates.end()) {
                old->second->Release();
                _delegates.erase(old);
            }
        }
        auto inserted = _delegates.insert(std::make_pair(id, delegate));
        if (inserted.second) {
            delegate->AddRef();
        }
        _adminLock.Unlock();
    }

    void SystemCommandImplementation::Unregister(Exchange::ISystemCommand::IExecutor* delegate)
    {
        _adminLock.Lock();
        auto old = std::find_if(_delegates.begin(), _delegates.end(),
                             [delegate](const std::pair<Exchange::ISystemCommand::CommandId,
                                                        Exchange::ISystemCommand::IExecutor*>& entry) {
            return entry.second == delegate;
        });
        _delegates.erase(old);
        _adminLock.Unlock();
    }

    bool SystemCommandImplementation::Execute(const Exchange::ISystemCommand::CommandId& id,
                                                       const Exchange::ISystemCommand::CommandParams& params)
    {
        _adminLock.Lock();
        auto delegate = _delegates.find(id);
        bool result = delegate != _delegates.end();
        if (result) {
            result = delegate->second->Execute(params);
        }
        _adminLock.Unlock();
        return result;
    }

    std::vector<Exchange::ISystemCommand::CommandId> SystemCommandImplementation::SupportedCommands() const
    {
        std::vector<Exchange::ISystemCommand::CommandId> result;
        _adminLock.Lock();
        for (auto& it : _delegates) {
            result.push_back(it.first);
        }
        _adminLock.Unlock();
        return result;
    }

    SERVICE_REGISTRATION(SystemCommandImplementation, 1, 0);

}
}
