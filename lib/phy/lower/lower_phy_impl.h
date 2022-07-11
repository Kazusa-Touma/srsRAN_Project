/*
 *
 * Copyright 2013-2022 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#ifndef SRSGNB_LIB_PHY_LOWER_LOWER_PHY_IMPL_H
#define SRSGNB_LIB_PHY_LOWER_LOWER_PHY_IMPL_H

#include "lower_phy_state_fsm.h"
#include "srsgnb/adt/circular_array.h"
#include "srsgnb/adt/optional.h"
#include "srsgnb/gateways/baseband/baseband_gateway.h"
#include "srsgnb/phy/lower/lower_phy_configuration.h"
#include "srsgnb/phy/lower/lower_phy_controller.h"
#include "srsgnb/phy/lower/lower_phy_input_gateway.h"
#include "srsgnb/phy/lower/lower_phy_rx_symbol_notifier.h"
#include "srsgnb/phy/lower/lower_phy_timing_notifier.h"
#include "srsgnb/phy/lower/modulation/ofdm_demodulator.h"
#include "srsgnb/phy/lower/modulation/ofdm_modulator.h"
#include "srsgnb/phy/resource_grid_pool.h"

namespace srsgnb {

/// Describes the factory configuration.
struct lower_phy_common_configuration {
  /// Provides OFDM modulators. Each entry belongs to a different sector.
  std::vector<std::unique_ptr<ofdm_symbol_modulator>> modulators;
  /// Provides OFDM demodulators. Each entry belongs to a different sector.
  std::vector<std::unique_ptr<ofdm_symbol_demodulator>> demodulators;
};

template <class RG>
class lower_phy_rg_buffer
{
private:
  /// Stores resource grid pointers for every sector.
  std::vector<RG*> grids;
  /// Protects grids access from concurrent read/write.
  std::mutex mutex;

public:
  void set_nof_sectors(unsigned nof_sectors) { grids.resize(nof_sectors); }

  void set_grid(RG& grid, unsigned sector_id)
  {
    std::unique_lock<std::mutex> lock(mutex);
    grids[sector_id] = &grid;
  }

  RG* get_grid(unsigned sector_id)
  {
    std::unique_lock<std::mutex> lock(mutex);
    return grids[sector_id];
  }

  /// Resets all pointers.
  void reset()
  {
    std::unique_lock<std::mutex> lock(mutex);
    for (auto& rg : grids) {
      rg = nullptr;
    }
  }
};

/// Describes a generic lower physical layer.
class lower_phy_impl : public lower_phy_controller
{
private:
  /// Number of resource grid buffers.
  static constexpr unsigned NOF_RG_BUFFER = 40;

  /// Logger.
  srslog::basic_logger& logger;
  /// Baseband gateway transmitter.
  baseband_gateway_transmitter& transmitter;
  /// Baseband gateway receiver.
  baseband_gateway_receiver& receiver;
  /// Receive symbol handler.
  lower_phy_rx_symbol_notifier& rx_symbol_notifier;
  /// Timing boundary handler.
  lower_phy_timing_notifier& timing_notifier;
  /// Stores downlink resource grids buffers. Each entry belongs to a slot.
  circular_array<lower_phy_rg_buffer<const resource_grid_reader>, NOF_RG_BUFFER> dl_rg_buffers;
  /// Stores uplink resource grids buffers. Each entry belongs to a slot.
  circular_array<lower_phy_rg_buffer<resource_grid>, NOF_RG_BUFFER> ul_rg_buffers;
  /// Stores radio baseband buffers for each stream. Common for transmit and receive. The number of entries indicates
  /// the number of streams.
  std::vector<baseband_gateway_buffer_dynamic> radio_buffers;
  /// Stores radio receive metadata for each stream. The number of entries indicates the number of streams.
  std::vector<baseband_gateway_receiver::metadata> receive_metadata;
  /// Stores OFDM modulators. Each entry belongs to a different sector.
  std::vector<std::unique_ptr<ofdm_symbol_modulator>> modulators;
  /// Stores OFDM demodulators. Each entry belongs to a different sector.
  std::vector<std::unique_ptr<ofdm_symbol_demodulator>> demodulators;
  /// Indicates the receive to transmit delay in clock ticks.
  const baseband_gateway_timestamp rx_to_tx_delay;
  /// Indicates the maximum allowed processing delay in slots.
  const unsigned max_processing_delay_slots;
  /// Indicates the number of symbols per slot.
  const unsigned nof_symbols_per_slot;
  /// Provides the sectors configuration.
  const std::vector<lower_phy_sector_description> sectors;
  /// Current uplink processing slot context.
  slot_point ul_slot_context = {};
  /// Current downlink processing slot context.
  slot_point dl_slot_context = {};
  /// Current symbol index within the processing slot.
  unsigned symbol_slot_idx = 0;
  /// Indicates the asynchronous processing shall stop.
  lower_phy_state_fsm state_fsm;

  /// \brief Processes uplink symbol.
  /// \param[in] symbol_idx Indicates the symbol index within a subframe.
  /// \return The radio timestamp of the received block.
  baseband_gateway_timestamp process_ul_symbol(unsigned symbol_idx);

  /// \brief Processes downlink symbol.
  /// \param[in] symbol_idx Indicates the symbol index within a subframe.
  /// \param[out] timestamp Indicates the radio timestamp for transmitting the symbol.
  void process_dl_symbol(unsigned symbol_idx, baseband_gateway_timestamp timestamp);

  /// \brief Processes uplink and downlink slot.
  void process_symbol();

  /// Runs the lower physical layer.
  void realtime_process_loop(task_executor& realtime_task_executor);

  // See interface for documentation.
  void request_prach_window(const prach_buffer_context& context, prach_buffer* buffer) override;

  // See interface for documentation.
  void request_uplink_slot(const resource_grid_context& context, resource_grid& grid) override;

  // See interface for documentation.
  void send(const resource_grid_context& context, const resource_grid_reader& grid) override;

public:
  /// \brief Constructs a generic lower physical layer.
  /// \param[in] common_config Provides the factory specific necessary parameters to construct the lower physical
  /// layer.
  /// \param[in] config Provides the common lower PHY parameters to construct the lower physical layer.
  explicit lower_phy_impl(lower_phy_common_configuration& common_config, const lower_phy_configuration& config);

  // See interface for documentation.
  void start(task_executor& realtime_task_executor) override;

  // See interface for documentation.
  void stop() override;
};

} // namespace srsgnb

#endif // SRSGNB_LIB_PHY_LOWER_LOWER_PHY_IMPL_H
