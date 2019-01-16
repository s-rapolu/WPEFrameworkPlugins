#pragma once

#include "Module.h"
#include <interfaces/IMemory.h>
#include <interfaces/ITestController.h>
#include "TestController.h"

namespace WPEFramework {
namespace Plugin {

class TestService : public PluginHost::IPlugin, public PluginHost::IWeb {
public:
    // maximum wait time for process to be spawned
    static constexpr uint32_t ImplWaitTime = 1000;

private:
    class Notification : public RPC::IRemoteProcess::INotification {
    private:
        Notification() = delete;
        Notification(const Notification&) = delete;

    public:
        explicit Notification(TestService* parent)
            : _parent(*parent)
        {
            ASSERT(parent != nullptr);
        }
        virtual ~Notification() {}

    public:
        virtual void Activated(RPC::IRemoteProcess* process) { _parent.Activated(process); }

        virtual void Deactivated(RPC::IRemoteProcess* process) { _parent.Deactivated(process); }

        BEGIN_INTERFACE_MAP(Notification)
        INTERFACE_ENTRY(RPC::IRemoteProcess::INotification)
        END_INTERFACE_MAP

    private:
        TestService& _parent;
    };

public:
    TestService()
        : _service(nullptr)
        , _notification(this)
        , _memory(nullptr)
        , _testsControllerImp(nullptr)
        , _pluginName("TestService")
        , _skipURL(0)
        , _pid(0)
    {
    }

    virtual ~TestService() {}

    BEGIN_INTERFACE_MAP(TestService)
    INTERFACE_ENTRY(PluginHost::IPlugin)
    INTERFACE_ENTRY(PluginHost::IWeb)
    INTERFACE_AGGREGATE(Exchange::IMemory, _memory)
    INTERFACE_AGGREGATE(Exchange::ITestController, _testsControllerImp)
    END_INTERFACE_MAP

    //   IPlugin methods
    // -------------------------------------------------------------------------------------------------------
    virtual const string Initialize(PluginHost::IShell* service) override;
    virtual void Deinitialize(PluginHost::IShell* service) override;
    virtual string Information() const override;

    //  IWeb methods
    // -------------------------------------------------------------------------------------------------------
    virtual void Inbound(Web::Request& request);
    virtual Core::ProxyType<Web::Response> Process(const Web::Request& request);

private:
    TestService(const TestService&) = delete;
    TestService& operator=(const TestService&) = delete;

    void Activated(RPC::IRemoteProcess* process);
    void Deactivated(RPC::IRemoteProcess* process);

    void ProcessTermination(uint32_t pid);

    PluginHost::IShell* _service;
    Core::Sink<Notification> _notification;
    Exchange::IMemory* _memory;
    Exchange::ITestController* _testsControllerImp;
    string _pluginName;
    uint8_t _skipURL;
    uint32_t _pid;
};

} // namespace Plugin
} // namespace WPEFramework
