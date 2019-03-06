#include "Module.h"
#include "Messenger.h"

/* If MESSENGER_MULTIUSERWEBCLIENT is set, the REST API will allow specifying a user name along with request.
   Useful for testing with only a single device. */
#define MESSENGER_MULTIUSERWEBCLIENT

namespace WPEFramework {

namespace Plugin {

    SERVICE_REGISTRATION(Messenger, 1, 0);

    static const string USERNAME = _T("WebUser"); // TODO: modify to uniquely identify the user/device

    static Core::ProxyPoolType<Web::JSONBodyType<Messenger::RoomsData>> jsonRoomsResponseFactory(1);
    static Core::ProxyPoolType<Web::JSONBodyType<Messenger::RoomMembersData>> jsonRoomMembersResponseFactory(1);
    static Core::ProxyPoolType<Web::TextBody> textBodyDataFactory(1);


    // IPlugin methods

    /* virtual */ const string Messenger::Initialize(PluginHost::IShell* service)
    {
        ASSERT(service != nullptr);
        ASSERT(_service == nullptr);
        ASSERT(_roomAdmin == nullptr);
        ASSERT(_rooms.empty() == true);
        ASSERT(_joinedRooms.empty() == true);

        _service = service;
        _service->AddRef();

        _skipUrl = _service->WebPrefix().length();

        _roomAdmin = service->Root<Exchange::IRoomAdministrator>(_pid, 2000, _T("RoomMaintainer"));
        ASSERT(_roomAdmin != nullptr);

        _roomAdmin->Register(this);

        return { };
    }

    /* virtual */ void Messenger::Deinitialize(PluginHost::IShell* service)
    {
        ASSERT(service == _service);

        // Exit all the rooms (if any) that were joined by this client
        for (auto& room : _joinedRooms) {
            room.second->Release();
        }

        _joinedRooms.clear();

        _roomAdmin->Unregister(this);
        _rooms.clear();

        _roomAdmin->Release();
        _roomAdmin = nullptr;

        _service->Release();
        _service = nullptr;
    }

    // IWeb methods

    /* virtual */ void Messenger::Inbound(Web::Request& request)
    {
        if ((request.Verb == Web::Request::HTTP_POST) || (request.Verb == Web::Request::HTTP_PUT)) {
            request.Body(Core::ProxyType<Web::IBody>(textBodyDataFactory.Element()));
        }
    }

    /* virtual */ Core::ProxyType<Web::Response> Messenger::Process(const Web::Request& request)
    {
        Core::ProxyType<Web::Response> result;
        Core::TextSegmentIterator index(Core::TextFragment(request.Path, _skipUrl, request.Path.length() - _skipUrl), false, '/');

        index.Next();

        if (request.Verb == Web::Request::HTTP_GET) {
            result = _GetMethod(index, request);
        }
        else if ((request.Verb == Web::Request::HTTP_POST) || (request.Verb == Web::Request::HTTP_PUT)) {
            result = _PostMethod(index, request);
        }

        return result;
    }

    Core::ProxyType<Web::Response> Messenger::_GetMethod(Core::TextSegmentIterator& index, const Web::Request& request)
    {
        Core::ProxyType<Web::Response> result(PluginHost::Factories::Instance().Response());
        result->ErrorCode = Web::STATUS_BAD_REQUEST;
        result->Message = _T("Unsupported GET request.");

        TRACE(Trace::Information, (_T("Messenger: received GET web request")));

        // GET http://<ip>/Service/Messenger
        // GET http://<ip>/Service/Messenger/Rooms
        if ((index.Next() == false) || (index.Current().Text() == "Rooms")) {
           auto response(jsonRoomsResponseFactory.Element());
            _GetRooms(response->Rooms);
            result->Body(Core::proxy_cast<Web::IBody>(response));
            result->ContentType = Web::MIMETypes::MIME_JSON;
            result->ErrorCode = Web::STATUS_OK;
            result->Message = _T("OK");
        }

        // GET http://<ip>/Service/Messenger/Room/<room-id>
        else if ((index.Current().Text() == "Room") && (index.Next() != false)) {
            auto response(jsonRoomMembersResponseFactory.Element());
            if (_GetRoomMembers(index.Current().Text(), response->RoomMembers) == true) {
                result->Body(Core::proxy_cast<Web::IBody>(response));
                result->ContentType = Web::MIMETypes::MIME_JSON;
                result->ErrorCode = Web::STATUS_OK;
                result->Message = _T("OK");
            }
            else {
                result->ErrorCode = Web::STATUS_NOT_FOUND;
                result->Message = _T("Room not found");
            }
        }

        return result;
    }

    Core::ProxyType<Web::Response> Messenger::_PostMethod(Core::TextSegmentIterator& index, const Web::Request& request)
    {
        Core::ProxyType<Web::Response> result(PluginHost::Factories::Instance().Response());
        result->ErrorCode = Web::STATUS_BAD_REQUEST;
        result->Message = _T("Unsupported POST request.");

        TRACE(Trace::Information, (_T("Messenger: received POST web request")));

        if (index.Next() != false) {
            // POST http://<ip>/Service/Messenger/Join/<room-id>[/<user-id>]
            if (index.Current().Text() == "Join") {
                if (index.Next() != false) {
                    string userId = USERNAME;
                    string roomId = index.Current().Text();

#ifdef MESSENGER_MULTIUSERWEBCLIENT
                    if (index.Next() != false) {
                        userId = index.Current().Text();
                    }
#endif

                    if (_JoinRoom(roomId, userId) == true) {
                        result->ErrorCode = Web::STATUS_OK;
                        result->Message = _T("OK");
                    }
                    else {
                        // BAD_REQUEST already set
                        result->Message = _T("Already joined");
                    }
                }
            }

            // POST http://<ip>/Service/Messenger/Leave/<room-id>[/<user-id>]
            else if (index.Current().Text() == "Leave") {
                if (index.Next() != false) {
                    string userId = USERNAME;
                    string roomId = index.Current().Text();

#ifdef MESSENGER_MULTIUSERWEBCLIENT
                    if (index.Next() != false) {
                        userId = index.Current().Text();
                    }
#endif

                    if (_LeaveRoom(roomId, userId) == true) {
                        result->ErrorCode = Web::STATUS_OK;
                        result->Message = _T("OK");
                    }
                    else
                    {
                        result->ErrorCode = Web::STATUS_NOT_FOUND;
                        result->Message = _T("Room or user not found");
                    }
                }
            }

            // POST http://<ip>/Service/Messenger/Send/<room-id>[/<user-id>]
            else if ((index.Current().Text() == "Send") && (request.HasBody() == true)) {
                if (index.Next() != false) {
                    string userId = USERNAME;
                    string roomId = index.Current().Text();

#ifdef MESSENGER_MULTIUSERWEBCLIENT
                    if (index.Next() != false) {
                        userId = index.Current().Text();
                    }
#endif

                    if (_SendMessage(roomId, userId, *request.Body<Web::TextBody>()) == true) {
                        result->ErrorCode = Web::STATUS_OK;
                        result->Message = _T("OK");
                    }
                    else
                    {
                        result->ErrorCode = Web::STATUS_NOT_FOUND;
                        result->Message = _T("Room or user not found");
                    }
                }
            }

        }

        return result;
    }


    // Web request handlers

    void Messenger::_GetRooms(Core::JSON::ArrayType<Core::JSON::String>& rooms) const
    {
        _adminLock.Lock();

        for (auto const& room : _rooms) {
            Core::JSON::String& element(rooms.Add());
            element = room;
        }

        _adminLock.Unlock();
    }

    bool Messenger::_GetRoomMembers(const string& roomId, Core::JSON::ArrayType<Core::JSON::String>& roomMembers) const
    {
        _joinedRoomsLock.Lock();

        for (auto const& roomUser : _joinedRooms) {
            if (roomUser.first.first == roomId) {
                Core::JSON::String& element(roomMembers.Add());
                element = roomUser.first.second;
            }
        }

        _joinedRoomsLock.Lock();

        return (roomMembers.Length() > 0);
    }

    bool Messenger::_JoinRoom(const string& roomId, const string& userId)
    {
        bool result = false;

        MsgNotification* sink = Core::Service<MsgNotification>::Create<MsgNotification>(this, roomId, userId);
        ASSERT(sink != nullptr);

        if (sink != nullptr) {
            Exchange::IRoomAdministrator::IRoom* room = _roomAdmin->Join(roomId, userId, sink);

            // Note: Join() can return nullptr if the user has already joined the room.
            if (room != nullptr) {
                _joinedRoomsLock.Lock();
                result = _joinedRooms.emplace(std::make_pair(roomId, userId), room).second;
                _joinedRoomsLock.Unlock();

                // Interested in join/leave notifications as well
                Callback* cb = Core::Service<Callback>::Create<Callback>(this, roomId, userId);
                ASSERT(cb != nullptr);

                if (cb != nullptr) {
                    room->SetCallback(cb);
                    cb->Release(); // Make room the only owner of the callback object.
                }
            }

            sink->Release(); // Make room the only owner of the notification object.
        }

        return result;
    }

    bool Messenger::_LeaveRoom(const string& roomId, const string& userId)
    {
        bool result = false;

        _joinedRoomsLock.Lock();

        auto it(_joinedRooms.find(std::make_pair(roomId, userId)));

        if (it != _joinedRooms.end()) {
            // Exit the room.
            (*it).second->Release();

            _joinedRooms.erase(it);
            result = true;
        }

        _joinedRoomsLock.Unlock();

        return result;
    }

    bool Messenger::_SendMessage(const string& roomId, const string& userId, const string& message)
    {
        bool result = false;

        _joinedRoomsLock.Lock();

        auto it(_joinedRooms.find(std::make_pair(roomId, userId)));

        if (it != _joinedRooms.end()) {
            // Send the message to the room.
            (*it).second->SendMessage(message);

            result = true;
        }

        _joinedRoomsLock.Unlock();

        return result;
    }


    // Messenger event handlers

    void Messenger::RoomCreatedHandler(const string& roomId)
    {
        TRACE(Trace::Information, (_T("Messenger: room '%s' created"), roomId.c_str()));

        _adminLock.Lock();
        _rooms.emplace(roomId);
        _adminLock.Unlock();
    }

    void Messenger::RoomDestroyedHandler(const string& roomId)
    {
        TRACE(Trace::Information, (_T("Messenger: room '%s' destroyed"), roomId.c_str()));

        _adminLock.Lock();
        _rooms.erase(roomId);
        _adminLock.Unlock();
    }

    void Messenger::UserJoinedHandler(const string& roomId, const string& addresseId, const string& userId)
    {
        TRACE(Trace::Information, (_T("Messenger: user '%s' was notified that '%s' joined room '%s'"),
                addresseId.c_str(), userId.c_str(), roomId.c_str()));

        // TODO ...
    }

    void Messenger::UserLeftHandler(const string& roomId, const string& addresseId, const string& userId)
    {
        TRACE(Trace::Information, (_T("Messenger: user '%s' was notified that '%s' left room '%s'"),
                addresseId.c_str(), userId.c_str(), roomId.c_str()));

        // TODO ...
    }

    void Messenger::MessageHandler(const string& roomId, const string& addresseeId, const string& senderId, const string& message)
    {
        TRACE(Trace::Information, (_T("Messenger: user '%s' sent a message to '%s' in room '%s': \"%s\""),
                senderId.c_str(), addresseeId.c_str(), roomId.c_str(), message.c_str()));

        // TODO ...
    }

} // namespace Plugin

} // WPEFramework
