#pragma once

#include "Module.h"
#include <interfaces/ICrashDummy.h>

namespace WPEFramework {
namespace Plugin {

class CrashDummyImplementation : public Exchange::ICrashDummy {

public:
    CrashDummyImplementation()
        : _observers()
        , _config()
        , _crashDelay(0)
    {
    }

    virtual ~CrashDummyImplementation() {}

private:
    CrashDummyImplementation(const CrashDummyImplementation&) = delete;
    CrashDummyImplementation& operator=(const CrashDummyImplementation&) = delete;

public:
    bool Configure(PluginHost::IShell* shell) override;
    void Crash() override;
    bool CrashNTimes(uint8_t n) override;
    void ExecPendingCrash() override;
    uint8_t PendingCrashCount() override;

    BEGIN_INTERFACE_MAP(CrashDummyImplementation)
    INTERFACE_ENTRY(Exchange::ICrashDummy)
    END_INTERFACE_MAP

private:
    class Config : public Core::JSON::Container {
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
        ~Config() {}

    public:
        Core::JSON::DecUInt8 CrashDelay;
    };

private:
    bool SetPendingCrashCount(uint8_t newCrashCount);

private:
    std::list<PluginHost::IStateControl::INotification*> _observers;
    Config _config;
    uint8_t _crashDelay;
};

} // namespace Plugin
} // namespace WPEFramework
