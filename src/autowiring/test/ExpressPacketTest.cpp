// Copyright (C) 2012-2015 Leap Motion, Inc. All rights reserved.
#include "stdafx.h"
#include <autowiring/autowiring.h>
#include "ExpressPacketFactory.h"

class ExpressPacketTest:
  public testing::Test
{
public:
  AutoCurrentContext ctxt;
};

TEST_F(ExpressPacketTest, VerifyTrivialCase) {
  AutoRequired<ExpressPacketFactory> epf;
}