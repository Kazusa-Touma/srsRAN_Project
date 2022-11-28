/*
 *
 * Copyright 2013-2022 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "pucch_processor_format2_test_data.h"
#include "srsgnb/phy/support/support_factories.h"
#include "srsgnb/phy/upper/channel_processors/channel_processor_factories.h"
#include "srsgnb/phy/upper/equalization/equalization_factories.h"
#include "fmt/ostream.h"
#include <gtest/gtest.h>

using namespace srsgnb;

using PucchProcessorF2Params = test_case_t;

namespace srsgnb {

std::ostream& operator<<(std::ostream& os, test_case_t test_case)
{
  fmt::print(os,
             "grid: {} RB x {} symb, PUCCH config: {}",
             test_case.context.grid_nof_prb,
             test_case.context.grid_nof_symbols,
             test_case.context.config);
  return os;
}

} // namespace srsgnb

class PucchProcessorF2Fixture : public ::testing::TestWithParam<PucchProcessorF2Params>
{
protected:
  // PUCCH processor factory.
  static std::shared_ptr<pucch_processor_factory> processor_factory;
  // PUCCH processor.
  std::unique_ptr<pucch_processor> processor;

  static void SetUpTestSuite()
  {
    if (!processor_factory) {
      // Create factories required by the PUCCH demodulator factory.
      std::shared_ptr<channel_equalizer_factory> equalizer_factory = create_channel_equalizer_factory_zf();
      ASSERT_NE(equalizer_factory, nullptr) << "Cannot create equalizer factory.";

      std::shared_ptr<channel_modulation_factory> demod_factory = create_channel_modulation_sw_factory();
      ASSERT_NE(demod_factory, nullptr) << "Cannot create channel modulation factory.";

      std::shared_ptr<pseudo_random_generator_factory> prg_factory = create_pseudo_random_generator_sw_factory();
      ASSERT_NE(prg_factory, nullptr) << "Cannot create pseudo-random generator factory.";

      // Create PUCCH demodulator factory.
      std::shared_ptr<pucch_demodulator_factory> pucch_demod_factory =
          create_pucch_demodulator_factory_sw(equalizer_factory, demod_factory, prg_factory);
      ASSERT_NE(pucch_demod_factory, nullptr) << "Cannot create PUCCH demodulator factory.";

      // Create factories required by the PUCCH channel estimator factory.
      std::shared_ptr<low_papr_sequence_generator_factory> lpg_factory =
          create_low_papr_sequence_generator_sw_factory();
      ASSERT_NE(lpg_factory, nullptr) << "Cannot create low PAPR sequence generator factory.";

      std::shared_ptr<low_papr_sequence_collection_factory> lpc_factory =
          create_low_papr_sequence_collection_sw_factory(lpg_factory);
      ASSERT_NE(lpc_factory, nullptr) << "Cannot create low PAPR sequence collection factory.";

      // Create channel estimator factory.
      std::shared_ptr<dmrs_pucch_estimator_factory> estimator_factory =
          create_dmrs_pucch_estimator_factory_sw(prg_factory, lpc_factory);
      ASSERT_NE(estimator_factory, nullptr) << "Cannot create DM-RS PUCCH estimator factory.";

      // Create PUCCH detector factory.
      std::shared_ptr<pucch_detector_factory> detector_factory =
          create_pucch_detector_factory_sw(lpc_factory, prg_factory);
      ASSERT_NE(detector_factory, nullptr) << "Cannot create PUCCH detector factory.";

      // Create UCI decoder factory.
      std::shared_ptr<short_block_detector_factory> short_block_det_factory = create_short_block_detector_factory_sw();
      ASSERT_NE(short_block_det_factory, nullptr) << "Cannot create short block detector factory.";

      uci_decoder_factory_sw_configuration decoder_factory_config = {};
      decoder_factory_config.decoder_factory                      = short_block_det_factory;

      std::shared_ptr<uci_decoder_factory> decoder_factory = create_uci_decoder_factory_sw(decoder_factory_config);
      ASSERT_NE(decoder_factory, nullptr) << "Cannot create UCI decoder factory.";

      channel_estimate::channel_estimate_dimensions channel_estimate_dimensions;
      channel_estimate_dimensions.nof_tx_layers = 1;
      channel_estimate_dimensions.nof_rx_ports  = 1;
      channel_estimate_dimensions.nof_symbols   = MAX_NSYMB_PER_SLOT;
      channel_estimate_dimensions.nof_prb       = MAX_RB;

      // Create PUCCH processor factory.
      processor_factory = create_pucch_processor_factory_sw(
          estimator_factory, detector_factory, pucch_demod_factory, decoder_factory, channel_estimate_dimensions);
      ASSERT_NE(processor_factory, nullptr) << "Cannot create PUCCH processor factory.";
    }
  }

  void SetUp() override
  {
    // Assert PUCCH Processor factory.
    ASSERT_NE(processor_factory, nullptr) << "Cannot create PUCCH processor factory.";

    // Create PUCCH processor.
    processor = processor_factory->create();
    ASSERT_NE(processor, nullptr) << "Cannot create PUCCH processor.";
  }
};

std::shared_ptr<pucch_processor_factory> PucchProcessorF2Fixture::processor_factory = nullptr;

TEST_P(PucchProcessorF2Fixture, PucchProcessorF2VectorTest)
{
  const test_case_t&                            test_case = GetParam();
  const context_t&                              context   = test_case.context;
  const pucch_processor::format2_configuration& config    = context.config;

  // Prepare resource grid.
  resource_grid_reader_spy grid;
  grid.write(test_case.grid.read());

  // Read expected UCI payload fields.
  std::vector<uint8_t> expected_harq_ack   = test_case.harq_ack.read();
  std::vector<uint8_t> expected_sr         = test_case.sr.read();
  std::vector<uint8_t> expected_csi_part_1 = test_case.csi_part_1.read();
  std::vector<uint8_t> expected_csi_part_2 = test_case.csi_part_2.read();

  // Process PUCCH.
  pucch_processor_result result = processor->process(grid, config);

  // Assert expected UCI payload.
  ASSERT_EQ(result.message.get_harq_ack_bits(), span<uint8_t>(expected_harq_ack));
  ASSERT_EQ(result.message.get_sr_bits(), span<uint8_t>(expected_sr));
  ASSERT_EQ(result.message.get_csi_part1_bits(), span<uint8_t>(expected_csi_part_1));
  ASSERT_EQ(result.message.get_csi_part2_bits(), span<uint8_t>(expected_csi_part_2));
}

// Creates test suite that combines all possible parameters.
INSTANTIATE_TEST_SUITE_P(PucchProcessorF2VectorTest,
                         PucchProcessorF2Fixture,
                         ::testing::ValuesIn(pucch_processor_format2_test_data));