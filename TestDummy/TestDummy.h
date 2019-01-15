#pragma once

#include "Module.h"
#include "TestData.h"
#include "ITest.h"

#include <interfaces/IMallocDummy.h>
#include <interfaces/IMemory.h>

#include <list>
#include <vector>

namespace WPEFramework
{
namespace Plugin
{
//ToDo: Move to separate module
namespace TestCore
{
    static Core::ProxyPoolType<Web::JSONBodyType<TestData::Methods>> methodsFactory(2);
    static Core::ProxyPoolType<Web::JSONBodyType<TestData::MethodDescription>> methodDescriptionFactory(2);
    static Core::ProxyPoolType<Web::JSONBodyType<TestData::Parameters>> parametersFactory(2);

    class TestClient : public Exchange::ITest
    {
        private:
            class TestMethod
            {
                private:
                    TestMethod& operator=(const TestMethod&) = delete;

                public:
                    TestMethod(string name, string desciption, const std::map<int, std::vector<string>> &input, const std::map<int, std::vector<string>> &output, const std::function<Core::ProxyType<Web::Response>(const Web::Request&)> &callback)
                        : _name(name)
                        , _description(desciption)
                        , _input(input)
                        , _output(output)
                        , _callback(callback)
                    {
                    }

                    virtual ~TestMethod() {}

                public:
                    string _name;
                    string _description;
                    std::map<int, std::vector<string>> _input;
                    std::map<int, std::vector<string>> _output;
                    std::function<Core::ProxyType<Web::Response>(const Web::Request&)> _callback;
            };

        private:
            TestClient(const TestClient&) = delete;
            TestClient& operator=(const TestClient&) = delete;

        public:
            TestClient()
            {
                _tests.insert(std::pair<Web::Request::type, std::list<TestMethod>>(Web::Request::type::HTTP_POST, {}));
                _tests.insert(std::pair<Web::Request::type, std::list<TestMethod>>(Web::Request::type::HTTP_GET, {}));
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
            BEGIN_INTERFACE_MAP(TestDummyImplementation)
                INTERFACE_ENTRY(Exchange::IMallocDummy)
            END_INTERFACE_MAP
#endif
            // ITest methods
            bool Reqister(const string &name, const string &desciption, const std::map<int, std::vector<string>> &input, const std::map<int, std::vector<string>> &output, Web::Request::type requestType, const std::function<Core::ProxyType<Web::Response>(const Web::Request&)> &processRequest)
            {
                bool status = false;
                TestMethod newTestMethod(name, desciption, input, output, processRequest);

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
                Core::ProxyType<Web::Response> result(PluginHost::Factories::Instance().Response());
                result->Message = _T("Method not supported");
                result->ErrorCode = Web::STATUS_BAD_REQUEST;

                index.Next();
                index.Next();

                if (index.Current().Text() != _T("Methods"))
                {
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
                }
                else // Handle default /Methods/... request separately
                {
                    if (!index.Next())
                    {
                        result = GetMethods();
                    }
                    else
                    {
                        string methodName = index.Current().Text();
                        index.Next();
                        string callType = index.Current().Text();
                        if(callType == _T("Description"))
                        {
                            result = GetMethodDescription(methodName);
                        }
                        else if (callType == _T("Parameters"))
                        {
                            result = GetMethodParameters(methodName);
                        }
                    }
                }

                return result;
            }
        private:
            Core::ProxyType<Web::Response> GetMethods(void)
            {
                Core::ProxyType<Web::Response> result(PluginHost::Factories::Instance().Response());
                result->ErrorCode = Web::STATUS_OK;
                result->Message = (_T("Handle Methods request"));

                Core::ProxyType<Web::JSONBodyType<TestData::Methods>> response(methodsFactory.Element());
                for (auto const& test : _tests)
                {
                    for (auto const& method : test.second)
                    {
                        response->MethodNames.Add(Core::JSON::String(method._name));
                    }
                }

                result->ContentType = Web::MIMETypes::MIME_JSON;
                result->Body(Core::proxy_cast<Web::IBody>(response));

                return result;
            }

            Core::ProxyType<Web::Response> GetMethodDescription(string methodName)
            {
                bool exit = false;
                Core::ProxyType<Web::Response> result(PluginHost::Factories::Instance().Response());
                result->ErrorCode = Web::STATUS_OK;
                result->Message = (_T("Handle Methods Description request"));

                Core::ProxyType<Web::JSONBodyType<TestData::MethodDescription>> response(methodDescriptionFactory.Element());
                for (auto const& test : _tests)
                {
                    for (auto const& method : test.second)
                    {
                        if (method._name == methodName)
                        {
                            response->Description = method._description;
                            exit = true;
                            break;
                        }
                    }
                    if (exit)
                        break;
                }

                if (!exit)
                {
                    result->Message = _T("Method not supported");
                    result->ErrorCode = Web::STATUS_BAD_REQUEST;
                }

                result->ContentType = Web::MIMETypes::MIME_JSON;
                result->Body(Core::proxy_cast<Web::IBody>(response));

                return result;
            }

            Core::ProxyType<Web::Response> GetMethodParameters(string methodName)
            {
                bool exit = false;
                Core::ProxyType<Web::Response> result(PluginHost::Factories::Instance().Response());
                result->ErrorCode = Web::STATUS_OK;
                result->Message = (_T("Handle Methods Parameters request"));

                Core::ProxyType<Web::JSONBodyType<TestData::Parameters>> response(parametersFactory.Element());
                for (auto const& test : _tests)
                {
                    for (auto const& method : test.second)
                    {
                        if (method._name == methodName)
                        {
                            //ToDo: Fill Parameters
                            for (auto const& in : method._input)
                            {
                                TestData::Parameters::Parameter newInParam;
                                newInParam.Type = in.second[0];
                                newInParam.Name = in.second[1];
                                newInParam.Comment = in.second[2];
                                response->Input.Add(newInParam);
                            }

                            for (auto const& out : method._output)
                            {
                                TestData::Parameters::Parameter newOutParam;
                                newOutParam.Type = out.second[0];
                                newOutParam.Name = out.second[1];
                                newOutParam.Comment = out.second[2];
                                response->Input.Add(newOutParam);
                            }
                            exit = true;
                            break;
                        }
                    }
                    if (exit)
                        break;
                }

                if (!exit)
                {
                    result->Message = _T("Method not supported");
                    result->ErrorCode = Web::STATUS_BAD_REQUEST;
                }

                result->ContentType = Web::MIMETypes::MIME_JSON;
                result->Body(Core::proxy_cast<Web::IBody>(response));

                return result;
            }

        private:
            std::map<Web::Request::type, std::list<TestMethod>> _tests;
    };
}
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
                , _mallocDummy(nullptr)
                , _pluginName("TestDummy")
                , _skipURL(0)
                , _pid(0)
                , _client(){}

            virtual ~TestDummy() {}

            BEGIN_INTERFACE_MAP(TestDummy)
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
            Exchange::IMallocDummy* _mallocDummy;
            string _pluginName;
            uint8_t _skipURL;
            uint32_t _pid;
            TestCore::TestClient _client;
    };

} // namespace Plugin
} // namespace WPEFramework
