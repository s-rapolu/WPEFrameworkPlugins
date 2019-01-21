#include "TestUtility.h"

namespace WPEFramework {

namespace Plugin {

class AllocateCommand;

using AllocateCommandBase = TestUtility::CommandAdministrator::CommandBase<AllocateCommand>;

class AllocateCommand : public AllocateCommandBase {
public: 
    AllocateCommand()
        : AllocateCommandBase( 
            AllocateCommandBase::SignatureBuilder(AllocateCommandBase::ParamSpecification<bool>("returns something"))
                .Parameter(AllocateCommandBase::ParamSpecification<uint8_t>("param1"))
                .Parameter(AllocateCommandBase::ParamSpecification<uint8_t>("param2"))
                             ) {

        TestUtility::CommandAdministrator::Instance().Announce(this);
    }

    virtual ~AllocateCommand() {
        TestUtility::CommandAdministrator::Instance().Revoke(this);
    };

    AllocateCommand(const AllocateCommand&) = delete;
    AllocateCommand& operator= (const AllocateCommand&) = delete;

    const string& Description() const override {
        return _description;
    }

    string Execute(const string& params) final {
        string result(_T("{\"value\" : "));
        Parameters parameters;
        parameters.FromString(params);
        bool resultvalue = Allocate(parameters.param1.Value(), parameters.param2.Value());
        result += std::to_string(resultvalue);
        result += _T("}");
        return result;
    }

    bool Allocate(const uint8_t number1, const uint8_t number2) {
        printf("memory allocated %u %u!!!!!!!\n", number1, number2);
        return true;
    }

    BEGIN_INTERFACE_MAP(AllocateCommand)
        INTERFACE_ENTRY(Exchange::ITestUtility::ICommand)   
    END_INTERFACE_MAP

private:

    class Parameters : public Core::JSON::Container {
        public:
            Parameters(const Parameters&) = delete;
            Parameters& operator=(const Parameters&) = delete;

            Parameters()
                : param1()
                , param2()
            {
                Add(_T("Param1"), &param1);
                Add(_T("Param2"), &param2);
            }

            virtual ~Parameters() = default;

        public:
            Core::JSON::DecUInt8 param1;
            Core::JSON::DecUInt8 param2;
        };

    const string _description = _T("Allocate huge amounts of memory!");

};

static AllocateCommand* g_singleton(Core::Service<AllocateCommand>::Create<AllocateCommand>());

}   
}
