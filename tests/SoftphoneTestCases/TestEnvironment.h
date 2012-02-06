#pragma once
#ifdef WIN32
#include "gtest\gtest.h"
#else
#include "gtest/gtest.h"
#endif
class CTestEnvironment :
	public testing::Environment
{
public:
	CTestEnvironment(void);
	~CTestEnvironment(void);
	virtual void SetUp();
	virtual void TearDown();
};

