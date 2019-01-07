#ifndef __CRASHDUMMY_H__
#define __CRASHDUMMY_H__

#include "Module.h"
#include <interfaces/ICrashDummy.h>


namespace WPEFramework {
namespace Plugin {

class CrashDummy
    : public PluginHost::IPlugin
    , public PluginHost::IWeb {
    public:
        CrashDummy()
        : _shell(nullptr)
        , _implementation(nullptr)
        , _notification(this)
        , _pid(0)
        , _skipURL(0) {
        };

        ~CrashDummy(){};

        void Deactivated(RPC::IRemoteProcess *process);

        // Build QueryInterface implementation, specifying all possible interfaces to be returned.
        BEGIN_INTERFACE_MAP(CrashDummy)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(PluginHost::IWeb)
            INTERFACE_AGGREGATE(Exchange::ICrashDummy, _implementation)
        END_INTERFACE_MAP

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
            ~Notification()
            {
            }

        public:
            virtual void Activated(RPC::IRemoteProcess*) {
            }
            virtual void Deactivated(RPC::IRemoteProcess* process) {
                _parent.Deactivated(process);
            }

            BEGIN_INTERFACE_MAP(Notification)
                INTERFACE_ENTRY(RPC::IRemoteProcess::INotification)
            END_INTERFACE_MAP

        private:
            CrashDummy& _parent;
        };

    private:
        CrashDummy(const CrashDummy&) = delete;
        CrashDummy& operator=(const CrashDummy&) = delete;

        PluginHost::IShell* _shell;
        Exchange::ICrashDummy* _implementation;
        Core::Sink<Notification> _notification;
        uint32_t _pid;
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

#endif // __CRASHDUMMY_H__