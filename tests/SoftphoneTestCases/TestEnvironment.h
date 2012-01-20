#pragma once
#include "gtest\gtest.h"
class CTestEnvironment :
	public testing::Environment
{
public:
	CTestEnvironment(void);
	~CTestEnvironment(void);
	virtual void SetUp();
	virtual void TearDown();
};

