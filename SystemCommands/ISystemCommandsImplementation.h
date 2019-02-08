#pragma once

#include "Module.h"
#include <interfaces/ISystemCommands.h>

namespace WPEFramework {
namespace Plugin {

    class ISystemCommandsImplementation : public Exchange::ISystemCommands {
    public:
        ISystemCommandsImplementation() = default;
        ~ISystemCommandsImplementation() override = default;

        BEGIN_INTERFACE_MAP(ISystemCommandsImplementation)
            INTERFACE_ENTRY(Exchange::ISystemCommands)
        END_INTERFACE_MAP

        uint32_t USBReset(const std::string& device) final;

    private:
        ISystemCommandsImplementation(const ISystemCommandsImplementation&) = delete;
        ISystemCommandsImplementation& operator=(const ISystemCommandsImplementation&) = delete;
    };

} // namespace Plugin
} // namespace WPEFramework
