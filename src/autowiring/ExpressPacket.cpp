#include "stdafx.h"
#include "ExpressPacket.h"
#include "AutoPacketFactory.h"

ExpressPacket::ExpressPacket(AutoPacketFactory& factory, std::shared_ptr<void>&& outstanding):
  m_parentFactory(factory.GetSelf<AutoPacketFactory>()),
  m_outstanding(std::move(outstanding))
{
}

ExpressPacket::~ExpressPacket(void) {
}
