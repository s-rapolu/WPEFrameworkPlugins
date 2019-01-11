#include "CrashDummy.h"

namespace WPEFramework {
namespace Plugin {

SERVICE_REGISTRATION(CrashDummy, 1, 0);

static Core::ProxyPoolType<Web::JSONBodyType<CrashDummy::CrashParameters>> crashParametersResFactory(1);
static Core::ProxyPoolType<Web::JSONBodyType<JSONObject::Methods>> methodsResFactory(2);
static Core::ProxyPoolType<Web::JSONBodyType<JSONObject::MethodDescription>> methodDescriptionResFactory(2);
static Core::ProxyPoolType<Web::JSONBodyType<JSONObject::Parameters>> parametersResFactory(2);
//
// IPlugin overrides
//
/* virtual */ const string CrashDummy::Initialize(PluginHost::IShell* shell)
{
    TRACE(Trace::Information, ("Initializing CrashDummy..."));

    string message;
    _pid = 0;

    ASSERT(_shell == nullptr);
    _shell = shell;
    _skipURL = static_cast<uint8_t>(_shell->WebPrefix().length());
    // Register the Process::Notification stuff. The Remote process might die before we get a
    // change to "register" the sink for these events !!! So do it ahead of instantiation.
    _shell->Register(&_notification);

    ASSERT(_implementation == nullptr);
    _implementation = _shell->Root<Exchange::ICrashDummy>(_pid, ImplWaitTime, _T("CrashDummyImplementation"));
    if (_implementation == nullptr) {
        message = _T("CrashDummy could not be instantiated.");
        _shell->Unregister(&_notification);
        _shell = nullptr;
    } else {
        _implementation->Configure(_shell);
        _implementation->ExecPendingCrash();
        TRACE(Trace::Information, ("CrashDummy Plugin initialized %p", _implementation));
    }

    return (message);
}

/* virtual */ void CrashDummy::Deinitialize(PluginHost::IShell* shell)
{
    TRACE(Trace::Information, ("Deinitializing CrashDummy..."));

    ASSERT(_shell == shell);
    ASSERT(_implementation != nullptr);

    _shell->Unregister(&_notification);

    uint32_t releaseStatus = _implementation->Release();

    if (releaseStatus != Core::ERROR_DESTRUCTION_SUCCEEDED) {
        ASSERT(_pid != 0);

        TRACE(Trace::Fatal, ("CrashDummy plugin (%d) is not properly destructed (status = %d)", _pid, releaseStatus));
        RPC::IRemoteProcess* process(_shell->RemoteProcess(_pid));

        // The process can disappear in the meantime...
        if (process != nullptr) {
            // But if it did not dissapear in the meantime, forcefully terminate it. Shoot to kill :-)
            process->Terminate();
            process->Release();
        }
    }

    // Deinitialize what we initialized..
    _shell = nullptr;
    _implementation = nullptr;

    return;
}

/* virtual */ string CrashDummy::Information() const
{
    return (_T(""));
}

//
// IWeb overrides
//
/* virtual */ void CrashDummy::Inbound(Web::Request& request)
{
    Core::TextSegmentIterator index(
        Core::TextFragment(request.Path, _skipURL, request.Path.length() - _skipURL), false, '/');

    if ((request.Verb == Web::Request::HTTP_PUT) || (request.Verb == Web::Request::HTTP_POST)) {
        if (index.Next() && index.Next()) {
            if (index.Current().Text() == _T("CrashNTimes")) {
                request.Body(crashParametersResFactory.Element());
            }
        }
    }

    return;
}

/* virtual */ Core::ProxyType<Web::Response> CrashDummy::Process(const Web::Request& request)
{
    TRACE(Trace::Information, ("Processing request...", __FUNCTION__));

    // Proxy object for response type.
    Core::ProxyType<Web::Response> response(PluginHost::Factories::Instance().Response());
    // Default
    response->Message = _T("Method not allowed");
    response->ErrorCode = Web::STATUS_METHOD_NOT_ALLOWED;

    Core::TextSegmentIterator index(
        Core::TextFragment(request.Path, _skipURL, request.Path.length() - _skipURL), false, '/');

    if ((request.Verb == Web::Request::HTTP_PUT) || (request.Verb == Web::Request::HTTP_POST)) {
        // PUT/POST - /CrashDummy/Crash
        if (index.Next() && index.Next()) {
            if (index.Current().Text() == _T("Crash")) {
                response->Message = _T("Got Crash request");
                response->ErrorCode = Web::STATUS_OK;
                ASSERT(_implementation != nullptr)
                _implementation->Crash();
            }
        }
    }

    // HARDCODED DATA
    std::list<string> methodNames;
    methodNames.push_back(_T("Crash"));
    methodNames.push_back(_T("CrashNTimes"));

    if (request.Verb == Web::Request::HTTP_GET) {
        if (index.Next() && index.Next()) {
            // GET - /CrashDummy/Methods
            if (index.Current().Text() == _T("Methods")) {
                if (!index.Next()) {
                    // GET - /CrashDummy/Methods
                    Core::ProxyType<Web::JSONBodyType<JSONObject::Methods>> body(methodsResFactory.Element());
                    // MOVE TO SOME METHOD WHEN READY
                    std::list<string>::const_iterator index(methodNames.begin());
                    while (index != methodNames.end()) {
                        Core::JSON::String newElement;
                        newElement = *index;
                        body->MethodNames.Add(newElement);
                        index++;
                    }

                    response->ContentType = Web::MIMETypes::MIME_JSON;
                    response->Body(Core::proxy_cast<Web::IBody>(body));
                    response->Message = _T("Got Methods request!");
                    response->ErrorCode = Web::STATUS_OK;
                } else {
                    string methodName = index.Current().Text();
                    auto it = std::find(methodNames.begin(), methodNames.end(), methodName);

                    if (it != methodNames.end()) {
                        if (index.Next()) {
                            if (index.Current().Text() == _T("Description")) {
                                // GET - /CrashDummy/Methods/<METHOD_NAME>/Description
                                Core::ProxyType<Web::JSONBodyType<JSONObject::MethodDescription>> body(
                                    methodDescriptionResFactory.Element());

                                // MOVE TO SOME METHOD WHEN READY
                                Core::JSON::String description;
                                description = _T("Some description");
                                body->Description = description;

                                response->ContentType = Web::MIMETypes::MIME_JSON;
                                response->Body(Core::proxy_cast<Web::IBody>(body));
                                response->Message = _T("Got Description request!");
                                response->ErrorCode = Web::STATUS_OK;
                            } else if (index.Current().Text() == _T("Parameters")) {
                                // GET - /CrashDummy/Methods/<METHOD_NAME>/Parameters
                                Core::ProxyType<Web::JSONBodyType<JSONObject::Parameters>> body(
                                    parametersResFactory.Element());

                                // input
                                Core::JSON::ArrayType<JSONObject::Parameters::Parameter> input;
                                JSONObject::Parameters::Parameter newElement;
                                newElement.Type = _T("IN/type1");
                                newElement.Name = _T("IN/name1");
                                newElement.Comment = _T("IN/comment1");
                                body->Input.Add(newElement);

                                newElement.Type = _T("IN/type2");
                                newElement.Name = _T("IN/name2");
                                newElement.Comment = _T("IN/comment2");
                                body->Input.Add(newElement);

                                // output
                                Core::JSON::ArrayType<JSONObject::Parameters::Parameter> output;
                                newElement.Type = _T("OUT/type1");
                                newElement.Name = _T("OUT/name1");
                                newElement.Comment = _T("OUT/comment1");
                                body->Output.Add(newElement);

                                newElement.Type = _T("OUT/type2");
                                newElement.Name = _T("OUT/name2");
                                newElement.Comment = _T("OUT/comment2");
                                body->Output.Add(newElement);

                                response->ContentType = Web::MIMETypes::MIME_JSON;
                                response->Body(Core::proxy_cast<Web::IBody>(body));
                                response->Message = _T("Got Prameters request!");
                                response->ErrorCode = Web::STATUS_OK;
                            } else {
                                response->Message = _T("Got Prameters request!");
                                response->ErrorCode = Web::STATUS_FORBIDDEN;
                            }
                        }
                    }
                }
            }
        }
    }

    return (response);
}

//
// CrashDummy
//
void CrashDummy::Activated()
{
    return;
}

void CrashDummy::Deactivated(RPC::IRemoteProcess* process)
{
    if (process->Id() == _pid) {
        ASSERT(_shell != nullptr);

        PluginHost::WorkerPool::Instance().Submit(
            PluginHost::IShell::Job::Create(_shell, PluginHost::IShell::DEACTIVATED, PluginHost::IShell::FAILURE));
    }

    return;
}

} // namespace Plugin
} // namespace WPEFramework
