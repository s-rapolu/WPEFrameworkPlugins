#pragma once

#include "Module.h"

namespace WPEFramework {
namespace TestCore {

struct ITestSuite : virtual public Core::IUnknown
{
    enum { ID = 0x13000011 };

    virtual ~ITestSuite() {}

    virtual void Setup(const string& body /*JSON*/) = 0;
    virtual string/*JSON*/ Execute(const string& testCase) = 0;
    virtual void Cleanup(void) = 0;

    //ToDo: Consider to change this interface
    virtual string/*JSON*/ GetTestCases(void) = 0;
    virtual string/*JSON*/ GetTestCaseDescription(const string& testCase) = 0;
    virtual string/*JSON*/ GetTestCaseParameters(const string& testCase) = 0;
};
} // namespace TestCore
} // namespace WPEFramework
