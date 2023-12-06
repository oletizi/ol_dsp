#include "gtest/gtest.h"
#include <ol_synthlib.h>

using namespace ol::synth;
TEST(BasicTestSuite, BasicTest) {
    EXPECT_EQ(1, 1);
}

class Person {
};

class Animal {
    Animal(const Person &friend1 = Person(), const Person &friend2 = Person()) : friend1_(friend1), friend2_(friend2) {}

private:
    const Person &friend1_;
    const Person &friend2_;
};


TEST(BasicTestSuite, ConstructorTest) {


}