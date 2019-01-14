#pragma once

#include "Module.h"
#include "TestData.h"
#include <interfaces/IMallocDummy.h>
#include <interfaces/IMemory.h>

#include <functional>
#include <list>

namespace WPEFramework
{
//ToDo: Potentially move this Interface to WPEFramework
namespace Exchange
{
    struct ITest
    {
        //ToDo: ID needs to be adjusted
        enum { ID = 0x12000011 };

        virtual ~ITest() {}
        virtual bool Reqister(const string &name, Web::Request::type requestType, const std::function<Core::ProxyType<Web::Response>(const Web::Request&)> &processRequest) = 0;
        virtual bool Unregister(const string &name) = 0;
        virtual Core::ProxyType<Web::Response> Process(const Web::Request& request, uint8_t skipURL) = 0;
    };
}
namespace Plugin
{
//ToDo: Move to separate module
namespace TestCore
{
    class TestClient : public Exchange::ITest
    {
        private:
            class MethodItem
            {
                private:
                    MethodItem& operator=(const MethodItem&) = delete;

                public:
                    MethodItem(string name, const std::function<Core::ProxyType<Web::Response>(const Web::Request&)> &callback)
                        : _name(name)
                        , _callback(callback)
                    {
                    }

                    virtual ~MethodItem() {}

                public:
                    string _name;
                    std::function<Core::ProxyType<Web::Response>(const Web::Request&)> _callback;
            };

        private:
            TestClient(const TestClient&) = delete;
            TestClient& operator=(const TestClient&) = delete;

        public:
            TestClient()
            {
                _tests.insert(std::pair<Web::Request::type, std::list<MethodItem>>(Web::Request::type::HTTP_POST, {}));
                _tests.insert(std::pair<Web::Request::type, std::list<MethodItem>>(Web::Request::type::HTTP_GET, {}));
            }

            virtual ~TestClient()
            {
                for (auto& test : _tests)
                {
                    test.second.clear();
                }
                _tests.clear();
            }
            //ToDo: Uncomment when this class will be mover to separate module
#if 0
            BEGIN_INTERFACE_MAP(MallocDummyImplementation)
                INTERFACE_ENTRY(Exchange::IMallocDummy)
            END_INTERFACE_MAP
#endif
            // ITest methods
            bool Reqister(const string &name, Web::Request::type requestType, const std::function<Core::ProxyType<Web::Response>(const Web::Request&)> &processRequest)
            {
                bool status = false;
                MethodItem newTestMethod(name, processRequest);

                for (auto& test : _tests)
                {
                    if (test.first == requestType)
                    {
                        test.second.push_back(newTestMethod);
                        status = true;
                    }
                }

                if (!status)
                {
                    SYSLOG(Trace::Fatal, (_T("Request type : %d is not supported"), requestType))
                }

                return status;
            }

            bool Unregister(const string &name)
            {
                //ToDo: Missing implementation
                return false;
            }

            Core::ProxyType<Web::Response> Process(const Web::Request& request, uint8_t skipURL)
            {
                bool exit = false;
                Core::TextSegmentIterator index(Core::TextFragment(request.Path, skipURL, request.Path.length() - skipURL), false, '/');
                Core::ProxyType<Web::Response> result;

                index.Next();
                index.Next();

                for (auto const& test : _tests)
                {
                    if (test.first == request.Verb)
                    {
                        for (auto const& method : test.second)
                        {
                            if (method._name == index.Current().Text())
                            {
                                result = method._callback(request);
                                exit = true;
                                break;
                            }
                        }
                    }
                    if (exit)
                        break;
                }

                return result;
            }
        private:
            std::map<Web::Request::type, std::list<MethodItem>> _tests;
    };
}
    class MallocDummy : public PluginHost::IPlugin, public PluginHost::IWeb
    {
        private:
            class Notification : public RPC::IRemoteProcess::INotification
            {
                private:
                    Notification() = delete;
                    Notification(const Notification&) = delete;

                public:
                    explicit Notification(MallocDummy* parent)
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
                    MallocDummy& _parent;
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

            MallocDummy()
                : _service(nullptr)
                , _notification(this)
                , _memory(nullptr)
                , _mallocDummy(nullptr)
                , _pluginName("MallocDummy")
                , _skipURL(0)
                , _pid(0)
                , _client(){}

            virtual ~MallocDummy() {}

            BEGIN_INTERFACE_MAP(MallocDummy)
                INTERFACE_ENTRY(PluginHost::IPlugin)
                INTERFACE_ENTRY(PluginHost::IWeb)
                INTERFACE_AGGREGATE(Exchange::IMemory, _memory)
                INTERFACE_AGGREGATE(Exchange::IMallocDummy, _mallocDummy)
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
            MallocDummy(const MallocDummy&) = delete;
            MallocDummy& operator=(const MallocDummy&) = delete;

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
            Exchange::IMallocDummy* _mallocDummy;
            string _pluginName;
            uint8_t _skipURL;
            uint32_t _pid;
            TestCore::TestClient _client;
    };

} // namespace Plugin
} // namespace WPEFramework
