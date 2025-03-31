#include "tests.h"

int main() {
    try {
        TestInitialization();
        TestAssignment();
        TestMoveAssignment();
        TestValueAccess();
        TestReset();
        TestEmplace();
    }
    catch (...) {
        assert(false);
    }
}