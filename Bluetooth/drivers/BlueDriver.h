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
        Channel(SerialDriver& parent, const string& name)
            : BaseClass (name)
            , _parent(parent) {
        }
        virtual ~Channel() {
        }

    private:
        virtual void Received(const Exchange::Bluetooth::Buffer& element) override{
            _parent.Received(element);
        }

    private:
        SerialDriver& _parent;
    };

protected:
    SerialDriver(const string& port, const uint32_t baudRate, const Core::SerialPort::FlowControl flowControl, const bool sendBreak)
        : Driver()
        , _port(*this, port) {
        if (_port.Open(100) == Core::ERROR_NONE) {

            ToTerminal();

            if (sendBreak == true) {
                _port.Link().SendBreak();
                SleepMs(500);
            }
 
            _port.Link().Configuration(Convert(baudRate), flowControl, 64, 64);

            _port.Link().Flush();
        }
    }

public:
    using Message = Exchange::Bluetooth;

    virtual ~SerialDriver() {
        if (_port.IsOpen() == true) {
            ToTerminal();
        }
        _port.Close(Core::infinite);
    }

public:
    uint32_t Setup (const unsigned long flags, const int protocol) {
        _port.Link().Flush();

        int ttyValue = N_HCI;
        if (::ioctl(static_cast<Core::IResource&>(_port.Link()).Descriptor(), TIOCSETD, &ttyValue) < 0) {
            TRACE_L1("Failed direct IOCTL to TIOCSETD, %d", errno);
        }
        else if (::ioctl(static_cast<Core::IResource&>(_port.Link()).Descriptor(), HCIUARTSETFLAGS, flags) < 0) {
            TRACE_L1("Failed HCIUARTSETFLAGS. [flags:%d]", flags);
        }
        else if (::ioctl(static_cast<Core::IResource&>(_port.Link()).Descriptor(), HCIUARTSETPROTO, protocol) < 0) {
            TRACE_L1("Failed HCIUARTSETPROTO. [protocol:%d]", protocol);
        }
        else {
            return (Core::ERROR_NONE);
        }

        return (Core::ERROR_GENERAL);
    }
    uint32_t Exchange (const Message::Request& request, Message::Response& response, const uint32_t allowedTime) {
        return (_port.Exchange(request,response, allowedTime));
    }
    void Flush() {
        _port.Link().Flush();
    }
    void SetBaudRate(const uint32_t baudRate) {
        _port.Link().SetBaudRate(Convert(baudRate));
    }

private:
    inline void ToTerminal() {
        int ttyValue = N_TTY;
        if (::ioctl(static_cast<Core::IResource&>(_port.Link()).Descriptor(), TIOCSETD, &ttyValue) < 0) {
            TRACE_L1("Failed direct IOCTL to TIOCSETD, %d", errno);
        }
    }
    Core::SerialPort::BaudRate Convert (const uint32_t baudRate) {
        if (baudRate <= 110)     return (Core::SerialPort::BAUDRATE_110);
        if (baudRate <= 300)     return (Core::SerialPort::BAUDRATE_300);
        if (baudRate <= 600)     return (Core::SerialPort::BAUDRATE_600);
        if (baudRate <= 1200)    return (Core::SerialPort::BAUDRATE_1200);
        if (baudRate <= 2400)    return (Core::SerialPort::BAUDRATE_2400);
        if (baudRate <= 4800)    return (Core::SerialPort::BAUDRATE_4800);
        if (baudRate <= 9600)    return (Core::SerialPort::BAUDRATE_9600);
        if (baudRate <= 19200)   return (Core::SerialPort::BAUDRATE_19200);
        if (baudRate <= 38400)   return (Core::SerialPort::BAUDRATE_38400);
        if (baudRate <= 57600)   return (Core::SerialPort::BAUDRATE_57600);
        if (baudRate <= 115200)  return (Core::SerialPort::BAUDRATE_115200);
        if (baudRate <= 230400)  return (Core::SerialPort::BAUDRATE_230400);
        if (baudRate <= 460800)  return (Core::SerialPort::BAUDRATE_460800);
        if (baudRate <= 500000)  return (Core::SerialPort::BAUDRATE_500000);
        if (baudRate <= 576000)  return (Core::SerialPort::BAUDRATE_576000);
        if (baudRate <= 921600)  return (Core::SerialPort::BAUDRATE_921600);
        if (baudRate <= 1000000) return (Core::SerialPort::BAUDRATE_1000000);
        if (baudRate <= 1152000) return (Core::SerialPort::BAUDRATE_1152000);
        if (baudRate <= 1500000) return (Core::SerialPort::BAUDRATE_1500000);
        if (baudRate <= 2000000) return (Core::SerialPort::BAUDRATE_2000000);
        if (baudRate <= 2500000) return (Core::SerialPort::BAUDRATE_2500000);
        if (baudRate <= 3000000) return (Core::SerialPort::BAUDRATE_3000000);
        if (baudRate <= 3500000) return (Core::SerialPort::BAUDRATE_3500000);
#ifdef B3710000
        if (baudRate <= 3710000) return (Core::SerialPort::BAUDRATE_3710000);
#endif
        if (baudRate <= 4000000) return (Core::SerialPort::BAUDRATE_4000000);
        return (Core::SerialPort::BAUDRATE_9600);
    }
    void Received(const Exchange::Bluetooth::Buffer& element) {
    }
    virtual uint32_t Prepare() {
        return (Core::ERROR_NONE);
    }
    virtual uint32_t Complete() {
        return (Core::ERROR_NONE);
    }

private:
    Channel _port;
};

} } // namespace WPEFramework::Bluetooth
