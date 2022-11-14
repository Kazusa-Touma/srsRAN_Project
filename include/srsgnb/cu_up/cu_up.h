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

#include "srsgnb/e1_interface/common/e1_common.h"

#include <string>

namespace srsgnb {
namespace srs_cu_up {

/// Interface to notify about E1 connections (from the CU-CP) to the CU-UP
class cu_up_e1_connection_notifier
{
public:
  virtual ~cu_up_e1_connection_notifier() = default;

  /// \brief Notifies the CU-UP about a successful E1 connection.
  virtual void on_e1_connection_establish() = 0;

  /// \brief Notifies the CU-CP about a dropped E1 connection.
  virtual void on_e1_connection_drop() = 0;
};

class cu_up_e1_interface
{
public:
  virtual ~cu_up_e1_interface() = default;

  /// \brief Get the E1 message handler interface.
  virtual e1_message_handler& get_e1_message_handler() = 0;

  /// \brief Get the state of the E1 connection.
  /// \return True if E1 is connected, false otherwise.
  virtual bool e1_is_connected() = 0;
};

class cu_up_interface : public cu_up_e1_connection_notifier, public cu_up_e1_interface
{
public:
  virtual ~cu_up_interface() = default;
};

} // namespace srs_cu_up
} // namespace srsgnb