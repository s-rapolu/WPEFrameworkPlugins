#pragma once

#include "Module.h"
#include <interfaces/ICrashDummy.h>

namespace WPEFramework {
namespace Plugin {

class CrashDummy
    : public PluginHost::IPlugin
    , public PluginHost::IWeb {

public:
    // maximum wait time for process to be spawned
    static constexpr uint32_t ImplWaitTime = 1000;

public:
    CrashDummy()
        : _shell(nullptr)
        , _skipURL(0)
        , _pid(0)
        , _implementation(nullptr)
        , _notification(this)
    {
    }

    ~CrashDummy() {}

private:
    CrashDummy(const CrashDummy&) = delete;
    CrashDummy& operator=(const CrashDummy&) = delete;

public:
    void Activated();
    void Deactivated(RPC::IRemoteProcess* process);

    // Build QueryInterface implementation, specifying all possible interfaces to be returned.
    BEGIN_INTERFACE_MAP(CrashDummy)
    INTERFACE_ENTRY(PluginHost::IPlugin)
    INTERFACE_ENTRY(PluginHost::IWeb)
    INTERFACE_AGGREGATE(Exchange::ICrashDummy, _implementation)
    END_INTERFACE_MAP

public:
    class CrashParameters : public Core::JSON::Container {
    private:
        CrashParameters(const CrashParameters&) = delete;
        CrashParameters& operator=(const CrashParameters&) = delete;

    public:
        CrashParameters()
            : Core::JSON::Container()
            , CrashCount()
        {
            Add(_T("crashCount"), &CrashCount);
        }

        ~CrashParameters() {}

    public:
        Core::JSON::DecUInt8 CrashCount;
    };

private:
    class Notification : public RPC::IRemoteProcess::INotification {

    private:
        Notification() = delete;
        Notification(const Notification&) = delete;
        Notification& operator=(const Notification&) = delete;

    public:
        explicit Notification(CrashDummy* parent)
            : _parent(*parent)
        {
            ASSERT(parent != nullptr);
        }
        ~Notification() {}

    public:
        virtual void Activated(RPC::IRemoteProcess*) override { _parent.Activated(); }
        virtual void Deactivated(RPC::IRemoteProcess* process) override { _parent.Deactivated(process); }

        BEGIN_INTERFACE_MAP(Notification)
        INTERFACE_ENTRY(RPC::IRemoteProcess::INotification)
        END_INTERFACE_MAP

    private:
        CrashDummy& _parent;
    };

private:
    PluginHost::IShell* _shell;
    uint8_t _skipURL;
    uint32_t _pid;
    Exchange::ICrashDummy* _implementation;
    Core::Sink<Notification> _notification;

public: // IPugin
    virtual const string Initialize(PluginHost::IShell* shell) override;
    virtual void Deinitialize(PluginHost::IShell* shell) override;
    virtual string Information() const override;

public: // IWeb
    virtual void Inbound(Web::Request& request) override;
    virtual Core::ProxyType<Web::Response> Process(const Web::Request& request) override;
};

} // namespace Plugin
} // namespace WPEFramework
