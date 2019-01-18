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
                , _previousTestSuiteName(EMPTY_STRING)
                , _previousTestSuite(nullptr)
            {}

            virtual ~TestControllerImplementation() {}

            // ITestController methods
            string /*JSON*/ Process(const string& path, const uint8_t skipUrl, const string& body /*JSON*/)
            {
                bool executed = false;
                // Return empty result in case of issue
                string /*JSON*/ response = EMPTY_STRING;

                Core::TextSegmentIterator index(Core::TextFragment(path, skipUrl, path.length() - skipUrl), false, '/');

                index.Next();
                index.Next();

                TestCore::TestController::Iterator testAreas(_testController.TestSuites());

                if (index.Current().Text() == _T("TestSuites"))
                {
                    response = _testController.GetTestSuites();
                    executed = true;
                }
                else
                {
                    string currentTestSuite = index.Current().Text();

                    while (testAreas.Next() == true)
                    {
                        if (testAreas.Key() == currentTestSuite)
                        {
                            //Found test suite
                            if ((currentTestSuite != _previousTestSuiteName) && (_previousTestSuiteName != EMPTY_STRING))
                            {
                                //Cleanup before run any kind of test from different Test Suite
                                _previousTestSuite->Cleanup();
                            }

                            index.Next();
                            //Get remaining paths, this will be treat as full method name
                            if (index.Remainder().Length() != 0)
                            {
                                //Setup each test before execution, it is up to Test Suite to handle Setup method
                                testAreas.Current()->Setup(body);
                                //Execute
                                response = testAreas.Current()->Execute(index.Remainder().Text());
                                executed = true;
                                _previousTestSuiteName = currentTestSuite;
                                _previousTestSuite = testAreas.Current();
                            }
                            break;
                        }
                    }
                }

                if (!executed)
                {
                    TRACE(Trace::Fatal, (_T("*** Test case method not found !!! ***")))
                }

                return response;
            }

            BEGIN_INTERFACE_MAP(TestControllerImplementation)
                INTERFACE_ENTRY(Exchange::ITestController)
            END_INTERFACE_MAP

        private:
            TestCore::TestController& _testController;
            string _previousTestSuiteName;
            TestCore::ITestSuite* _previousTestSuite;
    };

SERVICE_REGISTRATION(TestControllerImplementation, 1, 0);
}
