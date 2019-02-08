#include "ISystemCommandsImplementation.h"

#include <linux/usbdevice_fs.h>

namespace WPEFramework {
namespace Plugin {

        SERVICE_REGISTRATION(ISystemCommandsImplementation, 1, 0);

        uint32_t ISystemCommandsImplementation::USBReset(const std::string& device)
        {
            uint32_t result = Core::ERROR_NONE;
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

} // namespace Plugin
} // namespace WPEFramework
