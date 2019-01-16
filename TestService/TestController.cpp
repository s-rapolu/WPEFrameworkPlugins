#include "TestController.h"

namespace WPEFramework {
namespace TestCore {

/* static */ TestController& TestController::Instance()
{
    static TestController _singleton;
    return (_singleton);
}
} // namespace TestCore
} // namespace WPEFramework
