#include "Module.h"

#include "TestController.h"
#include <interfaces/ITestController.h>

namespace WPEFramework
{
    class TestControllerImplementation : public Exchange::ITestController
    {
        private:
            TestControllerImplementation(const TestControllerImplementation&) = delete;
            TestControllerImplementation& operator=(const TestControllerImplementation&) = delete;

        public:
            TestControllerImplementation()
                : _testController(TestCore::TestController::Instance())
            {}

            virtual ~TestControllerImplementation() {}

            // ITestController methods
            string /*JSON*/ Process(const string& path, const uint8_t skipUrl, const string& body /*JSON*/)
            {
                bool exit = false;
                string result = "";
                Core::TextSegmentIterator index(Core::TextFragment(path, skipUrl, path.length() - skipUrl), false, '/');

                index.Next();
                index.Next();

                if (index.Current().Text() != _T("Methods"))
                {
                    TestCore::TestController::Iterator testAreas(_testController.Producers());

                    while (testAreas.Next() == true)
                    {
                        const std::list<string> methodsPerGroup = (*testAreas)->GetMethods();
                        for (auto const& method : methodsPerGroup)
                        {
                            if (method == index.Current().Text())
                            {
                                //ToDo: Handling of Configure is missing
                                result = (*testAreas)->Execute(index.Current().Text());
                                exit = true;
                                break;
                            }
                        }
                        if (exit)
                            break;
                    }
                }
                else // Handle default /Methods/... request separately
                {
                    if (!index.Next())
                    {
                        //ToDo: How to return supported Methods
                    }
                    else
                    {
                        string methodName = index.Current().Text();
                        index.Next();
                        string callType = index.Current().Text();

                        TestCore::TestController::Iterator testAreas(_testController.Producers());

                        while (testAreas.Next() == true)
                        {
                            const std::list<string> methodsPerGroup = (*testAreas)->GetMethods();
                            for (auto const& method : methodsPerGroup)
                            {
                                if (method == index.Current().Text())
                                {
                                    if(callType == _T("Description"))
                                    {
                                        result = (*testAreas)->GetMethodDes(index.Current().Text());
                                        exit = true;
                                        break;
                                    }
                                    else if (callType == _T("Parameters"))
                                    {
                                        result = (*testAreas)->GetMethodParam(index.Current().Text());
                                        exit = true;
                                        break;
                                    }
                                }
                            }
                            if (exit)
                                break;
                        }
                    }
                }
                return result;
            }

            BEGIN_INTERFACE_MAP(TestControllerImplementation)
                INTERFACE_ENTRY(Exchange::ITestController)
            END_INTERFACE_MAP

        private:
            TestCore::TestController& _testController;
    };

SERVICE_REGISTRATION(TestControllerImplementation, 1, 0);
}
