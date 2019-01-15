#pragma once

#include "Module.h"
#include "TestClient.h"

#include <interfaces/ITestDummy.h>
#include <interfaces/IMemory.h>

namespace WPEFramework
{
namespace Plugin
{
    class TestDummy : public PluginHost::IPlugin, public PluginHost::IWeb
    {
        private:
            class Notification : public RPC::IRemoteProcess::INotification
            {
                private:
                    Notification() = delete;
                    Notification(const Notification&) = delete;

                public:
                    explicit Notification(TestDummy* parent)
                        : _parent(*parent)
                    {
                        ASSERT(parent != nullptr);
                    }
                    virtual ~Notification() {}

                public:
                    virtual void Activated(RPC::IRemoteProcess* /* process */) {}
                    virtual void Deactivated(RPC::IRemoteProcess* process)
                    {
                        _parent.Deactivated(process);
                    }

                    BEGIN_INTERFACE_MAP(Notification)
                        INTERFACE_ENTRY(RPC::IRemoteProcess::INotification)
                    END_INTERFACE_MAP

                private:
                    TestDummy& _parent;
            };
       public:
            class Data : public Core::JSON::Container
            {
                public:
                    class Statm : public Core::JSON::Container
                    {
                        private:
                            Statm(const Statm&) = delete;
                            Statm& operator=(const Statm&) = delete;

                        public:
                            Statm()
                                : Core::JSON::Container()
                                , Allocated(0)
                                , Size(0)
                                , Resident(0)
                            {
                                Add(_T("allocated"), &Allocated);
                                Add(_T("size"), &Size);
                                Add(_T("resident"), &Resident);
                            }
                            ~Statm()
                            {
                            }

                        public:
                            Core::JSON::DecSInt32 Allocated;
                            Core::JSON::DecSInt32 Size;
                            Core::JSON::DecSInt32 Resident;
                    };

                    class MallocData : public Core::JSON::Container
                    {
                        private:
                            MallocData(const MallocData&) = delete;
                            MallocData& operator=(const MallocData&) = delete;

                        public:
                            MallocData()
                                : Core::JSON::Container()
                                , Size(0)
                            {
                                Add(_T("size"), &Size);
                            }
                            ~MallocData()
                            {
                            }

                        public:
                            Core::JSON::DecSInt32 Size;
                    };

                private:
                    Data(const Data&) = delete;
                    Data& operator=(const Data&) = delete;

                public:
                    Data()
                        : Core::JSON::Container()
                        , Memory()
                        , Malloc()
                    {
                        Add(_T("statm"), &Memory);
                        Add(_T("malloc"), &Malloc);
                    }

                    virtual ~Data()
                    {
                    }

                public:
                    Statm Memory;
                    MallocData Malloc;
            };

            TestDummy()
                : _service(nullptr)
                , _notification(this)
                , _memory(nullptr)
                , _implementation(nullptr)
                , _pluginName("TestDummy")
                , _skipURL(0)
                , _pid(0)
                , _client(){}

            virtual ~TestDummy() {}

            BEGIN_INTERFACE_MAP(TestDummy)
                INTERFACE_ENTRY(PluginHost::IPlugin)
                INTERFACE_ENTRY(PluginHost::IWeb)
                INTERFACE_AGGREGATE(Exchange::IMemory, _memory)
                INTERFACE_AGGREGATE(Exchange::ITestDummy, _implementation)
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
            TestDummy(const TestDummy&) = delete;
            TestDummy& operator=(const TestDummy&) = delete;

            void Deactivated(RPC::IRemoteProcess* process);
            void ProcessTermination(uint32_t pid);
            void GetStatm(Data::Statm& statm);

            // Memory Test Methods
            Core::ProxyType<Web::Response> Statm(const Web::Request& request);
            Core::ProxyType<Web::Response> Malloc(const Web::Request& request);
            Core::ProxyType<Web::Response> Free(const Web::Request& request);

            PluginHost::IShell* _service;
            Core::Sink<Notification> _notification;
            Exchange::IMemory* _memory;
            Exchange::ITestDummy* _implementation;
            string _pluginName;
            uint8_t _skipURL;
            uint32_t _pid;
            TestCore::TestClient _client;
    };

} // namespace Plugin
} // namespace WPEFramework
