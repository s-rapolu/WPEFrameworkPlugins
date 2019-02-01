#pragma once

#include <map>

#include <interfaces/ISystemCommands.h>

namespace WPEFramework {
namespace Plugin {

    class SystemCommandsImplementation : public Exchange::ISystemCommands {
    public:
        SystemCommandsImplementation();
        ~SystemCommandsImplementation() override;

        uint32_t Execute(const Exchange::ISystemCommands::CommandId& id, const string& params) override;
        RPC::IStringIterator* SupportedCommands() const override;
        Exchange::ISystemCommands::ICommand* Command(const Exchange::ISystemCommands::CommandId& id) const override;

        static void Register(Exchange::ISystemCommands::ICommand* command) {
            Store::Instance().Register(command->Id(), command);
        }

        static void Unregister(Exchange::ISystemCommands::ICommand* command) {
            Store::Instance().Unregister(command);
        }

        BEGIN_INTERFACE_MAP(SystemCommandsImplementation)
            INTERFACE_ENTRY(Exchange::ISystemCommands)
        END_INTERFACE_MAP

    private:
        class Store {
        public:
            static Store& Instance() {
                static Store instance;
                return instance;
            }

            void Register(const Exchange::ISystemCommands::CommandId& id,
                          Exchange::ISystemCommands::ICommand* command) {
                _adminLock.Lock();
                auto inserted = _commands.insert(std::pair<Exchange::ISystemCommands::CommandId,
                                                           Exchange::ISystemCommands::ICommand*>(id, command));
                ASSERT(inserted.second);
                _adminLock.Unlock();
            }

            void Unregister(Exchange::ISystemCommands::ICommand* command) {
              _adminLock.Lock();
              auto found = std::find_if(_commands.begin(),
                                        _commands.end(),
                                        [command](const std::pair<Exchange::ISystemCommands::CommandId,
                                                  Exchange::ISystemCommands::ICommand*>& entry) {
                 return command == entry.second;
              });
              if (found != _commands.end())
                _commands.erase(found);
              _adminLock.Unlock();
            }

            std::vector<Exchange::ISystemCommands::CommandId> Commands() const {
              std::vector<Exchange::ISystemCommands::CommandId> result;
              _adminLock.Lock();
              for (auto& command : _commands) {
                result.push_back(command.first);
              }
              _adminLock.Unlock();
              return result;
            }

            Exchange::ISystemCommands::ICommand* Command(const Exchange::ISystemCommands::CommandId& id) const {
                Exchange::ISystemCommands::ICommand* result;
                _adminLock.Lock();
                auto found = _commands.find(id);
                if (found != _commands.end()) {
                  result = found->second;
                }
                _adminLock.Unlock();
                return result;
            }

        private:
            mutable Core::CriticalSection _adminLock;
            std::map<Exchange::ISystemCommands::CommandId,
                     Exchange::ISystemCommands::ICommand*> _commands;
        };
        SystemCommandsImplementation(const SystemCommandsImplementation&) = delete;
        SystemCommandsImplementation& operator=(const SystemCommandsImplementation&) = delete;
    };
}
}
