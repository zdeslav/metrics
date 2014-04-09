// test.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "client_tests.h"
#include "server_tests.h"

int _tmain(int argc, _TCHAR* argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

