#include "Module.h"
#include <interfaces/ICrashDummy.h>


namespace WPEFramework {
namespace Plugin {


class CrashDummyImplementation
    : public Exchange::ICrashDummy {

public:
    CrashDummyImplementation()
    : _observers(){

    };
    virtual ~CrashDummyImplementation() {

    };

    /*!
     * Method that results with SEGFAULT
    */
    void Crash()
    {
        TRACE_GLOBAL(Trace::Information, (_T("I WILL MAKE THE CRASH. TA-DAM!")));
        uint8_t *tmp = NULL;
        *tmp = 3; // segmentaion fault

        return;
    }

    BEGIN_INTERFACE_MAP(CrashDummyImplementation)
        INTERFACE_ENTRY(Exchange::ICrashDummy)
    END_INTERFACE_MAP

private:
    CrashDummyImplementation(const CrashDummyImplementation&) = delete;
    CrashDummyImplementation& operator=(const CrashDummyImplementation&) = delete;
    std::list<PluginHost::IStateControl::INotification*> _observers;

};

SERVICE_REGISTRATION(CrashDummyImplementation, 1, 0);



} // namespace Plugin
} // namespce WPEFramework