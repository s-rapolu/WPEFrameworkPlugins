#pragma once

#include "Module.h"

namespace WPEFramework {

namespace Exchange {

template <typename TYPE, typename LENGTH, const uint16_t BUFFERSIZE>
struct TypeLengthValueType {
    class Request {
    private:
        Request();
        Request(const Request<TYPE, LENGTH>& copy);
        Request<TYPE, LENGTH>& operator=(const Request<TYPE, LENGTH>& copy);

    protected:
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
            if ((offset < (sizeof(TYPE) + sizeof(LENGTH))) && ((length - result) > 0)) {
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
    class Response : public Request<TYPE, LENGTH> {
    private:
        Response ();
        Response(const Response<TYPE, LENGTH, BUFFERSIZE>& copy);
        Response<TYPE, LENGTH, BUFFERSIZE>& operator=(const Response<TYPE, LENGTH, BUFFERSIZE>& copy);

    public:
        inline Response (const TYPE type) : Request(type, _value) {
        }
        inline ~Response() {
        }

    public:
        bool Copy (const Request<TYPE, LENGTH>& buffer) {
            bool result = false;
            if (buffer.Type() == Type()) {
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
    class Buffer : public Request<TYPE, LENGTH> {
    private:
        Buffer(const Buffer<TYPE, LENGTH, BUFFERSIZE>& copy);
        Buffer<TYPE, LENGTH, BUFFERSIZE>& operator=(const Buffer<TYPE, LENGTH, BUFFERSIZE>& copy);

    public:
        inline Buffer() : Request<TYPE, LENGTH>(_value) {
        }
        inline ~Buffer() {
        }

    public:
        inline bool Next() {
            bool result = false;

            // Clear the current selected block, move on to the nex block, return true, if 
            // we have a next block..
            if ((Offset() > 4) && (OffSet() == Length())) {
                if ( (_used > 0) && (Offset() > 4) ) {
                    ::memcpy(_value[0], &(_value[Offset() - 4]), _used);
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
            if (Offset() < sizeof(TYPE)) {
                LENGTH offset = Offset();
                TYPE current = (offset > 0 ? Type() : 0);
                uint8_t loaded = Load(current, &(stream[result]), length - result, offset);
                result += loaded;
                Offset(offset + loaded);
                Type(type);
            }
            if ((Offset() < (sizeof(TYPE) + sizeof(LENGTH))) && ((length - result) > 0)) {
                LENGTH offset = Offset();
                LENGTH current = (offset > sizeof(TYPE) ? Length() : 0);
                uint8_t loaded = Load(current, &(stream[result]), length - result, offset - sizeof(TYPE));
                result += loaded;
                Offset(offset + loaded);
                Length(length);
            }
            if ((length - result) > 0) {
                uint16_t copyLength = std::min(Length() - Offset() - (sizeof(TYPE) + sizeof(LENGTH)), length - result);
                if (copyLength > 0) {
                    ::memcpy (&(_value[Offset()-(sizeof(TYPE) + sizeof(LENGTH))]), &(stream[result]), copyLength);
                    Offset(Offset() + copyLength);
                    result += copyLength;
                }
            }

            if (result < length) {
                uint16_t copyLength = std::min((2 * BUFFERSIZE) - Offset() - (sizeof(TYPE) + sizeof(LENGTH)) - _used, length - result);
                ::memcpy((&_value[Offset()-(sizeof(TYPE) + sizeof(LENGTH))) + _used], &stream[result], copyLength);
                _used += copyLength;
            }
         
            return ((Offset() >= (sizeof(TYPE) + sizeof(LENGTH))) && (Offset() == Length()));
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
        COMMAND_PKT = 0x00,
        EVENT_PKT   = 0x04
    };

    const uint32_t BUFFERSIZE = 255;

    class Request {
    private:
        Request();
        Request(const Request& copy);
        Request& operator=(const Request& copy);

    protected:
        inline Request(const command& cmd, const uint16 sequence, const uint8_t* value)
            : _command(cmd)
            , _sequence(sequence)
            , _length(0)
            , _offset(0)
            , _value(value) {
            ASSERT(value != nullptr);
        }
 
    public:
        inline Request(const command& cmd, const uint16 sequence, const uint8_t length, const uint8_t* value)
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
            Offset(0);
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
            return (_command != EVENT_PKT ? _sequence : (_length > 2 : ((_value[1] << 8) | _value[2]) : ~0));
        }
        inline command Response() const {
            return (_command != EVENT_PKT ? _command : (_length > 0 : _value[0] : ~0));
        }
        inline uint16_t Serialize(uint8_t stream[], const uint16_t length) const {
            uint16_t result = 0;

            if (_offset == 0) {
                _offset++;
                stream[result++] = _command;
            }
            if ((offset == 1) && ((length - result) > 0)) {
                _offset++;
                stream[result++] = static_cast<uint8_t>((_sequence >> 8) & 0xFF);
            }
            if ((offset == 2) && ((length - result) > 0)) {
                _offset++;
                if (_command != EVENT_PKT) {
                    stream[result++] = static_cast<uint8_t>(_sequence & 0xFF);
                }
            }
            if ((offset == 3) && ((length - result) > 0)) {
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
            return (result);
        }

    protected:
        inline void Command (const command cmd) {
            _command = cmd;
        }
        inline void Sequence (const uint16_t sequence) {
            _sequence = sequence;
        }
        inline void Length (const uint8_t length) {
            _length = length;
        }
        inline void Offset (const LENGTH offset) {
            _offset = offset;
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
        bool Copy (const Request<TYPE,LENGTH>& buffer) {
            bool result = false;

            if ( (buffer.Response() == Command()) && (buffer.Acknowledge() == Sequence())) {
                uint8_t copyLength = std::min(buffer.Length(), BUFFERSIZE);
                ::memcpy(_value, buffer.Value(), copyLength);
                Length(copyLength);
                result = true;
            }
 
            return (result);
        }

    private:
        uint8_t _value[BUFFERSIZE];
    };
    class Buffer : public Request<TYPE, LENGTH> {
    private:
        Buffer(const Buffer<TYPE, LENGTH, BUFFERSIZE>& copy);
        Buffer<TYPE, LENGTH, BUFFERSIZE>& operator=(const Buffer<TYPE, LENGTH, BUFFERSIZE>& copy);

    public:
        inline Buffer() : Request<TYPE, LENGTH>(_value) {
        }
        inline ~Buffer() {
        }

    public:
        inline bool Next() {
            bool result = false;

            // Clear the current selected block, move on to the nex block, return true, if 
            // we have a next block..
            if ((Offset() > 4) && (OffSet() == Length())) {
                if ( (_used > 0) && (Offset() > 4) ) {
                    ::memcpy(_value[0], &(_value[Offset() - 4]), _used);
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
                if (Command != EVENT_PKT) {
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

            if (result < length) {
                uint16_t copyLength = std::min((2 * BUFFERSIZE) - Offset() - 4 - _used, length - result);
                ::memcpy((&_value[Offset()-4) + _used], &stream[result], copyLength);
                _used += copyLength;
            }
         
            return ((Offset() >= 4) && (Offset() == Length()));
        }
        
    private:
        uint16_t _used;
        uint8_t _value[2 * BUFFERSIZE];
    };
};

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
            template <typenames... Args>
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
        template <typenames... Args>
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
            _synchronizer.Lock();

            _channel.Flush();
            _synchronizer.Flush();
            _buffer.Flush();

            _synchronizer.Unlock();

            return (OK);
        }
        uint32_t Exchange (const DATAEXCHANGE::Request& request, const std::function< bool (const DATAEXCHANGE::Request&) >& comparator, const uint32_t allowedTime) {
            uint32_t result = Core::ERROR_INPROGRESS;

            _synchronizer.Lock();

            if (_current == nullptr) {

                _synchronizer.Load(comparator);
                _synchronizer.Unlock();

                _channel.Trigger();

                result = _synchronizer.Aquire(allowedTime);

                _current = nullptr;
            }

            _synchronizer.Unlock();

            return (result);
 
            return (_synchronizer.Unlock();
        }
        uint32_t Exchange (const DATAEXCHANGE::Request& request, DATAEXCHANGE::Response& response, const uint32_t allowedTime) {

            uint32_t result = Core::ERROR_INPROGRESS;

            _synchronizer.Lock();

            if (_current == nullptr) {
             
                _synchronizer.Load(response);
                _current = &request;
                _synchronizer.Unlock();

                _channel.Trigger();

                result = _synchronizer.Aquire(allowedTime);

                _synchronizer.Lock();
                _current = nullptr;
            }

            _synchronizer.Unlock();

            return (result);
        }

        virtual void StateChange() {
        }
        virtual void Send(const DATAEXCHANGE::Request& element) {
        }
        virtual void Received(const DATAEXCHANGE::Buffer& element) {
        }

    private:
        // Methods to extract and insert data into the socket buffers
        uint16_t SendData(uint8_t* dataFrame, const uint16_t maxSendSize)
        {
            uint16_t result = 0;

            _synchronizer.Lock();

            if (_current != nullptr) {
                result = _current->Serialize(dataFrame, maxSendSize);

                if (result == 0) {
                    Send(*current);
                    _responses->Evaluate();
                }
            }

            _synchronizer.Unlock();

            return (result);
        }
        uint16_t ReceiveData(uint8_t* dataFrame, const uint16_t availableData)
        {
            _synchronizer.Lock();

            if (_current != nullptr) {
                if (_buffer->Deserialize(dataFrame, availableData) == true) {
                    do {
                        if (_responses->Evaluate(_buffer) == false) {
                            Received(_buffer);
                        }
                    } while (_buffer.Next() == true);
                }
            }

            _synchronizer.Unlock();

            return (availableData);
        }

    private:
        Handler _channel;
        const DATAEXCHANGE::Request* _current;
        DATAEXCHANGE::Buffer _buffer;
        Core::SynchronizeType<DATAEXCHANGE::Response, DATAEXCHANGE::Request> _responses;
    };

    template <typename SOURCE, typename TYPE, typename LENGTH, typename BUFFERSIZE>
    typedef MessageExchangeType<SOURCE, TypeLengthValueType<TYPE, LENGTH, BUFFERSIZE> > StreamTypeLengthValueType;
} }
