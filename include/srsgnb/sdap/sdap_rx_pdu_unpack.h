/*
 *
 * Copyright 2013-2022 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#pragma once

#include "srsgnb/adt/byte_buffer.h"

namespace srsgnb {

//: TODO: The sdap_entity interface is a temporary name that if it gets used by the final implementation we'll ned to
//: find a better name to avoid confusion with a SDAP entity. Entity here is used to represent a business domain entity.

/// SDAP entity receiving side. Retrieves an SDAP SDU from a lower layer data PDU.
class sdap_rx_pdu_unpack
{
public:
  virtual ~sdap_rx_pdu_unpack() = default;

  /// Decapsulates the SDAP header in the provided PDU converting it into an SDAP SDU.
  virtual bool decapsulate(byte_buffer& data) = 0;
};

} // namespace srsgnb