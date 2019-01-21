#include "TestUtility.h"

namespace WPEFramework {

namespace Plugin {

SERVICE_REGISTRATION(TestUtility, 1, 0);

TestUtility::CommandAdministrator& TestUtility::CommandAdministrator::Instance() {
    static TestUtility::CommandAdministrator singleton;
    return singleton;
}

template<typename DUMMY>
const char* const TestUtility::CommandAdministrator::TypeToNameConversion::Supported<uint8_t, DUMMY>::name = "unsigned int 8";
template<typename DUMMY>
const char* const TestUtility::CommandAdministrator::TypeToNameConversion::Supported<bool, DUMMY>::name = "boolean";

}   
}
