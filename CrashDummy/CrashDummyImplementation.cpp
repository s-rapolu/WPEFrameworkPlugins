#include "Module.h"
#include <interfaces/ICrashDummy.h>


namespace WPEFramework {
namespace Plugin {


class CrashDummyImplementation
    : public Exchange::ICrashDummy {

public:
    CrashDummyImplementation()
    : wtf(0)
    , _observers(){

    };
    virtual ~CrashDummyImplementation() {

    };

    void Crash()
    {
        TRACE(Trace::Information, (_T("I WILL MAKE A CRASH. TA-DAM!")));
        return;
    }

    BEGIN_INTERFACE_MAP(CrashDummyImplementation)
        INTERFACE_ENTRY(Exchange::ICrashDummy)
    END_INTERFACE_MAP

private:
    CrashDummyImplementation(const CrashDummyImplementation&) = delete;
    CrashDummyImplementation& operator=(const CrashDummyImplementation&) = delete;
    int wtf;
    std::list<PluginHost::IStateControl::INotification*> _observers;

};

SERVICE_REGISTRATION(CrashDummyImplementation, 1, 0);



} // namespace Plugin
} // namespce WPEFramework