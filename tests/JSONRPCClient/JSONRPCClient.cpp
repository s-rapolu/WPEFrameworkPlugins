#define MODULE_NAME JSONRPC_Test

#include <core/core.h>
#include <jsonrpc/jsonrpc.h>
#include <interfaces/IPerformance.h>

#include "../JSONRPCPlugin/Data.h"

using namespace WPEFramework;

namespace WPEFramework {

ENUM_CONVERSION_BEGIN(::JsonValue::type)

    { JsonValue::type::EMPTY, _TXT("empty") },
    { JsonValue::type::BOOLEAN, _TXT("boolean") },
    { JsonValue::type::NUMBER, _TXT("number") },
    { JsonValue::type::STRING, _TXT("string") },

ENUM_CONVERSION_END(::JsonValue::type)

ENUM_CONVERSION_BEGIN(Data::Response::state)

    { Data::Response::ACTIVE, _TXT("Activate") },
    { Data::Response::INACTIVE, _TXT("Inactivate") },
    { Data::Response::IDLE, _TXT("Idle") },
    { Data::Response::FAILURE, _TXT("Failure") },

ENUM_CONVERSION_END(Data::Response::state)

}

bool ParseOptions(int argc, char** argv, Core::NodeId& comChannel)
{
    int index = 1;
    const char* hostname = _T("127.0.0.1:8899");
    bool showHelp = false;

    while ((index < argc) && (!showHelp)) {
        if (strcmp(argv[index], "-remote") == 0) {
            hostname = argv[index + 1];
			index++;
		} else if (strcmp(argv[index], "-h") == 0) {
            showHelp = true;
        }
        index++;
    }

    if (!showHelp) {
        comChannel = Core::NodeId(hostname);
    }

    return (showHelp);
}

void ShowMenu()
{
    printf("Enter\n"
           "\tI : Invoke a synchronous method for getting the server time\n"
           "\tT : Invoke a synchronous method with aggregated parameters\n"
           "\tR : Register for a-synchronous feedback\n"
           "\tU : Unregister for a-synchronous feedback\n"
           "\tS : Send message to registered clients\n"
		   "\tP : Read Property.\n"
           "\t0 : Set property @ value 0.\n"
           "\t1 : Set property @ value 1.\n"
           "\t2 : Set property @ value 2.\n"
           "\t3 : Set property @ value 3.\n"
           "\tW : Read Window Property.\n"
           "\t4 : Set property @ value { 0, 2, 1280, 720 }.\n"
           "\t5 : Set property @ value { 200, 300, 720, 100 }.\n"
           "\tO : Set properties using an opaque variant JSON parameter\n"
           "\tV : Get properties using an opaque variant JSON parameter\n"
		   "\tB : Get and Set readonly and writeonly properties\n"
           "\tF : Read Property with index\n"
           "\tJ : Write Property with index\n"
           "\tE : Invoke and exchange an opaque variant JSON parameter\n"
           "\tC : Callback, using a static method, wait for a response from the otherside a-synchronously\n"
           "\tG : Callback, using a class method, wait for a response from the otherside a-synchronously\n"
           "\tD : Demonstrating the possibilities with JsonObject\n\n"
           "\tX : Measure COM Performance\n"
           "\tY : Measure JSONRPC performance\n"
           "\tZ : Measure MessagePack performance\n"
           "\tL : Legacy invoke on version 1 clueless...\n"
           "\t+ : Register for a-synchronous events on Version 1 interface\n"
           "\t- : Unregister for a-synchronous events on Version 1 interface\n"
           "\tH : Help\n"
           "\tQ : Quit\n");
}

namespace Handlers {

// The methods to be used for handling the incoming methods can be static methods or
// methods in classes. In the client tthere is a demonstration of a static method,
// in the server is a demonstartion of an object method.
// To avoid name clashses it is recomended to put the handlers in a namespace (clock,
// for example, alreay exists in the global namespace and you get very interesting
// compiler warnings if there is a name clash)
static void clock(const Core::JSON::String& parameters)
{
    printf("Received a new time: %s\n", parameters.Value().c_str());
}

static void clock_legacy(const Data::Time& parameters)
{
    printf("Receiving legacy clock events. %d:%d:%d\n", parameters.Hours.Value(), parameters.Minutes.Value(), parameters.Seconds.Value());
}

static void async_callback(const Data::Response& response)
{
    printf("Finally we are triggered. GLOBAL: @ %s\n", Core::Time(response.Time.Value(), false).ToRFC1123().c_str());
}

class Callbacks {
public:
	void async_callback_complete(const Data::Response& response, const Core::JSONRPC::Error* result) {
        printf("Finally we are triggered. Pointer to: %p @ %s\n", result, Core::Time(response.Time.Value(), false).ToRFC1123().c_str());
	}
};

class MessageHandler {
public:
    explicit MessageHandler(const string& recipient)
        : _recipient(recipient)
        , _remoteObject(_T("JSONRPCPlugin.1"), (recipient + _T(".client.events")).c_str()) {
    }

    void message_received(const Core::JSON::String& message) {
        printf("Message received for %s: %s\n", _recipient.c_str(), message.Value().c_str());
    }

    void Subscribe() {
        if (_remoteObject.Subscribe<Core::JSON::String>(1000, _T("message"), &MessageHandler::message_received, this) == Core::ERROR_NONE) {
            printf("Installed a notification handler and registered for the notifications for message events for %s\n", _recipient.c_str());
        }
        else {
            printf("Failed to install a notification handler\n");
        }
    }

    void Unsubscribe() {
        _remoteObject.Unsubscribe(1000, _T("message"));
    }

private:
    string _recipient;
    JSONRPC::Client _remoteObject;
};
}

// Performance measurement functions/methods and definitions
// ---------------------------------------------------------------------------------------------
typedef std::function<uint32_t(const uint16_t size, const uint8_t buffer[])> PerformanceFunction;
constexpr uint32_t MeasurementLoops = 20;
static uint8_t swapPattern[] = { 0x00, 0x55, 0xAA, 0xFF };

static void Measure(const TCHAR info[], const uint8_t patternLength, const uint8_t pattern[], PerformanceFunction& subject)
{
    uint8_t dataFrame[4096];
    uint16_t index = 0;
    uint8_t patternIndex = 0;

    ASSERT(patternLength != 0);

    while (index < sizeof(dataFrame)) {

        dataFrame[index++] = pattern[patternIndex++];

        patternIndex %= (patternLength - 1);
    }

    printf("Measurements [%s]:\n", info);
    uint64_t time;
    Core::StopWatch measurement;

	for (uint32_t run = 0; run < MeasurementLoops; run++) {
        subject(0, dataFrame);
    }
    time = measurement.Elapsed();
    printf("Data outbound:    [0], inbound:    [4]. Total: %llu. Average: %llu\n", time, time / MeasurementLoops);

    measurement.Reset();
    for (uint32_t run = 0; run < MeasurementLoops; run++)
    {
        subject(16, dataFrame);
    }
    time = measurement.Elapsed();
    printf("Data outbound:   [16], inbound:    [4]. Total: %llu. Average: %llu\n", time, time / MeasurementLoops);

    measurement.Reset();
    for (uint32_t run = 0; run < MeasurementLoops; run++)
    {
        subject(128, dataFrame);
    }
    time = measurement.Elapsed();
    printf("Data outbound:  [128], inbound:    [4]. Total: %llu. Average: %llu\n", time, time / MeasurementLoops);

    measurement.Reset();
    for (uint32_t run = 0; run < MeasurementLoops; run++) {
        subject(256, dataFrame);
    }
    time = measurement.Elapsed();
    printf("Data outbound:  [256], inbound:    [4]. Total: %llu. Average: %llu\n", time, time / MeasurementLoops);

    measurement.Reset();
    for (uint32_t run = 0; run < MeasurementLoops; run++) {
        subject(512, dataFrame);
    }
    time = measurement.Elapsed();
    printf("Data outbound:  [512], inbound:    [4]. Total: %llu. Average: %llu\n", time, time / MeasurementLoops);

    measurement.Reset();
    for (uint32_t run = 0; run < MeasurementLoops; run++) {
        subject(1024, dataFrame);
    }
    time = measurement.Elapsed();
    printf("Data outbound: [1024], inbound:    [4]. Total: %llu. Average: %llu\n", time, time / MeasurementLoops);

    measurement.Reset();
    for (uint32_t run = 0; run < MeasurementLoops; run++) {
        subject(2048, dataFrame);
    }
    time = measurement.Elapsed();
    printf("Data outbound: [2048], inbound:    [4]. Total: %llu. Average: %llu\n", time, time / MeasurementLoops);
}

static void PrintObject(const JsonObject::Iterator& iterator)
{
    JsonObject::Iterator index = iterator;
    while (index.Next() == true) {
        printf("Element [%s]: <%s> = \"%s\"\n",
            index.Label(),
            Core::EnumerateType<JsonValue::type>(index.Current().Content()).Data(),
            index.Current().Value().c_str());
    }
}

int main(int argc, char** argv)
{
    // Additional scoping neede to have a proper shutdown of the STACK object:
    // JSONRPC::Client remoteObject
    {

        Core::NodeId comChannel;
        ShowMenu();
        int element;
        Handlers::Callbacks testCallback;

        ParseOptions(argc, argv, comChannel);

		SleepMs(2000);

		// Lets also open up channels over the COMRPC protocol to do performance measurements
        // to compare JSONRPC v.s. COMRPC

        // Make sure we have an engine to handle the incoming requests over the COMRPC channel.
        // They are time multiplexed so 1 engine to rule them all. The nex line instantiates the
        // framework to connect to a COMRPC server running at the <connector> address. once the
        // connection is established, interfaces can be requested.
        Core::ProxyType<RPC::CommunicatorClient> client(Core::ProxyType<RPC::CommunicatorClient>::Create(
            comChannel,
            Core::ProxyType<RPC::InvokeServerType<4, 1>>::Create(Core::Thread::DefaultStackSize())));

        ASSERT(client.IsValid() == true);

        // Open up the COMRPC Client connection.
        if (client->Open(2000) != Core::ERROR_NONE) {
            printf("Failed to open up a COMRPC link with the server. Is the server running ?");
        }

        // The JSONRPC Client library is expecting the THUNDER_ACCESS environment variable to be set and pointing
        // to the JSONRPC Server, this can be a domain socket (use at least 1 slash in it, or a TCP address.
        Core::SystemInfo::SetEnvironment(_T("THUNDER_ACCESS"), (_T("127.0.0.1:80")));

        // This is not mandatory, just an easy way to use the VisualStudio environment to Start 2 projects at once, 1 project
        // being the JSONRPC Server running the plugins and the other one this client. However, give the sevrver a bit of time
        // to bring up Plugin JSONRPCPlugin, before we hook up to it. If one starts this App, after the Server is up and running
        // this is not nessecary.
        SleepMs(1000);
        printf("Preparing JSONRPC!!!\n");

        // Create a remoteObject.  This is the way we can communicate with the Server.
        // The parameters:
        // 1. [mandatory] This is the designator of the module we will connect to.
        // 2. [optional]  This is the designator used for the code we have on my side.
        // 3. [optional]  should the websocket under the hood call directly the plugin
        //                or will it be rlayed through thejsonrpc dispatcher (default,
        //                use jsonrpc dispatcher)
        JSONRPC::Client remoteObject(_T("JSONRPCPlugin.2"), _T("client.events.88"));
        JSONRPC::Client legacyObject(_T("JSONRPCPlugin.1"), _T("client.events.33"));
        Handlers::MessageHandler testMessageHandlerJohn("john");
        Handlers::MessageHandler testMessageHandlerJames("james");
        
        do {
            printf("\n>");
            element = toupper(getchar());

            switch (element) {
            case '0':
            case '1':
            case '2':
            case '3': {
                // Lets trigger some action on server side to get a property, usinng a synchronous RPC call.
                // The parameters:
                // 1. [mandatory] Time to wait for the round trip to complete to the server to register.
                // 2. [mandatory] Property to read (See JSONRPCPlugin::JSONRPCPlugin)
                // 3. [mandatory] Parameter that holds the information to "SET" on the other side.
                Core::JSON::String value;
                value = (string(_T("< ")) + static_cast<char>(element) + string(_T(" >")));
                remoteObject.Set(1000, _T("data"), value);
                break;
            }
            case '4': {
                if (remoteObject.Set<Data::Geometry>(1000, _T("geometry"), 0, 2, 1280, 720) == Core::ERROR_NONE) {
                    printf("Set Window rectangle { 0, 2, 1280, 720 }\n");
                } else {
                    printf("Failed to set Window!\n");
                }
                break;
            }
            case '5': {
                if (remoteObject.Set<Data::Geometry>(1000, _T("geometry"), 200, 300, 720, 100) == Core::ERROR_NONE) {
                    printf("Set Window rectangle { 200, 300, 720, 100 }\n");
                } else {
                    printf("Failed to set Window!\n");
                }
                break;
            }
            case 'W': {
                Data::Geometry window;
                if (remoteObject.Get<Data::Geometry>(1000, _T("geometry"), window) == Core::ERROR_NONE) {
                    printf("Window rectangle { %u, %u, %u, %u}\n", window.X.Value(), window.Y.Value(), window.Width.Value(), window.Height.Value());
                } else {
                    printf("Oopsy daisy, could not get the Geometry parameters!\n");
                }
                break;
            }
            case 'P': {
                // Lets trigger some action on server side to get a property, usinng a synchronous RPC call.
                // The parameters:
                // 1. [mandatory] Time to wait for the round trip to complete to the server to register.
                // 2. [mandatory] Property to read (See JSONRPCPlugin::JSONRPCPlugin)
                // 3. [mandatory] Response to be received from the other side.
                Core::JSON::String value;
                remoteObject.Get(1000, _T("data"), value);
                printf("Read propety from the remote object: %s\n", value.Value().c_str());
                break;
            }
            case 'F': {
                Core::JSON::DecUInt32 value;
                if (remoteObject.Get(1000, "array", 1, value) == Core::ERROR_NONE) {
                    printf("Requested the value of: %d from index 1\n", value.Value());
                } else {
                    printf("Indexed property failed to get!!\n");
                }
                break;
            }
            case 'J': {
                Core::JSON::DecUInt32 value;
                value = 12;

                if (remoteObject.Set(1000, "array", 1, value) == Core::ERROR_NONE) {
                    printf("Assigned the value of: %d to index 1\n", value.Value());
                } else {
                    printf("Indexed property failed to set!!\n");                
				}
                break;
            }
            case 'B': {
				// read readonly property
				Core::JSON::String value;
				uint32_t result = remoteObject.Get(1000, _T("status"), value);
				printf("Read readonly propety from the remote object (result = %s): %s\n", Core::ErrorToString(result), value.Value().c_str());
				// write readonly property -> should fail
				value = (string(_T("Bogus")));
				result = remoteObject.Set(1000, _T("status"), value);
				printf("Write readonly propety from the remote object result: %s\n", Core::ErrorToString(result));
				// write writeonly property 
				value = (string(_T("<5>")));
				result = remoteObject.Set(1000, _T("value"), value);
				printf("Write writeonly propety from the remote object result: %s\n", Core::ErrorToString(result));
				// read writeonly property -> should fail
				result = remoteObject.Get(1000, _T("value"), value);
				printf("Read writeonly propety from the remote object, result = %s\n", Core::ErrorToString(result));
				break;
			}
			case 'I': {
                // Lets trigger some action on server side to get some feedback. The regular synchronous RPC call.
                // The parameters:
                // 1. [mandatory] Time to wait for the round trip to complete to the server to register.
                // 2. [mandatory] Method name to call (See JSONRPCPlugin::JSONRPCPlugin - 14)
                // 3. [mandatory] Parameters to be send to the other side.
                // 4. [mandatory] Response to be received from the other side.
                Core::JSON::String result;
                remoteObject.Invoke<void, Core::JSON::String>(1000, _T("time"), result);
                printf("received time: %s\n", result.Value().c_str());
                break;
            }
            case 'T': {
                // Lets trigger some action on server side to get some feedback. The regular synchronous RPC call.
                // The parameters:
                // 1. [mandatory] Time to wait for the round trip to complete to the server to register.
                // 2. [mandatory] Method name to call (See JSONRPCPlugin::JSONRPCPlugin - 14)
                // 3. [mandatory] Parameters to be send to the other side.
                // 4. [mandatory] Response to be received from the other side.
                Data::Response response;
                remoteObject.Invoke<Data::Parameters, Data::Response>(1000, _T("extended"), Data::Parameters(_T("JustSomeText"), true), response);
                printf("received time: %ju - %s\n", (intmax_t)response.Time.Value(), response.State.Data());
                break;
            }
            case 'R': {
                // We have a handler, called Handlers::clock to handle the events coming from the Server.
                // If we register this handler, it will also automatically be register this handler on the server side.
                // The parameters:
                // 1. [mandatory] Time to wait for the round trip to complete to the server to register.
                // 2. [mandatory] Event name to subscribe to on server side (See JSONRPCPlugin::SendTime - 44)
                // 3. [mandatory] Code to handle this event, it is allowed to use a lambda here, or a object method (see plugin)
                if (remoteObject.Subscribe<Core::JSON::String>(1000, _T("clock"), &Handlers::clock) == Core::ERROR_NONE) {
                    printf("Installed a notification handler and registered for the notifications for clock events\n");
                } else {
                    printf("Failed to install a notification handler\n");
                }

                testMessageHandlerJohn.Subscribe();
                testMessageHandlerJames.Subscribe();

                break;

            }
            case 'U': {
                // We are no longer interested inm the events, ets get ride of the notifications.
                // The parameters:
                // 1. [mandatory] Time to wait for the round trip to complete to the server to register.
                // 2. [mandatory] Event name which was used during the registration

                remoteObject.Unsubscribe(1000, _T("clock"));
                testMessageHandlerJohn.Unsubscribe();
                testMessageHandlerJames.Unsubscribe();
                printf("Unregistered and removed all notification handlers\n");
                break;
            }
            case 'S': {
                // Lets trigger some action on server side to get some messages back. The regular synchronous RPC call.
                // The parameters:
                // 1. [mandatory] Time to wait for the round trip to complete to the server to register.
                // 2. [mandatory] Method name to call (See JSONRPCPlugin::JSONRPCPlugin - 14)
                // 3. [mandatory] Parameters to be send to the other side.
                // 4. [mandatory] Response to be received from the other side.
                static string recipient("all");
                remoteObject.Invoke<Data::MessageParameters, void>(1000, _T("postmessage"), Data::MessageParameters(recipient, _T("message for ") + recipient));
                printf("message send to %s\n", recipient.c_str());
                recipient == "all" ? recipient = "john" : (recipient == "john" ? recipient = "james" : (recipient == "james" ? recipient = "all" : recipient = "all" ) );
                break;
            }
            case 'O': {
                // Set properties, using a n Opaque value:
                remoteObject.SetProperty("window", { { { "x", 123 }, { "y", 456 }, { "width", 789 }, { "height", 1112 } } });
                printf("Send out an opaque struct to set the window properties\n");
                break;
            }
            case 'V': {
                // Get properties, using an Opaque value:
                JsonObject result;
                remoteObject.GetProperty("window", result);
                JsonValue value(result.Get("x"));
                if (value.Content() == JsonValue::type::EMPTY) {
                    printf("<x> value not available\n");
                } else if (value.Content() != JsonValue::type::NUMBER) {
                    printf("<x> is expected to be a number but it is: %s\n", Core::EnumerateType<JsonValue::type>(value.Content()).Data());
                } else {
                    printf("<x>: %d\n", static_cast<uint32_t>(value.Number()));
                }
                value = result.Get("y");
                if (value.Content() == JsonValue::type::EMPTY) {
                    printf("<y> value not available\n");
                } else if (value.Content() != JsonValue::type::NUMBER) {
                    printf("<y> is expected to be a number but it is: %s\n", Core::EnumerateType<JsonValue::type>(value.Content()).Data());
                } else {
                    printf("<y>: %d\n", static_cast<uint32_t>(value.Number()));
                }
                value = result.Get("width");
                if (value.Content() == JsonValue::type::EMPTY) {
                    printf("<width> value not available\n");
                } else if (value.Content() != JsonValue::type::NUMBER) {
                    printf("<width> is expected to be a number but it is: %s\n", Core::EnumerateType<JsonValue::type>(value.Content()).Data());
                } else {
                    printf("<width>: %d\n", static_cast<uint32_t>(value.Number()));
                }
                value = result.Get("height");
                if (value.Content() == JsonValue::type::EMPTY) {
                    printf("<height> value not available\n");
                } else if (value.Content() != JsonValue::type::NUMBER) {	
                    printf("<height> is expected to be a number but it is: %s\n", Core::EnumerateType<JsonValue::type>(value.Content()).Data());
                } else {
                    printf("<height>: %d\n", static_cast<uint32_t>(value.Number()));
                }
                break;
            }
            case 'E': {
                // Set this one up, the old fasion way
                JsonObject parameters;
                JsonObject response;
                parameters["x"] = 67;
                parameters["y"] = 99;
                parameters["width"] = false; // Deliberate error, see what happens..
                parameters["height"] = -1299; // Interesting a negative hide... No one is warning us :-)
                PrintObject(parameters.Variants());
                if (remoteObject.Invoke("swap", parameters, response) == Core::ERROR_NONE) {
                    PrintObject(response.Variants());
                } else {
                    printf("Something went wrong during the invoke\n");
                }

                break;
            }
            case 'C': {
                if (remoteObject.Dispatch<Core::JSON::DecUInt8>(30000, "async", { 10, true }, &Handlers::async_callback) != Core::ERROR_NONE) {
                    printf("Something went wrong during the invoke\n");
				}
                break;
            }
            case 'G': {
                if (remoteObject.Dispatch<void>(30000, "waitcall", &Handlers::Callbacks::async_callback_complete, &testCallback) != Core::ERROR_NONE) {
                    printf("Something went wrong during the invoke\n");
                }
                break;
            }
            case 'D': {
                string serialized;
                JsonObject demoObject;
                demoObject["x"] = 12;
                demoObject["name"] = "Pierre";
                demoObject["switch"] = true;
                demoObject.ToString(serialized);
                printf("The serialized values are: %s\n", serialized.c_str());

				JsonObject newObject;
                newObject = serialized;
                string newString;
                newObject.ToString(newString);
                printf("The serialized values are [instantiated from the string, printed above]: %s\n", newString.c_str());

				JsonObject otherObject = R"({"zoomSetting": "FULL", "SomethingElse": true, "Numbers": 123 })";
                otherObject["member"] = 76;
                string otherString;
                otherObject.ToString(otherString);
                printf("The serialized values are [otherObject]: %s\n", otherString.c_str());
                break;
            }
            case 'X':
			{
                if ((client.IsValid() == false) || (client->IsOpen() == false)) {
                    printf("Can not measure the performance of COMRPC, there is no connection.\n");
                } else {
                    Core::StopWatch measurement;
                    Exchange::IPerformance* perf = client->Aquire<Exchange::IPerformance>(2000, _T("JSONRPCPlugin"), ~0);
                    if (perf == nullptr) {
                        printf("Instantiation failed. An performance interface was not returned. It took: %lld ticks\n", measurement.Elapsed());
                    } else {
                        printf("Instantiating and retrieving the interface took: %lld ticks\n", measurement.Elapsed());

						PerformanceFunction implementation = [perf](const uint16_t length, const uint8_t buffer[]) -> uint32_t {
                            return (perf->Send(length, buffer));
                        };

                        Measure(_T("COMRPC"), sizeof(swapPattern), swapPattern, implementation);

						perf->Release();
                    }
				}
                break;
			}
            case 'Y':
			{
                PerformanceFunction implementation = [&remoteObject](const uint16_t length, const uint8_t buffer[]) -> uint32_t {
                    string stringBuffer;
                    Data::JSONDataBuffer message;
                    Core::JSON::DecUInt32 result;
                    Core::ToString(buffer, length, false, stringBuffer);
                    message.Data = stringBuffer;
                    remoteObject.Invoke<Data::JSONDataBuffer, Core::JSON::DecUInt32>(3000, _T("send"), message, result);
                    return (result.Value());
                };

                Measure(_T("JSONRPC"), sizeof(swapPattern), swapPattern, implementation);
                break;
			}
            case 'Z':
			{
                break;
			}
            case 'L':
			{
                // !!!!!!!!! calling the clueless method on INTERFACE VERSION 1 !!!!!!!!!!!!!!!!!!!!!!!!!!
                // Lets trigger some action on server side to get some feedback. The regular synchronous RPC call.
                // The parameters:
                // 1. [mandatory] Time to wait for the round trip to complete to the server to register.
                // 2. [mandatory] Method name to call (See JSONRPCPlugin::JSONRPCPlugin - 14)
                // 3. [mandatory] Parameters to be send to the other side.
                // 4. [mandatory] Response to be received from the other side.
                Core::JSON::String result;
                result = _T("This should be an eche server on interface 1");
                legacyObject.Invoke<Core::JSON::String, Core::JSON::String>(1000, _T("clueless"), result, result);
                printf("received time: %s\n", result.Value().c_str());
                break;
            }
            case '+':
				// !!!!!!!!! Subscribing to events on INTERFACE VERSION 1 !!!!!!!!!!!!!!!!!!!!!!!!!!
                // We have a handler, called Handlers::clock to handle the events coming from the Server.
                // If we register this handler, it will also automatically be register this handler on the server side.
                // The parameters:
                // 1. [mandatory] Time to wait for the round trip to complete to the server to register.
                // 2. [mandatory] Event name to subscribe to on server side (See JSONRPCPlugin::SendTime - 44)
                // 3. [mandatory] Code to handle this event, it is allowed to use a lambda here, or a object method (see plugin)
                if (legacyObject.Subscribe<Data::Time>(1000, _T("clock"), &Handlers::clock_legacy) == Core::ERROR_NONE) {
                    printf("Installed a notification handler and registered for the notifications for clock events\n");
                } else {
                    printf("Failed to install a notification handler\n");
                }
                break;
            case '-':
                // !!!!!!!!! Unsubscribing to events on INTERFACE VERSION 1 !!!!!!!!!!!!!!!!!!!!!!!!!!
                // We are no longer interested inm the events, ets get ride of the notifications.
                // The parameters:
                // 1. [mandatory] Time to wait for the round trip to complete to the server to register.
                // 2. [mandatory] Event name which was used during the registration
                legacyObject.Unsubscribe(1000, _T("clock"));
                printf("Unregistered and removed all notification handlers\n");
                break;

            case '?':
            case 'H':
                ShowMenu();
            }

        } while (element != 'Q');

		// We are done with the COMRPC connections, no need to create new ones.
		client.Release();
    }

    printf("Leaving app.\n");

    Core::Singleton::Dispose();

    return (0);
}
