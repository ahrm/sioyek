#include <CppUnitTest.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

//Microsoft native test fraework

namespace UnitTest1 {

	TEST_CLASS(SomeTest) {
public:
	TEST_METHOD(onePlusOne) {
		Assert::AreEqual(3, 1 + 1);
	}
	};
}
