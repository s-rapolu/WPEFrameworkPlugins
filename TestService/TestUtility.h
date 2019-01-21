#pragma once

#include "Module.h"
#include <interfaces/ITestUtility.h>

namespace WPEFramework {
namespace Plugin {

class TestUtility : public PluginHost::IPlugin, public PluginHost::IWeb, public Exchange::ITestUtility {

public:
    TestUtility(const TestUtility&) = delete;
    TestUtility& operator=(const TestUtility&) = delete;
    TestUtility() = default;
    virtual ~TestUtility() = default;

    BEGIN_INTERFACE_MAP(TestUtility)
        INTERFACE_ENTRY(IPlugin)
        INTERFACE_ENTRY(IWeb)
        INTERFACE_ENTRY(Exchange::ITestUtility)
    END_INTERFACE_MAP

public:
    //  IPlugin methods
    // -------------------------------------------------------------------------------------------------------

    const string Initialize(PluginHost::IShell* service) override {
        return string();
    }

    void Deinitialize(PluginHost::IShell* service) override {
    }

    string Information() const override {
        return string();
    }

    //  IWeb methods
    // -------------------------------------------------------------------------------------------------------
    void Inbound(WPEFramework::Web::Request& request) override {
    }

    Core::ProxyType<Web::Response> Process(const WPEFramework::Web::Request& request) override {
        Core::ProxyType<Web::Response> result;
        result->ErrorCode = Web::STATUS_NOT_IMPLEMENTED;
        result->Message = "Not implemented";
   }

    //  ITestUtility methods
    // -------------------------------------------------------------------------------------------------------
    Exchange::ITestUtility::ICommand::IIterator* Commands() const override {
        return CommandAdministrator::Instance().Commands();
    }

    Exchange::ITestUtility::ICommand* Command(const string& name) const override {
        return CommandAdministrator::Instance().Command(name);
    }

public:
    class CommandAdministrator {
    
    private:
        using CommandContainer = std::map<string, Exchange::ITestUtility::ICommand*>;

        class Iterator : public Exchange::ITestUtility::ICommand::IIterator {
        public:
            using IteratorImpl = Core::IteratorMapType<CommandContainer, Exchange::ITestUtility::ICommand*, string>;

            explicit Iterator(const CommandContainer& commands) 
                : Exchange::ITestUtility::ICommand::IIterator()
                , _container(commands)
                , _iterator(_container) {

                  }

            virtual ~Iterator() = default;

            Iterator(const Iterator&) = delete;
            Iterator& operator= (const Iterator&) = delete;

            BEGIN_INTERFACE_MAP(Iterator)
                INTERFACE_ENTRY(Exchange::ITestUtility::ICommand::IIterator)
            END_INTERFACE_MAP

            void Reset() override {
                _iterator.Reset(0);
            }

            bool IsValid() const override {
                return _iterator.IsValid();
            }

            bool Next() override {
                return _iterator.Next();
            }

            Exchange::ITestUtility::ICommand* Command() const override {
                return *_iterator;
            }

            private:
                CommandContainer _container;
                IteratorImpl _iterator;
        };

    private: 

        // get outside template class to prevent multiple statics per TESTCLASS
        class TypeToNameConversion {
            private:
                TypeToNameConversion() = default;

                // DUMMY to solve complete specialization not allowed in class
                template<typename T, typename DUMMY = void> struct Supported : std::false_type { };
                template<typename DUMMY> struct Supported<uint8_t, DUMMY> : std::true_type { static const char* const name; };
                template<typename DUMMY> struct Supported<bool, DUMMY> : std::true_type { static const char* const name; };

            public: 

                template<typename TYPENAME>
                static const char* TypeToName() {
                    using CheckType = Supported<TYPENAME>;

                    static_assert( CheckType::value, "Type not supported");
                    return CheckType::name;
                }
        };

    public:

        // no support for output parameters yet, left as exercise for reader ;)
        template<class TESTCLASS>
        class CommandBase : public Exchange::ITestUtility::ICommand {
        public:
            template<typename type> 
            struct ParamSpecification {
                ParamSpecification(const char* name) : _name(name) {}
                const char* _name;
            };

            // probably we could use the JSON container and integrate it with the parameters to parse, left as exercise for reader ;)
            class SignatureBuilder {
            public:
                SignatureBuilder(const SignatureBuilder&) = delete;
                SignatureBuilder& operator=(const SignatureBuilder&) = delete;

                template<typename RETURNTYPE>
                SignatureBuilder(const ParamSpecification<RETURNTYPE>& returnspecification)
                    : _signature()
                {
                    _signature += _T("{\n\"returndescription\" : \"");
                    _signature += returnspecification._name;
                    _signature += _T("\",\n");
                    _signature += _T("\n\"returntype\" : \"");
                    _signature += TypeToNameConversion::TypeToName<RETURNTYPE>();
                    _signature += _T("\",\n");
                    _signature += _T("\n\"name\" : \"");
                    _signature += CommandBase<TESTCLASS>::DetermineName();
                    _signature += _T("\",\n \"Params\" : [\n");
                }

                template<typename TYPE>
                SignatureBuilder& Parameter(const ParamSpecification<TYPE>& parameter) {
                    _signature += _T("{\"parametername\" : \"");
                    _signature += parameter._name;
                    _signature += _T("\",\n");
                    _signature += _T("\n\"type\" : \"");
                    _signature += TypeToNameConversion::TypeToName<TYPE>();
                    _signature += _T("\"},\n");
                    return *this;
                }

                virtual ~SignatureBuilder() = default;

            private:

                friend class CommandBase;

                const string& ToString() const {
                    _signature += _T("\n]\n}");
                    return _signature;
                }

                mutable string _signature;
            };


            explicit CommandBase(const SignatureBuilder& signature) 
            : Exchange::ITestUtility::ICommand()
            ,  _name(DetermineName())
            ,  _signature(signature.ToString()) {
            }

            virtual ~CommandBase() = default;

            CommandBase(const CommandBase&) = delete;
            CommandBase& operator= (const CommandBase&) = delete;

            //if we would like returning of result to be done in a consistent way (e.g. output params) you could also override Execute and use Template method pattern

            const string& Signature() const override {
                return _signature;
            } 

            const string& Name() const final {
                return _name;
            }

        private:
            static string DetermineName() {
                return Demangle<TESTCLASS>();
            }

            // should be moved to core
            template<typename TYPE>
            static string Demangle() {
                int status;
                char * demangled = abi::__cxa_demangle(typeid(TYPE).name(),0,0,&status);
                string result(demangled);
                free(demangled);
                return result;
            }

            string _name;
            string _signature;
        };

    public: 
        static CommandAdministrator& Instance();

        CommandAdministrator() 
            : _adminLock()
            , _container() {

            }

        ~CommandAdministrator() = default;

        CommandAdministrator(const CommandAdministrator&) = delete;
        CommandAdministrator& operator= (const CommandAdministrator&) = delete;

        void Announce(Exchange::ITestUtility::ICommand* command) {

            ASSERT(command != nullptr);

            _adminLock.Lock();
            _container[command->Name()] = command;
            _adminLock.Unlock();
        }

        void Revoke(Exchange::ITestUtility::ICommand* command) {

            ASSERT(command != nullptr);

            _adminLock.Lock();
            _container.erase(command->Name());
            _adminLock.Unlock();
        }

        Exchange::ITestUtility::ICommand* Command(const string& name) {
             Exchange::ITestUtility::ICommand* command = nullptr;
            _adminLock.Lock();
            command = _container[name];
            _adminLock.Unlock();
            return command;
        }

        Exchange::ITestUtility::ICommand::IIterator* Commands() const {
            Exchange::ITestUtility::ICommand::IIterator* iterator = nullptr;
            _adminLock.Lock();
            iterator = Core::Service<Iterator>::Create<Exchange::ITestUtility::ICommand::IIterator>(_container);
            _adminLock.Unlock();
            return iterator;
        }

        private:
            mutable Core::CriticalSection _adminLock;     
            CommandContainer _container;
    };
};

}
}
