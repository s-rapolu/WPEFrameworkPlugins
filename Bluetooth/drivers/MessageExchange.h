#pragma once

#include "Module.h"

namespace WPEFramework {

namespace Exchange {

template <typename TYPE, typename LENGTH, const uint16_t BUFFERSIZE>
struct TypeLengthValueType {
    class Request {
    private:
        Request();
        Request(const Request& copy);
        Request& operator=(const Request& copy);

    protected:
        inline Request(const uint8_t* value)
            : _length(0)
            , _offset(0)
            , _value(value) {
            ASSERT(value != nullptr);
        }
        inline Request(const TYPE& type, const uint8_t* value)
            : _type(type)
            , _length(0)
            , _offset(0)
            , _value(value) {
            ASSERT(value != nullptr);
        }
 
    public:
        inline Request(const TYPE& type, const LENGTH& length, const uint8_t* value)
            : _type(type)
            , _length(length)
            , _offset(0)
            , _value(value) {
            ASSERT((value != nullptr) || (length == 0));
        }
        inline ~Request() {
        }

    public:
        inline void Reset () {
            Offset(0);
        }
        inline const TYPE& Type() const {
            return (_type);
        }
        inline const LENGTH& Length() const {
            return (_length);
        }
        inline const uint8_t* Value() const {
            return (_value);
        }
        inline uint16_t Serialize(uint8_t stream[], const uint16_t length) const {
            uint16_t result = 0;

            if (_offset < sizeof(TYPE)) {
                result += Store(_type, stream, length, _offset);
            }
            if ((_offset < (sizeof(TYPE) + sizeof(LENGTH))) && ((length - result) > 0)) {
                result += Store(_length, &(stream[result]), length-result, _offset - sizeof(TYPE));
            }
            if ((length - result) > 0) {
                uint16_t copyLength = std::min((_length - (_offset - (sizeof(TYPE) + sizeof(LENGTH)))), (length - result));

                if (copyLength > 0) {
                    ::memcpy (&(stream[result]), &(_value[_offset - (sizeof(TYPE) + sizeof(LENGTH))]), copyLength);
                    _offset += copyLength;
                    result += copyLength;
                }
            }
            return (result);
            
            uint16_t copyLength = (_length - _offset > length ?  length : _length - _offset);
            if (copyLength > 0) {
                ::memcpy (stream, &(_value[_offset]), copyLength);
                _offset += copyLength;
            }

            return (copyLength);
        }

    protected:
        inline void Type (const TYPE type) {
            _type = type;
        }
        inline void Length (const LENGTH length) {
            _length = length;
        }
        inline LENGTH Offset() const {
            return(_offset);
        }
        inline void Offset (const LENGTH offset) {
            _offset = offset;
        }

    private:
        template<typename FIELD>
        uint8_t Store(const FIELD number, uint8_t stream[], const uint16_t length, const uint8_t offset) const {
            uint8_t stored = 0;
            uint8_t index = offset;
            ASSERT (offset < sizeof(FIELD));
            while ((index < sizeof(FIELD)) && (stored < length)) {
                stream[index++] = static_cast<uint8_t>(number >> (8 * (sizeof(TYPE) - 1 - index)));
                stored++;
            }
            return(stored);
        }

    private:
        TYPE _type;
        LENGTH _length;
        mutable LENGTH _offset;
        const uint8_t* _value;
    };
    class Response : public Request {
    private:
        Response ();
        Response(const Response& copy);
        Response& operator=(const Response& copy);

    public:
        inline Response (const TYPE type) : Request(type, _value) {
        }
        inline ~Response() {
        }

    public:
        bool Copy (const Request& buffer) {
            bool result = false;
            if (buffer.Type() == Request::Type()) {
                LENGTH copyLength = std::min(buffer.Length(), BUFFERSIZE);
                ::memcpy(_value, buffer.Value(), copyLength);
                Length(copyLength);
                result = true;
            }
 
           return (result);
        }

    private:
        uint8_t _value[BUFFERSIZE];
    };
    class Buffer : public Request {
    private:
        Buffer(const Buffer& copy);
        Buffer& operator=(const Buffer& copy);

    public:
        inline Buffer() : Request(_value) {
        }
        inline ~Buffer() {
        }

    public:
        inline bool Next() {
            bool result = false;

            // Clear the current selected block, move on to the nex block, return true, if 
            // we have a next block..
            if ((Request::Offset() > 4) && (Request::OffSet() == Request::Length())) {
                if ( (_used > 0) && (Request::Offset() > 4) ) {
                    ::memcpy(_value[0], &(_value[Request::Offset() - 4]), _used);
                }
                uint8_t length = _used;
                _used = 0;
                Request::Offset(0);
                result = Deserialize(_value, length);
            }
            return (result);
        }
        inline bool Deserialize(const uint8_t stream[], const uint16_t length) {
            uint16_t result = 0;
            if (Request::Offset() < sizeof(TYPE)) {
                LENGTH offset = Request::Offset();
                TYPE current = (offset > 0 ? Request::Type() : 0);
                uint8_t loaded = Load(current, &(stream[result]), length - result, offset);
                result += loaded;
                Request::Offset(offset + loaded);
                Request::Type(current);
            }
            if ((Request::Offset() < (sizeof(TYPE) + sizeof(LENGTH))) && ((length - result) > 0)) {
                LENGTH offset = Request::Offset();
                LENGTH current = (offset > sizeof(TYPE) ? Request::Length() : 0);
                uint8_t loaded = Load(current, &(stream[result]), length - result, offset - sizeof(TYPE));
                result += loaded;
                Request::Offset(offset + loaded);
                Request::Length(current);
            }
            if ((length - result) > 0) {
                uint16_t copyLength = std::min(Request::Length() - Request::Offset() - (sizeof(TYPE) + sizeof(LENGTH)), length - result);
                if (copyLength > 0) {
                    ::memcpy (&(_value[Request::Offset()-(sizeof(TYPE) + sizeof(LENGTH))]), &(stream[result]), copyLength);
                    Request::Offset(Request::Offset() + copyLength);
                    result += copyLength;
                }
            }

            if (result < length) {
                uint16_t copyLength = std::min((2 * BUFFERSIZE) - Request::Offset() - (sizeof(TYPE) + sizeof(LENGTH)) - _used, length - result);
                ::memcpy(&(_value[Request::Offset()-(sizeof(TYPE) + sizeof(LENGTH)) + _used]), &(stream[result]), copyLength);
                _used += copyLength;
            }
         
            return ((Request::Offset() >= (sizeof(TYPE) + sizeof(LENGTH))) && (Request::Offset() == Request::Length()));
        }
        
    private:
        template<typename FIELD>
        uint8_t Load(FIELD& number, uint8_t stream[], const uint16_t length, const uint8_t offset) const {
            uint8_t loaded = 0;
            uint8_t index = offset;
            ASSERT (offset < sizeof(FIELD));
            while ((index < sizeof(FIELD)) && (loaded < length)) {
                number |= (stream[loaded++] >> (8 * (sizeof(FIELD) - 1 - index)));
                index++;
            }
            return(loaded);
        }

    private:
        uint16_t _used;
        uint8_t _value[2 * BUFFERSIZE];
    };
};

struct Bluetooth {

    enum command : uint8_t {
        COMMAND_PKT = 0x01,
        EVENT_PKT   = 0x04
    };

    static constexpr uint32_t BUFFERSIZE = 255;

    static void DumpFrame (const char header[], const uint8_t length, const uint8_t stream[]) {
        printf("%s ", header);
        for (uint8_t index = 0; index < length; index++) {
            if (index != 0) { printf(":"); }
            printf("%02X", stream[index]);
        }
        printf("\n");
    }
    class Request {
    private:
        Request();
        Request(const Request& copy);
        Request& operator=(const Request& copy);

    protected:
        inline Request(const uint8_t* value)
            : _command(EVENT_PKT)
            , _sequence(0)
            , _length(0)
            , _offset(0)
            , _value(value) {
            ASSERT(value != nullptr);
        }
        inline Request(const command& cmd, const uint16_t sequence, const uint8_t* value)
            : _command(cmd)
            , _sequence(sequence)
            , _length(0)
            , _offset(0)
            , _value(value) {
            ASSERT(value != nullptr);
        }
 
    public:
        inline Request(const command& cmd, const uint16_t sequence, const uint8_t length, const uint8_t* value)
            : _command(cmd)
            , _sequence(sequence)
            , _length(length)
            , _offset(0)
            , _value(value) {
            ASSERT((value != nullptr) || (length == 0));
        }
        inline ~Request() {
        }

    public:
        inline void Reset () {
            Request::Offset(0);
        }
        inline command Command() const {
            return (_command);
        }
        inline uint16_t Sequence() const {
            return (_sequence);
        }
        inline uint8_t Length() const {
            return (_length);
        }
        inline const uint8_t* Value() const {
            return (_value);
        }
        inline uint16_t Acknowledge () const {
            return (_command != EVENT_PKT ? _sequence : (_length > 2 ? ((_value[1] << 8) | _value[2]) : ~0));
        }
        inline command Response() const {
            return (_command != EVENT_PKT ? _command : static_cast<command>(_length > 0 ? _value[0] : ~0));
        }
        inline uint8_t DataLength() const {
            return (_length - EventOffset());
        }
        inline const uint8_t& operator[] (const uint8_t index) const {
            ASSERT (index < (_length - EventOffset()));
            return (_value[index - EventOffset()]);
        }
        inline uint16_t Serialize(uint8_t stream[], const uint16_t length) const {
            uint16_t result = 0;

            if (_offset == 0) {
                _offset++;
                stream[result++] = _command;
            }
            if ((_offset == 1) && ((length - result) > 0)) {
                _offset++;
                stream[result++] = static_cast<uint8_t>((_sequence >> 8) & 0xFF);
            }
            if ((_offset == 2) && ((length - result) > 0)) {
                _offset++;
                if (_command != EVENT_PKT) {
                    stream[result++] = static_cast<uint8_t>(_sequence & 0xFF);
                }
            }
            if ((_offset == 3) && ((length - result) > 0)) {
                _offset++;
                stream[result++] = _length;
            }
            if ((length - result) > 0) {
                uint16_t copyLength = ((_length - (_offset - 4)) > (length - result) ?  (length - result) : (_length - (_offset - 4)));
                if (copyLength > 0) {
                    ::memcpy (&(stream[result]), &(_value[_offset - 4]), copyLength);
                    _offset += copyLength;
                    result += copyLength;
                }
            }

            DumpFrame("OUT:", result, stream);
            return (result);
        }

    protected:
        inline uint8_t EventOffset() const {
            return (_command != EVENT_PKT ? 0 : (_length > 2 ? 3 : _length));
        }
        inline void Command (const command cmd) {
            _command = cmd;
        }
        inline void Sequence (const uint16_t sequence) {
            _sequence = sequence;
        }
        inline void Length (const uint8_t length) {
            _length = length;
        }
        inline void Offset (const uint8_t offset) {
            _offset = offset;
        }
        inline uint8_t Offset () const {
            return (_offset);
        }

    private:
        command _command;
        uint16_t _sequence;
        uint8_t _length;
        mutable uint8_t _offset;
        const uint8_t* _value;
    };
    class Response : public Request {
    private:
        Response ();
        Response(const Response& copy);
        Response& operator=(const Response& copy);

    public:
        inline Response (const command cmd, const uint16_t sequence) : Request(cmd, sequence, _value) {
        }
        inline ~Response() {
        }

    public:
        bool Copy (const Request& buffer) {
            bool result = false;

            if ( (buffer.Response() == Command()) && (buffer.Acknowledge() == Sequence())) {
                uint8_t copyLength = static_cast<uint8_t>(buffer.Length() <  BUFFERSIZE ? buffer.Length() : BUFFERSIZE);
                ::memcpy(_value, buffer.Value(), copyLength);
                Length(copyLength);
                result = true;
            }
 
            return (result);
        }

    private:
        uint8_t _value[BUFFERSIZE];
    };
    class Buffer : public Request {
    private:
        Buffer(const Buffer& copy);
        Buffer& operator=(const Buffer& copy);

    public:
        inline Buffer() : Request(_value) {
        }
        inline ~Buffer() {
        }

    public:
        inline bool Next() {
            bool result = false;

            // Clear the current selected block, move on to the nex block, return true, if 
            // we have a next block..
            if ((Offset() > 4) && (Offset() == Length())) {
                if ( (_used > 0) && (Offset() > 4) ) {
                    ::memcpy(_value, &(_value[Offset() - 4]), _used);
                }
                uint8_t length = _used;
                _used = 0;
                Offset(0);
                result = Deserialize(_value, length);
            }
            return (result);
        }
        inline bool Deserialize(const uint8_t stream[], const uint16_t length) {
            uint16_t result = 0;
            if (Offset() < 1) {
                Command(static_cast<command>(stream[result++]));
                Offset(1);
            }
            if ((Offset() < 2) && ((length - result) > 0)) {
                Sequence(stream[result++]);
                Offset(2);
            }
            if ((Offset() < 3) && ((length - result) > 0)) {
                if (Command() != EVENT_PKT) {
                    uint16_t sequence = (Sequence() << 8) | stream[result++];
                    Sequence(sequence);
                }
                Offset(3);
            }
            if ((Offset() < 4) && ((length - result) > 0)) {
                Length(stream[result++]);
                Offset(4);
            }
            if ((length - result) > 0) {
                uint16_t copyLength = std::min(Length() - Offset() - 4, length - result);
                if (copyLength > 0) {
                    ::memcpy (&(_value[Offset()-4]), &(stream[result]), copyLength);
                    Offset(Offset() + copyLength);
                    result += copyLength;
                }
            }
            DumpFrame("IN: ", result, stream);
            if (result < length) {
                uint16_t copyLength = std::min(static_cast<uint16_t>((2 * BUFFERSIZE) - Offset() - 4 - _used), static_cast<uint16_t>(length - result));
                ::memcpy(&(_value[Offset() - 4 + _used]), &stream[result], copyLength);
                _used += copyLength;
            }
         
            return ((Offset() >= 4) && (Offset() == Length()));
        }
        
    private:
        uint16_t _used;
        uint8_t _value[2 * BUFFERSIZE];
    };
};

} // namespace Exchange

namespace Core {
    template <typename SOURCE, typename DATAEXCHANGE>
    class MessageExchangeType {
    private:
        class Handler : public SOURCE {
        private:
            Handler() = delete;
            Handler(const Handler&) = delete;
            Handler& operator=(const Handler&) = delete;

        public:
            template <typename... Args>
            Handler(MessageExchangeType<SOURCE, DATAEXCHANGE>& parent, Args... args)
                : SOURCE(args...)
                , _parent(parent)
            {
            }
            ~Handler()
            {
            }

        protected:
            virtual void StateChange()
            {
                _parent.StateChange();
            }
            virtual uint16_t SendData(uint8_t* dataFrame, const uint16_t maxSendSize)
            {
                return (_parent.SendData(dataFrame, maxSendSize));
            }
            virtual uint16_t ReceiveData(uint8_t* dataFrame, const uint16_t receivedSize)
            {
                printf ("Here is DATA ------------------------------------->\n");
                return (_parent.ReceiveData(dataFrame, receivedSize));
            }

        private:
            MessageExchangeType<SOURCE, DATAEXCHANGE>& _parent;
        };

        MessageExchangeType(const MessageExchangeType<SOURCE, DATAEXCHANGE>&);
        MessageExchangeType<SOURCE, DATAEXCHANGE>& operator=(const MessageExchangeType<SOURCE, DATAEXCHANGE>&);

    public:
	#ifdef __WIN32__ 
	#pragma warning( disable : 4355 )
	#endif
        template <typename... Args>
        MessageExchangeType(Args... args)
            : _channel(*this, args...)
            , _current(nullptr)
            , _responses()
        {
        }
	#ifdef __WIN32__ 
	#pragma warning( default : 4355 )
	#endif
        virtual ~MessageExchangeType()
        {
        }

    public:
        inline SOURCE& Link()
        {
            return (_channel);
        }
        inline string RemoteId() const
        {
            return (_channel.RemoteId());
        }
        inline bool IsOpen() const
        {
            return (_channel.IsOpen());
        }
        inline uint32_t Open(uint32_t waitTime)
        {
            return (_channel.Open(waitTime));
        }
        inline uint32_t Close(uint32_t waitTime)
        {
            return (_channel.Close(waitTime));
        }
        inline uint32_t Flush()
        {
            _responses.Lock();

            _channel.Flush();
            _responses.Flush();
            _buffer.Flush();

            _responses.Unlock();

            return (OK);
        }
        uint32_t Exchange (const typename DATAEXCHANGE::Request& request, typename DATAEXCHANGE::Response& response, const uint32_t allowedTime) {

            uint32_t result = Core::ERROR_INPROGRESS;

            _responses.Lock();

            if (_current == nullptr) {
             
                _responses.Load(response);
                _current = &request;
                _responses.Unlock();

                printf("Trigger...\n");
                _channel.Trigger();

                SleepMs(2000);

                result = _responses.Aquire(allowedTime);
                printf("DOne it all...%d\n", allowedTime);

                _responses.Lock();
                _current = nullptr;
            }

            _responses.Unlock();

            return (result);
        }

        virtual void StateChange() {
        }
        virtual void Send(const typename DATAEXCHANGE::Request& element) {
        }
        virtual void Received(const typename DATAEXCHANGE::Buffer& element) {
        }

    private:
        // Methods to extract and insert data into the socket buffers
        uint16_t SendData(uint8_t* dataFrame, const uint16_t maxSendSize)
        {
            uint16_t result = 0;

            _responses.Lock();

            if (_current != nullptr) {
                result = _current->Serialize(dataFrame, maxSendSize);

                if (result == 0) {
                    Send(*_current);
                }
            }

            _responses.Unlock();

            return (result);
        }
        uint16_t ReceiveData(uint8_t* dataFrame, const uint16_t availableData)
        {
            _responses.Lock();

            if (_current != nullptr) {
                if (_buffer.Deserialize(dataFrame, availableData) == true) {
                    do {
                        if (_responses.Evaluate(_buffer) == false) {
                            Received(_buffer);
                        }
                    } while (_buffer.Next() == true);
                }
            }

            _responses.Unlock();

            return (availableData);
        }

    private:
        Handler _channel;
        const typename DATAEXCHANGE::Request* _current;
        typename DATAEXCHANGE::Buffer _buffer;
        typename Core::SynchronizeType<typename DATAEXCHANGE::Response> _responses;
    };

    template <typename SOURCE, typename TYPE, typename LENGTH, const uint32_t BUFFER_SIZE>
    using StreamTypeLengthValueTypeEx = MessageExchangeType<SOURCE, Exchange::TypeLengthValueType<TYPE, LENGTH, BUFFER_SIZE> >;
} }
