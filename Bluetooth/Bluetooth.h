#pragma once

#include "Module.h"
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/mgmt.h>

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

} // namespace Core

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
                ::memset(&_address, 0, sizeof(_address));
                str2ba(address, &_address);
            }
            Address(const Address& address) : _length(address._length) {
                ::memcpy (&_address, &(address._address), sizeof(_address));
            }
            ~Address() {
            }

        public:
            Address& operator= (const Address& rhs) {
                _length = rhs._length;
                ::memcpy (&_address, &(rhs._address), sizeof(_address));
                return (*this);
            }
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
                int deviceId = hci_get_route(const_cast<bdaddr_t*>(Data()));

                if (deviceId >= 0) {
                    result = Core::NodeId(static_cast<uint16_t>(deviceId), 0);
                }

                return (result);
            }
            bool operator== (const Address& rhs) const {
                return ((_length == rhs._length) && (memcmp(rhs._address.b, _address.b, _length) == 0));
            }
            bool operator!= (const Address& rhs) const {
                return(!operator==(rhs));
            }
 
            string ToString() const {
                static constexpr TCHAR _hexArray[] = "0123456789ABCDEF";
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

        virtual ~IOutbound() {};

        virtual uint16_t Serialize(uint8_t stream[], const uint16_t length) const = 0;
    };

    class ManagementFrame : public IOutbound {
    private: 
        ManagementFrame() = delete;
        ManagementFrame(const ManagementFrame&) = delete;
        ManagementFrame& operator= (const ManagementFrame&) = delete;

    public:
        ManagementFrame(const uint16_t index)
            : _offset(0)
            , _maxSize(64)
            , _buffer(reinterpret_cast<uint8_t*>(::malloc(_maxSize))) {
            _mgmtHeader.index = htobs(index);
            _mgmtHeader.len = 0;
        }
        virtual ~ManagementFrame() {
            ::free (_buffer);
        }
           
    public:
        virtual uint16_t Serialize(uint8_t stream[], const uint16_t length) const {
            uint16_t result = 0;
            if (_offset < sizeof(_mgmtHeader)) {
                uint8_t copyLength = std::min(static_cast<uint8_t>(sizeof(_mgmtHeader) - _offset), static_cast<uint8_t>(length));
                ::memcpy (stream, &(reinterpret_cast<const uint8_t*>(&_mgmtHeader)[_offset]), copyLength);
                result   = copyLength;
                _offset += copyLength;
            }
            if (result < length) {
                uint8_t copyLength = std::min(static_cast<uint8_t>(_mgmtHeader.len - (_offset - sizeof(_mgmtHeader))), static_cast<uint8_t>(length - result));
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
        mutable uint16_t _offset;
        uint16_t _maxSize;
        struct mgmt_hdr _mgmtHeader;
        uint8_t* _buffer;
    };

    class CommandFrame : public IOutbound {
    private: 
        CommandFrame(const CommandFrame&) = delete;
        CommandFrame& operator= (const CommandFrame&) = delete;

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
        virtual uint16_t Serialize(uint8_t stream[], const uint16_t length) const {
            uint16_t copyLength = std::min(static_cast<uint16_t>(_size - _offset), length);
            if (copyLength > 0) {
                ::memcpy (stream, &(_buffer[_offset]), copyLength);
                _offset += copyLength;
            }
            return (copyLength);
        }
        void Clear() {
            _offset = 0;
            _size = 0;
        }
        template<typename VALUE>
        inline CommandFrame& Add(const VALUE& value) {

            if ((sizeof(VALUE) + _size) > _maxSize) {
                _maxSize = _maxSize + (8 * sizeof(VALUE));
                uint8_t* newBuffer = reinterpret_cast<uint8_t*>(::malloc(_maxSize));
                ::memcpy(newBuffer, _buffer, _size);
                ::free(_buffer);
            }
            ::memcpy (&(_buffer[_size]), &value, sizeof(VALUE));
            _size += sizeof(VALUE);

            return (*this);
        }
    private:
        mutable uint16_t _offset;
        uint16_t _maxSize;
        uint8_t* _buffer;
        uint16_t _size;
    };

    class SynchronousSocket : public Core::SocketBluetooth {
    private:
        SynchronousSocket() = delete;
        SynchronousSocket(const SynchronousSocket&) = delete;
        SynchronousSocket& operator=(const SynchronousSocket&) = delete;

    public:
        SynchronousSocket(const Core::NodeId& localNode, const uint16_t bufferSize)
            : Core::SocketBluetooth(localNode, bufferSize)
            , _adminLock()
            , _outbound(nullptr)
            , _reevaluate (false, true)
            , _waitCount(0) {
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

                _adminLock.Lock();
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

            _adminLock.Lock();

            if ( (_outbound != nullptr) && (_outbound != reinterpret_cast<const IOutbound*>(~0)) ) {
                result = _outbound->Serialize(dataFrame, maxSendSize);

                if (result == 0) {
                    _outbound = reinterpret_cast<const IOutbound*>(~0);
                    Reevaluate();
                }
            }

            _adminLock.Unlock();

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
        HCISocket(const Core::NodeId& source) 
            : SynchronousSocket(source, 256) {
        }
        virtual ~HCISocket() {
        }

    public:
   };

} // namespace Bluetooth

} // namespace WPEFramework
