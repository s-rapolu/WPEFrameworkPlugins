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

                SYSLOG(Trace::Fatal, (_T("*** TestControllerImplementation::Process ***")))
                bool execute = false;
                // Return empty result in case of issue
                string result = EMPTY_STRING;

                Core::TextSegmentIterator index(Core::TextFragment(path, skipUrl, path.length() - skipUrl), false, '/');

                index.Next();
                index.Next();

                if (index.Current().Text() == _T("TestSuites"))
                {
                    TestCore::TestController::Iterator testAreas(_testController.TestSuites());
                    while (testAreas.Next() == true)
                    {
                        //ToDo: Build response in TestController, for now list registered TestSuites
                        string group = testAreas.Key();
                        SYSLOG(Trace::Fatal, (_T("*** Test Groups: %s ***"), group.c_str()))
                    }
                }
                else
                {
                    string testSuite = index.Current().Text();
                    SYSLOG(Trace::Fatal, (_T("*** TestSuite %s ***"), testSuite.c_str()))

                    TestCore::TestController::Iterator testAreas(_testController.TestSuites());
                    while (testAreas.Next() == true)
                    {
                        if (testAreas.Key() == testSuite)
                        {
                            //Found test suite
                            index.Next();
                            //Get remaining paths, this is full method name
                            if (index.Remainder().Length() != 0)
                            {
                                string testSuiteMethod = index.Remainder().Text();
                                testAreas.Current()->Execute(testSuiteMethod);
                                execute = true;
                            }
                            break;
                        }
                    }
                }

                if (!execute)
                {
                    SYSLOG(Trace::Fatal, (_T("*** Test case method not found ***")))
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
