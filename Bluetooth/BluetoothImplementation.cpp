#include "Module.h"
#include "BluetoothJSONContainer.h"
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/l2cap.h>
#include <bluetooth/mgmt.h>
#include <gio/gio.h>
#include <glib.h>
#include <interfaces/IBluetooth.h>
#include <linux/uhid.h>
#include <linux/input.h>
#include <map>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <thread>
#include <unistd.h>
#include <unordered_set>

#define THREAD_EXIT_LIMIT               100
#define ADAPTER_INDEX                   0X00
#define HCI_DEVICE_ID                   0
#define ENABLE_MODE                     0x01
#define ONE_BYTE                        0X01
#define DATA_BUFFER_LENGTH              23
// SCAN PARAMETERS
#define SCAN_ENABLE                     0x01
#define SCAN_DISABLE                    0x00
#define SCAN_TYPE                       0x01
#define OWN_TYPE                        0X00
#define SCAN_TIMEOUT                    1000
#define SCAN_FILTER_POLICY              0x00
#define SCAN_INTERVAL                   0x0010
#define SCAN_WINDOW                     0x0010
#define SCAN_FILTER_DUPLICATES          1
#define EIR_NAME_SHORT                  0x08
#define EIR_NAME_COMPLETE               0x09
// L2CAP
#define LE_PUBLIC_ADDRESS               0x00
#define LE_RANDOM_ADDRESS               0x01
#define LE_ATT_CID                      4
// GATT
#define ATT_OP_ERROR                    0x01
#define ATT_OP_FIND_INFO_REQ            0x04
#define ATT_OP_FIND_INFO_RESP           0x05
#define ATT_OP_FIND_BY_TYPE_REQ         0x06
#define ATT_OP_FIND_BY_TYPE_RESP        0x07
#define ATT_OP_READ_BY_TYPE_REQ         0x08
#define ATT_OP_READ_BY_TYPE_RESP        0x09
#define ATT_OP_READ_REQ                 0x0A
#define ATT_OP_READ_BLOB_REQ            0x0C
#define ATT_OP_READ_BLOB_RESP           0x0D
#define ATT_OP_WRITE_REQ                0x12
#define ATT_OP_HANDLE_NOTIFY            0x1B
#define ATT_OP_WRITE_RESP               0x13
// UUID
#define PRIMARY_SERVICE_UUID            0x2800
#define HID_UUID                        0x1812
#define PNP_UUID                        0x2a50
#define DEVICE_NAME_UUID                0x2a00
#define REPORT_MAP_UUID                 0x2a4b
#define REPORT_UUID                     0x2a4d
#define UHID_PATH                       "/dev/uhid"

namespace WPEFramework {

namespace Core {

    class SocketBluetooth : public Core::SocketPort {
    private:
        SocketBluetooth() = delete;
        SocketBluetooth(const SocketBluetooth&) = delete;
        SocketBluetooth& operator=(const SocketBluetooth&) = delete;

    public:
    public:
        SocketBluetooth( const Core::NodeId& localNode, const uint16_t bufferSize) :
            SocketPort((localNode.Extension() == BTPROTO_HCI ? SocketPort::RAW : SocketPort::SEQUENCED),localNode, Core::NodeId(), bufferSize, bufferSize) {
        }
        virtual ~SocketBluetooth() {
        }

    public:
        // Methods to extract and insert data into the socket buffers
        virtual uint16_t SendData(uint8_t* dataFrame, const uint16_t maxSendSize) = 0;
        virtual uint16_t ReceiveData(uint8_t* dataFrame, const uint16_t receivedSize) = 0;

        // Signal a state change, Opened, Closed or Accepted
        virtual void StateChange() = 0;
    };

} } // namespace WPEFramework::Core

namespace WPEFramework {

namespace Bluetooth {

        class Address {
        public:
            Address() : _length(0) {
            }
            Address(const int handle) : _length(0) {
                if (handle > 0) {
                    if (hci_devba(handle, &_address) >= 0) {
                        _length = sizeof(_address);
                    }
                }
            }
            Address(const TCHAR address[]) : _length(sizeof(_address)) {
                ::memset(_address, 0, sizeof(_address));
                str2ba(address, &_address);
            }
            Address(const Address& address) : _length(copy._length) {
                ::memcpy (_address, address._address, sizeof(_address));
            }
            ~Address() {
            }

        public:
            bool IsValid() const {
                return (_length == sizeof(_address));
            }
            bool Default() {
                _length = 0;
                int deviceId = hci_get_route(nullptr);
                if ((deviceId >= 0) && (hci_devba(deviceId, &_address) >= 0)) {
                    _length = sizeof(_address);
                }
                return (IsValid());
            }
            const bdaddr_t* Data () const {
                return (IsValid() ? &_address : nullptr);
            }
            uint8_t Length () const {
                return (_length);
            }
            Core::NodeId NodeId() const {
                Core::NodeId result;
                int deviceId = hci_get_route(Data());

                if (deviceId >= 0) {
                    result = Core::NodeId(static_cast<uint16_t>(deviceId), 0);
                }

                return (result);
            }
 
            string ToString() const {
                static constexpr _hexArray[] = "0123456789ABCDEF";
                string result;

                if (IsValid() == true) {
                    for (uint8_t index = 1; index < _length; index++) {
                        if (result.empty() == false) {
                            result += ':';
                        }
                        result += _hexArray[(_address.b[index] >> 4) & 0x0F];
                        result += _hexArray[_address.b[index] & 0x0F];
                    }
                }

                return (result);
            }

        private:
            bdaddr_t _address;
            uint8_t _length;
        };

    struct IOutbound {

        virtual IOutbound() {} = 0;

        virtual uint16_t Serialize(uint8_t stream, const uint16_t length) const = 0;
    }

    class ManagementFrame : public IOutbound {
    private: 
        ManagementFrame() = delete;
        ManagementFrame(const Managment&) = delete;
        ManagementFrame& operator= (const Managment&) = delete;

    public:
        ManagementFrame(const uint16_t index)
            : _offset(0)
            , _maxSize(64)
            , _buffer(reinterpret_cast<uint8_t*>(::malloc(_maxSize))) {
            _mgmtHeader.index = htobs(index);
            _mgmtHeader.len = 0;
        }
        virtual ~Managment() {
            ::free (_buffer);
        }
           
    public:
        virtual uint16_t Serialize(uint8_t stream, const uint16_t length) const {
            uint16_t result = 0;
            if (_offset < sizeof(_mgmtHeader)) {
                uint8_t copyLength = std::min(sizeof(_mgmtHeader) - _offset, length);
                ::memcpy (stream, reinterpret_cast<const uint8_t*>(&_mgmtHeader)[_offset], copyLength);
                result   = copyLength;
                _offset += copyLength;
            }
            if (result < length) {
                uint8_t copyLength = std::min(_mgmtHeader.len - (_offset - sizeof(_mgmtHeader)), length - result);
                if (copyLength > 0) {
                    ::memcpy (&(stream[result]), &(_buffer[_offset - sizeof(_mgmtHeader)]), copyLength);
                    result  += copyLength;
                    _offset += copyLength;
                }
            }
            return (result);
        }
        template<typename VALUE>
        inline ManagementFrame& Set (const uint16_t opcode, const VALUE& value) {
            _offset = 0;

            if (sizeof(VALUE) > _maxSize) {
                ::free(_buffer);
                _maxSize = sizeof(VALUE);
                _buffer = reinterpret_cast<uint8_t*>(::malloc(_maxSize));
            }
            ::memcpy (_buffer, &value, sizeof(VALUE));
            _mgmtHeader.len = htobs(sizeof(VALUE));
            _mgmtHeader.opcode = htobs(opcode);

            return (*this);
        }
    private:
        uint16_t _offset;
        uint16_t _maxSize;
        struct mgmt_hdr _mgmtHeader;
        uint8_t* _buffer;
    };

    class CommandFrame : public IOutbound {
    private: 
        CommandFrame() = delete;
        CommandFrame(const Managment&) = delete;
        CommandFrame& operator= (const Managment&) = delete;

    public:
        CommandFrame()
            : _offset(0)
            , _maxSize(64)
            , _buffer(reinterpret_cast<uint8_t*>(::malloc(_maxSize)))
            , _size(0) {
        }
        virtual ~CommandFrame() {
            ::free (_buffer);
        }
           
    public:
        virtual uint16_t Serialize(uint8_t stream, const uint16_t length) const {
            uint16_t copyLength = std::min(_size - _offset, length);
            if (copyLength > 0) {
                ::memcpy (stream, &(_buffer[_offset]), copyLength);
                _offset += copyLength;
            }
            return (_copyLength);
        }
        template<typename VALUE>
        inline CommandFrame& Set (const uint16_t opcode, const VALUE& value) {
            _offset = 0;

            if (sizeof(VALUE) > _maxSize) {
                ::free(_buffer);
                _maxSize = sizeof(VALUE);
                _buffer = reinterpret_cast<uint8_t*>(::malloc(_maxSize));
            }
            ::memcpy (_buffer, &value, sizeof(VALUE));
            _mgmtHeader.len = htobs(sizeof(VALUE));
            _mgmtHeader.opcode = htobs(opcode);

            return (*this);
        }
    private:
        uint16_t _offset;
        uint16_t _maxSize;
        uint8_t* _buffer;
        uint16_t _size;
    };

    class SynchronousSocket : public SocketBluetooth {
    private:
        SynchronousSocket() = delete;
        SynchronousSocket(const SynchronousSocket&) = delete;
        SynchronousSocket& operator=(const SynchronousSocket&) = delete;

    public:
        SynchronousSocket(const Core::NodeId& localNode, const uint16_t bufferSize) :
            SocketPort((localNode.Extension() == BTPROTO_HCI ? SocketPort::RAW : SocketPort::SEQUENCED), localNode, Core::NodeId(), bufferSize, bufferSize) {
        }
        virtual ~SynchronousSocket() {
        }

        uint32_t Send(const IOutbound& message, const uint32_t waitTime) {

            _adminLock.Lock();

            uint32_t result = ClaimSlot(message, waitTime);

            if (_outbound == &message) {

                ASSERT (result == Core::ERROR_NONE);

                _adminLock.Unlock();

                Trigger();

                result = CompletedSlot (message, waitTime);

                _adminLock.Lock();

                _outbound = nullptr;

                Reevaluate();
            }

            _adminLock.Unlock();

            return (result);
        }

    private:        
        void Reevaluate() {
            _reevaluate.SetEvent();

            while (_waitCount != 0) {
                SleepMs(0);
            }

            _reevaluate.ResetEvent();
        }
        uint32_t ClaimSlot (const IOutbound& request, const uint32_t allowedTime) {
            uint32_t result = Core::ERROR_NONE;

            while ( (IsOpen() == true) && (_outbound != nullptr) && (result == Core::ERROR_NONE) ) {

                _waitCount++;

                _adminLock.Unlock();

                result = _reevaluate.Lock(allowedTime);

                _waitCount--;

                _adminLock.Lock();
            }

            if (_outbound == nullptr) {
                if (IsOpen() == true) {
                    _outbound = &request;
                }
                else {
                    result = Core::ERROR_CONNECTION_CLOSED;
                }
            }

            return (result);
        }
        uint32_t CompletedSlot (const IOutbound& request, const uint32_t allowedTime) {

            uint32_t result = Core::ERROR_NONE;

            if (_outbound == &request) {

                _waitCount++;

                _adminLock.Unlock();

                result == _reevaluate.Lock(allowedTime);

                _waitCount--;

                _outbound.Lock();
            }

            if ((result == Core::ERROR_NONE) && (_outbound == &request)) {
                result = Core::ERROR_ASYNC_ABORTED;
            }

            return (result);
        }

        // Methods to extract and insert data into the socket buffers
        virtual uint16_t SendData(uint8_t* dataFrame, const uint16_t maxSendSize) override
        {
            uint16_t result = 0;

            _responses.Lock();

            if ( (_outbound != nullptr) && (_outound != reinterpret_cast<const IOutbound*>(~0)) ) {
                result = _outbound->Serialize(dataFrame, maxSendSize);

                if (result == 0) {
                    _outbound = reinterpret_cast<const IOutbound*>(~0);
                    Reevaluate();
                }
            }

            _responses.Unlock();

            return (result);
        }
        virtual uint16_t ReceiveData(uint8_t* dataFrame, const uint16_t availableData) override
        {
            return (availableData);
        }
        virtual void StateChange () override
	{
	    if (IsOpen() == false) {
                _reevaluate.SetEvent();
            }
            else {
                _reevaluate.ResetEvent();
            }
        }

    private:
        Core::CriticalSection _adminLock;
        const IOutbound* _outbound;
        Core::Event _reevaluate;
        std::atomic<uint32_t> _waitCount;
    };

    class HCISocket : public SynchronousSocket {
    private:
        HCISocket() = delete;
        HCISocket(const HCISocket&) = delete;
        HCISocket& operator= (const HCISocket&) = delete;

    public:
        HCISocket(const NodeId& source) 
            : SynchronousSocket(source, 256) {
        }
        virtual ~HCISocket() {
        }

    public:
   };

} } // namespace WPEFramework::Bluetooth

namespace WPEFramework {

namespace Plugin {

    class BluetoothImplementation : public Exchange::IBluetooth, public Core::Thread
    {
    private:
        BluetoothImplementation(const BluetoothImplementation&) = delete;
        BluetoothImplementation& operator=(const BluetoothImplementation&) = delete;

    public:
        BluetoothImplementation()
            : _scanning(false)
        {
        }

        virtual ~BluetoothImplementation()
        {
            // TODO
        }

    private:
        struct  handleRange {
            uint16_t startHandle;
            uint16_t endHandle;
        };

        struct  characteristicInfo {
            uint16_t handle;
            uint16_t offset;
        };

        struct deviceInformation {
            string name;
            uint16_t vendorID;
            uint16_t productID;
            uint16_t version;
            uint16_t country;
            std::vector<uint8_t> reportMap;
        };

        struct connectedDeviceInfo {
            string name;
            uint32_t l2capSocket;
            uint32_t connectionHandle;
            uint32_t uhidFD;
            bool connected;
        };

    public:
        virtual uint32_t Configure(PluginHost::IShell* service)
        {
            ManagmentFrame mgmtFrame(ADAPTER_INDEX)
            SynchronousSocket socket (Core::NodeId(HCI_DEV_NONE, HCI_CHANNEL_CONTROL), 128);
            struct mgmt_mode modeFlags;

            modeFlags.val = htobs(ENABLE_MODE);

            if (socket.Send(mgmtFrame.Set(MGMT_OP_SET_POWERED, modeFlags), 1000) != Core::ERROR_NONE) {
                result = "Failed to power on bluetooth adaptor";
            }
            // Enable Bondable on adaptor.
            else if (socket.Send(mgmtFrame.Set(MGMT_OP_SET_BONDABLE, modeFlags), 1000) != Core::ERROR_NONE) {
                result = "Failed to enable Bondable";
            }
            // Enable Simple Secure Simple Pairing.
            else if (socket.Send(mgmtFrame.Set(MGMT_OP_SET_SSP, modeFlags), 1000) != Core::ERROR_NONE) {
                result = "Failed to enable Simple Secure Simple Pairing";
            }
            // Enable Low Energy
            else if (socket.Send(mgmtFrame.Set(MGMT_OP_SET_LE, modeFlags), 1000) != Core::ERROR_NONE) {
                result = "Failed to enable Low Energy";
            }
            // Enable Secure Connections
            else if (socket.Send(mgmtFrame.Set(MGMT_OP_SET_SECURE_CONN, modeFlags), 1000) != Core::ERROR_NONE) {
                result = "Failed to enable Secure Connections";
            }
            // See if we can load the default Bleutooth address
            else if (_address.Default() == false) {
                result = "Could not get the Bluetooth address";
            }
            else {
                _hciSocket.LocalNode(_address.NodeId());

                if (_hciSocket.Open(100) != Core::ERROR_NONE) {
                    result = "Could not open the HCI control socket";
                }
            }                

            return (message);
        }

    private:
        BEGIN_INTERFACE_MAP(BluetoothImplementation)
            INTERFACE_ENTRY(Exchange::IBluetooth)
        END_INTERFACE_MAP

    private:
        bool Scan();
        bool StopScan();
        void GetDeviceName(uint8_t*, ssize_t, char*);
        void ScanningThread();
        string DiscoveredDevices();
        string PairedDevices();
        bool Connect(string);
        bool Disconnect(string);
        bool Pair(string);
        bool IsScanning() { return _scanning; };
        string ConnectedDevices();
        bool CheckForHIDProfile(uint32_t);
        bool ReadHIDDeviceInformation(uint32_t, struct deviceInformation*);
        uint32_t CreateHIDDeviceNode(bdaddr_t, bdaddr_t, struct deviceInformation*);
        bool EnableInputReportNotification(uint32_t);
        void ReadingThread(string);
        bool SendCommand(uint32_t, uint8_t, handleRange, uint16_t, uint16_t, characteristicInfo, ssize_t, uint8_t*, uint8_t);
        bool PairDevice(string, uint32_t&, sockaddr_l2&);

    private:
        Core::NodeId
        uint32_t _hciHandle;
        uint32_t _hciSocket;
        std::atomic<bool> _scanning;
        std::map<string, string> _discoveredDeviceIdMap;
        std::map<string, connectedDeviceInfo> _pairedDeviceInfoMap;
        std::map<string, string> _connectedDeviceInfoMap;
        Core::JSON::ArrayType<BTDeviceInfo> _jsonDiscoveredDevices;
        Core::JSON::ArrayType<BTDeviceInfo> _jsonPairedDevices;
        Core::JSON::ArrayType<BTDeviceInfo> _jsonConnectedDevices;
        Core::CriticalSection _scanningThreadLock;
        };

    void BluetoothImplementation::GetDeviceName(uint8_t* inputData, ssize_t inputDataLength, char* deviceName)
    {
        ssize_t offset = 0;

        while (offset < inputDataLength) {
            uint8_t fieldLength = inputData[0];
            ssize_t nameLength;

            /* Check for the end of EIR */
            if (fieldLength == 0)
                break;

            if (offset + fieldLength > inputDataLength)
                break;

            switch (inputData[1]) {
                case EIR_NAME_SHORT:
                case EIR_NAME_COMPLETE:
                    nameLength = fieldLength - 1;
                    memcpy(deviceName, &inputData[2], nameLength);
                    return;
            }

            offset += fieldLength + 1;
            inputData += fieldLength + 1;
        }
    }

    void BluetoothImplementation::ScanningThread()
    {
        unsigned char hciEventBuf[HCI_MAX_EVENT_SIZE];
        char deviceAddress[20];
        char deviceName[50];
        struct hci_filter oldHciFilter;
        struct hci_filter newHciFilter;
        socklen_t oldHciFilterLen;
        evt_le_meta_event *leMetaEvent;
        le_advertising_info *leAdvertisingInfo;
        BTDeviceInfo jsonDiscoveredDevice;

        // Get old HCI filter.
        oldHciFilterLen = sizeof(oldHciFilter);
        getsockopt(_hciSocket, SOL_HCI, HCI_FILTER, &oldHciFilter, &oldHciFilterLen);

        // Setup new HCI filter.
        hci_filter_clear(&newHciFilter);
        hci_filter_set_ptype(HCI_EVENT_PKT, &newHciFilter);
        hci_filter_set_event(EVT_LE_META_EVENT, &newHciFilter);
        setsockopt(_hciSocket, SOL_HCI, HCI_FILTER, &newHciFilter, sizeof(newHciFilter));

        TRACE(Trace::Information, ("Trying to Lock _scanningThread"));
        _scanningThreadLock.Lock();
        TRACE(Trace::Information, ("Locked scanningThread"));

        while (_scanning) {
            read(_hciSocket, hciEventBuf, sizeof(hciEventBuf));
            leMetaEvent = (evt_le_meta_event *)(hciEventBuf + (1 + HCI_EVENT_HDR_SIZE));

            if (leMetaEvent->subevent != 0x02)
                continue;

            leAdvertisingInfo = (le_advertising_info *)(leMetaEvent->data + 1);
            ba2str(&leAdvertisingInfo->bdaddr,deviceAddress);

            memset(deviceName, 0, sizeof(deviceName));
            GetDeviceName(leAdvertisingInfo->data, leAdvertisingInfo->length, deviceName);

            if (_discoveredDeviceIdMap.find(deviceAddress) == _discoveredDeviceIdMap.end()) {
                TRACE(Trace::Information, ("Discovered Device Name : [%s] Address : [%s]", deviceName, deviceAddress));
                _discoveredDeviceIdMap.insert(std::make_pair(deviceAddress, deviceName));
                jsonDiscoveredDevice.Name = deviceName;
                jsonDiscoveredDevice.Address = deviceAddress;
                _jsonDiscoveredDevices.Add(jsonDiscoveredDevice);
            }
        }

        setsockopt(_hciSocket, SOL_HCI, HCI_FILTER, &oldHciFilter, sizeof(oldHciFilter));
        TRACE(Trace::Information, ("Trying to UnLock scanningThread"));
        _scanningThreadLock.Unlock();
        TRACE(Trace::Information, ("UnLocked scanningThread"));
    }

    bool BluetoothImplementation::Scan()
    {
        std::thread scanningThread;

        if (!_scanning) {
            TRACE(Trace::Information, ("Start BT Scan"));
            // Clearing previously discovered devices.
            _discoveredDeviceIdMap.clear();
            _jsonDiscoveredDevices.Clear();

            if (hci_le_set_scan_parameters(_hciSocket, SCAN_TYPE, htobs(SCAN_INTERVAL), htobs(SCAN_WINDOW), OWN_TYPE, SCAN_FILTER_POLICY, SCAN_TIMEOUT) < 0) {
                TRACE(Trace::Error, ("Failed to Set Scan Parameters"));
                return false;
            }

            if (hci_le_set_scan_enable(_hciSocket, SCAN_ENABLE, SCAN_FILTER_DUPLICATES, SCAN_TIMEOUT) < 0) {
                TRACE(Trace::Error, ("Failed to Start Scan"));
                return false;
            }

            _scanning = true;
            // Starting Scan Thread.
            scanningThread = std::thread(&BluetoothImplementation::ScanningThread, this);
            scanningThread.detach();

            TRACE(Trace::Information, ("BT Scan Started"));
            return true;
        } else {
            TRACE(Trace::Error, ("Scanning already started!"));
            return false;
        }
    }

    bool BluetoothImplementation::StopScan()
    {
        if (_scanning) {
            TRACE(Trace::Information, ("Stop BT Scan"));
            _scanning = false;
            TRACE(Trace::Information, ("Trying to Lock _scanningThread"));
            _scanningThreadLock.Lock();
            TRACE(Trace::Information, ("Locked scanningThread"));
            if (hci_le_set_scan_enable(_hciSocket, SCAN_DISABLE, SCAN_FILTER_DUPLICATES, SCAN_TIMEOUT) < 0) {
                TRACE(Trace::Error, ("Failed to Stop Scan"));
                TRACE(Trace::Information, ("Trying to UnLock scanningThread"));
                _scanningThreadLock.Unlock();
                TRACE(Trace::Information, ("UnLocked scanningThread"));
                return false;
            }
            TRACE(Trace::Information, ("Trying to UnLock scanningThread"));
            _scanningThreadLock.Unlock();
            TRACE(Trace::Information, ("UnLocked scanningThread"));
            TRACE(Trace::Information, ("Stopped BT Scan"));
            return true;
        } else {
            TRACE(Trace::Error, ("Scanning is not started!"));
            return false;
        }
    }

    string BluetoothImplementation::DiscoveredDevices()
    {
        std::string deviceInfoList;

        _jsonDiscoveredDevices.ToString(deviceInfoList);
        return deviceInfoList;
    }

    bool BluetoothImplementation::PairDevice(string deviceId, uint32_t& l2capSocket, sockaddr_l2& destinationSocket)
    {

        struct sockaddr_l2 sourceSocket;
        struct bt_security btSecurity;
        bdaddr_t sourceAddress;
        bdaddr_t destinationAddress;

        l2capSocket = socket(PF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
        if (l2capSocket < 0) {
            TRACE(Trace::Error, ("Failed to Open L2CAP Socket"));
            return false;
        }

        memset(&sourceSocket, 0, sizeof(sourceSocket));
        hci_devba(_hciHandle, &sourceAddress);
        sourceSocket.l2_bdaddr = sourceAddress;
        sourceSocket.l2_family=AF_BLUETOOTH;
        sourceSocket.l2_cid = htobs(LE_ATT_CID);
        sourceSocket.l2_bdaddr_type = BDADDR_LE_PUBLIC;

        if (bind(l2capSocket, (sockaddr*)&sourceSocket , sizeof(sourceSocket)) < 0) {
            TRACE(Trace::Error, ("Failed to Bind the Device [%s]", deviceId.c_str()));
            close(l2capSocket);
            return false;
        }

        memset(&destinationSocket, 0, sizeof(destinationSocket));
        destinationSocket.l2_family = AF_BLUETOOTH;
        destinationSocket.l2_psm = 0;
        destinationSocket.l2_cid = htobs(LE_ATT_CID);
        destinationSocket.l2_bdaddr_type = BDADDR_LE_PUBLIC;
        str2ba(deviceId.c_str(), &destinationSocket.l2_bdaddr);

        memset(&btSecurity, 0, sizeof(btSecurity));
        btSecurity.level = BT_SECURITY_MEDIUM;
        if (setsockopt(l2capSocket, SOL_BLUETOOTH, BT_SECURITY, &btSecurity, sizeof(btSecurity))) {
            TRACE(Trace::Error, ("Failed to set L2CAP Security level for Device [%s]", deviceId.c_str()));
            close(l2capSocket);
            return false;
        }

        return true;
    }

    bool BluetoothImplementation::Pair(string deviceId)
    {
        struct sockaddr_l2 destinationSocket;
        struct l2cap_conninfo l2capConnInfo;
        struct connectedDeviceInfo connectedDevice;
        socklen_t l2capConnInfoLen;
        BTDeviceInfo jsonConnectedDevice;
        bdaddr_t sourceAddress;
        bdaddr_t destinationAddress;
        std::thread readingThread;

        if (!PairDevice(deviceId, connectedDevice.l2capSocket, destinationSocket)) {
            TRACE(Trace::Error, ("Failed to Pair the Device [%s]", deviceId.c_str()));
            return false;
        }

        if (connect(connectedDevice.l2capSocket, (sockaddr*)&destinationSocket, sizeof(destinationSocket)) != 0) {
            TRACE(Trace::Error, ("Failed to Connect the Device [%s]", deviceId.c_str()));
            close(connectedDevice.l2capSocket);
            return false;
        }

        connectedDevice.connected = true;

        l2capConnInfoLen = sizeof(l2capConnInfo);
        getsockopt(connectedDevice.l2capSocket, SOL_L2CAP, L2CAP_CONNINFO, &l2capConnInfo, &l2capConnInfoLen);
        TRACE(Trace::Information, ("L2CAP Connection Handle of Device [%s] is [%d]", deviceId.c_str(), l2capConnInfo.hci_handle));
        connectedDevice.connectionHandle = l2capConnInfo.hci_handle;

        hci_devba(_hciHandle, &sourceAddress);
        str2ba(deviceId.c_str(), &destinationAddress);
        if (CheckForHIDProfile(connectedDevice.l2capSocket)) {
            struct deviceInformation hidDeviceInfo;
            memset(&hidDeviceInfo, 0, sizeof(hidDeviceInfo));

            if (!ReadHIDDeviceInformation(connectedDevice.l2capSocket, &hidDeviceInfo)) {
                TRACE(Trace::Error, ("Failed to read HID Device Information"));
                close(connectedDevice.l2capSocket);
                return false;
            }

            connectedDevice.uhidFD = CreateHIDDeviceNode(sourceAddress, destinationAddress, &hidDeviceInfo);
            if (connectedDevice.uhidFD == 0) {
                TRACE(Trace::Error, ("Failed to create HID Device node"));
                close(connectedDevice.l2capSocket);
                return false;
            }

            if (!EnableInputReportNotification(connectedDevice.l2capSocket)) {
                TRACE(Trace::Error, ("Failed to enable input report notification"));
                close(connectedDevice.l2capSocket);
                return false;
            }

            if (_connectedDeviceInfoMap.find(deviceId.c_str()) == _connectedDeviceInfoMap.end()) {
                connectedDevice.name = hidDeviceInfo.name;
                _pairedDeviceInfoMap.insert(std::make_pair(deviceId.c_str(), connectedDevice));
                _connectedDeviceInfoMap.insert(std::make_pair(deviceId.c_str(), hidDeviceInfo.name));
                jsonConnectedDevice.Address = deviceId;
                jsonConnectedDevice.Name = hidDeviceInfo.name;
                _jsonPairedDevices.Add(jsonConnectedDevice);
                _jsonConnectedDevices.Add(jsonConnectedDevice);
            }

            readingThread= std::thread(&BluetoothImplementation::ReadingThread, this, deviceId);
            readingThread.detach();

            TRACE(Trace::Information, ("Connected BT Device [%s]", deviceId.c_str()));
            return true;

        } else {
            TRACE(Trace::Error, ("The Device [%s] doesn't support HID Profile", deviceId.c_str()));
            close(connectedDevice.l2capSocket);
            return false;
        }

        TRACE(Trace::Information, ("Paired BT Device. Device ID : [%s]", deviceId.c_str()));
        return true;
    }

    string BluetoothImplementation::PairedDevices()
    {
        std::string deviceInfoList;

        _jsonPairedDevices.ToString(deviceInfoList);
        return deviceInfoList;
    }

    bool BluetoothImplementation::SendCommand(uint32_t l2capSocket, uint8_t opcode, handleRange param1, uint16_t param2, uint16_t param3, characteristicInfo param4, ssize_t paramLength, uint8_t* data, uint8_t response)
    {
        struct iovec iov[paramLength];
        struct iovec iov1;
        int dataLength;

        iov[0].iov_base = &opcode;
        iov[0].iov_len = sizeof(opcode);

        if (opcode == ATT_OP_READ_BLOB_REQ || opcode == ATT_OP_WRITE_REQ) {
            iov[1].iov_base = &param4;
            iov[1].iov_len = sizeof(param4);

        } else {
            iov[1].iov_base = &param1;
            iov[1].iov_len = sizeof(param1);
        }

        if (paramLength > 2) {
            iov[2].iov_base = &param2;
            iov[2].iov_len = sizeof(param2);

            if (paramLength == 4) {
                iov[3].iov_base = &param3;
                iov[3].iov_len = sizeof(param3);
            }
        }

        if (writev(l2capSocket, iov, paramLength) < 0) {
            TRACE(Trace::Error, ("Couldn't Write to L2CAP Socket"));
            return false;
        }

        memset(data, 0, DATA_BUFFER_LENGTH);
        iov1.iov_base = data;
        iov1.iov_len = DATA_BUFFER_LENGTH;

        while (dataLength = readv(l2capSocket, &iov1, 1)) {
            if (dataLength < 0) {
                TRACE(Trace::Error, ("Couldn't Read to L2CAP Socket"));
                return false;
            }

            if (data[0] == ATT_OP_ERROR) {
                TRACE(Trace::Information, ("SendCommand Error [ATT_OP_ERROR], Opcode - [%02x]", opcode));
                break;
            }

            if (data[0] == response) {
#if 0
                // For Debugging Purpose.
                for (ssize_t index=0 ; index < DATA_BUFFER_LENGTH ; index++)
                    TRACE(Trace::Information, (" %02x",data[index]));
#endif
                break;
            }
        }

        return true;
    }

    bool BluetoothImplementation::CheckForHIDProfile(uint32_t l2capSocket)
    {
        uint8_t dataBuffer[DATA_BUFFER_LENGTH];
        struct handleRange range;
        struct characteristicInfo characteristic;

        range.startHandle = htobs(0x0001);
        range.endHandle = htobs(0xffff);
        memset(&characteristic, 0, sizeof(characteristic));

        // Checking for HID Profile.
        SendCommand(l2capSocket, htobs(ATT_OP_FIND_BY_TYPE_REQ), range, htobs(PRIMARY_SERVICE_UUID), htobs(HID_UUID), characteristic, 4, dataBuffer, htobs(ATT_OP_FIND_BY_TYPE_RESP));
        if (dataBuffer[0] == ATT_OP_ERROR) {
            TRACE(Trace::Error, ("The Device not support HID Profile"));
            return false;
        } else if (dataBuffer[0] == ATT_OP_FIND_BY_TYPE_RESP) {
            TRACE(Trace::Information, ("The Device support HID Profile"));
            return true;
        }
    }

    bool BluetoothImplementation::ReadHIDDeviceInformation(uint32_t l2capSocket, struct deviceInformation* deviceInfo)
    {
        uint16_t nameHandle;
        uint16_t reportMapHandle;
        uint16_t reportMapOffsets[] = { 0x00, 0x16, 0x2c, 0x42, 0x58, 0x6e, 0x84, 0x9a };
        uint8_t deviceName[DATA_BUFFER_LENGTH];
        uint8_t dataBuffer[DATA_BUFFER_LENGTH];

        struct handleRange range;
        range.startHandle = htobs(0x0001);
        range.endHandle = htobs(0xffff);

        struct characteristicInfo characteristic;
        memset(&characteristic, 0, sizeof(characteristic));

        // Getting Device Vendor ID, Product ID, Version.
        SendCommand(l2capSocket, htobs(ATT_OP_READ_BY_TYPE_REQ), range, htobs(PNP_UUID), 0, characteristic, 3, dataBuffer, htobs(ATT_OP_READ_BY_TYPE_RESP));
        memcpy(&deviceInfo->vendorID, &dataBuffer[5], sizeof(deviceInfo->vendorID));
        memcpy(&deviceInfo->productID, &dataBuffer[7], sizeof(deviceInfo->productID));
        memcpy(&deviceInfo->version, &dataBuffer[9], sizeof(deviceInfo->version));

        // Getting Device Name.
        SendCommand(l2capSocket, htobs(ATT_OP_READ_BY_TYPE_REQ), range, htobs(DEVICE_NAME_UUID), 0, characteristic, 3, dataBuffer, htobs(ATT_OP_READ_BY_TYPE_RESP));
        memset(deviceName, 0, sizeof(deviceName));
        memcpy(&nameHandle, &dataBuffer[2], sizeof(nameHandle));
        TRACE(Trace::Information, ("Device Name handle is [0x%02x]", nameHandle));

        characteristic.handle = htobs(nameHandle);
        characteristic.offset = htobs(0x00);
        SendCommand(l2capSocket, htobs(ATT_OP_READ_BLOB_REQ), range, 0, 0, characteristic, 2, dataBuffer, htobs(ATT_OP_READ_BLOB_RESP));
        memcpy(&deviceName, &dataBuffer[1], sizeof(deviceName));
        std::stringstream nameString;
        nameString << deviceName;
        deviceInfo->name = nameString.str();
        TRACE(Trace::Information, ("Device Name is [%s]", deviceInfo->name.c_str()));
        deviceInfo->country = 0;

        // Getting Report Descriptor Map.
        range.startHandle = htobs(0x0001);
        range.endHandle = htobs(0xffff);
        SendCommand(l2capSocket, ATT_OP_READ_BY_TYPE_REQ, range, htobs(REPORT_MAP_UUID), 0, characteristic, 3, dataBuffer, htobs(ATT_OP_READ_BY_TYPE_RESP));
        memcpy(&reportMapHandle, &dataBuffer[2], sizeof(reportMapHandle));
        TRACE(Trace::Information, ("Device Report Descriptor Map handle is [0x%02x]", reportMapHandle));

        ssize_t index = 0;
        characteristic.handle = htobs(reportMapHandle);
        for (index=0; index < 8; index++) {
            characteristic.offset = htobs(reportMapOffsets[index]);
            SendCommand(l2capSocket, htobs(ATT_OP_READ_BLOB_REQ), range, 0, 0, characteristic, 2, dataBuffer, htobs(ATT_OP_READ_BLOB_RESP));

            for (ssize_t count=1; count < DATA_BUFFER_LENGTH; count++)
                deviceInfo->reportMap.push_back(dataBuffer[count]);
        }

        TRACE(Trace::Information, ("Device Information : Name - [%s] Vendor ID - [%d] Product ID - [%d] Version - [%d]", deviceInfo->name.c_str(), deviceInfo->vendorID, deviceInfo->productID, deviceInfo->version));

#if 0
        // Commenting this Information as it is not needed for normal case.
        TRACE(Trace::Information, ("Report Map :"));
        for (index=0; index < deviceInfo->reportMap.size(); index++)
            TRACE(Trace::Critical, (" %02x", deviceInfo->reportMap[index]));
#endif
        return true;
    }

    uint32_t BluetoothImplementation::CreateHIDDeviceNode(bdaddr_t sourceAddress, bdaddr_t destinationAddress, struct deviceInformation* hidDeviceInfo)
    {
        struct uhid_event uhidEvent;
        uint8_t reportDescriptor[250];
        uint32_t uhidFD;

        uhidFD = open(UHID_PATH, O_RDWR | O_CLOEXEC);

        if (uhidFD < 0) {
            TRACE(Trace::Error, ("Failed to open UHID node"));
            return -1;
        }

        // Creating UHID Node.
        memset(&uhidEvent, 0, sizeof(uhidEvent));
        uhidEvent.type = UHID_CREATE;
        uhidEvent.u.create.bus = BUS_BLUETOOTH;
        uhidEvent.u.create.vendor = hidDeviceInfo->vendorID;
        uhidEvent.u.create.product = hidDeviceInfo->productID;
        uhidEvent.u.create.version = hidDeviceInfo->version;
        uhidEvent.u.create.country = hidDeviceInfo->country;
        std::copy(hidDeviceInfo->reportMap.begin(), hidDeviceInfo->reportMap.end(), reportDescriptor);
        uhidEvent.u.create.rd_data = reportDescriptor;
        uhidEvent.u.create.rd_size = sizeof(reportDescriptor);
        strcpy((char*)uhidEvent.u.create.name, hidDeviceInfo->name.c_str());
        ba2str(&sourceAddress, (char *)uhidEvent.u.create.phys);
        ba2str(&destinationAddress, (char *)uhidEvent.u.create.uniq);

        if (write(uhidFD, &uhidEvent, sizeof(uhidEvent)) < 0) {
            TRACE(Trace::Error, ("Failed to create UHID node"));
            close(uhidFD);
            return -1;
        }

        TRACE(Trace::Information, ("Successfully created UHID node"));
        return uhidFD;
    }

    bool BluetoothImplementation::EnableInputReportNotification(uint32_t l2capSocket)
    {
        uint16_t clientConfigurationHandle;
        uint16_t reportHandle;
        uint8_t dataBuffer[DATA_BUFFER_LENGTH];

        struct handleRange range;
        range.startHandle = htobs(0x0001);
        range.endHandle = htobs(0xffff);

        struct characteristicInfo characteristic;
        memset(&characteristic, 0, sizeof(characteristic));

        // Enable Input Report Notifications.
        SendCommand(l2capSocket, htobs(ATT_OP_READ_BY_TYPE_REQ), range, htobs(REPORT_UUID), 0, characteristic, 3, dataBuffer, htobs(ATT_OP_READ_BY_TYPE_RESP));
        int reportHandles[] = { dataBuffer[2], dataBuffer[2] + 4, dataBuffer[2] + 8 };

        for (auto iterator : reportHandles) {
            memcpy(&reportHandle, &iterator, sizeof(reportHandle));
            clientConfigurationHandle = reportHandle + 1;
            TRACE(Trace::Information, ("Report Handle - [0x%02x] Client Characteristic Configuration Handle - [0x%02x]", reportHandle, clientConfigurationHandle));

            characteristic.handle = htobs(clientConfigurationHandle);
            characteristic.offset = htobs(ENABLE_MODE);
            SendCommand(l2capSocket, htobs(ATT_OP_WRITE_REQ), range, 0, 0, characteristic, 2, dataBuffer, htobs(ATT_OP_WRITE_RESP));
        }

        return true;
    }

    void BluetoothImplementation::ReadingThread(string deviceId)
    {
        struct iovec iov;
        uint8_t dataBuffer[4096];
        struct pollfd pollSocket;
        int result;
        int dataLength;
        int count;

        const auto& iterator = _pairedDeviceInfoMap.find(deviceId.c_str());
        if (iterator != _pairedDeviceInfoMap.end()) {
            pollSocket.fd = iterator->second.l2capSocket;
            pollSocket.events = POLLIN;

            TRACE(Trace::Information, ("Starting Reading Thread for device : [%s]", deviceId.c_str()));

            while (result = poll(&pollSocket, 1, -1)) {

                if (!iterator->second.connected)
                    break;

                if (result < 0) {
                    TRACE(Trace::Error, ("Cannot Poll for L2CAP Socket"));
                    break;
                }

                if (pollSocket.revents & POLLIN) {
                    dataLength = read(iterator->second.l2capSocket, dataBuffer, sizeof(dataBuffer));

                    if (dataBuffer[0] == ATT_OP_HANDLE_NOTIFY) {
                        struct uhid_event uhidEvent;
                        memset(&uhidEvent, 0, sizeof(uhidEvent));
                        uhidEvent.type = UHID_INPUT;
                        uhidEvent.u.input.size = dataLength;
                        uhidEvent.u.input.data[0] = 0x01;
                        count = 0;
                        for (ssize_t index=2; index < dataLength ; index++) {
                            if (dataBuffer[index] != 0x00) {
                                count++;
                                uhidEvent.u.input.data[count] = dataBuffer[index];
                            }
                        }

                        iov.iov_base = &uhidEvent;
                        iov.iov_len = sizeof(uhidEvent);

                        if (writev(iterator->second.uhidFD, &iov, 1) < 0)
                            TRACE(Trace::Error, ("Cannot write to UHID Node"));
                    }
                }
            }

            TRACE(Trace::Information, ("Reading Thread Finished for device : [%s]", deviceId.c_str()));
        } else
            TRACE(Trace::Information, ("Failed to start Reading Thread for device : [%s]", deviceId.c_str()));
    }


    bool BluetoothImplementation::Connect(string deviceId)
    {
        struct l2cap_conninfo l2capConnInfo;
        socklen_t l2capConnInfoLen;
        std::thread readingThread;
        struct sockaddr_l2 destinationSocket;
        uint32_t l2capSocket;

        const auto& iterator = _pairedDeviceInfoMap.find(deviceId.c_str());
        if (iterator != _pairedDeviceInfoMap.end()) {

            if (!PairDevice(deviceId, l2capSocket, destinationSocket)) {
                TRACE(Trace::Error, ("Failed to Pair the Device [%s]", deviceId.c_str()));
                return false;
            }

            if (connect(l2capSocket, (sockaddr*)&destinationSocket, sizeof(destinationSocket)) != 0) {
                TRACE(Trace::Error, ("Failed to Connect the Device [%s]", deviceId.c_str()));
                close(l2capSocket);
                return false;
            }

            iterator->second.connected = true;

            l2capConnInfoLen = sizeof(l2capConnInfo);
            getsockopt(l2capSocket, SOL_L2CAP, L2CAP_CONNINFO, &l2capConnInfo, &l2capConnInfoLen);
            TRACE(Trace::Information, ("L2CAP Connection Handle of Device [%s] is [%d]", deviceId.c_str(), l2capConnInfo.hci_handle));
            iterator->second.connectionHandle = l2capConnInfo.hci_handle;

            _connectedDeviceInfoMap.insert(std::make_pair(deviceId.c_str(), iterator->second.name));

            readingThread= std::thread(&BluetoothImplementation::ReadingThread, this, deviceId);
            readingThread.detach();

        }
        return true;
    }

    string BluetoothImplementation::ConnectedDevices()
    {
        std::string deviceInfoList;
        BTDeviceInfo jsonConnectedDevice;

        _jsonConnectedDevices.Clear();
        if (_connectedDeviceInfoMap.size() > 0) {
            for (auto& connectedDevice : _connectedDeviceInfoMap) {
                jsonConnectedDevice.Address = connectedDevice.first;
                jsonConnectedDevice.Name = connectedDevice.second;
                _jsonConnectedDevices.Add(jsonConnectedDevice);
            }
        }

        _jsonConnectedDevices.ToString(deviceInfoList);
        return deviceInfoList;
    }

    bool BluetoothImplementation::Disconnect(string deviceId)
    {
        const auto& iterator = _pairedDeviceInfoMap.find(deviceId.c_str());
        if (iterator != _pairedDeviceInfoMap.end()) {
            hci_disconnect(_hciSocket, iterator->second.connectionHandle, 0, SCAN_TIMEOUT);
            iterator->second.connected = false;
            close(iterator->second.l2capSocket);
            TRACE(Trace::Information, ("Updating Connected Device Info Map"));
            _connectedDeviceInfoMap.erase(deviceId);

            TRACE(Trace::Information, ("Disconnected BT Device. Device ID : [%s]", deviceId.c_str()));

            return true;
        }

        TRACE(Trace::Error, ("Invalid BT Device ID. Device ID : [%s]", deviceId.c_str()));
        return false;
    }

    SERVICE_REGISTRATION(BluetoothImplementation, 1, 0);

}
} /* namespace WPEFramework::Plugin */
