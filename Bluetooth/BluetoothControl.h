#pragma once

#include "BlueDriver.h"
#include "Bluetooth.h"

#include <functional>
#include <interfaces/IBluetooth.h>
#include <linux/uhid.h>

namespace WPEFramework {
namespace Plugin {

    class BluetoothControl : public PluginHost::IPlugin, public PluginHost::IWeb, public Exchange::IBluetooth {
    private:
        BluetoothControl(const BluetoothControl&) = delete;
        BluetoothControl& operator=(const BluetoothControl&) = delete;

        class Scanner : public Bluetooth::HCISocket::IScanning, public Bluetooth::HCISocket {
        private:
            Scanner() = delete;
            Scanner(const Scanner&) = delete;
            Scanner& operator=(const Scanner&) = delete;

        private:
            // The bluetooth library has some unexpected behaviour. For example, the scan of NON-BLE devices
            // is a blocking call for the duration of the passed in time. Which is, I think, very intrusive
            // fo any responsive design. If a RESTFull call would start a scan, the call would last the duration
            // of the scan, which is typicall >= 10Secods which is unacceptable, so it needs to be decoupled.
            // This decoupling is done on this internal Worker thread.
            class Job : public Core::IDispatch {
            private:
                Job() = delete;
                Job(const Job&) = delete;
                Job& operator=(const Job&) = delete;

                enum scanMode {
                    LOW_ENERGY = 0x01,
                    REGULAR = 0x02,
                    PASSIVE = 0x04,
                    LIMITED = 0x08
                };

            public:
                Job(Scanner* parent)
                    : _parent(*parent)
                    , _mode(0)
                {
                }
                virtual ~Job()
                {
                }

            public:
                void Load(const uint16_t scanTime, const uint32_t type, const uint8_t flags)
                {
                    if (_mode == 0) {
                        _mode = REGULAR;
                        _scanTime = scanTime;
                        _type = type;
                        _flags = flags;
                        PluginHost::WorkerPool::Instance().Submit(Core::ProxyType<Core::IDispatch>(*this));
                    }
                }
                void Load(const uint16_t scanTime, const bool limited, const bool passive)
                {
                    if (_mode == 0) {
                        _mode = LOW_ENERGY | (passive ? PASSIVE : 0) | (limited ? LIMITED : 0);
                        _scanTime = scanTime;
                        PluginHost::WorkerPool::Instance().Submit(Core::ProxyType<Core::IDispatch>(*this));
                    }
                }

            private:
                virtual void Dispatch()
                {

                    if ((_mode & REGULAR) != 0) {
                        TRACE(Trace::Information, (_T("Start regular scan: %s"), Core::Time::Now().ToRFC1123().c_str()));
                        _parent.Run(_scanTime, _type, _flags);
                    } else {
                        TRACE(Trace::Information, (_T("Start Low Energy scan: %s"), Core::Time::Now().ToRFC1123().c_str()));
                        _parent.Run(_scanTime, ((_mode & LIMITED) != 0), ((_mode & PASSIVE) != 0));
                    }
                    TRACE(Trace::Information, (_T("Scan completed: %s"), Core::Time::Now().ToRFC1123().c_str()));
                    _mode = 0;
                }

            private:
                Scanner& _parent;
                uint16_t _scanTime;
                uint32_t _type;
                uint8_t _flags;
                uint8_t _mode;
            };

        public:
            Scanner(BluetoothControl& parent)
                : Bluetooth::HCISocket()
                , _parent(parent)
                , _activity(Core::ProxyType<Job>::Create(this))
            {
            }
            virtual ~Scanner()
            {
                PluginHost::WorkerPool::Instance().Revoke(_activity);
            }

        public:
            virtual void DiscoveredDevice(const bool lowEnergy, const Bluetooth::Address& address, const string& name) override
            {
                _parent.DiscoveredDevice(lowEnergy, address, name);
            }
            void Scan(const uint16_t scanTime, const uint32_t type, const uint8_t flags)
            {
                _activity->Load(scanTime, type, flags);
            }
            void Scan(const uint16_t scanTime, const bool limited, const bool passive)
            {
                _activity->Load(scanTime, limited, passive);
            }
            uint32_t Open(const Bluetooth::Address& address)
            {
                uint32_t result = Core::ERROR_NONE;

                if (IsOpen() == false) {
                    Bluetooth::HCISocket::LocalNode(address.NodeId(HCI_CHANNEL_RAW));
                    result = Bluetooth::HCISocket::Open(Core::infinite);
                }

                return (result);
            }
            uint32_t Close()
            {
                return (Bluetooth::HCISocket::Close(Core::infinite));
            }

        private:
            virtual uint16_t Deserialize(const uint8_t* stream, const uint16_t length) override
            {
                uint16_t result = Bluetooth::HCISocket::Deserialize(stream, length);

                if ((result == 0) && (length >= (HCI_EVENT_HDR_SIZE + 1))) {
                    const hci_event_hdr* hdr = reinterpret_cast<const hci_event_hdr*>(&(stream[1]));
                    const uint8_t* ptr = reinterpret_cast<const uint8_t*>(&(stream[1 + HCI_EVENT_HDR_SIZE]));
                    uint16_t len = (length - (1 + HCI_EVENT_HDR_SIZE));

                    if (hdr->evt == EVT_LE_META_EVENT) {
                        const evt_le_meta_event* eventMetaData = reinterpret_cast<const evt_le_meta_event*>(ptr);

                        _parent.Notification(eventMetaData->subevent, len - EVT_LE_META_EVENT_SIZE, &(ptr[EVT_LE_META_EVENT_SIZE]));
                    }
                }

                return (result);
            }
            void Run(const uint16_t scanTime, const uint32_t type, const uint8_t flags)
            {
                Bluetooth::HCISocket::Scan(this, scanTime, type, flags);
            }
            void Run(const uint16_t scanTime, const bool limited, const bool passive)
            {
                Bluetooth::HCISocket::Scan(this, scanTime, limited, passive);
            }

        private:
            BluetoothControl& _parent;
            Core::ProxyType<Job> _activity;
        };

        class GATTRemote : public Bluetooth::GATTSocket {
        private:
            GATTRemote() = delete;
            GATTRemote(const GATTRemote&) = delete;
            GATTRemote& operator=(const GATTRemote&) = delete;

            class Sink : public Command::ICallback {
            private:
                Sink() = delete;
                Sink(const Sink&) = delete;
                Sink& operator=(const Sink&) = delete;

            public:
                Sink(GATTRemote& parent)
                    : _parent(parent)
                {
                }
                virtual ~Sink()
                {
                }

            public:
                virtual void Completed(const uint32_t error) override
                {
                    _parent.Completed(error);
                }

            private:
                GATTRemote& _parent;
            };

            // UUID
            static constexpr uint16_t REPORT_UUID = 0x2a4d;
            static constexpr uint16_t PRIMARY_SERVICE_UUID = 0x2800;
            static constexpr uint16_t CHARACTERISTICS_UUID = 0x2803;
            static constexpr uint16_t HID_UUID = 0x1812;
            static constexpr uint16_t PNP_UUID = 0x2a50;
            static constexpr uint16_t DEVICE_NAME_UUID = 0x2a00;
            static constexpr uint16_t REPORT_MAP_UUID = 0x2a4b;

            enum state {
                METADATA_TYPE,
                METADATA_ID,
                METADATA_NAME_HANDLE,
                METADATA_NAME,
                METADATA_DESCRIPTORS_HANDLE,
                METADATA_DESCRIPTORS,
                METADATA_ENABLE,
                OPERATIONAL,
                ERROR
            };

            class Metadata {
            private:
                Metadata(const Metadata&) = delete;
                Metadata& operator=(const Metadata&) = delete;

            public:
                Metadata()
                    : _vendorId(0)
                    , _productId(0)
                    , _version(0)
                    , _name()
                    , _blob()
                {
                }
                ~Metadata()
                {
                }

            public:
                inline uint16_t VendorId() const
                {
                    return (_vendorId);
                }
                inline uint16_t ProductId() const
                {
                    return (_productId);
                }
                inline uint16_t Version() const
                {
                    return (_version);
                }
                inline const string& Name() const
                {
                    return (_name);
                }
                inline const uint8_t* Blob() const
                {
                    return (_blob);
                }
                inline uint16_t Length() const
                {
                    return (sizeof(_blob));
                }
                inline uint16_t Country() const
                {
                    return (0);
                }

            private:
                friend class GATTRemote;

                uint16_t _vendorId;
                uint16_t _productId;
                uint16_t _version;
                string _name;
                uint8_t _blob[8 * 22];
            };

            class InputDevice {
            private:
                InputDevice(const InputDevice&) = delete;
                InputDevice& operator=(const InputDevice&) = delete;

            public:
                InputDevice()
                    : _descriptor(-1)
                {
                }
                ~InputDevice()
                {
                    Close();
                }

            public:
                bool IsOpen() const
                {
                    return (_descriptor != -1);
                }
                bool Send(const uint8_t length, const uint8_t data[])
                {
                    struct uhid_event uhidEvent;
                    memset(&uhidEvent, 0, sizeof(uhidEvent));
                    uhidEvent.type = UHID_INPUT;
                    uhidEvent.u.input.size = length;
                    uhidEvent.u.input.data[0] = 0x01;
                    uint16_t count = 0;
                    uint16_t index = 0;
                    while (index < length) {
                        // This is an inherited construction. I do not trust it as I would expect that the
                        // uhidEvent.u.input.size should be updated as well. Till we tested it we leave it
                        // like this.
                        if (data[index] != 0x00) {
                            uhidEvent.u.input.data[count++] = data[index];
                        }
                        index++;
                    }

                    return (::write(_descriptor, &uhidEvent, sizeof(uhidEvent)) >= 0);
                }
                uint32_t Open(const string& path, const string& source, const string& destination, const Metadata& info)
                {
                    uint32_t result = Core::ERROR_ALREADY_CONNECTED;

                    if (_descriptor == -1) {
                        result = Core::ERROR_OPENING_FAILED;
                        _descriptor = ::open(path.c_str(), O_RDWR | O_CLOEXEC);

                        if (_descriptor >= 0) {
                            struct uhid_event uhidEvent;
                            result = Core::ERROR_NONE;

                            // Creating UHID Node.
                            memset(&uhidEvent, 0, sizeof(uhidEvent));
                            uhidEvent.type = UHID_CREATE;
                            uhidEvent.u.create.bus = BUS_BLUETOOTH;
                            uhidEvent.u.create.vendor = info.VendorId();
                            uhidEvent.u.create.product = info.ProductId();
                            uhidEvent.u.create.version = info.Version();
                            uhidEvent.u.create.country = info.Country();
                            strncpy(reinterpret_cast<char*>(uhidEvent.u.create.name), info.Name().c_str(), sizeof(uhidEvent.u.create.name));
                            uhidEvent.u.create.rd_data = const_cast<uint8_t*>(info.Blob());
                            uhidEvent.u.create.rd_size = info.Length();
                            strncpy(reinterpret_cast<char*>(uhidEvent.u.create.phys), source.c_str(), sizeof(uhidEvent.u.create.phys));
                            strncpy(reinterpret_cast<char*>(uhidEvent.u.create.uniq), destination.c_str(), sizeof(uhidEvent.u.create.uniq));

                            if (::write(_descriptor, &uhidEvent, sizeof(uhidEvent)) < 0) {
                                ::close(_descriptor);
                                _descriptor = -1;
                                result = Core::ERROR_WRITE_ERROR;
                            }
                        }
                    }
                    return (result);
                }
                uint32_t Close()
                {
                    if (_descriptor == -1) {
                        close(_descriptor);
                    }
                    return (Core::ERROR_NONE);
                }

            private:
                int _descriptor;
            };

        public:
            GATTRemote(const Bluetooth::Address& remoteNode, const string& hidPath)
                : Bluetooth::GATTSocket(
                      Bluetooth::Address().AnyInterface().NodeId(Bluetooth::Address::LE_PUBLIC_ADDRESS, Bluetooth::GATTSocket::LE_ATT_CID, 0),
                      remoteNode.NodeId(Bluetooth::Address::LE_PUBLIC_ADDRESS, Bluetooth::GATTSocket::LE_ATT_CID, 0),
                      64)
                , _inputHandler(nullptr)
                , _device()
                , _hidPath(hidPath)
                , _state(METADATA_TYPE)
                , _metadata()
                , _sink(*this)
            {

                GATTSocket::Open(1000);
            }
            virtual ~GATTRemote()
            {
                if (GATTSocket::IsOpen() == true) {
                    GATTSocket::Close(Core::infinite);
                }
            }

        private:
            virtual uint16_t Deserialize(const uint8_t* /* dataFrame */, const uint16_t availableData) override
            {
                return (availableData);
            }
            virtual void Operational() override
            {
                _state = METADATA_TYPE;
                Security(BT_SECURITY_MEDIUM, 0);
                FindByType(1000, 0x0001, 0xFFFF, GATTSocket::UUID(PRIMARY_SERVICE_UUID), HID_UUID, &_sink);
            }
            virtual void Received(const uint8_t dataFrame[], const uint16_t availableData)
            {
                if (_state == OPERATIONAL) {
                    if (dataFrame[0] == ATT_OP_HANDLE_NOTIFY) {
                        // We got a key press.. where to ?
                        if (_device.IsOpen() == true) {
                            _device.Send(availableData - 2, &(dataFrame[2]));
                        }
                    }
                }
            }
            void Completed(const uint32_t error)
            {
                if (error == Core::ERROR_NONE) {
                    const uint8_t* data(Result().Data()); // FAILS!!!!
                    const uint16_t length(Result().Length());
                    fprintf(stderr, "%s -- %d\n", __FUNCTION__, __LINE__);
                    fflush(stderr);
                    switch (_state) {
                    case METADATA_TYPE: {
                        if (Result().Empty() == false) {
                            ReadByType(1000, 0x0001, 0xFFFF, GATTSocket::UUID(PNP_UUID), &_sink);
                            _state = METADATA_ID;
                        } else {
                            _state = ERROR;
                        }
                        break;
                    }
                    case METADATA_ID: {
                        TRACE(Trace::Information, (_T("Checking for METADATA_ID, length: %d"), length));
                        _state = METADATA_NAME_HANDLE;
                        _metadata._vendorId = (data[0] << 8) | data[1];
                        _metadata._productId = (data[2] << 8) | data[3];
                        _metadata._version = (data[4] << 8) | data[5];
                        ReadByType(10000, 0x0001, 0xFFFF, GATTSocket::UUID(DEVICE_NAME_UUID), &_sink);
                        break;
                    }
                    case METADATA_NAME_HANDLE: {
                        Command::Response& response(Result());
                        if (response.Next() == true) {
                            _state = METADATA_NAME;
                            ReadBlob(1000, response.Handle(), &_sink);
                        } else {
                            _state = ERROR;
                        }
                        break;
                    }
                    case METADATA_NAME: {
                        TRACE(Trace::Information, (_T("Checking for METADATA_NAME")));
                        _state = METADATA_DESCRIPTORS_HANDLE;
                        _metadata._name = string(reinterpret_cast<const char*>(data), length);
                        ReadByType(10000, 0x0001, 0xFFFF, GATTSocket::UUID(REPORT_MAP_UUID), &_sink);
                        break;
                    }
                    case METADATA_DESCRIPTORS_HANDLE: {
                        Command::Response& response(Result());
                        if (response.Next() == true) {
                            _state = METADATA_DESCRIPTORS;
                            ReadBlob(1000, response.Handle(), &_sink);
                        } else {
                            _state = ERROR;
                        }
                        break;
                    }
                    case METADATA_DESCRIPTORS: {
                        TRACE(Trace::Information, (_T("Checking for METADATA_DESCRIPTORS")));
                        _state = METADATA_ENABLE;
                        uint16_t copyLength = std::min(length, static_cast<uint16_t>(sizeof(_metadata._blob)));
                        ::memcpy(_metadata._blob, data, copyLength);
                        WriteByType(10000, 0x0001, 0xFFFF, GATTSocket::UUID(REPORT_UUID), GATTSocket::UUID(htobs(1)), &_sink);
                        break;
                    }
                    case METADATA_ENABLE: {
                        if (_hidPath.empty() == false) {
                            _device.Open(_hidPath, LocalId(), RemoteId(), _metadata);
                            if (_device.IsOpen() == true) {
                                _state = OPERATIONAL;
                            }
                        } else {
                            _inputHandler = PluginHost::InputHandler::KeyHandler();
                            if (_inputHandler != nullptr) {
                                _state = OPERATIONAL;
                            }
                        }
                        break;
                    }
                    default:
                        ASSERT(false);
                    }
                }
            }

        private:
            PluginHost::VirtualInput* _inputHandler;
            InputDevice _device;
            const string _hidPath;
            state _state;
            Metadata _metadata;
            Sink _sink;
        };

        class Config : public Core::JSON::Container {
        private:
            Config(const Config&);
            Config& operator=(const Config&);

        public:
            Config()
                : Core::JSON::Container()
                , Interface(0)
                , HIDPath()
            {
                Add(_T("interface"), &Interface);
                Add(_T("hidpath"), &HIDPath);
            }
            ~Config()
            {
            }

        public:
            Core::JSON::DecUInt8 Interface;
            Core::JSON::String HIDPath;
        };

    public:
        class EXTERNAL DeviceImpl : public IBluetooth::IDevice {
        private:
            DeviceImpl() = delete;
            DeviceImpl(const DeviceImpl&) = delete;
            DeviceImpl& operator=(const DeviceImpl&) = delete;

            static constexpr uint16_t ACTION_MASK = 0x0FFF;

        public:
            static constexpr uint32_t MAX_ACTION_TIMEOUT = 2000; /* 2S to setup a connection ? */

            enum state : uint16_t {
                DECOUPLED = 0x0001,
                METADATA = 0x0002,
                CONNECTING = 0x0004,
                DISCONNECTING = 0x0008,
                CONNECTED = 0x2000,
                PAIRED = 0x4000,
                LOWENERGY = 0x8000
            };

            class JSON : public Core::JSON::Container {
            private:
                JSON& operator=(const JSON&);

            public:
                JSON()
                    : Core::JSON::Container()
                    , Address()
                    , Name()
                    , LowEnergy(false)
                    , Connected(false)
                    , Paired(false)
                    , Reason(0)
                {
                    Add(_T("address"), &Address);
                    Add(_T("name"), &Name);
                    Add(_T("le"), &LowEnergy);
                    Add(_T("connected"), &Connected);
                    Add(_T("paired"), &Paired);
                    Add(_T("reason"), &Reason);
                }
                JSON(const JSON& copy)
                    : Core::JSON::Container()
                    , Address()
                    , Name()
                    , LowEnergy(false)
                    , Connected(false)
                    , Paired(false)
                    , Reason(0)
                {
                    Add(_T("address"), &Address);
                    Add(_T("name"), &Name);
                    Add(_T("le"), &LowEnergy);
                    Add(_T("connected"), &Connected);
                    Add(_T("paired"), &Paired);
                    Add(_T("reason"), &Reason);
                    Address = copy.Address;
                    Name = copy.Name;
                    LowEnergy = copy.LowEnergy;
                    Connected = copy.Connected;
                    Paired = copy.Paired;
                    Reason = copy.Reason;
                }
                virtual ~JSON()
                {
                }

            public:
                JSON& Set(const DeviceImpl* source)
                {
                    if (source != nullptr) {
                        Address = source->Address();
                        Name = source->Name();
                        LowEnergy = source->LowEnergy();
                        Connected = source->IsConnected();
                        Paired = source->IsPaired();
                    } else {
                        Address.Clear();
                        Name.Clear();
                        LowEnergy.Clear();
                        Paired.Clear();
                        Connected.Clear();
                    }
                    return (*this);
                }
                Core::JSON::String Address;
                Core::JSON::String Name;
                Core::JSON::Boolean LowEnergy;
                Core::JSON::Boolean Connected;
                Core::JSON::Boolean Paired;
                Core::JSON::DecUInt16 Reason;
            };

            class IteratorImpl : public IBluetooth::IDevice::IIterator {
            private:
                IteratorImpl() = delete;
                IteratorImpl(const IteratorImpl&) = delete;
                IteratorImpl& operator=(const IteratorImpl&) = delete;

            public:
                IteratorImpl(const std::list<DeviceImpl*>& container)
                {
                    std::list<DeviceImpl*>::const_iterator index = container.begin();
                    while (index != container.end()) {
                        IBluetooth::IDevice* element = (*index);
                        element->AddRef();
                        _list.push_back(element);
                        index++;
                    }
                }
                virtual ~IteratorImpl()
                {
                    while (_list.size() != 0) {
                        _list.front()->Release();
                        _list.pop_front();
                    }
                }

            public:
                virtual void Reset() override
                {
                    _index = 0;
                }
                virtual bool IsValid() const override
                {
                    return ((_index != 0) && (_index <= _list.size()));
                }
                virtual bool Next() override
                {
                    if (_index == 0) {
                        _index = 1;
                        _iterator = _list.begin();
                    } else if (_index <= _list.size()) {
                        _index++;
                        _iterator++;
                    }
                    return (IsValid());
                }
                virtual IBluetooth::IDevice* Current()
                {
                    ASSERT(IsValid() == true);
                    IBluetooth::IDevice* result = nullptr;
                    result = (*_iterator);
                    ASSERT(result != nullptr);
                    result->AddRef();
                    return (result);
                }

                BEGIN_INTERFACE_MAP(IteratorImpl)
                INTERFACE_ENTRY(IBluetooth::IDevice::IIterator)
                END_INTERFACE_MAP

            private:
                uint32_t _index;
                std::list<IBluetooth::IDevice*> _list;
                std::list<IBluetooth::IDevice*>::iterator _iterator;
            };
            class FeatureIterator {
            public:
                FeatureIterator()
                    : _index(-1)
                {
                    ::memset(_features, 0, sizeof(_features));
                }
                FeatureIterator(const uint8_t length, const uint8_t data[])
                    : _index(-1)
                {
                    uint8_t copyLength = std::min(length, static_cast<uint8_t>(sizeof(_features)));
                    ::memcpy(_features, data, copyLength);
                    if (copyLength < sizeof(_features)) {
                        ::memset(&_features[copyLength], 0, (sizeof(_features) - copyLength));
                    }
                }
                FeatureIterator(const FeatureIterator& copy)
                    : _index(copy._index)
                {
                    ::memcpy(_features, copy._features, sizeof(_features));
                }
                ~FeatureIterator()
                {
                }

            public:
                FeatureIterator& operator=(const FeatureIterator& rhs)
                {
                    _index = rhs._index;
                    ::memcpy(_features, rhs._features, sizeof(_features));

                    return (*this);
                }

                void Reset()
                {
                    _index = -1;
                }
                bool IsValid() const
                {
                    return ((_index >= 0) && (_index < static_cast<int16_t>(sizeof(_features) * 8)));
                }
                bool Next()
                {
                    _index++;

                    while ((_index < static_cast<int16_t>(sizeof(_features) * 8)) && ((_features[_index >> 3] & (1 << (_index & 0x7))) == 0)) {
                        _index++;
                    }
                    return (_index < static_cast<int16_t>(sizeof(_features) * 8));
                }
                uint8_t Feature() const
                {
                    return (_index);
                }
                const TCHAR* Text() const
                {
                    uint16_t index = (((index & 0xF8) << 5) | (1 << (_index & 0x7)));
                    return (FeatureToText(index));
                }
                bool HasFeatures(const uint8_t byte, uint8_t bit) const
                {
                    return (byte < sizeof(_features) ? (_features[byte] & bit) != 0 : false);
                }

            private:
                const TCHAR* FeatureToText(const uint16_t index) const;

            private:
                int16_t _index;
                uint8_t _features[8];
            };

        public:
            DeviceImpl(Bluetooth::HCISocket* administrator, Bluetooth::HCISocket* application, const bool lowEnergy, const Bluetooth::Address& address, const string& name)
                : _administrator(administrator)
                , _application(application)
                , _address(address)
                , _name(name)
                , _state(static_cast<state>(lowEnergy ? LOWENERGY : 0))
            {
                ASSERT(_administrator != nullptr);
                ASSERT(_application != nullptr);
            }
            ~DeviceImpl()
            {
            }

        public:
            virtual uint32_t Pair() override
            {
                _state.SetState(PAIRED);
                uint8_t type((_state & LOWENERGY) != 0 ? Bluetooth::Address::LE_PUBLIC_ADDRESS : Bluetooth::Address::BREDR_ADDRESS);

                return (_administrator->Pair(_address, type, Bluetooth::HCISocket::capabilities::NO_INPUT_NO_OUTPUT));
            }
            virtual uint32_t Unpair() override
            {
                return (_administrator->Unpair(_address));
            }
            virtual string Address() const override
            {
                return (_address.ToString());
            }
            virtual string Name() const override
            {
                return (_name);
            }
            virtual bool IsDiscovered() const override
            {
                return ((_state & (METADATA | DECOUPLED)) == 0);
            }
            virtual bool IsPaired() const override
            {
                return ((_state & PAIRED) != 0);
            }
            inline bool LowEnergy() const
            {
                return ((_state & LOWENERGY) != 0);
            }
            inline void Clear()
            {
                _state.Lock();
                if ((IsConnected() == false) && (IsPaired() == false) && ((_state & ACTION_MASK) == 0)) {
                    _state.SetState(static_cast<state>(_state.GetState() | DECOUPLED));
                }
                _state.Unlock();
            }
            inline void Discovered()
            {
                _state.Lock();
                if (((_state & ACTION_MASK) == DECOUPLED) && (_application != nullptr)) {
                    _state.SetState(static_cast<state>(_state.GetState() & (~DECOUPLED)));
                }
                _state.Unlock();
            }
            inline bool operator==(const Bluetooth::Address& rhs) const
            {
                return (_address == rhs);
            }
            inline bool operator!=(const Bluetooth::Address& rhs) const
            {
                return (_address != rhs);
            }

            BEGIN_INTERFACE_MAP(DeviceImpl)
            INTERFACE_ENTRY(IBluetooth::IDevice)
            END_INTERFACE_MAP

            const Bluetooth::Address& Locator() const
            {
                return (_address);
            }
            uint32_t WaitState(const uint32_t state, const uint32_t waitTime)
            {
                return (_state.WaitState(state, waitTime));
            }
            FeatureIterator Features() const
            {
                return (FeatureIterator(static_cast<uint8_t>(sizeof(_features)), _features));
            }

            virtual void Notification(const uint8_t subEvent, const uint16_t length, const uint8_t* dataFrame) = 0;

        protected:
            void SetFeatures(const uint8_t length, const uint8_t feature[])
            {
                uint8_t copyLength = std::min(length, static_cast<uint8_t>(sizeof(_features)));
                ::memcpy(_features, feature, copyLength);
            }
            template <typename MESSAGE>
            uint32_t Send(const uint32_t waitTime, Core::IOutbound::ICallback* callback, const state value, MESSAGE& message)
            {

                uint32_t result = Core::ERROR_ALREADY_RELEASED;

                _state.Lock();

                if (_application != nullptr) {
                    _application->Send(waitTime, static_cast<Core::IOutbound&>(message), callback, &message);
                    result = Core::ERROR_NONE;
                } else if ((_state & ACTION_MASK) == value) {
                    _state.SetState(static_cast<state>(_state.GetState() & (~value)));
                }

                _state.Unlock();

                return (result);
            }
            uint32_t SetState(const state value)
            {
                uint32_t result = Core::ERROR_INPROGRESS;

                _state.Lock();

                if ((_state & ACTION_MASK) == 0) {

                    result = Core::ERROR_NONE;

                    _state.SetState(static_cast<state>(_state.GetState() | value));
                }

                _state.Unlock();

                return (result);
            }
            uint32_t ClearState(const state value)
            {
                uint32_t result = Core::ERROR_INPROGRESS;

                _state.Lock();

                if ((_state & ACTION_MASK) == value) {

                    result = Core::ERROR_NONE;

                    _state.SetState(static_cast<state>(_state.GetState() & (~value)));
                }

                _state.Unlock();

                return (result);
            }
            void SetName(const string& name)
            {
                _name = name;
            }

        private:
            Bluetooth::HCISocket* _administrator;
            Bluetooth::HCISocket* _application;
            Bluetooth::Address _address;
            string _name;
            Core::StateTrigger<state> _state;
            uint8_t _features[8];
        };

        class EXTERNAL DeviceRegular : public DeviceImpl, Core::IOutbound::ICallback {
        private:
            DeviceRegular() = delete;
            DeviceRegular(const DeviceRegular&) = delete;
            DeviceRegular& operator=(const DeviceRegular&) = delete;

        public:
            DeviceRegular(Bluetooth::HCISocket* administrator, Bluetooth::HCISocket* application, const Bluetooth::Address& address, const string& name)
                : DeviceImpl(administrator, application, false, address, name)
                , _handle(~0)
                , _devName(name)
            {

                if (SetState(METADATA) == Core::ERROR_NONE) {

                    _name.Clear();
                    _name->bdaddr = *(Locator().Data());
                    _name->pscan_mode = 0x00;
                    _name->pscan_rep_mode = 0x02;
                    _name->clock_offset = 0x0000;
                    Send(MAX_ACTION_TIMEOUT, this, METADATA, _name);
                }
            }
            virtual ~DeviceRegular()
            {
            }

        public:
            virtual bool IsConnected() const override
            {
                return (_handle != static_cast<uint16_t>(~0));
            }
            virtual uint32_t Connect() override
            {
                uint32_t result = Core::ERROR_INPROGRESS;

                if (SetState(CONNECTING) == Core::ERROR_NONE) {

                    _connect.Clear();
                    _connect->bdaddr = *(Locator().Data());
                    _connect->pkt_type = htobs(HCI_DM1 | HCI_DM3 | HCI_DM5 | HCI_DH1 | HCI_DH3 | HCI_DH5);
                    _connect->pscan_rep_mode = 0x02;
                    _connect->clock_offset = 0x0000;
                    _connect->role_switch = 0x01;
                    result = Send(MAX_ACTION_TIMEOUT, this, CONNECTING, _connect);
                    sleep(5);
                }

                return (result);
            }
            virtual uint32_t Disconnect(const uint16_t reason) override
            {
                uint32_t result = Core::ERROR_INPROGRESS;

                if (SetState(DISCONNECTING) == Core::ERROR_NONE) {
                    _disconnect->handle = htobs(_handle);
                    _disconnect->reason = (reason & 0xFF);
                    result = Send(MAX_ACTION_TIMEOUT, this, DISCONNECTING, _disconnect);
                }

                return (result);
            }

        private:
            virtual void Notification(const uint8_t subEvent, const uint16_t length, const uint8_t* dataFrame)
            {
            }
            virtual void Updated(const Core::IOutbound& data, const uint32_t error_code) override
            {
                if (data.Id() == Bluetooth::HCISocket::Command::RemoteName::ID) {
#if 0 //Commented since name is already in hand.
                    // Metadata is flowing in, handle it..
                    // _cmds.name.Response().bdaddr;
                    const char* longName = reinterpret_cast<const char*>(_name.Response().name);
                    uint8_t index = 0;
                    while (index < HCI_MAX_NAME_LENGTH) {
                        printf("%c", ::isprint(longName[index]) ? longName[index] : '.');
                        index++;
                    }
                    index = 0;
                    while ((index < HCI_MAX_NAME_LENGTH) && (::isprint(longName[index]) != 0)) {
                        index++;
                    }

                    SetName(std::string(longName, index));
                    TRACE(Trace::Information, (_T("Loaded Long Device Name: %s"),longName));
#endif
                    SetName(_devName.c_str());
                    TRACE(Trace::Information, (_T("Loaded Device name :[%s]"),_devName.c_str()));
                    ClearState(METADATA);
                } else if (data.Id() == Bluetooth::HCISocket::Command::Connect::ID) {
                    TRACE(Trace::Information, (_T("Connected")));
                    // looks like we are connected..
                    _handle = _connect.Response().handle;
                    ClearState(CONNECTING);
                } else if (data.Id() == Bluetooth::HCISocket::Command::Disconnect::ID) {
                    if (error_code == Core::ERROR_NONE) {
                        TRACE(Trace::Information, (_T("Disconnected")));
                        ClearState(DISCONNECTING);
                        _handle = ~0;
                    } else {
                        TRACE(Trace::Error, (_T("Disconnected Failed!")));
                    }
                    // Seems no need to clear the state, since the Disconnected Failed.
                    //ClearState(DISCONNECTING);
                }
            }

        private:
            uint16_t _handle;
            Bluetooth::HCISocket::Command::Connect _connect;
            Bluetooth::HCISocket::Command::Disconnect _disconnect;
            Bluetooth::HCISocket::Command::RemoteName _name;
            string _devName;
        };

        class EXTERNAL DeviceLowEnergy : public DeviceImpl, Core::IOutbound::ICallback {
        private:
            DeviceLowEnergy() = delete;
            DeviceLowEnergy(const DeviceLowEnergy&) = delete;
            DeviceLowEnergy& operator=(const DeviceLowEnergy&) = delete;

        public:
            DeviceLowEnergy(Bluetooth::HCISocket* administrator, Bluetooth::HCISocket* application, const Bluetooth::Address& address, const string& name)
                : DeviceImpl(administrator, application, true, address, name)
                , _handle(~0)
            {
                Connect();
            }
            virtual ~DeviceLowEnergy()
            {
                Disconnect(0);
            }

        public:
            virtual bool IsConnected() const override
            {
                return (_handle != static_cast<uint16_t>(~0));
            }

        private:
            virtual uint32_t Connect() override
            {
                uint32_t result = Core::ERROR_INPROGRESS;

                if (SetState(CONNECTING) == Core::ERROR_NONE) {

                    _connect.Clear();
                    _connect->interval = htobs(0x0004);
                    _connect->window = htobs(0x0004);
                    _connect->initiator_filter = 0;
                    _connect->peer_bdaddr_type = LE_PUBLIC_ADDRESS;
                    _connect->peer_bdaddr = *(Locator().Data());
                    _connect->own_bdaddr_type = LE_PUBLIC_ADDRESS;
                    _connect->min_interval = htobs(0x000F);
                    _connect->max_interval = htobs(0x000F);
                    _connect->latency = htobs(0x0000);
                    _connect->supervision_timeout = htobs(0x0C80);
                    _connect->min_ce_length = htobs(0x0001);
                    _connect->max_ce_length = htobs(0x0001);
                    result = Send(MAX_ACTION_TIMEOUT, this, CONNECTING, _connect);
                }

                return (result);
            }
            virtual uint32_t Disconnect(const uint16_t reason) override
            {
                uint32_t result = Core::ERROR_INPROGRESS;

                if (SetState(DISCONNECTING) == Core::ERROR_NONE) {

                    _disconnect->handle = htobs(_handle);
                    _disconnect->reason = reason & 0xFF;
                    result = Send(MAX_ACTION_TIMEOUT, this, DISCONNECTING, _disconnect);
                }

                return (result);
            }
            virtual void Notification(const uint8_t subEvent, const uint16_t length, const uint8_t* dataFrame)
            {
                if ((subEvent == EVT_LE_READ_REMOTE_USED_FEATURES_COMPLETE) && (length >= sizeof(evt_read_remote_features_complete))) {
                    const evt_read_remote_features_complete* info = reinterpret_cast<const evt_read_remote_features_complete*>(dataFrame);
                    if (_handle == info->handle) {
                        SetFeatures(sizeof(info->features), info->features);
                    }
                } else if (subEvent == EVT_LE_ADVERTISING_REPORT) {
                    // const le_advertising_info* advertisingInfo = reinterpret_cast<const le_advertising_info*>(dataFrame);
                    // uint16_t len = advertisingInfo->length;
                    // const uint8_t* buffer = advertisingInfo->data;
                    TRACE(Trace::Error, (_T("++EVT_LE_ADVERTISING_REPORT: What to do?")));
                }
            }
            virtual void Updated(const Core::IOutbound& data, const uint32_t error_code) override
            {
                if (data.Id() == Bluetooth::HCISocket::Command::ConnectLE::ID) {
                    if ((error_code == Core::ERROR_NONE) && (_connect.Response().status == 0)) {
                        _handle = _connect.Response().handle;
                        ClearState(CONNECTING);
                        TRACE(Trace::Information, (_T("Connected using handle: %d"),_handle));
                    } else {
                        TRACE(Trace::Error, (_T("Connec Failed!")));
                    }
                } else if (data.Id() == Bluetooth::HCISocket::Command::Disconnect::ID) {
                    if (error_code == Core::ERROR_NONE) {
                        TRACE(Trace::Information, (_T("Disconnected")));
                        _handle = ~0;
                        ClearState(DISCONNECTING);
                    } else {
                        TRACE(Trace::Error, (_T("Disconnected Failed!")));
                    }
                }
            }

        private:
            Bluetooth::HCISocket::Command::ConnectLE _connect;
            Bluetooth::HCISocket::Command::Disconnect _disconnect;
            uint16_t _handle;
        };

        class EXTERNAL Status : public Core::JSON::Container {
        private:
            Status(const Status&) = delete;
            Status& operator=(const Status&) = delete;

        public:
            Status()
                : Scanning()
                , Devices()
            {
                Add(_T("scanning"), &Scanning);
                Add(_T("devices"), &Devices);
            }
            virtual ~Status()
            {
            }

        public:
            Core::JSON::Boolean Scanning;
            Core::JSON::ArrayType<DeviceImpl::JSON> Devices;
        };

    public:
        BluetoothControl()
            : _skipURL(0)
            , _adminLock()
            , _service(nullptr)
            , _driver(nullptr)
            , _administrator(Core::NodeId(HCI_DEV_NONE, HCI_CHANNEL_CONTROL))
            , _application(*this)
            , _btAddress()
            , _interface()
            , _devices()
            , _observers()
            , _gattRemotes()
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
        virtual IBluetooth::IDevice* Device(const string&) override;
        virtual IBluetooth::IDevice::IIterator* Devices() override;

    private:
        Core::ProxyType<Web::Response> GetMethod(Core::TextSegmentIterator& index);
        Core::ProxyType<Web::Response> PutMethod(Core::TextSegmentIterator& index, const Web::Request& request);
        Core::ProxyType<Web::Response> PostMethod(Core::TextSegmentIterator& index, const Web::Request& request);
        Core::ProxyType<Web::Response> DeleteMethod(Core::TextSegmentIterator& index, const Web::Request& request);
        void RemoveDevices(std::function<bool(DeviceImpl*)> filter);
        void DiscoveredDevice(const bool lowEnergy, const Bluetooth::Address& address, const string& name);
        void Notification(const uint8_t subEvent, const uint16_t length, const uint8_t* dataFrame);
        DeviceImpl* Find(const string&);

    private:
        uint8_t _skipURL;
        Core::CriticalSection _adminLock;
        PluginHost::IShell* _service;
        Bluetooth::Driver* _driver;
        Bluetooth::HCISocket _administrator;
        Scanner _application;
        Bluetooth::Address _btAddress;
        Bluetooth::Driver::Interface _interface;
        std::list<DeviceImpl*> _devices;
        std::list<IBluetooth::INotification*> _observers;
        std::list<GATTRemote> _gattRemotes;
        static string _HIDPath;
    };
} //namespace Plugin

} //namespace Solution
