#pragma once

#include "Module.h"

namespace WPEFramework {
namespace TestCore {

class TestCaseDescription : public Core::JSON::Container {
private:
    TestCaseDescription(const TestCaseDescription&) = delete;
    TestCaseDescription& operator=(const TestCaseDescription&) = delete;

public:
    TestCaseDescription()
        : Core::JSON::Container()
        , Description()
    {
        Add(_T("description"), &Description);
    }

    ~TestCaseDescription() {}

public:
    Core::JSON::String Description;
};

class TestCases : public Core::JSON::Container {
private:
    TestCases(const TestCases&) = delete;
    TestCases& operator=(const TestCases&) = delete;

public:
    TestCases()
        : Core::JSON::Container()
        , TestCaseNames()
    {
        Add(_T("testCases"), &TestCaseNames);
    }
    ~TestCases() {}

public:
    Core::JSON::ArrayType<Core::JSON::String> TestCaseNames;
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
} //namespace TestCore
} // namespace WPEFramework
