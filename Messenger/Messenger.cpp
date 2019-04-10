#include "Module.h"
#include "Messenger.h"

namespace WPEFramework {

namespace Plugin {

    SERVICE_REGISTRATION(Messenger, 1, 0);

    // IPlugin methods

    /* virtual */ const string Messenger::Initialize(PluginHost::IShell* service)
    {
        ASSERT(service != nullptr);
        ASSERT(_service == nullptr);
        ASSERT(_roomAdmin == nullptr);
        ASSERT(_rooms.empty() == true);

        _service = service;
        _service->AddRef();

        _roomAdmin = service->Root<Exchange::IRoomAdministrator>(_pid, 2000, _T("RoomMaintainer"));
        ASSERT(_roomAdmin != nullptr);

        _roomAdmin->Register(this);

        return { };
    }

    /* virtual */ void Messenger::Deinitialize(PluginHost::IShell* service)
    {
        ASSERT(service == _service);

        // Exit all the rooms (if any) that were joined by this client
        for (auto& room : _rooms) {
            room.second->Release();
        }

        _rooms.clear();

        _roomAdmin->Unregister(this);

        _roomAdmin->Release();
        _roomAdmin = nullptr;

        _service->Release();
        _service = nullptr;
    }

    // Web request handlers

    string Messenger::JoinRoom(const string& roomName, const string& userName)
    {
        bool result = false;

        // TODO: replace with a hash!
        string roomId = roomName + userName;

        MsgNotification* sink = Core::Service<MsgNotification>::Create<MsgNotification>(this, roomId);
        ASSERT(sink != nullptr);

        if (sink != nullptr) {
            Exchange::IRoomAdministrator::IRoom* room = _roomAdmin->Join(roomName, userName, sink);

            // Note: Join() can return nullptr if the user has already joined the room.
            if (room != nullptr) {

                _adminLock.Lock();
                bool emplaced = _rooms.emplace(roomId, room).second;
                _adminLock.Unlock();
                ASSERT(emplaced);

                // Interested in join/leave notifications as well
                Callback* cb = Core::Service<Callback>::Create<Callback>(this, roomId);
                ASSERT(cb != nullptr);

                if (cb != nullptr) {
                    room->SetCallback(cb);
                    cb->Release(); // Make room the only owner of the callback object.
                }

                result = true;
            }

            sink->Release(); // Make room the only owner of the notification object.
        }

        return (result? roomId : string{});
    }

    bool Messenger::LeaveRoom(const string& roomId)
    {
        bool result = false;

        _adminLock.Lock();

        auto it(_rooms.find(roomId));

        if (it != _rooms.end()) {
            // Exit the room.
            (*it).second->Release();
            // Invalidate the room ID.
            _rooms.erase(it);
            result = true;
        }

        _adminLock.Unlock();

        return result;
    }

    bool Messenger::SendMessage(const string& roomId, const string& message)
    {
        bool result = false;

        _adminLock.Lock();

        auto it(_rooms.find(roomId));

        if (it != _rooms.end()) {
            // Send the message to the room.
            (*it).second->SendMessage(message);
            result = true;
        }

        _adminLock.Unlock();

        return result;
    }

} // namespace Plugin

} // WPEFramework
