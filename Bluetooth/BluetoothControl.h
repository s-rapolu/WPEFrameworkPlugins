#pragma once

#include "Module.h"
#include "BlueDriver.h"
#include "Bluetooth.h"

#include <interfaces/IBluetooth.h>

namespace WPEFramework {
namespace Plugin {

    class BluetoothControl : public PluginHost::IPlugin, public PluginHost::IWeb, public Exchange::IBluetooth {
    private:
        BluetoothControl(const BluetoothControl&) = delete;
        BluetoothControl& operator=(const BluetoothControl&) = delete;

        class Config : public Core::JSON::Container {
        private:
            Config(const Config&);
            Config& operator=(const Config&);

        public:
            Config()
                : Core::JSON::Container()
                , Interface(0)
            {
                Add(_T("interface"), &Interface);
            }
            ~Config()
            {
            }

        public:
            Core::JSON::DecUInt8 Interface;
        };

    public:
        class EXTERNAL Device : public IBluetooth::IDevice {
        private: 
            Device& operator=(const Device&) = delete;

            enum state : uint8_t {
                DISCOVERED = 0x01,
                PAIRED     = 0x02,
                CONNECTED  = 0x04
            };

        public:
            class JSON : public Core::JSON::Container {
            private:
                JSON& operator= (const JSON&);

            public:
                JSON()
                    : Core::JSON::Container()
                    , Address()
                    , Name()
                {
                    Add(_T("address"), &Address);
                    Add(_T("name"), &Name);
                }
                JSON(const JSON& copy)
                    : Core::JSON::Container()
                    , Address()
                    , Name()
                {
                    Add(_T("address"), &Address);
                    Add(_T("name"), &Name);
                    Address = copy.Address;
                    Name = copy.Name;
                }
                virtual ~JSON() {
                }

            public:
                JSON& operator= (const IBluetooth::IDevice* source) {
                    if (source != nullptr) {
                        Address = source->Address();
                        Name = source->Name();
                    }
                    else {
                        Address.Clear();
                        Name.Clear();
                    }
                    return (*this);
                }
                Core::JSON::String Address;
                Core::JSON::String Name;
            };

        public:
            Device () 
                : _address()
                , _name()
                , _state(0) {
            }
            Device (const Bluetooth::Address& address, const string& name) 
                : _address(address)
                , _name(name)
                , _state(0) {
            }
            Device (const Device& copy) 
                : _address(copy._address)
                , _name(copy._name) 
                , _state(copy._state) {
            }
            ~Device() {
            }

        public:
            virtual string Address() const override {
                return (_address.ToString());
            }
            virtual string Name() const override {
                return (_name);
            }
            virtual bool IsDiscovered () const override {
                return ((_state & DISCOVERED) != 0);
            }
            virtual bool IsPaired() const override {
                return ((_state & PAIRED) != 0);
            }
            virtual bool IsConnected() const override {
                return ((_state & CONNECTED) != 0);
            }

            BEGIN_INTERFACE_MAP(Device)
                INTERFACE_ENTRY(IBluetooth::IDevice)
            END_INTERFACE_MAP

        private:
            Bluetooth::Address _address;
            string _name;
            uint8_t _state;
        };

        class EXTERNAL Status : public Core::JSON::Container {
        private:
            Status(const Status&) = delete;
            Status& operator=(const Status&) = delete;

        public:
            Status()
                : Scanning()
                , DeviceList() {
                Add(_T("scanning"), &Scanning);
                Add(_T("deviceList"), &DeviceList);
            }
            virtual ~Status() {
            }

        public:
            Core::JSON::Boolean Scanning;
            Core::JSON::ArrayType<Device::JSON> DeviceList;
        };

    public:
        BluetoothControl()
            : _skipURL(0)
            , _service(nullptr)
            , _driver(nullptr)
            , _hciSocket(Core::NodeId(HCI_DEV_NONE, HCI_CHANNEL_CONTROL), 256)
            , _btAddress()
            , _interface()
        {
        }
        virtual ~BluetoothControl()
        {
        }

    public:
        BEGIN_INTERFACE_MAP(BluetoothControl)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(PluginHost::IWeb)
            INTERFACE_ENTRY(Exchange::IBluetooth)
        END_INTERFACE_MAP

    public:
        //  IPlugin methods
        // -------------------------------------------------------------------------------------------------------

        // First time initialization. Whenever a plugin is loaded, it is offered a Service object with relevant
        // information and services for this particular plugin. The Service object contains configuration information that
        // can be used to initialize the plugin correctly. If Initialization succeeds, return nothing (empty string)
        // If there is an error, return a string describing the issue why the initialisation failed.
        // The Service object is *NOT* reference counted, lifetime ends if the plugin is deactivated.
        // The lifetime of the Service object is guaranteed till the deinitialize method is called.
        virtual const string Initialize(PluginHost::IShell* service);

        // The plugin is unloaded from the webbridge. This is call allows the module to notify clients
        // or to persist information if needed. After this call the plugin will unlink from the service path
        // and be deactivated. The Service object is the same as passed in during the Initialize.
        // After this call, the lifetime of the Service object ends.
        virtual void Deinitialize(PluginHost::IShell* service);

        // Returns an interface to a JSON struct that can be used to return specific metadata information with respect
        // to this plugin. This Metadata can be used by the MetaData plugin to publish this information to the outside world.
        virtual string Information() const;

        //  IWeb methods
        // -------------------------------------------------------------------------------------------------------
        virtual void Inbound(Web::Request& request);
        virtual Core::ProxyType<Web::Response> Process(const Web::Request& request);

        //  IBluetooth methods
        // -------------------------------------------------------------------------------------------------------
        virtual bool IsScanning() const override;
        virtual uint32_t Register(IBluetooth::INotification* notification) override;
        virtual uint32_t Unregister(IBluetooth::INotification* notification) override;

        virtual bool Scan(const bool enable) override;
        virtual bool Pair(const string&) override;
        virtual bool Connect(const string&) override;
        virtual bool Disconnect(const string&) override;

    private:
        Core::ProxyType<Web::Response> GetMethod(Core::TextSegmentIterator& index);
        Core::ProxyType<Web::Response> PutMethod(Core::TextSegmentIterator& index, const Web::Request& request);
        Core::ProxyType<Web::Response> PostMethod(Core::TextSegmentIterator& index, const Web::Request& request);
        Core::ProxyType<Web::Response> DeleteMethod(Core::TextSegmentIterator& index);

    private:
        uint8_t _skipURL;
        PluginHost::IShell* _service;
        Bluetooth::Driver* _driver;
        Bluetooth::SynchronousSocket _hciSocket;
        Bluetooth::Address _btAddress;
        Bluetooth::Driver::Interface _interface;
        std::list<IBluetooth::IDevice*> _devices;
        std::list<IBluetooth::INotification*> _observers;
    };
} //namespace Plugin

} //namespace Solution
