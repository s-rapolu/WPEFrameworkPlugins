#pragma once

#include "Module.h"
#include <interfaces/IMessenger.h>
#include <set>

namespace WPEFramework {

namespace Plugin {

    class Messenger : public PluginHost::IPlugin
                    , public PluginHost::IWeb
                    , public Exchange::IRoomAdministrator::INotification {
    public:
        Messenger(const Messenger&) = delete;
        Messenger& operator=(const Messenger&) = delete;

        Messenger()
            : _pid(0)
            , _skipUrl(0)
            , _service(nullptr)
            , _roomAdmin(nullptr)
            , _rooms()
            , _joinedRooms()
            , _adminLock()
            , _joinedRoomsLock()
        { /* empty */ }

        class RoomsData : public Core::JSON::Container {
        public:
            RoomsData(const RoomsData&) = delete;
            RoomsData& operator=(const RoomsData&) = delete;

            RoomsData()
                : Rooms()
            {
                Add(_T("rooms"), &Rooms);
            }

            Core::JSON::ArrayType<Core::JSON::String> Rooms;
        }; // class RoomsData

        class RoomMembersData : public Core::JSON::Container {
        public:
            RoomMembersData(const RoomMembersData&) = delete;
            RoomMembersData& operator=(const RoomMembersData&) = delete;

            RoomMembersData()
                : RoomMembers()
            {
                Add(_T("members"), &RoomMembers);
            }

            Core::JSON::ArrayType<Core::JSON::String> RoomMembers;
        }; // class RoomMembersData

        // IPlugin methods
        virtual const string Initialize(PluginHost::IShell* service) override;
        virtual void Deinitialize(PluginHost::IShell* service) override;
        virtual string Information() const override  { return { }; }

        // IWeb methods
        virtual void Inbound(Web::Request& request) override;
        virtual Core::ProxyType<Web::Response> Process(const Web::Request& request) override;

        // IMessenger::INotification methods
        void Created(const string& roomId) override { RoomCreatedHandler(roomId); }
        void Destroyed(const string& roomId) override { RoomDestroyedHandler(roomId); }

        // Notification handling
        class MsgNotification : public Exchange::IRoomAdministrator::IRoom::IMsgNotification {
        public:
            MsgNotification(const MsgNotification&) = delete;
            MsgNotification& operator=(const MsgNotification&) = delete;

            MsgNotification(Messenger* messenger, const string& roomId, const string& userId)
                : _messenger(messenger)
                , _roomId(roomId)
                , _userId(userId)
            { /* empty */ }

            // IRoom::Notification methods
            virtual void Message(const string& senderId, const string& message) override
            {
                ASSERT(_messenger != nullptr);
                _messenger->MessageHandler(_roomId, _userId, senderId, message);
            }

            // QueryInterface implementation
            BEGIN_INTERFACE_MAP(Callback)
                INTERFACE_ENTRY(Exchange::IRoomAdministrator::IRoom::IMsgNotification)
            END_INTERFACE_MAP

        private:
            Messenger* _messenger;
            string _roomId;
            string _userId;
        }; // class Notification

        // Callback handling
        class Callback : public Exchange::IRoomAdministrator::IRoom::ICallback {
        public:
            Callback(const Callback&) = delete;
            Callback& operator=(const Callback&) = delete;

            Callback(Messenger* messenger, const string& roomId, const string& userId)
                : _messenger(messenger)
                , _userId(userId)
                , _roomId(roomId)
            { /* empty */}

            // IRoom::ICallback methods
            virtual void Joined(const string& userId) override
            {
                ASSERT(_messenger != nullptr);
                _messenger->UserJoinedHandler(_roomId, _userId, userId);
            }

            virtual void Left(const string& userId) override
            {
                ASSERT(_messenger != nullptr);
                _messenger->UserLeftHandler(_roomId, _userId, userId);
            }

            // QueryInterface implementation
            BEGIN_INTERFACE_MAP(Callback)
                INTERFACE_ENTRY(Exchange::IRoomAdministrator::IRoom::ICallback)
            END_INTERFACE_MAP

        private:
            Messenger* _messenger;
            string _userId;
            string _roomId;
        }; // class Callback

        // Messenger event methods
        void RoomCreatedHandler(const string& roomId);
        void RoomDestroyedHandler(const string& roomId);
        void UserJoinedHandler(const string& roomId, const string& addresseeId, const string& userId);
        void UserLeftHandler(const string& roomId, const string& addresseeId, const string& userId);
        void MessageHandler(const string& roomId, const string& addresseeId, const string& senderId, const string& message);

        // QueryInterface implementation
        BEGIN_INTERFACE_MAP(Messenger)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(PluginHost::IWeb)
            INTERFACE_ENTRY(Exchange::IRoomAdministrator::INotification)
            INTERFACE_AGGREGATE(Exchange::IRoomAdministrator, _roomAdmin)
        END_INTERFACE_MAP

    private:
        Core::ProxyType<Web::Response> _GetMethod(Core::TextSegmentIterator& index, const Web::Request& request);
        Core::ProxyType<Web::Response> _PostMethod(Core::TextSegmentIterator& index, const Web::Request& request);

        // Web request handlers
        void _GetRooms(Core::JSON::ArrayType<Core::JSON::String>& rooms) const;
        bool _GetRoomMembers(const string& roomId, Core::JSON::ArrayType<Core::JSON::String>& roomMembers) const;
        bool _JoinRoom(const string& roomId, const string& userId);
        bool _LeaveRoom(const string& roomId, const string& userId);
        bool _SendMessage(const string& roomId, const string& userId, const string& message);

        uint32_t _pid;
        size_t _skipUrl;
        PluginHost::IShell* _service;
        Exchange::IRoomAdministrator* _roomAdmin;
        std::set<string> _rooms;
        std::map<std::pair<string /* roomId */, string /* userId */>, Exchange::IRoomAdministrator::IRoom*> _joinedRooms;
        mutable Core::CriticalSection _adminLock;
        mutable Core::CriticalSection _joinedRoomsLock;
    }; // class Messenger

} // namespace Plugin

} // namespace WPEFramework
