/*
 *
 * Copyright 2021-2024 Software Radio Systems Limited
 *
 * This file is part of srsRAN.
 *
 * srsRAN is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * srsRAN is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * A copy of the GNU Affero General Public License can be found in
 * the LICENSE file in the top-level directory of this distribution
 * and at http://www.gnu.org/licenses/.
 *
 */

#pragma once

#include "srsran/du/du_cell_config.h"

namespace srsran {
namespace srs_du {

/// \brief Configuration that enable DU modes operation for testing purposes.
struct du_test_config {
  struct test_ue_config {
    rnti_t             rnti;
    uint16_t           nof_ues;
    optional<unsigned> auto_ack_indication_delay;
    bool               pdsch_active;
    bool               pusch_active;
    unsigned           cqi;
    unsigned           ri;
    unsigned           pmi;
    unsigned           i_1_1;
    optional<unsigned> i_1_3;
    unsigned           i_2;
    std::string        working_mode;
    std::string        path;
    double             static_buffer_size;
    unsigned           min_buffer_size;
    unsigned           max_buffer_size;
    unsigned           buffer_step;
    unsigned           buffer_interval;
  };

  /// \brief Creation of a phantom UE for testing purposes.
  optional<test_ue_config> test_ue;
};

} // namespace srs_du
} // namespace srsran