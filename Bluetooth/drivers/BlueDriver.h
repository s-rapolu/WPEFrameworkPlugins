#pragma once

#include "Module.h"
#include "MessageExchange.h"

namespace WPEFramework {

namespace Bluetooth {

#define HCIUARTSETPROTO		_IOW('U', 200, int)
#define HCIUARTGETPROTO		_IOR('U', 201, int)
#define HCIUARTGETDEVICE	_IOR('U', 202, int)
#define HCIUARTSETFLAGS		_IOW('U', 203, int)
#define HCIUARTGETFLAGS		_IOR('U', 204, int)

#define HCI_UART_H4     0
#define HCI_UART_BCSP   1
#define HCI_UART_3WIRE  2
#define HCI_UART_H4DS   3
#define HCI_UART_LL     4
#define HCI_UART_ATH3K  5
#define HCI_UART_INTEL  6
#define HCI_UART_BCM    7
#define HCI_UART_QCA    8
#define HCI_UART_AG6XX  9
#define HCI_UART_NOKIA  10
#define HCI_UART_MRVL   11

class EXTERNAL Driver {
private:
    Driver(const Driver&) = delete;
    Driver& operator= (const Driver&) = delete;

protected:
    Driver() {
    }

public:
    virtual ~Driver() {
    }

public:
    static Driver* Instance (const string& config);
};

class EXTERNAL SerialDriver : public Driver {
private:
    SerialDriver() = delete;
    SerialDriver(const SerialDriver&) = delete;
    SerialDriver& operator= (const SerialDriver&) = delete;

    class Channel : public Core::MessageExchangeType<Core::SerialPort, Exchange::Bluetooth> {
    private:
        Channel() = delete;
        Channel(const Channel&) = delete;
        Channel& operator=(const Channel&) = delete;

        typedef MessageExchangeType<Core::SerialPort, Exchange::Bluetooth> BaseClass;

    public:
        Channel(SerialDriver& parent, const string& name, const Core::SerialPort::BaudRate baudrate, const Core::SerialPort::FlowControl flowControl)
            : BaseClass ()
            , _parent(parent) {
            printf("Opening port: %s\n", name.c_str());
            Link().Configuration(name, baudrate, flowControl, 64, 64);
        }
        virtual ~Channel() {
        }

    public:
        inline void SetBaudrate(const Core::SerialPort::BaudRate baudrate) {
            Link().SetBaudrate(baudrate);
        }
        void Flush() {
            Link().Flush();
        }
        void SendBreak() {
            Link().SendBreak();
        }
        int Control(int request, ...) {
            va_list args;
            va_start(args, request);
            int result = Link().Control(request, args);
            va_end(args);
            return (result);
        }


    private:
        virtual void Received(const Exchange::Bluetooth::Buffer& element) override{
            _parent.Received(element);
        }

    private:
        SerialDriver& _parent;
    };

protected:
    SerialDriver(const string& port, const Core::SerialPort::BaudRate baudRate, const Core::SerialPort::FlowControl flowControl, const bool sendBreak)
        : Driver()
        , _port(*this, port, baudRate, flowControl) {
        if (_port.Open(100) == Core::ERROR_NONE) {
            if (sendBreak == true) {
        fprintf(stderr, "%s -- %d -- \n", __FUNCTION__, __LINE__);
                _port.SendBreak();
        fprintf(stderr, "%s -- %d -- \n", __FUNCTION__, __LINE__);
                SleepMs(500);
            }
            _port.Flush();
        }
    }

public:
    using Message = Exchange::Bluetooth;

    virtual ~SerialDriver() {
        _port.Close(Core::infinite);
    }


public:
    uint32_t Initialize(const Core::SerialPort::BaudRate baudRate, const unsigned long flags, const int protocol) {
        uint32_t result = Core::ERROR_UNAVAILABLE;

        if (_port.IsOpen() == true) {
       	    
            result = Prepare();
            fprintf(stderr, "%s -- %d -- Result: %d\n", __FUNCTION__, __LINE__, result);

            if (result == Core::ERROR_NONE) {
	        _port.Flush();
                _port.SetBaudrate(baudRate);

                if (result == Core::ERROR_NONE) {
	            int i = N_HCI;
	            if ( (_port.Control(TIOCSETD, &i) == Core::ERROR_NONE) &&
	                 (_port.Control(HCIUARTSETFLAGS, &flags) == Core::ERROR_NONE) &&
	                 (_port.Control(HCIUARTSETPROTO, &protocol) == Core::ERROR_NONE) ) {

                        result = Complete();
                    }
                }
            }
        }

        return (result);
    }
    uint32_t Deinitialize() {
    }
    uint32_t Exchange (const Message::Request& request, Message::Response& response, const uint32_t allowedTime) {
        return (_port.Exchange(request,response, allowedTime));
    }
    void Flush() {
        _port.Flush();
    }

private:
    void Received(const Exchange::Bluetooth::Buffer& element) {
    }
    virtual uint32_t Prepare() {
    }
    virtual uint32_t Complete() {
    }

private:
    Channel _port;
};

} } // namespace WPEFramework::Bluetooth
