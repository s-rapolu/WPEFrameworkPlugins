#pragma once

#include "Module.h"

namespace WPEFramework {
namespace Plugin {
namespace TestMetadata {

class MethodDescription : public Core::JSON::Container {
private:
    MethodDescription(const MethodDescription&) = delete;
    MethodDescription& operator=(const MethodDescription&) = delete;

public:
    MethodDescription()
        : Core::JSON::Container()
        , Description()
    {
        Add(_T("description"), &Description);
    }

    ~MethodDescription() {}

public:
    Core::JSON::String Description;
};

class Methods : public Core::JSON::Container {
private:
    Methods(const Methods&) = delete;
    Methods& operator=(const Methods&) = delete;

public:
    Methods()
        : Core::JSON::Container()
        , MethodNames()
    {
        Add(_T("methodNames"), &MethodNames);
    }
    ~Methods() {}

public:
    Core::JSON::ArrayType<Core::JSON::String> MethodNames;
};

class Parameters : public Core::JSON::Container {
private:
    Parameters(const Parameters&) = delete;
    Parameters& operator=(const Parameters&) = delete;

public:
    class Parameter : public Core::JSON::Container {
    private:
        Parameter& operator=(const Parameter& rhs) = delete;

    public:
        Parameter()
            : Core::JSON::Container()
            , Name()
            , Type()
            , Comment()

        {
            addFields();
        }

        Parameter(const string& name, const string& type, const string& comment)
            : Core::JSON::Container()
            , Name(name)
            , Type(type)
            , Comment(comment)
        {
            addFields();
        }

        Parameter(const Parameter& copy)
            : Core::JSON::Container()
            , Name(copy.Name)
            , Type(copy.Type)
            , Comment(copy.Comment)
        {
            addFields();
        }

        ~Parameter() {}

    private:
        void addFields()
        {
            Add(_T("name"), &Name);
            Add(_T("type"), &Type);
            Add(_T("comment"), &Comment);
        }

    public:
        Core::JSON::String Name;
        Core::JSON::String Type;
        Core::JSON::String Comment;
    };

public:
    Parameters()
        : Core::JSON::Container()
        , Input()
        , Output()
    {
        Add(_T("input"), &Input);
        Add(_T("output"), &Output);
    }
    ~Parameters() {}

public:
    Core::JSON::ArrayType<Parameter> Input;
    Core::JSON::ArrayType<Parameter> Output;
};
} // namespace TestMetadata
} // namespace Plugin
} // namespace WPEFramework
