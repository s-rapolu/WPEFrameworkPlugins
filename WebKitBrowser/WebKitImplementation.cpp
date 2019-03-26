#include <memory>

#include "Module.h"

#include <wpe/webkit.h>

#include <glib.h>

#include "HTML5Notification.h"
#include "WebKitBrowser.h"

#include <iostream>

namespace WPEFramework {
namespace Plugin {

    // -----------------------------------------------------------------------------------------------------
    // Hide all NASTY C details that come with the POC libraries !!!!!
    // -----------------------------------------------------------------------------------------------------

    /* ---------------------------------------------------------------------------------------------------
struct CustomLoopHandler
{
	GSource source;
	uint32_t attentionPending;
};
static gboolean source_prepare(GSource*, gint*)
{
	return (false);
}
static gboolean source_check(GSource* mySource)
{
	return (static_cast<CustomLoopHandler*>(mySource)->attentionPending != 0);
}
static gboolean source_dispatch (GSource*, GSourceFunc callback, gpointer)
{
	uint32_t attention (static_cast<CustomLoopHandler*>(mySource)->attentionPending);

}
static GSourceFuncs _handlerIntervention =
{
	source_prepare,
	source_check,
	source_dispatch,
	nullptr
};
--------------------------------------------------------------------------------------------------- */
    static Exchange::IBrowser* implementation = nullptr;

    static void CloseDown()
    {
        // Seems we are destructed.....If we still have a pointer to the implementation, Kill it..
        if (implementation != nullptr) {
            delete implementation;
            implementation = nullptr;
        }
    }

    class WebKitImplementation : public Core::Thread, public Exchange::IBrowser, public PluginHost::IStateControl {
    public:
        class Config : public Core::JSON::Container {
        private:
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;

        public:
            class JavaScriptSettings : public Core::JSON::Container {
            private:
                JavaScriptSettings(const JavaScriptSettings&) = delete;
                JavaScriptSettings& operator=(const JavaScriptSettings&) = delete;

            public:
                JavaScriptSettings()
                    : Core::JSON::Container()
                    , UseLLInt(true)
                    , UseJIT(true)
                    , UseDFG(true)
                    , UseFTL(true)
                    , UseDOM(true)
                    , DumpOptions(_T("0"))
                {
                    Add(_T("useLLInt"), &UseLLInt);
                    Add(_T("useJIT"), &UseJIT);
                    Add(_T("useDFG"), &UseDFG);
                    Add(_T("useFTL"), &UseFTL);
                    Add(_T("useDOM"), &UseDOM);
                    Add(_T("dumpOptions"), &DumpOptions);
                }
                ~JavaScriptSettings()
                {
                }

            public:
                Core::JSON::Boolean UseLLInt;
                Core::JSON::Boolean UseJIT;
                Core::JSON::Boolean UseDFG;
                Core::JSON::Boolean UseFTL;
                Core::JSON::Boolean UseDOM;
                Core::JSON::String DumpOptions;
            };

        public:
            Config()
                : Core::JSON::Container()
                , UserAgent()
                , URL(_T("http://www.google.com"))
                , CookieStorage()
                , LocalStorage()
                , Secure(false)
                , Whitelist()
                , InjectedBundle()
                , Transparent(false)
                , Compositor()
                , Inspector()
                , FPS(false)
                , Cursor(false)
                , Touch(false)
                , MSEBuffers()
                , MemoryProfile()
                , MemoryPressure()
                , MediaDiskCache(true)
                , DiskCache()
                , XHRCache(false)
                , Languages()
                , CertificateCheck(true)
                , ClientIdentifier()
                , AllowWindowClose(false)
                , NonCompositedWebGLEnabled(false)
                , EnvironmentOverride(false)
                , Automation(false)
                , WebGLEnabled(true)
                , ThreadedPainting()
                , Width(1280)
                , Height(720)
                , PTSOffset(0)
                , ScaleFactor(1.0)
            {
                Add(_T("useragent"), &UserAgent);
                Add(_T("url"), &URL);
                Add(_T("cookiestorage"), &CookieStorage);
                Add(_T("localstorage"), &LocalStorage);
                Add(_T("secure"), &Secure);
                Add(_T("whitelist"), &Whitelist);
                Add(_T("injectedbundle"), &InjectedBundle);
                Add(_T("transparent"), &Transparent);
                Add(_T("compositor"), &Compositor);
                Add(_T("inspector"), &Inspector);
                Add(_T("fps"), &FPS);
                Add(_T("cursor"), &Cursor);
                Add(_T("touch"), &Touch);
                Add(_T("msebuffers"), &MSEBuffers);
                Add(_T("memoryprofile"), &MemoryProfile);
                Add(_T("memorypressure"), &MemoryPressure);
                Add(_T("mediadiskcache"), &MediaDiskCache);
                Add(_T("diskcache"), &DiskCache);
                Add(_T("xhrcache"), &XHRCache);
                Add(_T("languages"), &Languages);
                Add(_T("certificatecheck"), &CertificateCheck);
                Add(_T("javascript"), &JavaScript);
                Add(_T("clientidentifier"), &ClientIdentifier);
                Add(_T("windowclose"), &AllowWindowClose);
                Add(_T("noncompositedwebgl"), &NonCompositedWebGLEnabled);
                Add(_T("environmentoverride"), &EnvironmentOverride);
                Add(_T("automation"), &Automation);
                Add(_T("webgl"), &WebGLEnabled);
                Add(_T("threadedpainting"), &ThreadedPainting);
                Add(_T("width"), &Width);
                Add(_T("height"), &Height);
                Add(_T("ptsoffset"), &PTSOffset);
                Add(_T("scalefactor"), &ScaleFactor);
            }
            ~Config()
            {
            }

        public:
            Core::JSON::String UserAgent;
            Core::JSON::String URL;
            Core::JSON::String CookieStorage;
            Core::JSON::String LocalStorage;
            Core::JSON::Boolean Secure;
            Core::JSON::String Whitelist;
            Core::JSON::String InjectedBundle;
            Core::JSON::Boolean Transparent;
            Core::JSON::String Compositor;
            Core::JSON::String Inspector;
            Core::JSON::Boolean FPS;
            Core::JSON::Boolean Cursor;
            Core::JSON::Boolean Touch;
            Core::JSON::String MSEBuffers;
            Core::JSON::String MemoryProfile;
            Core::JSON::String MemoryPressure;
            Core::JSON::Boolean MediaDiskCache;
            Core::JSON::String DiskCache;
            Core::JSON::Boolean XHRCache;
            Core::JSON::ArrayType<Core::JSON::String> Languages;
            Core::JSON::Boolean CertificateCheck;
            JavaScriptSettings JavaScript;
            Core::JSON::String ClientIdentifier;
            Core::JSON::Boolean AllowWindowClose;
            Core::JSON::Boolean NonCompositedWebGLEnabled;
            Core::JSON::Boolean EnvironmentOverride;
            Core::JSON::Boolean Automation;
            Core::JSON::Boolean WebGLEnabled;
            Core::JSON::String ThreadedPainting;
            Core::JSON::DecUInt16 Width;
            Core::JSON::DecUInt16 Height;
            Core::JSON::DecSInt16 PTSOffset;
            Core::JSON::DecUInt16 ScaleFactor;
        };

    private:
        WebKitImplementation(const WebKitImplementation&) = delete;
        WebKitImplementation& operator=(const WebKitImplementation&) = delete;

    public:
        WebKitImplementation()
            : Core::Thread(0, _T("WebKitBrowser"))
            , _config()
            , _URL()
            , _dataPath()
            , _view()
            , _adminLock()
            , _fps(0)
            , _loop(nullptr)
            , _context(nullptr)
            , _state(PluginHost::IStateControl::UNINITIALIZED)
            , _hidden(false)
            , _time(0)
            , _compliant(false)
            , _guid(g_dbus_generate_guid())
        {

            // Register an @Exit, in case we are killed, with an incorrect ref count !!
            if (atexit(CloseDown) != 0) {
                TRACE_L1("Could not register @exit handler. Error: %d.", errno);
                exit(EXIT_FAILURE);
            }

            // The WebKitBrowser (WPE) can only be instantiated once (it is a process wide singleton !!!!)
            ASSERT(implementation == nullptr);

            implementation = this;
        }
        virtual ~WebKitImplementation()
        {
            Block();

            g_free(_guid);

            if (_loop != nullptr)
                g_main_loop_quit(_loop);

            if (Wait(Core::Thread::STOPPED | Core::Thread::BLOCKED, 6000) == false)
                TRACE_L1("Bailed out before the end of the WPE main app was reached. %d", 6000);

            implementation = nullptr;
        }

    public:
        virtual void SetURL(const string& URL)
        {
            _URL = URL;

            TRACE(Trace::Information, (_T("New URL: %s"), _URL.c_str()));

            if (_context != nullptr) {
                g_main_context_invoke(
                    _context,
                    [](gpointer customdata) -> gboolean {
                        WebKitImplementation* object = static_cast<WebKitImplementation*>(customdata);
                        webkit_web_view_load_uri(object->_view, object->_URL.c_str());
                        return FALSE;
                    },
                    this);
            }
        }
        virtual string GetURL() const
        {
            return _URL;
        }
        virtual uint32_t GetFPS() const
        {
            return _fps;
        }
        virtual PluginHost::IStateControl::state State() const
        {
            return (_state);
        }
        virtual uint32_t Request(PluginHost::IStateControl::command command)
        {
            uint32_t result = Core::ERROR_ILLEGAL_STATE;

            _adminLock.Lock();

            if (_state == PluginHost::IStateControl::UNINITIALIZED) {
                // Seems we are passing state changes before we reached an operational browser.
                // Just move the state to what we would like it to be :-)
                _state = (command == PluginHost::IStateControl::SUSPEND ? PluginHost::IStateControl::SUSPENDED : PluginHost::IStateControl::RESUMED);
                result = Core::ERROR_NONE;
            } else {
                switch (command) {
                case PluginHost::IStateControl::SUSPEND:
                    if (_state == PluginHost::IStateControl::RESUMED) {
                        Suspend();
                        result = Core::ERROR_NONE;
                    }
                    break;
                case PluginHost::IStateControl::RESUME:
                    if (_state == PluginHost::IStateControl::SUSPENDED) {
                        Resume();
                        result = Core::ERROR_NONE;
                    }
                    break;
                default:
                    break;
                }
            }

            _adminLock.Unlock();

            return (result);
        }
        virtual void Register(PluginHost::IStateControl::INotification* sink)
        {
            _adminLock.Lock();

            // Make sure a sink is not registered multiple times.
            ASSERT(std::find(_stateControlClients.begin(), _stateControlClients.end(), sink) == _stateControlClients.end());

            _stateControlClients.push_back(sink);
            sink->AddRef();

            _adminLock.Unlock();

            TRACE_L1("Registered a sink on the browser %p", sink);
        }
        virtual void Unregister(PluginHost::IStateControl::INotification* sink)
        {
            _adminLock.Lock();

            std::list<PluginHost::IStateControl::INotification*>::iterator index(std::find(_stateControlClients.begin(), _stateControlClients.end(), sink));

            // Make sure you do not unregister something you did not register !!!
            ASSERT(index != _stateControlClients.end());

            if (index != _stateControlClients.end()) {
                (*index)->Release();
                _stateControlClients.erase(index);
                TRACE_L1("Unregistered a sink on the browser %p", sink);
            }

            _adminLock.Unlock();
        }
        virtual void Hide(const bool hidden)
        {
            if (hidden == true) {
                Hide();
            } else {
                Show();
            }
        }
        virtual void Register(Exchange::IBrowser::INotification* sink)
        {
            _adminLock.Lock();

            // Make sure a sink is not registered multiple times.
            ASSERT(std::find(_notificationClients.begin(), _notificationClients.end(), sink) == _notificationClients.end());

            _notificationClients.push_back(sink);
            sink->AddRef();

            _adminLock.Unlock();

            TRACE_L1("Registered a sink on the browser %p", sink);
        }

        virtual void Unregister(Exchange::IBrowser::INotification* sink)
        {
            _adminLock.Lock();

            std::list<Exchange::IBrowser::INotification*>::iterator index(std::find(_notificationClients.begin(), _notificationClients.end(), sink));

            // Make sure you do not unregister something you did not register !!!
            ASSERT(index != _notificationClients.end());

            if (index != _notificationClients.end()) {
                (*index)->Release();
                _notificationClients.erase(index);
                TRACE_L1("Unregistered a sink on the browser %p", sink);
            }

            _adminLock.Unlock();
        }

        void OnURLChanged(const string& URL)
        {
            _adminLock.Lock();

            _URL = URL;

            std::list<Exchange::IBrowser::INotification*>::iterator index(_notificationClients.begin());

            while (index != _notificationClients.end()) {
                (*index)->URLChanged(URL);
                index++;
            }

            _adminLock.Unlock();
        }
        void OnLoadFinished()
        {
            _adminLock.Lock();

            string URL = Core::ToString(webkit_web_view_get_uri(_view));

            std::list<Exchange::IBrowser::INotification*>::iterator index(_notificationClients.begin());

            while (index != _notificationClients.end()) {
                (*index)->LoadFinished(URL);
                index++;
            }

            _adminLock.Unlock();
        }
        void OnStateChange(const PluginHost::IStateControl::state newState)
        {
            _adminLock.Lock();

            if (_state != newState) {
                _state = newState;

                std::list<PluginHost::IStateControl::INotification*>::iterator index(_stateControlClients.begin());

                while (index != _stateControlClients.end()) {
                    (*index)->StateChange(newState);
                    index++;
                }
            }

            _adminLock.Unlock();
        }
        void Hidden(const bool hidden)
        {
            _adminLock.Lock();

            if (hidden != _hidden) {
                _hidden = hidden;

                std::list<Exchange::IBrowser::INotification*>::iterator index(_notificationClients.begin());

                while (index != _notificationClients.end()) {
                    (*index)->Hidden(hidden);
                    index++;
                }
            }

            _adminLock.Unlock();
        }
        void OnJavaScript(const std::vector<string>& text) const
        {
            for (const string& line : text) {
                std::cout << "  " << line << std::endl;
            }
        }
        virtual uint32_t Configure(PluginHost::IShell* service)
        {
            _dataPath = service->DataPath();
            _config.FromString(service->ConfigLine());

            bool environmentOverride(WebKitBrowser::EnvironmentOverride(_config.EnvironmentOverride.Value()));

            if ((environmentOverride == false) || (Core::SystemInfo::GetEnvironment(_T("WPE_WEBKIT_URL"), _URL) == false)) {
                _URL = _config.URL.Value();
            }

            Core::SystemInfo::SetEnvironment(_T("QUEUEPLAYER_FLUSH_MODE"), _T("3"), false);
            Core::SystemInfo::SetEnvironment(_T("HOME"), service->PersistentPath());

            if (_config.ClientIdentifier.IsSet() == true) {
                string value(service->Callsign() + ',' + _config.ClientIdentifier.Value());
                Core::SystemInfo::SetEnvironment(_T("CLIENT_IDENTIFIER"), value, !environmentOverride);
            } else {
                Core::SystemInfo::SetEnvironment(_T("CLIENT_IDENTIFIER"), service->Callsign(), !environmentOverride);
            }

            // Set dummy window for gst-gl
            Core::SystemInfo::SetEnvironment(_T("GST_GL_WINDOW"), _T("dummy"), !environmentOverride);

            // MSE Buffers
            if (_config.MSEBuffers.Value().empty() == false)
                Core::SystemInfo::SetEnvironment(_T("MSE_MAX_BUFFER_SIZE"), _config.MSEBuffers.Value(), !environmentOverride);

            // Memory Pressure
            if (_config.MemoryPressure.Value().empty() == false)
                Core::SystemInfo::SetEnvironment(_T("WPE_POLL_MAX_MEMORY"), _config.MemoryPressure.Value(), !environmentOverride);

            // Memory Profile
            if (_config.MemoryProfile.Value().empty() == false)
                Core::SystemInfo::SetEnvironment(_T("WPE_RAM_SIZE"), _config.MemoryProfile.Value(), !environmentOverride);

            // GStreamer on-disk buffering
            if (_config.MediaDiskCache.Value() == false)
                Core::SystemInfo::SetEnvironment(_T("WPE_SHELL_DISABLE_MEDIA_DISK_CACHE"), _T("1"), !environmentOverride);
            else
                Core::SystemInfo::SetEnvironment(_T("WPE_SHELL_MEDIA_DISK_CACHE_PATH"), service->PersistentPath(), !environmentOverride);

            // Disk Cache
            if (_config.DiskCache.Value().empty() == false)
                Core::SystemInfo::SetEnvironment(_T("WPE_DISK_CACHE_SIZE"), _config.DiskCache.Value(), !environmentOverride);

            if (_config.XHRCache.Value() == false)
                Core::SystemInfo::SetEnvironment(_T("WPE_DISABLE_XHR_RESPONSE_CACHING"), _T("1"), !environmentOverride);

            // Enable cookie persistent storage
            if (_config.CookieStorage.Value().empty() == false)
                Core::SystemInfo::SetEnvironment(_T("WPE_SHELL_COOKIE_STORAGE"), _T("1"), !environmentOverride);

            // Use cairo noaa compositor
            if (_config.Compositor.Value().empty() == false)
                Core::SystemInfo::SetEnvironment(_T("CAIRO_GL_COMPOSITOR"), _config.Compositor.Value(), !environmentOverride);

            // WebInspector
            if (_config.Inspector.Value().empty() == false) {
                if (_config.Automation.Value())
                    Core::SystemInfo::SetEnvironment(_T("WEBKIT_INSPECTOR_SERVER"), _config.Inspector.Value(), !environmentOverride);
                else
                    Core::SystemInfo::SetEnvironment(_T("WEBKIT_LEGACY_INSPECTOR_SERVER"), _config.Inspector.Value(), !environmentOverride);
            }

            // RPI mouse support
            if (_config.Cursor.Value() == true)
                Core::SystemInfo::SetEnvironment(_T("WPE_BCMRPI_CURSOR"), _T("1"), !environmentOverride);

            // RPI touch support
            if (_config.Touch.Value() == true)
                Core::SystemInfo::SetEnvironment(_T("WPE_BCMRPI_TOUCH"), _T("1"), !environmentOverride);

            // WPE allows the LLINT to be used if true
            if (_config.JavaScript.UseLLInt.Value() == false) {
                Core::SystemInfo::SetEnvironment(_T("JSC_useLLInt"), _T("false"), !environmentOverride);
            }

            // WPE allows the baseline JIT to be used if true
            if (_config.JavaScript.UseJIT.Value() == false) {
                Core::SystemInfo::SetEnvironment(_T("JSC_useJIT"), _T("false"), !environmentOverride);
            }

            // WPE allows the DFG JIT to be used if true
            if (_config.JavaScript.UseDFG.Value() == false) {
                Core::SystemInfo::SetEnvironment(_T("JSC_useDFGJIT"), _T("false"), !environmentOverride);
            }

            // WPE allows the FTL JIT to be used if true
            if (_config.JavaScript.UseFTL.Value() == false) {
                Core::SystemInfo::SetEnvironment(_T("JSC_useFTLJIT"), _T("false"), !environmentOverride);
            }

            // WPE allows the DOM JIT to be used if true
            if (_config.JavaScript.UseDOM.Value() == false) {
                Core::SystemInfo::SetEnvironment(_T("JSC_useDOMJIT"), _T("false"), !environmentOverride);
            }

            // WPE DumpOptions
            if (_config.JavaScript.DumpOptions.Value().empty() == false) {
                Core::SystemInfo::SetEnvironment(_T("JSC_dumpOptions"), _config.JavaScript.DumpOptions.Value(), !environmentOverride);
            }

            // ThreadedPainting
            if (_config.ThreadedPainting.Value().empty() == false) {
                Core::SystemInfo::SetEnvironment(_T("WEBKIT_NICOSIA_PAINTING_THREADS"), _config.ThreadedPainting.Value(), !environmentOverride);
            }

            // PTSOffset
            if (_config.PTSOffset.IsSet() == true) {
                string ptsoffset(Core::NumberType<int16_t>(_config.PTSOffset.Value()).Text());
                Core::SystemInfo::SetEnvironment(_T("PTS_REPORTING_OFFSET_MS"), ptsoffset, !environmentOverride);
            }

            string width(Core::NumberType<uint16_t>(_config.Width.Value()).Text());
            string height(Core::NumberType<uint16_t>(_config.Height.Value()).Text());
            Core::SystemInfo::SetEnvironment(_T("WEBKIT_RESOLUTION_WIDTH"), width, !environmentOverride);
            Core::SystemInfo::SetEnvironment(_T("WEBKIT_RESOLUTION_HEIGHT"), height, !environmentOverride);

            if (width.empty() == false) {
                Core::SystemInfo::SetEnvironment(_T("GST_VIRTUAL_DISP_WIDTH"), width, !environmentOverride);
            }

            if (height.empty() == false) {
                Core::SystemInfo::SetEnvironment(_T("GST_VIRTUAL_DISP_HEIGHT"), height, !environmentOverride);
            }

            // Oke, so we are good to go.. Release....
            Core::Thread::Run();

            return (Core::ERROR_NONE);
        }

        void NotifyClosure()
        {
            _adminLock.Lock();

            std::list<Exchange::IBrowser::INotification*>::iterator index(_notificationClients.begin());

            while (index != _notificationClients.end()) {
                (*index)->Closure();
                index++;
            }

            _adminLock.Unlock();
        }

        void SetFPS(const uint32_t fps)
        {
            _fps = fps;
        }

        BEGIN_INTERFACE_MAP(WebKitImplementation)
        INTERFACE_ENTRY(Exchange::IBrowser)
        INTERFACE_ENTRY(PluginHost::IStateControl)
        END_INTERFACE_MAP

    private:
        void Hide()
        {
            if (_context != nullptr) {
                _time = Core::Time::Now().Ticks();
                g_main_context_invoke(
                    _context,
                    [](gpointer customdata) -> gboolean {
                        WebKitImplementation* object = static_cast<WebKitImplementation*>(customdata);

                        auto* backend = webkit_web_view_backend_get_wpe_backend(webkit_web_view_get_backend(object->_view));
                        wpe_view_backend_remove_activity_state(backend, wpe_view_activity_state_visible | wpe_view_activity_state_focused);

                        object->Hidden(true);

                        TRACE_L1("Internal Hide Notification took %d mS.", static_cast<uint32_t>(Core::Time::Now().Ticks() - object->_time));

                        return FALSE;
                    },
                    this);
            }
        }
        void Show()
        {
            if (_context != nullptr) {
                _time = Core::Time::Now().Ticks();
                g_main_context_invoke(
                    _context,
                    [](gpointer customdata) -> gboolean {
                        WebKitImplementation* object = static_cast<WebKitImplementation*>(customdata);

                        auto* backend = webkit_web_view_backend_get_wpe_backend(webkit_web_view_get_backend(object->_view));
                        wpe_view_backend_add_activity_state(backend, wpe_view_activity_state_visible | wpe_view_activity_state_focused);

                        object->Hidden(false);

                        TRACE_L1("Internal Show Notification took %d mS.", static_cast<uint32_t>(Core::Time::Now().Ticks() - object->_time));

                        return FALSE;
                    },
                    this);
            }
        }
        void Suspend()
        {
            if (_context == nullptr) {
                _state = PluginHost::IStateControl::SUSPENDED;
            } else {
                _time = Core::Time::Now().Ticks();
                g_main_context_invoke(
                    _context,
                    [](gpointer customdata) -> gboolean {
                        WebKitImplementation* object = static_cast<WebKitImplementation*>(customdata);

                        auto* backend = webkit_web_view_backend_get_wpe_backend(webkit_web_view_get_backend(object->_view));
                        wpe_view_backend_remove_activity_state(backend, wpe_view_activity_state_in_window | wpe_view_activity_state_focused);

                        object->OnStateChange(PluginHost::IStateControl::SUSPENDED);

                        TRACE_L1("Internal Suspend Notification took %d mS.", static_cast<uint32_t>(Core::Time::Now().Ticks() - object->_time));

                        return FALSE;
                    },
                    this);
            }
        }
        void Resume()
        {
            if (_context == nullptr) {
                _state = PluginHost::IStateControl::RESUMED;
            } else {
                _time = Core::Time::Now().Ticks();

                g_main_context_invoke(
                    _context,
                    [](gpointer customdata) -> gboolean {
                        WebKitImplementation* object = static_cast<WebKitImplementation*>(customdata);

                        auto* backend = webkit_web_view_backend_get_wpe_backend(webkit_web_view_get_backend(object->_view));
                        uint32_t state = wpe_view_activity_state_in_window;
                        if (!object->_hidden)
                            state |= wpe_view_activity_state_focused;
                        wpe_view_backend_add_activity_state(backend, state);

                        object->OnStateChange(PluginHost::IStateControl::RESUMED);

                        TRACE_L1("Internal Resume Notification took %d mS.", static_cast<uint32_t>(Core::Time::Now().Ticks() - object->_time));

                        return FALSE;
                    },
                    this);
            }
        }

        static void initializeWebExtensionsCallback(WebKitWebContext* context, WebKitImplementation* browser)
        {
            webkit_web_context_set_web_extensions_directory(context, browser->_dataPath.c_str());
            GVariant* data = g_variant_new("(sms)", browser->_guid, !browser->_config.Whitelist.Value().empty() ? browser->_config.Whitelist.Value().c_str() : nullptr);
            webkit_web_context_set_web_extensions_initialization_user_data(context, data);
        }

        static gboolean geolocationManagerStartCallback(WebKitGeolocationManager* manager)
        {
            std::cerr << "in WebKitGeolocationManager::start" << std::endl;
            auto* position = webkit_geolocation_position_new(51.49, 4.40, 1.0);
            webkit_geolocation_manager_update_position(manager, position);
            webkit_geolocation_position_free(position);
            return TRUE;
        }

        static void wpeNotifyWPEFrameworkMessageReceivedCallback(WebKitUserContentManager*, WebKitJavascriptResult* message, WebKitImplementation* browser)
        {
            JSCValue* args = webkit_javascript_result_get_js_value(message);
            JSCValue* arrayLengthValue = jsc_value_object_get_property(args, "length");
            int arrayLength = jsc_value_to_int32(arrayLengthValue);
            g_object_unref(arrayLengthValue);

            std::vector<string> messageStrings;
            for (int i = 0; i < arrayLength; ++i) {
                JSCValue* itemValue = jsc_value_object_get_property_at_index(args, i);
                char* itemStr = jsc_value_to_string(itemValue);
                g_object_unref(itemValue);
                messageStrings.push_back(Core::ToString(itemStr));
                g_free(itemStr);
            }

            browser->OnJavaScript(messageStrings);
        }

        static gboolean decidePolicyCallback(WebKitWebView*, WebKitPolicyDecision* decision, WebKitPolicyDecisionType)
        {
            webkit_policy_decision_use(decision);
            return TRUE;
        }

        static void uriChangedCallback(WebKitWebView* webView, GParamSpec*, WebKitImplementation* browser)
        {
            browser->OnURLChanged(Core::ToString(webkit_web_view_get_uri(webView)));
        }

        static void loadChangedCallback(WebKitWebView* webView, WebKitLoadEvent loadEvent, WebKitImplementation* browser)
        {
            if (loadEvent == WEBKIT_LOAD_FINISHED)
                browser->OnLoadFinished();
        }

        static void webProcessTerminatedCallback(WebKitWebView* webView, WebKitWebProcessTerminationReason reason)
        {
            switch (reason) {
            case WEBKIT_WEB_PROCESS_CRASHED:
                SYSLOG(Trace::Fatal, ("CRASH: WebProcess crashed: exiting ..."));
                break;
            case WEBKIT_WEB_PROCESS_EXCEEDED_MEMORY_LIMIT:
                SYSLOG(Trace::Fatal, ("CRASH: WebProcess terminated due to memory limit: exiting ..."));
                break;
            }
            exit(1);
        }

        static void closeCallback(WebKitWebView*, WebKitImplementation* browser)
        {
            browser->NotifyClosure();
        }

        static gboolean decidePermissionCallback(WebKitWebView*, WebKitPermissionRequest* permissionRequest)
        {
            webkit_permission_request_allow(permissionRequest);
            return TRUE;
        }

        static gboolean showNotificationCallback(WebKitWebView*, WebKitNotification* notification, WebKitImplementation* browser)
        {
            TRACE_GLOBAL(HTML5Notification, (_T("%s - %s"), webkit_notification_get_title(notification), webkit_notification_get_body(notification)));
            return TRUE;
        }

        static WebKitWebView* createWebViewForAutomationCallback(WebKitAutomationSession* session, WebKitImplementation* browser)
        {
            return browser->_view;
        }

        static void automationStartedCallback(WebKitWebContext* context, WebKitAutomationSession* session, WebKitImplementation* browser)
        {
            WebKitApplicationInfo *info = webkit_application_info_new();
            webkit_application_info_set_name(info, "WPEWebKitBrowser");
            webkit_application_info_set_version(info, 1, 0, 0);
            webkit_automation_session_set_application_info(session, info);
            webkit_application_info_unref(info);

            g_signal_connect(session, "create-web-view", reinterpret_cast<GCallback>(createWebViewForAutomationCallback), browser);
        }

        virtual uint32_t Worker()
        {
            _context = g_main_context_new();
            _loop = g_main_loop_new(_context, FALSE);
            g_main_context_push_thread_default(_context);

            bool automationEnabled = _config.Automation.Value();

            WebKitWebContext* context;
            if (automationEnabled) {
                context = webkit_web_context_new_ephemeral();
                webkit_web_context_set_automation_allowed(context, TRUE);
                g_signal_connect(context, "automation-started", reinterpret_cast<GCallback>(automationStartedCallback), this);
            } else {
                gchar* wpeStoragePath;
                if (_config.LocalStorage.IsSet() == true && _config.LocalStorage.Value().empty() == false)
                    wpeStoragePath = g_build_filename(_config.LocalStorage.Value().c_str(), "wpe", "local-storage", nullptr);
                else
                    wpeStoragePath = g_build_filename(g_get_user_cache_dir(), "wpe", "local-storage", nullptr);
                g_mkdir_with_parents(wpeStoragePath, 0700);

                gchar* wpeDiskCachePath = g_build_filename(g_get_user_cache_dir(), "wpe", "disk-cache", nullptr);
                g_mkdir_with_parents(wpeDiskCachePath, 0700);

                auto* websiteDataManager = webkit_website_data_manager_new("local-storage-directory", wpeStoragePath, "disk-cache-directory", wpeDiskCachePath, nullptr);
                g_free(wpeStoragePath);
                g_free(wpeDiskCachePath);

                context = webkit_web_context_new_with_website_data_manager(websiteDataManager);
                g_object_unref(websiteDataManager);
            }

            if (_config.InjectedBundle.Value().empty() == false) {
                // Set up injected bundle. Will be loaded once WPEWebProcess is started.
                g_signal_connect(context, "initialize-web-extensions", G_CALLBACK(initializeWebExtensionsCallback), this);
            }

            if (!webkit_web_context_is_ephemeral(context)) {
                gchar* cookieDatabasePath;
                if (_config.CookieStorage.IsSet() == true && _config.CookieStorage.Value().empty() == false)
                    cookieDatabasePath = g_build_filename(_config.CookieStorage.Value().c_str(), "cookies.db", nullptr);
                else
                    cookieDatabasePath = g_build_filename(g_get_user_cache_dir(), "cookies.db", nullptr);

                auto* cookieManager = webkit_web_context_get_cookie_manager(context);
                webkit_cookie_manager_set_persistent_storage(cookieManager, cookieDatabasePath, WEBKIT_COOKIE_PERSISTENT_STORAGE_SQLITE);
            }

            if (!_config.CertificateCheck)
                webkit_web_context_set_tls_errors_policy(context, WEBKIT_TLS_ERRORS_POLICY_IGNORE);

            auto* languages = static_cast<char**>(g_new0(char*, _config.Languages.Length() + 1));
            Core::JSON::ArrayType<Core::JSON::String>::Iterator index(_config.Languages.Elements());

            for (unsigned i = 0; index.Next(); ++i)
                languages[i] = g_strdup(index.Current().Value().c_str());

            webkit_web_context_set_preferred_languages(context, languages);
            g_strfreev(languages);

            auto* geolocationManager = webkit_web_context_get_geolocation_manager(context);
            g_signal_connect(geolocationManager, "start", reinterpret_cast<GCallback>(geolocationManagerStartCallback), nullptr);

            gboolean secure = _config.Secure.Value();
            auto* preferences = webkit_settings_new_with_settings(
                "enable-web-security", secure,
                "allow-running-of-insecure-content", !secure,
                "allow-display-of-insecure-content", !secure,
                "javascript-can-close-window", _config.AllowWindowClose.Value(),
                "enable-webgl", _config.WebGLEnabled.Value(),
                "enable-write-console-messages-to-stdout", TRUE,
                nullptr);

            if (_config.UserAgent.IsSet() == true && _config.UserAgent.Value().empty() == false)
                webkit_settings_set_user_agent(preferences, _config.UserAgent.Value().c_str());

            _view = WEBKIT_WEB_VIEW(g_object_new(WEBKIT_TYPE_WEB_VIEW,
                "backend", webkit_web_view_backend_new(wpe_view_backend_create(), nullptr, nullptr),
                "web-context", context,
                "settings", preferences,
                "is-controlled-by-automation", automationEnabled,
                nullptr));
            g_object_unref(context);
            g_object_unref(preferences);

            if (_config.Transparent.Value() == true) {
                WebKitColor transparent = { 0, 0, 0, 0 };
                webkit_web_view_set_background_color(_view, &transparent);
            }

            webkit_web_view_set_custom_device_scale_factor(_view, _config.ScaleFactor.Value());

            unsigned frameDisplayedCallbackID = 0;
            if (_config.FPS.Value() == true) {
                frameDisplayedCallbackID = webkit_web_view_add_frame_displayed_callback(_view, [](WebKitWebView*, gpointer userData) {
                    static unsigned s_frameCount = 0;
                    static gint64 lastDumpTime = g_get_monotonic_time();

                    ++s_frameCount;
                    gint64 time = g_get_monotonic_time();
                    if (time - lastDumpTime >= G_USEC_PER_SEC) {
                        auto* browser = static_cast<WebKitImplementation*>(userData);
                        browser->SetFPS(s_frameCount * G_USEC_PER_SEC * 1.0 / (time - lastDumpTime));
                        s_frameCount = 0;
                        lastDumpTime = time;
                    }
                }, this, nullptr);
            }

            auto* userContentManager = webkit_web_view_get_user_content_manager(_view);
            webkit_user_content_manager_register_script_message_handler_in_world(userContentManager, "wpeNotifyWPEFramework", _guid);
            g_signal_connect(userContentManager, "script-message-received::wpeNotifyWPEFramework",
                reinterpret_cast<GCallback>(wpeNotifyWPEFrameworkMessageReceivedCallback), this);

            g_signal_connect(_view, "decide-policy", reinterpret_cast<GCallback>(decidePolicyCallback), nullptr);
            g_signal_connect(_view, "notify::uri", reinterpret_cast<GCallback>(uriChangedCallback), this);
            g_signal_connect(_view, "load-changed", reinterpret_cast<GCallback>(loadChangedCallback), this);
            g_signal_connect(_view, "web-process-terminated", reinterpret_cast<GCallback>(webProcessTerminatedCallback), nullptr);
            g_signal_connect(_view, "close", reinterpret_cast<GCallback>(closeCallback), this);
            g_signal_connect(_view, "permission-request", reinterpret_cast<GCallback>(decidePermissionCallback), nullptr);
            g_signal_connect(_view, "show-notification", reinterpret_cast<GCallback>(showNotificationCallback), this);

            SetURL(_URL);

            // Move into the correct state, as requested
            auto* backend = webkit_web_view_backend_get_wpe_backend(webkit_web_view_get_backend(_view));
            _adminLock.Lock();
            if ((_state == PluginHost::IStateControl::SUSPENDED) || (_state == PluginHost::IStateControl::UNINITIALIZED)) {
                _state = PluginHost::IStateControl::UNINITIALIZED;
                wpe_view_backend_add_activity_state(backend, wpe_view_activity_state_visible);
                OnStateChange(PluginHost::IStateControl::SUSPENDED);
            } else {
                _state = PluginHost::IStateControl::UNINITIALIZED;
                wpe_view_backend_add_activity_state(backend, wpe_view_activity_state_visible | wpe_view_activity_state_focused | wpe_view_activity_state_in_window);
                OnStateChange(PluginHost::IStateControl::RESUMED);
            }
            _adminLock.Unlock();

            g_main_loop_run(_loop);

            if (frameDisplayedCallbackID)
                webkit_web_view_remove_frame_displayed_callback(_view, frameDisplayedCallbackID);
            webkit_user_content_manager_unregister_script_message_handler_in_world(userContentManager, "wpeNotifyWPEFramework", _guid);

            g_clear_object(&_view);
            g_main_context_pop_thread_default(_context);
            g_main_loop_unref(_loop);
            g_main_context_unref(_context);

            return Core::infinite;
        }

    private:
        Config _config;
        string _URL;
        string _dataPath;

        WebKitWebView* _view;
        Core::CriticalSection _adminLock;
        uint32_t _fps;
        GMainLoop* _loop;
        GMainContext* _context;
        std::list<Exchange::IBrowser::INotification*> _notificationClients;
        std::list<PluginHost::IStateControl::INotification*> _stateControlClients;
        PluginHost::IStateControl::state _state;
        bool _hidden;
        uint64_t _time;
        bool _compliant;
        char* _guid;
    };

    SERVICE_REGISTRATION(WebKitImplementation, 1, 0);

} // namespace Plugin

namespace WebKitBrowser {

    // TODO: maybe nice to expose this in the config.json
    static const TCHAR* mandatoryProcesses[] = {
        _T("WPENetworkProcess"),
        _T("WPEWebProcess")
    };

    class MemoryObserverImpl : public Exchange::IMemory {
    private:
        MemoryObserverImpl();
        MemoryObserverImpl(const MemoryObserverImpl&);
        MemoryObserverImpl& operator=(const MemoryObserverImpl&);

        enum { TYPICAL_STARTUP_TIME = 10 }; /* in Seconds */
    public:
        MemoryObserverImpl(const uint32_t id)
            : _main(id == 0 ? Core::ProcessInfo().Id() : id)
            , _children(_main.Id())
            , _startTime(Core::Time::Now().Ticks() + (TYPICAL_STARTUP_TIME * 1000000))
        { // IsOperation true till calculated time (microseconds)
        }
        ~MemoryObserverImpl()
        {
        }

    public:
        virtual void Observe(const uint32_t pid)
        {
            if (pid != 0) {
                _main = Core::ProcessInfo(pid);
                _startTime = Core::Time::Now().Ticks() + (TYPICAL_STARTUP_TIME * 1000000);
            } else {
                _startTime = 0;
            }
        }

        virtual uint64_t Resident() const
        {
            uint32_t result(0);

            if (_startTime != 0) {
                if (_children.Count() < 2) {
                    _children = Core::ProcessInfo::Iterator(_main.Id());
                }

                result = _main.Resident();

                _children.Reset();

                while (_children.Next() == true) {
                    result += _children.Current().Resident();
                }
            }

            return (result);
        }
        virtual uint64_t Allocated() const
        {
            uint32_t result(0);

            if (_startTime != 0) {
                if (_children.Count() < 2) {
                    _children = Core::ProcessInfo::Iterator(_main.Id());
                }

                result = _main.Allocated();

                _children.Reset();

                while (_children.Next() == true) {
                    result += _children.Current().Allocated();
                }
            }

            return (result);
        }
        virtual uint64_t Shared() const
        {
            uint32_t result(0);

            if (_startTime != 0) {
                if (_children.Count() < 2) {
                    _children = Core::ProcessInfo::Iterator(_main.Id());
                }

                result = _main.Shared();

                _children.Reset();

                while (_children.Next() == true) {
                    result += _children.Current().Shared();
                }
            }

            return (result);
        }
        virtual uint8_t Processes() const
        {
            // Refresh the children list !!!
            _children = Core::ProcessInfo::Iterator(_main.Id());
            return ((_startTime == 0) || (_main.IsActive() == true) ? 1 : 0) + _children.Count();
        }
        virtual const bool IsOperational() const
        {
            uint32_t requiredProcesses = 0;

            if (_startTime != 0) {
                const uint8_t requiredChildren = (sizeof(mandatoryProcesses) / sizeof(mandatoryProcesses[0]));

                //!< We can monitor a max of 32 processes, every mandatory process represents a bit in the requiredProcesses.
                // In the end we check if all bits are 0, what means all mandatory processes are still running.
                requiredProcesses = (0xFFFFFFFF >> (32 - requiredChildren));

                //!< If there are less children than in the the mandatoryProcesses struct, we are done and return false.
                if (_children.Count() >= requiredChildren) {

                    _children.Reset();

                    //!< loop over all child processes as long as we are operational.
                    while ((requiredProcesses != 0) && (true == _children.Next())) {

                        uint8_t count(0);
                        string name(_children.Current().Name());

                        while ((count < requiredChildren) && (name != mandatoryProcesses[count])) {
                            ++count;
                        }

                        //<! this is a mandatory process and if its still active reset its bit in requiredProcesses.
                        //   If not we are not completely operational.
                        if ((count < requiredChildren) && (_children.Current().IsActive() == true)) {
                            requiredProcesses &= (~(1 << count));
                        }
                    }
                }
            }

            // TRACE_L1("requiredProcess = %X, IsStarting = %s, main.IsActive = %s", requiredProcesses, IsStarting() ? _T("true") : _T("false"), _main.IsActive() ? _T("true") : _T("false"));
            return (((requiredProcesses == 0) || (true == IsStarting())) && (true == _main.IsActive()));
        }

        BEGIN_INTERFACE_MAP(MemoryObserverImpl)
        INTERFACE_ENTRY(Exchange::IMemory)
        END_INTERFACE_MAP

    private:
        inline const bool IsStarting() const
        {
            return (_startTime == 0) || (Core::Time::Now().Ticks() < _startTime);
        }

    private:
        Core::ProcessInfo _main;
        mutable Core::ProcessInfo::Iterator _children;
        uint64_t _startTime; // !< Reference for monitor
    };

    Exchange::IMemory* MemoryObserver(const uint32_t PID)
    {
        return (Core::Service<MemoryObserverImpl>::Create<Exchange::IMemory>(PID));
    }
}
} // namespace WebKitBrowser
