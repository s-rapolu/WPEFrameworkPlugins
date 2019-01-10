#include "Module.h"
#include "BlueDriver.h"

namespace WPEFramework {

namespace Bluetooth {

class Broadcom43XX : public SerialDriver {
private:
    Broadcom43XX() = delete;
    Broadcom43XX(const Broadcom43XX&) = delete;
    Broadcom43XX& operator= (const Broadcom43XX&) = delete;

    static constexpr uint8_t BCM43XX_CLOCK_48 = 1;
    static constexpr uint8_t BCM43XX_CLOCK_24 = 2;
    static constexpr uint8_t CMD_SUCCESS = 0;

    // { "bcm43xx",    0x0000, 0x0000, HCI_UART_H4,   115200, 3000000, FLOW_CTL, DISABLE_PM, NULL, bcm43xx, NULL  },

public:
    Broadcom43XX(const string& portName, const string& directory, const string& MACAddress, const bool sendBreak) 
        : SerialDriver(portName, Core::SerialPort::BAUDRATE_115200, Core::SerialPort::OFF, sendBreak)
        , _directory(directory)
        , _name()
        , _MACLength(0) {
    }
    virtual ~Broadcom43XX() {
    }

private:
    string FindFirmware(const string& directory, const string& chipName) {
        string result;
        Core::Directory index(directory.c_str(), "*");

        while ((result.empty() == true) && (index.Next() == true)) {

            if (index.Name() == chipName) {
                result = index.Current();
            }
            else if ( (index.IsDirectory() == true) && (index.Name() != _T(".")) && (index.Name() != _T("..")) ) {
            
                result = FindFirmware(index.Current(), chipName);
            }
        }

        return (result);
    }
    uint32_t Reset() {
        Message::Response response (Exchange::Bluetooth::COMMAND_PKT, 0x030C);
        uint32_t result = Exchange (Message::Request(Exchange::Bluetooth::COMMAND_PKT, 0x030C, 0, nullptr), response, 500);

        fprintf(stderr, "%s -- %d -- Result: %d\n", __FUNCTION__, __LINE__, result);
	if ((result == Core::ERROR_NONE) && (response[0] != CMD_SUCCESS)) {
        fprintf(stderr, "%s -- %d -- Result: %d\n", __FUNCTION__, __LINE__, result);
            TRACE_L1("Failed to reset chip, command failure\n");
	    result = Core::ERROR_GENERAL;
	}

	return result;
    }
    uint32_t LoadName() {
        Message::Response response (Exchange::Bluetooth::COMMAND_PKT, 0x140C);
        uint32_t result = Exchange (Message::Request(Exchange::Bluetooth::COMMAND_PKT, 0x140C, 0, nullptr), response, 500);

	if ((result == Core::ERROR_NONE) && (response[0] != CMD_SUCCESS)) {
            TRACE_L1("Failed to read local name, command failure\n");
            result = Core::ERROR_GENERAL;
        }
        if (result == Core::ERROR_NONE) {
            _name = string(reinterpret_cast<const TCHAR*>(&(response[1])), response.DataLength()-1); 
        }
        return (result);
    }
    uint32_t SetClock(const uint8_t clock) {
        Message::Response response (Exchange::Bluetooth::COMMAND_PKT, 0x45fc);
        uint32_t result = Exchange (Message::Request(Exchange::Bluetooth::COMMAND_PKT, 0x45fc, 1, &clock), response, 500);

	if ((result == Core::ERROR_NONE) && (response[0] != CMD_SUCCESS)) {
            TRACE_L1("Failed to read local name, command failure\n");
            result = Core::ERROR_GENERAL;
        }

        return (result);
    }
    uint32_t SetSpeed(const Core::SerialPort::BaudRate baudrate) {
        uint32_t result = Core::ERROR_NONE;

        if (baudrate > 3000000) {
            result = SetClock(BCM43XX_CLOCK_48);
            Flush();
        }

        if (result == Core::ERROR_NONE) {
            uint8_t data[6];

            data[0] = 0x00;
            data[1] = 0x00;
            data[2] = static_cast<uint8_t>(baudrate & 0xFF);
            data[3] = static_cast<uint8_t>((baudrate >> 8) & 0xFF);
            data[4] = static_cast<uint8_t>((baudrate >> 16) & 0xFF);
            data[5] = static_cast<uint8_t>((baudrate >> 24) & 0xFF);

           Message::Response response (Exchange::Bluetooth::COMMAND_PKT, 0x18fc);
           uint32_t result = Exchange (Message::Request(Exchange::Bluetooth::COMMAND_PKT, 0x18fc, sizeof(data), data), response, 500);

	    if ((result == Core::ERROR_NONE) && (response[0] != CMD_SUCCESS)) {
                TRACE_L1("Failed to read local name, command failure\n");
                result = Core::ERROR_GENERAL;
            }
        }

        return 0;
    }
    uint32_t MACAddress (const uint8_t length, const uint8_t address[]) {
        uint8_t data[6]; 
        ::memcpy (data, address, std::min(length, static_cast<uint8_t>(sizeof(data))));
        ::memset (&(data[length]), 0, sizeof(data) - std::min(length, static_cast<uint8_t>(sizeof(data))));

        Message::Response response (Exchange::Bluetooth::COMMAND_PKT, 0x01fc);
        uint32_t result = Exchange (Message::Request(Exchange::Bluetooth::COMMAND_PKT, 0x01fc, sizeof(data), address), response, 500);

	if ((result == Core::ERROR_NONE) && (response[0] != CMD_SUCCESS)) {
            TRACE_L1("Failed to set the MAC address\n");
            result = Core::ERROR_GENERAL;
        }

        return (result);
    }
    uint32_t Firmware (const string& directory, const string& name) {
        uint32_t result = Core::ERROR_UNAVAILABLE;
        string searchPath = Core::Directory::Normalize(directory);
        string firmwareName = FindFirmware(searchPath, name);
       
        if (firmwareName.empty() == false) { 
            int fd = open(firmwareName.c_str(), O_RDONLY);

            if (fd >= 0) {
                Message::Response response (Exchange::Bluetooth::COMMAND_PKT, 0x2efc);
                uint32_t result = Exchange (Message::Request(Exchange::Bluetooth::COMMAND_PKT, 0x2efc, 0, nullptr), response, 500);

                Flush();

                if ((result == Core::ERROR_NONE) && (response[6] != CMD_SUCCESS)) {
                    TRACE_L1("Failed to set chip to download firmware\n");
	            result = Core::ERROR_GENERAL;
                }
     
                if (result == Core::ERROR_NONE) {
                    int loaded;
                    uint8_t tx_buf[255];

                    /* Wait 50ms to let the firmware placed in download mode */
                    SleepMs(50);

                    Flush();

                    while ((result == Core::ERROR_NONE) && ((loaded = read(fd, tx_buf, 3)) > 0)) {
                        uint16_t code = (tx_buf[0] << 8) | tx_buf[1];
                        uint8_t len = tx_buf[2];

                        if (read(fd, tx_buf, len) < 0) {
                            result = Core::ERROR_READ_ERROR;
                        }
                        else {
                            Message::Response response (Exchange::Bluetooth::COMMAND_PKT, code);
                            result = Exchange (Message::Request(Exchange::Bluetooth::COMMAND_PKT, code, len, tx_buf), response, 500);
                            Flush();
                        }
                    }

                    if ((loaded != 0) && (result == Core::ERROR_NONE)) {
                        result = Core::ERROR_NEGATIVE_ACKNOWLEDGE;
                    }

                    /* Wait for firmware ready */
                    SleepMs(2000);
                }

                close(fd);
            }
        }
    }
    virtual uint32_t Prepare() override {
        fprintf(stderr, "%s -- %d --\n", __FUNCTION__, __LINE__);
        uint32_t result = Reset();
        printf("Going for a sleep @ %s\n", __FUNCTION__);
        SleepMs(10000);

        fprintf(stderr, "%s -- %d -- Result: %d\n", __FUNCTION__, __LINE__, result);
        if (result == Core::ERROR_NONE) {
            result = LoadName();
        fprintf(stderr, "%s -- %d -- Result: %d\n", __FUNCTION__, __LINE__, result);
            if (result == Core::ERROR_NONE) {
                result = Firmware(_directory, _name);
        fprintf(stderr, "%s -- %d -- Result: %d\n", __FUNCTION__, __LINE__, result);
                if (result == Core::ERROR_NONE) {
                    result = SetSpeed(Core::SerialPort::BAUDRATE_3000000);
        fprintf(stderr, "%s -- %d -- Result: %d\n", __FUNCTION__, __LINE__, result);
                    if (result == Core::ERROR_NONE) {
                        result = Reset();
        fprintf(stderr, "%s -- %d -- Result: %d\n", __FUNCTION__, __LINE__, result);
                    }
                }
            }
        }
    }

private:    
    const string _directory;
    string _name;
    uint8_t _MACLength;
    uint8_t _MACAddress[6];
};

/* static */ Driver* Driver::Instance(const string& input) {
    class Config : public Core::JSON::Container {
    private:
        Config(const Config&);
        Config& operator=(const Config&);

    public:
        Config()
            : Core::JSON::Container()
            , Port(_T("/dev/ttyAMA0"))
            , Firmware(_T("/etc/firmware/"))
            , MACAddress()
            , Break(true) {
            Add(_T("port"), &Port);
            Add(_T("firmware"), &Firmware);
            Add(_T("address"), &MACAddress);
            Add(_T("break"), &Break);
        }
        ~Config() {
        }

    public:
        Core::JSON::String Port;
        Core::JSON::String Firmware;
        Core::JSON::String MACAddress;
        Core::JSON::Boolean Break;
    } config;

    config.FromString(input);

    Broadcom43XX* driver = new Broadcom43XX(config.Port.Value(), config.Firmware.Value(), config.MACAddress.Value(), config.Break.Value());

    uint32_t result = driver->Initialize(Core::SerialPort::BAUDRATE_3000000, 0, HCI_UART_H4);

    if (result != Core::ERROR_NONE) {
        TRACE_L1 ("Failed to start the Bleutooth driver.");
        fprintf(stderr, "%s -- %d -- Result: %d\n", __FUNCTION__, __LINE__, result);
        delete driver;
        driver = nullptr;
    }
    return (driver);
}

} } // namespace WPEFramework::Bluetooth
