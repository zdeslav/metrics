#pragma once

#include "gtest/gtest.h"

// fixture ctor/dtor are called before/after each test in fixture
class MyFixture : public testing::Test
{
protected:
    int m_target;
    MyFixture() : m_target(10) {}
    ~MyFixture() { m_target = 0; }
};

/*TEST_F(MyFixture, FirstTest)
{
    EXPECT_EQ(m_target, 10);
    m_target = 1;
}

TEST_F(MyFixture, SecondTest)
{
    EXPECT_EQ(m_target, 10);
    m_target = 2;
}   */