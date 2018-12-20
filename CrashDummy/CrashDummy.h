#ifndef __CRASH_DUMMY_H__
#define __CRASH_DUMMY_H__

#include "Module.h"

namespace WPEFramework {
namespace Plugin {

class CrashDummy
    : public PluginHost::IPlugin
    , public PluginHost::IWeb {
    public:
        CrashDummy()
        : _shell(nullptr)
        , _skipURL(0) {
        };

        ~CrashDummy(){};

        // Build QueryInterface implementation, specifying all possible interfaces to be returned.
        BEGIN_INTERFACE_MAP(CrashDummy)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(PluginHost::IWeb)
        END_INTERFACE_MAP

    private:
        CrashDummy(const CrashDummy&) = delete;
        CrashDummy& operator=(const CrashDummy&) = delete;

        PluginHost::IShell* _shell;
        uint8_t _skipURL;

    public: // IPugin
        virtual const string Initialize(PluginHost::IShell* shell) override;
        virtual void Deinitialize(PluginHost::IShell* shell) override;
        virtual string Information() const override;

    public: //IWeb
        virtual void Inbound(Web::Request& request) override;
        virtual Core::ProxyType<Web::Response> Process(const Web::Request& request) override;
};

} // namespace WPEFramework
} // namespace Plugin

#endif // __CRASH_DUMMY_H__