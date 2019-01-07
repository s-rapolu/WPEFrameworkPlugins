#include "Module.h"
#include <interfaces/ICrashDummy.h>


namespace WPEFramework {
namespace Plugin {


class CrashDummyImplementation
    : public Exchange::ICrashDummy {

public:
    CrashDummyImplementation()
    : _observers()
    , _config()
    , _crashDelay(0)
    {

    };

    virtual ~CrashDummyImplementation()
    {

    };

    /*!
     * Method that results with SEGFAULT
    */
    void Crash() override;
    bool Configure(PluginHost::IShell *shell) override;

    BEGIN_INTERFACE_MAP(CrashDummyImplementation)
    INTERFACE_ENTRY(Exchange::ICrashDummy)
    END_INTERFACE_MAP

private:
    class Config
        : public Core::JSON::Container {
    private:
        Config(const Config&) = delete;
        Config& operator=(const Config&) = delete;

    public:
        Config()
            : Core::JSON::Container()
            , CrashDelay(0)
        {
            Add(_T("crashDelay"), &CrashDelay);
        }
        ~Config()
        {
        }

    public:
        Core::JSON::DecUInt8 CrashDelay;
    };

private:
    CrashDummyImplementation(const CrashDummyImplementation&) = delete;
    CrashDummyImplementation& operator=(const CrashDummyImplementation&) = delete;

    std::list<PluginHost::IStateControl::INotification*> _observers;
    Config _config;
    uint8_t _crashDelay;
};

SERVICE_REGISTRATION(CrashDummyImplementation, 1, 0);

void CrashDummyImplementation::Crash()
{
    TRACE_GLOBAL(Trace::Information, ("%s", __FUNCTION__));

    TRACE_GLOBAL(Trace::Information, ("CrashDelay = %d", _crashDelay) );
    sleep(_crashDelay);

    TRACE_GLOBAL(Trace::Information, (_T("I WILL MAKE THE CRASH. TA-DAM!")));
    uint8_t *tmp = NULL;
    *tmp = 3; // segmentaion fault

    return;
}

bool CrashDummyImplementation::Configure(PluginHost::IShell *shell)
{
    TRACE_GLOBAL(Trace::Information, ("%s", __FUNCTION__));

    bool status = false;

    ASSERT(shell != nullptr);
    status = _config.FromString(shell->ConfigLine());

    _crashDelay = _config.CrashDelay.Value();

    return status;
}

} // namespace Plugin
} // namespce WPEFramework