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

#include "srsran/phy/upper/channel_processors/channel_processor_factories.h"
#include "pbch_encoder_impl.h"
#include "pbch_modulator_impl.h"
#include "pdcch_encoder_impl.h"
#include "pdcch_modulator_impl.h"
#include "pdcch_processor_impl.h"
#include "pdcch_processor_pool.h"
#include "pdsch_encoder_hw_impl.h"
#include "pdsch_encoder_impl.h"
#include "pdsch_modulator_impl.h"
#include "pdsch_processor_concurrent_impl.h"
#include "pdsch_processor_impl.h"
#include "pdsch_processor_lite_impl.h"
#include "pdsch_processor_pool.h"
#include "pdsch_processor_validator_impl.h"
#include "prach_detector_generic_impl.h"
#include "prach_detector_pool.h"
#include "prach_generator_impl.h"
#include "pucch_demodulator_impl.h"
#include "pucch_detector_impl.h"
#include "pucch_processor_impl.h"
#include "pucch_processor_pool.h"
#include "ssb_processor_impl.h"
#include "ssb_processor_pool.h"
#include "srsran/phy/support/support_formatters.h"
#include "srsran/phy/upper/channel_modulation/channel_modulation_factories.h"
#include "srsran/phy/upper/sequence_generators/sequence_generator_factories.h"
#include "srsran/srsvec/bit.h"
#include "srsran/srsvec/zero.h"
#include "srsran/support/timestamp_logger.h"
#include "srsran/support/scheduler.h"
#include <fmt/core.h>

using namespace srsran;

namespace {

class pbch_encoder_factory_sw : public pbch_encoder_factory
{
public:
  pbch_encoder_factory_sw(std::shared_ptr<crc_calculator_factory>          crc_calc_factory_,
                          std::shared_ptr<pseudo_random_generator_factory> prg_factory_,
                          std::shared_ptr<polar_factory>                   polar_fec_factory_) :
    crc_calc_factory(std::move(crc_calc_factory_)),
    prg_factory(std::move(prg_factory_)),
    polar_fec_factory(std::move(polar_fec_factory_))
  {
    srsran_assert(crc_calc_factory, "Invalid CRC factory.");
    srsran_assert(prg_factory, "Invalid CRC factory.");
    srsran_assert(polar_fec_factory, "Invalid polar factory.");
  }

  std::unique_ptr<pbch_encoder> create() override
  {
    return std::make_unique<pbch_encoder_impl>(
        create_crc_calculator_factory_sw("auto")->create(crc_generator_poly::CRC24C),
        create_pseudo_random_generator_sw_factory()->create(),
        polar_fec_factory->create_interleaver(),
        polar_fec_factory->create_allocator(),
        polar_fec_factory->create_code(),
        polar_fec_factory->create_encoder(),
        polar_fec_factory->create_rate_matcher());
  }

private:
  std::shared_ptr<crc_calculator_factory>          crc_calc_factory;
  std::shared_ptr<pseudo_random_generator_factory> prg_factory;
  std::shared_ptr<polar_factory>                   polar_fec_factory;
};

class pbch_modulator_factory_sw : public pbch_modulator_factory
{
private:
  std::shared_ptr<channel_modulation_factory>      modulator_factory;
  std::shared_ptr<pseudo_random_generator_factory> prg_factory;

public:
  pbch_modulator_factory_sw(std::shared_ptr<channel_modulation_factory>      modulator_factory_,
                            std::shared_ptr<pseudo_random_generator_factory> prg_factory_) :
    modulator_factory(std::move(modulator_factory_)), prg_factory(std::move(prg_factory_))
  {
    srsran_assert(modulator_factory, "Invalid modulator factory.");
    srsran_assert(prg_factory, "Invalid PRG factory.");
  }

  std::unique_ptr<pbch_modulator> create() override
  {
    return std::make_unique<pbch_modulator_impl>(modulator_factory->create_modulation_mapper(), prg_factory->create());
  }
};

class pdcch_modulator_factory_sw : public pdcch_modulator_factory
{
private:
  std::shared_ptr<channel_modulation_factory>      modulator_factory;
  std::shared_ptr<pseudo_random_generator_factory> prg_factory;

public:
  pdcch_modulator_factory_sw(std::shared_ptr<channel_modulation_factory>      modulator_factory_,
                             std::shared_ptr<pseudo_random_generator_factory> prg_factory_) :
    modulator_factory(std::move(modulator_factory_)), prg_factory(std::move(prg_factory_))
  {
    srsran_assert(modulator_factory, "Invalid modulator factory.");
    srsran_assert(prg_factory, "Invalid PRG factory.");
  }

  std::unique_ptr<pdcch_modulator> create() override
  {
    return std::make_unique<pdcch_modulator_impl>(modulator_factory->create_modulation_mapper(), prg_factory->create());
  }
};

class pdcch_encoder_factory_sw : public pdcch_encoder_factory
{
private:
  std::shared_ptr<crc_calculator_factory> crc_factory;
  std::shared_ptr<polar_factory>          polar_code_factory;

public:
  pdcch_encoder_factory_sw(std::shared_ptr<crc_calculator_factory> crc_factory_,
                           std::shared_ptr<polar_factory>          polar_code_factory_) :
    crc_factory(std::move(crc_factory_)), polar_code_factory(std::move(polar_code_factory_))
  {
    srsran_assert(crc_factory, "Invalid CRC calculator factory.");
    srsran_assert(polar_code_factory, "Invalid Polar code factory.");
  }

  std::unique_ptr<pdcch_encoder> create() override
  {
    return std::make_unique<pdcch_encoder_impl>(crc_factory->create(crc_generator_poly::CRC24C),
                                                polar_code_factory->create_interleaver(),
                                                polar_code_factory->create_allocator(),
                                                polar_code_factory->create_code(),
                                                polar_code_factory->create_encoder(),
                                                polar_code_factory->create_rate_matcher());
  }
};

class pdcch_processor_factory_sw : public pdcch_processor_factory
{
private:
  std::shared_ptr<pdcch_encoder_factory>        encoder_factory;
  std::shared_ptr<pdcch_modulator_factory>      modulator_factory;
  std::shared_ptr<dmrs_pdcch_processor_factory> dmrs_factory;

public:
  pdcch_processor_factory_sw(std::shared_ptr<pdcch_encoder_factory>        encoder_factory_,
                             std::shared_ptr<pdcch_modulator_factory>      modulator_factory_,
                             std::shared_ptr<dmrs_pdcch_processor_factory> dmrs_factory_) :
    encoder_factory(std::move(encoder_factory_)),
    modulator_factory(std::move(modulator_factory_)),
    dmrs_factory(std::move(dmrs_factory_))
  {
    srsran_assert(encoder_factory, "Invalid encoder factory.");
    srsran_assert(modulator_factory, "Invalid modulator factory.");
    srsran_assert(dmrs_factory, "Invalid DMRS factory.");
  }

  std::unique_ptr<pdcch_processor> create() override
  {
    return std::make_unique<pdcch_processor_impl>(
        encoder_factory->create(), modulator_factory->create(), dmrs_factory->create());
  }

  std::unique_ptr<pdcch_pdu_validator> create_validator() override
  {
    return std::make_unique<pdcch_processor_validator_impl>();
  }
};

class pdcch_processor_pool_factory : public pdcch_processor_factory
{
public:
  pdcch_processor_pool_factory(std::shared_ptr<pdcch_processor_factory> factory_, unsigned nof_concurrent_threads_) :
    factory(std::move(factory_)), nof_concurrent_threads(nof_concurrent_threads_)
  {
    srsran_assert(factory, "Invalid PDCCH processor factory.");
    srsran_assert(nof_concurrent_threads > 1, "Number of concurrent threads must be greater than one.");
  }

  std::unique_ptr<pdcch_processor> create() override
  {
    if (!processors) {
      std::vector<std::unique_ptr<pdcch_processor>> procs(nof_concurrent_threads);

      for (auto& processor : procs) {
        processor = factory->create();
      }

      processors = std::make_shared<pdcch_processor_pool::pdcch_processor_pool_type>(std::move(procs));
    }

    return std::make_unique<pdcch_processor_pool>(std::move(processors));
  }

  std::unique_ptr<pdcch_processor> create(srslog::basic_logger& logger, bool enable_logging_broadcast) override
  {
    if (!processors) {
      std::vector<std::unique_ptr<pdcch_processor>> procs(nof_concurrent_threads);

      for (auto& processor : procs) {
        processor = factory->create(logger, enable_logging_broadcast);
      }

      processors = std::make_shared<pdcch_processor_pool::pdcch_processor_pool_type>(std::move(procs));
    }

    return std::make_unique<pdcch_processor_pool>(std::move(processors));
  }

  std::unique_ptr<pdcch_pdu_validator> create_validator() override { return factory->create_validator(); }

private:
  std::shared_ptr<pdcch_processor_factory>                         factory;
  unsigned                                                         nof_concurrent_threads;
  std::shared_ptr<pdcch_processor_pool::pdcch_processor_pool_type> processors;
};

class pdsch_encoder_factory_sw : public pdsch_encoder_factory
{
private:
  std::shared_ptr<ldpc_encoder_factory>      encoder_factory;
  std::shared_ptr<ldpc_rate_matcher_factory> rate_matcher_factory;
  std::shared_ptr<ldpc_segmenter_tx_factory> segmenter_factory;

public:
  explicit pdsch_encoder_factory_sw(pdsch_encoder_factory_sw_configuration& config) :
    encoder_factory(std::move(config.encoder_factory)),
    rate_matcher_factory(std::move(config.rate_matcher_factory)),
    segmenter_factory(std::move(config.segmenter_factory))
  {
    srsran_assert(encoder_factory, "Invalid LDPC encoder factory.");
    srsran_assert(rate_matcher_factory, "Invalid LDPC RM factory.");
    srsran_assert(segmenter_factory, "Invalid LDPC segmenter factory.");
  }

  std::unique_ptr<pdsch_encoder> create() override
  {
    return std::make_unique<pdsch_encoder_impl>(
        segmenter_factory->create(), encoder_factory->create(), rate_matcher_factory->create());
  }
};

/// HW-accelerated PDSCH encoder factory.
class pdsch_encoder_factory_hw : public pdsch_encoder_factory
{
private:
  bool                                                   cb_mode;
  unsigned                                               max_tb_size;
  std::shared_ptr<crc_calculator_factory>                crc_factory;
  std::shared_ptr<ldpc_segmenter_tx_factory>             segmenter_factory;
  std::shared_ptr<hal::hw_accelerator_pdsch_enc_factory> hw_encoder_factory;

public:
  explicit pdsch_encoder_factory_hw(const pdsch_encoder_factory_hw_configuration& config) :
    cb_mode(config.cb_mode),
    max_tb_size(config.max_tb_size),
    crc_factory(std::move(config.crc_factory)),
    segmenter_factory(std::move(config.segmenter_factory)),
    hw_encoder_factory(std::move(config.hw_encoder_factory))
  {
    srsran_assert(crc_factory, "Invalid CRC factory.");
    srsran_assert(segmenter_factory, "Invalid LDPC segmenter factory.");
    srsran_assert(hw_encoder_factory, "Invalid hardware accelerator factory.");
  }

  std::unique_ptr<pdsch_encoder> create() override
  {
    pdsch_encoder_hw_impl::sch_crc crc = {
        crc_factory->create(crc_generator_poly::CRC16),
        crc_factory->create(crc_generator_poly::CRC24A),
        crc_factory->create(crc_generator_poly::CRC24B),
    };
    return std::make_unique<pdsch_encoder_hw_impl>(
        cb_mode, max_tb_size, crc, segmenter_factory->create(), hw_encoder_factory->create());
  }
};

class pdsch_modulator_factory_sw : public pdsch_modulator_factory
{
private:
  std::shared_ptr<channel_modulation_factory>      modulator_factory;
  std::shared_ptr<pseudo_random_generator_factory> prg_factory;

public:
  pdsch_modulator_factory_sw(std::shared_ptr<channel_modulation_factory>      modulator_factory_,
                             std::shared_ptr<pseudo_random_generator_factory> prg_factory_) :
    modulator_factory(std::move(modulator_factory_)), prg_factory(std::move(prg_factory_))
  {
    srsran_assert(modulator_factory, "Invalid modulator factory.");
    srsran_assert(prg_factory, "Invalid PRG factory.");
  }

  std::unique_ptr<pdsch_modulator> create() override
  {
    return std::make_unique<pdsch_modulator_impl>(modulator_factory->create_modulation_mapper(), prg_factory->create());
  }
};

class pdsch_processor_factory_sw : public pdsch_processor_factory
{
private:
  std::shared_ptr<pdsch_encoder_factory>        encoder_factory;
  std::shared_ptr<pdsch_modulator_factory>      modulator_factory;
  std::shared_ptr<dmrs_pdsch_processor_factory> dmrs_factory;

public:
  pdsch_processor_factory_sw(std::shared_ptr<pdsch_encoder_factory>        encoder_factory_,
                             std::shared_ptr<pdsch_modulator_factory>      modulator_factory_,
                             std::shared_ptr<dmrs_pdsch_processor_factory> dmrs_factory_) :
    encoder_factory(std::move(encoder_factory_)),
    modulator_factory(std::move(modulator_factory_)),
    dmrs_factory(std::move(dmrs_factory_))
  {
    srsran_assert(encoder_factory, "Invalid encoder factory.");
    srsran_assert(modulator_factory, "Invalid modulator factory.");
    srsran_assert(dmrs_factory, "Invalid DMRS factory.");
  }

  std::unique_ptr<pdsch_processor> create() override
  {
    return std::make_unique<pdsch_processor_impl>(
        encoder_factory->create(), modulator_factory->create(), dmrs_factory->create());
  }

  std::unique_ptr<pdsch_pdu_validator> create_validator() override
  {
    return std::make_unique<pdsch_processor_validator_impl>();
  }
};

class pdsch_processor_concurrent_factory_sw : public pdsch_processor_factory
{
public:
  pdsch_processor_concurrent_factory_sw(std::shared_ptr<crc_calculator_factory>          crc_factory,
                                        std::shared_ptr<ldpc_encoder_factory>            encoder_factory,
                                        std::shared_ptr<ldpc_rate_matcher_factory>       rate_matcher_factory,
                                        std::shared_ptr<pseudo_random_generator_factory> prg_factory_,
                                        std::shared_ptr<channel_modulation_factory>      modulator_factory,
                                        std::shared_ptr<dmrs_pdsch_processor_factory>    dmrs_factory,
                                        task_executor&                                   executor_,
                                        unsigned                                         nof_concurrent_threads) :
    prg_factory(std::move(prg_factory_)), executor(executor_)
  {
    srsran_assert(crc_factory, "Invalid CRC calculator factory.");
    srsran_assert(encoder_factory, "Invalid encoder factory.");
    srsran_assert(rate_matcher_factory, "Invalid rate matcher factory.");
    srsran_assert(prg_factory, "Invalid PRG factory.");
    srsran_assert(modulator_factory, "Invalid modulator factory.");
    srsran_assert(dmrs_factory, "Invalid DM-RS factory.");
    srsran_assert(nof_concurrent_threads > 1, "Number of concurrent threads must be greater than one.");

    // Create vector of codeblock processors.
    std::vector<std::unique_ptr<pdsch_codeblock_processor>> cb_processors;
    for (unsigned i_encoder = 0; i_encoder != nof_concurrent_threads; ++i_encoder) {
      cb_processors.emplace_back(
          std::make_unique<pdsch_codeblock_processor>(crc_factory->create(crc_generator_poly::CRC24A),
                                                      crc_factory->create(crc_generator_poly::CRC24B),
                                                      crc_factory->create(crc_generator_poly::CRC16),
                                                      encoder_factory->create(),
                                                      rate_matcher_factory->create(),
                                                      prg_factory->create(),
                                                      modulator_factory->create_modulation_mapper()));
    }

    // Create pool of codeblock processors. It is common for all PDSCH processors.
    cb_processor_pool =
        std::make_shared<pdsch_processor_concurrent_impl::codeblock_processor_pool>(std::move(cb_processors));

    // Create vector of PDSCH DM-RS generators.
    std::vector<std::unique_ptr<dmrs_pdsch_processor>> dmrs_generators;
    for (unsigned i_encoder = 0; i_encoder != nof_concurrent_threads; ++i_encoder) {
      dmrs_generators.emplace_back(dmrs_factory->create());
    }

    // Create pool of PDSCH DM-RS generators. It is common for all PDSCH processors.
    dmrs_generator_pool =
        std::make_shared<pdsch_processor_concurrent_impl::pdsch_dmrs_generator_pool>(std::move(dmrs_generators));
  }

  std::unique_ptr<pdsch_processor> create() override
  {
    return std::make_unique<pdsch_processor_concurrent_impl>(
        cb_processor_pool, prg_factory->create(), dmrs_generator_pool, executor);
  }

  std::unique_ptr<pdsch_pdu_validator> create_validator() override
  {
    return std::make_unique<pdsch_processor_validator_impl>();
  }

private:
  std::shared_ptr<pseudo_random_generator_factory>                            prg_factory;
  task_executor&                                                              executor;
  std::shared_ptr<pdsch_processor_concurrent_impl::codeblock_processor_pool>  cb_processor_pool;
  std::shared_ptr<pdsch_processor_concurrent_impl::pdsch_dmrs_generator_pool> dmrs_generator_pool;
};

class pdsch_processor_lite_factory_sw : public pdsch_processor_factory
{
private:
  std::shared_ptr<ldpc_segmenter_tx_factory>       segmenter_factory;
  std::shared_ptr<ldpc_encoder_factory>            encoder_factory;
  std::shared_ptr<ldpc_rate_matcher_factory>       rate_matcher_factory;
  std::shared_ptr<pseudo_random_generator_factory> scrambler_factory;
  std::shared_ptr<channel_modulation_factory>      modulator_factory;
  std::shared_ptr<dmrs_pdsch_processor_factory>    dmrs_factory;

public:
  pdsch_processor_lite_factory_sw(std::shared_ptr<ldpc_segmenter_tx_factory>       segmenter_factory_,
                                  std::shared_ptr<ldpc_encoder_factory>            encoder_factory_,
                                  std::shared_ptr<ldpc_rate_matcher_factory>       rate_matcher_factory_,
                                  std::shared_ptr<pseudo_random_generator_factory> scrambler_factory_,
                                  std::shared_ptr<channel_modulation_factory>      modulator_factory_,
                                  std::shared_ptr<dmrs_pdsch_processor_factory>    dmrs_factory_) :
    segmenter_factory(std::move(segmenter_factory_)),
    encoder_factory(std::move(encoder_factory_)),
    rate_matcher_factory(std::move(rate_matcher_factory_)),
    scrambler_factory(std::move(scrambler_factory_)),
    modulator_factory(std::move(modulator_factory_)),
    dmrs_factory(std::move(dmrs_factory_))
  {
    srsran_assert(segmenter_factory, "Invalid segmenter factory.");
    srsran_assert(encoder_factory, "Invalid encoder factory.");
    srsran_assert(rate_matcher_factory, "Invalid rate matcher factory.");
    srsran_assert(scrambler_factory, "Invalid scrambler factory.");
    srsran_assert(modulator_factory, "Invalid modulator factory.");
    srsran_assert(dmrs_factory, "Invalid DM-RS factory.");
  }

  std::unique_ptr<pdsch_processor> create() override
  {
    return std::make_unique<pdsch_processor_lite_impl>(segmenter_factory->create(),
                                                       encoder_factory->create(),
                                                       rate_matcher_factory->create(),
                                                       scrambler_factory->create(),
                                                       modulator_factory->create_modulation_mapper(),
                                                       dmrs_factory->create());
  }

  std::unique_ptr<pdsch_pdu_validator> create_validator() override
  {
    return std::make_unique<pdsch_processor_validator_impl>();
  }
};

class pdsch_processor_pool_factory : public pdsch_processor_factory
{
public:
  pdsch_processor_pool_factory(std::shared_ptr<pdsch_processor_factory> factory_,
                               unsigned                                 max_nof_processors_,
                               bool                                     blocking_) :
    factory(std::move(factory_)), max_nof_processors(max_nof_processors_), blocking(blocking_)
  {
    srsran_assert(factory, "Invalid PDSCH processor factory.");
    srsran_assert(max_nof_processors >= 1,
                  "The number of processors (i.e., {}) must be greater than or equal to one.",
                  max_nof_processors);
  }

  std::unique_ptr<pdsch_processor> create() override
  {
    // Create processors.
    std::vector<std::unique_ptr<pdsch_processor>> processors(max_nof_processors);
    for (std::unique_ptr<pdsch_processor>& processor : processors) {
      processor = factory->create();
    }

    return std::make_unique<pdsch_processor_pool>(processors, blocking);
  }

  std::unique_ptr<pdsch_processor> create(srslog::basic_logger& logger, bool enable_logging_broadcast) override
  {
    // Create processors with logging.
    std::vector<std::unique_ptr<pdsch_processor>> processors(max_nof_processors);
    for (std::unique_ptr<pdsch_processor>& processor : processors) {
      processor = factory->create(logger, enable_logging_broadcast);
    }

    return std::make_unique<pdsch_processor_pool>(processors, blocking);
  }

  std::unique_ptr<pdsch_pdu_validator> create_validator() override { return factory->create_validator(); }

private:
  std::shared_ptr<pdsch_processor_factory> factory;
  unsigned                                 max_nof_processors;
  bool                                     blocking;
};

class prach_detector_factory_sw : public prach_detector_factory
{
private:
  std::shared_ptr<dft_processor_factory>   dft_factory;
  std::shared_ptr<prach_generator_factory> prach_gen_factory;
  unsigned                                 idft_long_size;
  unsigned                                 idft_short_size;
  bool                                     combine_symbols;

public:
  prach_detector_factory_sw(std::shared_ptr<dft_processor_factory>         dft_factory_,
                            std::shared_ptr<prach_generator_factory>       prach_gen_factory_,
                            const prach_detector_factory_sw_configuration& config) :
    dft_factory(std::move(dft_factory_)),
    prach_gen_factory(std::move(prach_gen_factory_)),
    idft_long_size(config.idft_long_size),
    idft_short_size(config.idft_short_size),
    combine_symbols(config.combine_symbols)
  {
    srsran_assert(dft_factory, "Invalid DFT factory.");
    srsran_assert(prach_gen_factory, "Invalid PRACH generator factory.");
  }

  std::unique_ptr<prach_detector> create() override
  {
    dft_processor::configuration idft_long_config;
    idft_long_config.size = idft_long_size;
    idft_long_config.dir  = dft_processor::direction::INVERSE;
    dft_processor::configuration idft_short_config;
    idft_short_config.size = idft_short_size;
    idft_short_config.dir  = dft_processor::direction::INVERSE;
    return std::make_unique<prach_detector_generic_impl>(dft_factory->create(idft_long_config),
                                                         dft_factory->create(idft_short_config),
                                                         prach_gen_factory->create(),
                                                         combine_symbols);
  }

  std::unique_ptr<prach_detector_validator> create_validator() override
  {
    return std::make_unique<prach_detector_validator_impl>();
  }
};

class prach_detector_pool_factory : public prach_detector_factory
{
public:
  prach_detector_pool_factory(std::shared_ptr<prach_detector_factory> factory_, unsigned nof_concurrent_threads_) :
    factory(std::move(factory_)), nof_concurrent_threads(nof_concurrent_threads_)
  {
    srsran_assert(factory, "Invalid PRACH detector factory.");
    srsran_assert(nof_concurrent_threads > 1, "Number of concurrent threads must be greater than one.");
  }

  std::unique_ptr<prach_detector> create() override
  {
    std::vector<std::unique_ptr<prach_detector>> detectors(nof_concurrent_threads);

    for (auto& detector : detectors) {
      detector = factory->create();
    }

    return std::make_unique<prach_detector_pool>(std::move(detectors));
  }

  std::unique_ptr<prach_detector> create(srslog::basic_logger& logger, bool log_all_opportunities) override
  {
    std::vector<std::unique_ptr<prach_detector>> detectors(nof_concurrent_threads);

    for (auto& detector : detectors) {
      detector = factory->create(logger, log_all_opportunities);
    }

    return std::make_unique<prach_detector_pool>(std::move(detectors));
  }

  std::unique_ptr<prach_detector_validator> create_validator() override { return factory->create_validator(); }

private:
  std::shared_ptr<prach_detector_factory> factory;
  unsigned                                nof_concurrent_threads;
};

class prach_generator_factory_sw : public prach_generator_factory
{
public:
  std::unique_ptr<prach_generator> create() override { return std::make_unique<prach_generator_impl>(); }
};

class pucch_detector_factory_sw : public pucch_detector_factory
{
private:
  std::shared_ptr<low_papr_sequence_collection_factory> low_papr_factory;
  std::shared_ptr<pseudo_random_generator_factory>      prg_factory;
  std::shared_ptr<channel_equalizer_factory>            eqzr_factory;

public:
  pucch_detector_factory_sw(std::shared_ptr<low_papr_sequence_collection_factory> lpcf,
                            std::shared_ptr<pseudo_random_generator_factory>      prgf,
                            std::shared_ptr<channel_equalizer_factory>            eqzrf) :
    low_papr_factory(std::move(lpcf)), prg_factory(std::move(prgf)), eqzr_factory(std::move(eqzrf))
  {
    srsran_assert(low_papr_factory, "Invalid low-PAPR sequence collection factory.");
    srsran_assert(prg_factory, "Invalid pseudorandom generator factory.");
    srsran_assert(eqzr_factory, "Invalid channel equalizer factory.");
  }

  std::unique_ptr<pucch_detector> create() override
  {
    std::array<float, NRE> alphas;
    std::generate(alphas.begin(), alphas.end(), [n = 0U]() mutable {
      return (TWOPI * static_cast<float>(n++) / static_cast<float>(NRE));
    });
    return std::make_unique<pucch_detector_impl>(
        low_papr_factory->create(1, 0, alphas), prg_factory->create(), eqzr_factory->create());
  }
};

class pucch_processor_factory_sw : public pucch_processor_factory
{
public:
  pucch_processor_factory_sw(std::shared_ptr<dmrs_pucch_estimator_factory>&       dmrs_factory_,
                             std::shared_ptr<pucch_detector_factory>&             detector_factory_,
                             std::shared_ptr<pucch_demodulator_factory>           demodulator_factory_,
                             std::shared_ptr<uci_decoder_factory>                 decoder_factory_,
                             const channel_estimate::channel_estimate_dimensions& channel_estimate_dimensions_) :
    dmrs_factory(std::move(dmrs_factory_)),
    detector_factory(std::move(detector_factory_)),
    demodulator_factory(std::move(demodulator_factory_)),
    decoder_factory(std::move(decoder_factory_)),
    channel_estimate_dimensions(channel_estimate_dimensions_)
  {
    srsran_assert(dmrs_factory, "Invalid DM-RS estimator factory.");
    srsran_assert(detector_factory, "Invalid detector factory.");
    srsran_assert(demodulator_factory, "Invalid PUCCH demodulator factory.");
    srsran_assert(decoder_factory, "Invalid UCI decoder factory.");
    srsran_assert((channel_estimate_dimensions.nof_symbols != 0) && (channel_estimate_dimensions.nof_prb != 0) &&
                      (channel_estimate_dimensions.nof_rx_ports != 0) &&
                      (channel_estimate_dimensions.nof_tx_layers != 0),
                  "Channel estimate dimensions cannot be zero.");
    srsran_assert(channel_estimate_dimensions.nof_prb <= MAX_RB,
                  "Number of RB, i.e., {}, exceeds maximum bandwidth, i.e., {}.",
                  channel_estimate_dimensions.nof_prb,
                  MAX_RB);
    srsran_assert(channel_estimate_dimensions.nof_symbols <= MAX_NSYMB_PER_SLOT,
                  "Number of OFDM symbols, i.e., {}, exceeds maximum slot symbols, i.e., {}.",
                  channel_estimate_dimensions.nof_symbols,
                  MAX_NSYMB_PER_SLOT);
    srsran_assert(channel_estimate_dimensions.nof_rx_ports <= static_cast<unsigned>(MAX_PUCCH_RX_PORTS),
                  "Number of receive ports, i.e., {}, exceeds maximum supported receive ports, i.e., {}.",
                  channel_estimate_dimensions.nof_rx_ports,
                  static_cast<unsigned>(MAX_PUCCH_RX_PORTS));
    srsran_assert(channel_estimate_dimensions.nof_tx_layers <= pucch_constants::MAX_LAYERS,
                  "Number of transmit layers, i.e., {}, exceeds maximum PUCCH transmit layers, i.e., {}.",
                  channel_estimate_dimensions.nof_tx_layers,
                  pucch_constants::MAX_LAYERS);
  }

  std::unique_ptr<pucch_processor> create() override
  {
    return std::make_unique<pucch_processor_impl>(dmrs_factory->create_format1(),
                                                  dmrs_factory->create_format2(),
                                                  detector_factory->create(),
                                                  demodulator_factory->create(),
                                                  decoder_factory->create(),
                                                  channel_estimate_dimensions);
  }

  std::unique_ptr<pucch_pdu_validator> create_validator() override
  {
    return std::make_unique<pucch_pdu_validator_impl>(channel_estimate_dimensions);
  }

private:
  static constexpr unsigned                     MAX_PUCCH_RX_PORTS = 4;
  std::shared_ptr<dmrs_pucch_estimator_factory> dmrs_factory;
  std::shared_ptr<pucch_detector_factory>       detector_factory;
  std::shared_ptr<pucch_demodulator_factory>    demodulator_factory;
  std::shared_ptr<uci_decoder_factory>          decoder_factory;
  channel_estimate::channel_estimate_dimensions channel_estimate_dimensions;
};

class pucch_processor_pool_factory : public pucch_processor_factory
{
public:
  pucch_processor_pool_factory(std::shared_ptr<pucch_processor_factory> factory_, unsigned nof_concurrent_threads_) :
    factory(std::move(factory_)), nof_concurrent_threads(nof_concurrent_threads_)
  {
    srsran_assert(factory, "Invalid PUCCH processor factory.");
    srsran_assert(nof_concurrent_threads > 1, "Number of concurrent threads must be greater than one.");
  }

  std::unique_ptr<pucch_processor> create() override
  {
    std::vector<std::unique_ptr<pucch_processor>> processors(nof_concurrent_threads);

    for (auto& processor : processors) {
      processor = factory->create();
    }

    return std::make_unique<pucch_processor_pool>(std::move(processors));
  }

  std::unique_ptr<pucch_processor> create(srslog::basic_logger& logger) override
  {
    std::vector<std::unique_ptr<pucch_processor>> processors(nof_concurrent_threads);

    for (auto& processor : processors) {
      processor = factory->create(logger);
    }

    return std::make_unique<pucch_processor_pool>(std::move(processors));
  }

  std::unique_ptr<pucch_pdu_validator> create_validator() override { return factory->create_validator(); }

private:
  std::shared_ptr<pucch_processor_factory> factory;
  unsigned                                 nof_concurrent_threads;
};

class ssb_processor_factory_sw : public ssb_processor_factory
{
public:
  explicit ssb_processor_factory_sw(ssb_processor_factory_sw_configuration& config) :
    encoder_factory(std::move(config.encoder_factory)),
    modulator_factory(std::move(config.modulator_factory)),
    dmrs_factory(std::move(config.dmrs_factory)),
    pss_factory(std::move(config.pss_factory)),
    sss_factory(std::move(config.sss_factory))
  {
    srsran_assert(encoder_factory, "Invalid encoder factory");
    srsran_assert(modulator_factory, "Invalid modulator factory");
    srsran_assert(dmrs_factory, "Invalid DMRS factory");
    srsran_assert(pss_factory, "Invalid PSS factory");
    srsran_assert(sss_factory, "Invalid SSS factory");
  }

  std::unique_ptr<ssb_processor> create() override
  {
    ssb_processor_config config;
    config.encoder   = encoder_factory->create();
    config.modulator = modulator_factory->create();
    config.dmrs      = dmrs_factory->create();
    config.pss       = pss_factory->create();
    config.sss       = sss_factory->create();

    return std::make_unique<ssb_processor_impl>(std::move(config));
  }

  std::unique_ptr<ssb_pdu_validator> create_validator() override
  {
    return std::make_unique<ssb_processor_validator_impl>();
  }

private:
  std::shared_ptr<pbch_encoder_factory>        encoder_factory;
  std::shared_ptr<pbch_modulator_factory>      modulator_factory;
  std::shared_ptr<dmrs_pbch_processor_factory> dmrs_factory;
  std::shared_ptr<pss_processor_factory>       pss_factory;
  std::shared_ptr<sss_processor_factory>       sss_factory;
};

class ssb_processor_pool_factory : public ssb_processor_factory
{
public:
  ssb_processor_pool_factory(std::shared_ptr<ssb_processor_factory> factory_, unsigned nof_concurrent_threads_) :
    factory(std::move(factory_)), nof_concurrent_threads(nof_concurrent_threads_)
  {
    srsran_assert(factory, "Invalid SSB processor factory.");
    srsran_assert(nof_concurrent_threads > 1, "Number of concurrent threads must be greater than one.");
  }

  std::unique_ptr<ssb_processor> create() override
  {
    if (!processors) {
      std::vector<std::unique_ptr<ssb_processor>> instances(nof_concurrent_threads);

      for (auto& processor : instances) {
        processor = factory->create();
      }

      processors = std::make_shared<ssb_processor_pool::ssb_processor_pool_type>(std::move(instances));
    }

    return std::make_unique<ssb_processor_pool>(std::move(processors));
  }

  std::unique_ptr<ssb_processor> create(srslog::basic_logger& logger) override
  {
    if (!processors) {
      std::vector<std::unique_ptr<ssb_processor>> instances(nof_concurrent_threads);

      for (auto& processor : instances) {
        processor = factory->create(logger);
      }

      processors = std::make_shared<ssb_processor_pool::ssb_processor_pool_type>(std::move(instances));
    }

    return std::make_unique<ssb_processor_pool>(std::move(processors));
  }

  std::unique_ptr<ssb_pdu_validator> create_validator() override { return factory->create_validator(); }

private:
  std::shared_ptr<ssb_processor_factory>                       factory;
  unsigned                                                     nof_concurrent_threads;
  std::shared_ptr<ssb_processor_pool::ssb_processor_pool_type> processors;
};

class pucch_demodulator_factory_sw : public pucch_demodulator_factory
{
public:
  std::unique_ptr<pucch_demodulator> create() override
  {
    return std::make_unique<pucch_demodulator_impl>(
        equalizer_factory->create(), demodulation_factory->create_demodulation_mapper(), prg_factory->create());
  }

  pucch_demodulator_factory_sw(std::shared_ptr<channel_equalizer_factory>       equalizer_factory_,
                               std::shared_ptr<channel_modulation_factory>      demodulation_factory_,
                               std::shared_ptr<pseudo_random_generator_factory> prg_factory_) :
    equalizer_factory(std::move(equalizer_factory_)),
    demodulation_factory(std::move(demodulation_factory_)),
    prg_factory(std::move(prg_factory_))
  {
    srsran_assert(equalizer_factory, "Invalid equalizer factory.");
    srsran_assert(demodulation_factory, "Invalid demodulation factory.");
    srsran_assert(prg_factory, "Invalid PRG factory.");
  }

private:
  std::shared_ptr<channel_equalizer_factory>       equalizer_factory;
  std::shared_ptr<channel_modulation_factory>      demodulation_factory;
  std::shared_ptr<pseudo_random_generator_factory> prg_factory;
};

} // namespace

std::shared_ptr<pbch_encoder_factory>
srsran::create_pbch_encoder_factory_sw(std::shared_ptr<crc_calculator_factory>          crc_factory,
                                       std::shared_ptr<pseudo_random_generator_factory> prg_factory,
                                       std::shared_ptr<polar_factory>                   polar_factory)
{
  return std::make_shared<pbch_encoder_factory_sw>(
      std::move(crc_factory), std::move(prg_factory), std::move(polar_factory));
}

std::shared_ptr<pbch_modulator_factory>
srsran::create_pbch_modulator_factory_sw(std::shared_ptr<channel_modulation_factory>      modulator_factory,
                                         std::shared_ptr<pseudo_random_generator_factory> prg_factory)
{
  return std::make_shared<pbch_modulator_factory_sw>(std::move(modulator_factory), std::move(prg_factory));
}

std::shared_ptr<pdcch_encoder_factory>
srsran::create_pdcch_encoder_factory_sw(std::shared_ptr<crc_calculator_factory> crc_factory,
                                        std::shared_ptr<polar_factory>          encoder_factory)
{
  return std::make_shared<pdcch_encoder_factory_sw>(std::move(crc_factory), std::move(encoder_factory));
}

std::shared_ptr<pdcch_modulator_factory>
srsran::create_pdcch_modulator_factory_sw(std::shared_ptr<channel_modulation_factory>      modulator_factory,
                                          std::shared_ptr<pseudo_random_generator_factory> prg_factory)
{
  return std::make_shared<pdcch_modulator_factory_sw>(std::move(modulator_factory), std::move(prg_factory));
}

std::shared_ptr<pdcch_processor_factory>
srsran::create_pdcch_processor_factory_sw(std::shared_ptr<pdcch_encoder_factory>        encoder_factory,
                                          std::shared_ptr<pdcch_modulator_factory>      modulator_factory,
                                          std::shared_ptr<dmrs_pdcch_processor_factory> dmrs_factory)
{
  return std::make_shared<pdcch_processor_factory_sw>(
      std::move(encoder_factory), std::move(modulator_factory), std::move(dmrs_factory));
}

std::shared_ptr<pdcch_processor_factory>
srsran::create_pdcch_processor_pool_factory(std::shared_ptr<pdcch_processor_factory> processor_factory,
                                            unsigned                                 nof_concurrent_threads)
{
  return std::make_shared<pdcch_processor_pool_factory>(processor_factory, nof_concurrent_threads);
}

std::shared_ptr<pdsch_encoder_factory>
srsran::create_pdsch_encoder_factory_sw(pdsch_encoder_factory_sw_configuration& config)
{
  return std::make_shared<pdsch_encoder_factory_sw>(config);
}

std::shared_ptr<pdsch_encoder_factory>
srsran::create_pdsch_encoder_factory_hw(const pdsch_encoder_factory_hw_configuration& config)
{
  return std::make_shared<pdsch_encoder_factory_hw>(config);
}

std::shared_ptr<pdsch_modulator_factory>
srsran::create_pdsch_modulator_factory_sw(std::shared_ptr<channel_modulation_factory>      modulator_factory,
                                          std::shared_ptr<pseudo_random_generator_factory> prg_factory)
{
  return std::make_shared<pdsch_modulator_factory_sw>(std::move(modulator_factory), std::move(prg_factory));
}

std::shared_ptr<pdsch_processor_factory>
srsran::create_pdsch_processor_factory_sw(std::shared_ptr<pdsch_encoder_factory>        encoder_factory,
                                          std::shared_ptr<pdsch_modulator_factory>      modulator_factory,
                                          std::shared_ptr<dmrs_pdsch_processor_factory> dmrs_factory)
{
  return std::make_shared<pdsch_processor_factory_sw>(
      std::move(encoder_factory), std::move(modulator_factory), std::move(dmrs_factory));
}

std::shared_ptr<pdsch_processor_factory>
srsran::create_pdsch_concurrent_processor_factory_sw(std::shared_ptr<crc_calculator_factory>          crc_factory,
                                                     std::shared_ptr<ldpc_encoder_factory>            ldpc_enc_factory,
                                                     std::shared_ptr<ldpc_rate_matcher_factory>       ldpc_rm_factory,
                                                     std::shared_ptr<pseudo_random_generator_factory> prg_factory,
                                                     std::shared_ptr<channel_modulation_factory>      modulator_factory,
                                                     std::shared_ptr<dmrs_pdsch_processor_factory>    dmrs_factory,
                                                     task_executor&                                   executor,
                                                     unsigned nof_concurrent_threads)
{
  return std::make_shared<pdsch_processor_concurrent_factory_sw>(std::move(crc_factory),
                                                                 std::move(ldpc_enc_factory),
                                                                 std::move(ldpc_rm_factory),
                                                                 std::move(prg_factory),
                                                                 std::move(modulator_factory),
                                                                 std::move(dmrs_factory),
                                                                 executor,
                                                                 nof_concurrent_threads);
}

std::shared_ptr<pdsch_processor_factory>
srsran::create_pdsch_lite_processor_factory_sw(std::shared_ptr<ldpc_segmenter_tx_factory>       segmenter_factory,
                                               std::shared_ptr<ldpc_encoder_factory>            encoder_factory,
                                               std::shared_ptr<ldpc_rate_matcher_factory>       rate_matcher_factory,
                                               std::shared_ptr<pseudo_random_generator_factory> scrambler_factory,
                                               std::shared_ptr<channel_modulation_factory>      modulator_factory,
                                               std::shared_ptr<dmrs_pdsch_processor_factory>    dmrs_factory)
{
  return std::make_shared<pdsch_processor_lite_factory_sw>(std::move(segmenter_factory),
                                                           std::move(encoder_factory),
                                                           std::move(rate_matcher_factory),
                                                           std::move(scrambler_factory),
                                                           std::move(modulator_factory),
                                                           std::move(dmrs_factory));
}

std::shared_ptr<pdsch_processor_factory>
srsran::create_pdsch_processor_pool(std::shared_ptr<pdsch_processor_factory> pdsch_proc_factory,
                                    unsigned                                 max_nof_processors,
                                    bool                                     blocking)
{
  return std::make_shared<pdsch_processor_pool_factory>(std::move(pdsch_proc_factory), max_nof_processors, blocking);
}

std::shared_ptr<prach_detector_factory>
srsran::create_prach_detector_factory_sw(std::shared_ptr<dft_processor_factory>         dft_factory,
                                         std::shared_ptr<prach_generator_factory>       prach_gen_factory,
                                         const prach_detector_factory_sw_configuration& config)
{
  return std::make_shared<prach_detector_factory_sw>(std::move(dft_factory), std::move(prach_gen_factory), config);
}

std::shared_ptr<prach_detector_factory>
srsran::create_prach_detector_pool_factory(std::shared_ptr<prach_detector_factory> factory,
                                           unsigned                                nof_concurrent_threads)
{
  return std::make_shared<prach_detector_pool_factory>(std::move(factory), nof_concurrent_threads);
}

std::shared_ptr<pucch_processor_factory> srsran::create_pucch_processor_factory_sw(
    std::shared_ptr<dmrs_pucch_estimator_factory>        dmrs_factory,
    std::shared_ptr<pucch_detector_factory>              detector_factory,
    std::shared_ptr<pucch_demodulator_factory>           demodulator_factory,
    std::shared_ptr<uci_decoder_factory>                 decoder_factory,
    const channel_estimate::channel_estimate_dimensions& channel_estimate_dimensions)
{
  return std::make_shared<pucch_processor_factory_sw>(
      dmrs_factory, detector_factory, demodulator_factory, decoder_factory, channel_estimate_dimensions);
}

std::shared_ptr<pucch_processor_factory>
srsran::create_pucch_processor_pool_factory(std::shared_ptr<pucch_processor_factory> factory,
                                            unsigned                                 nof_concurrent_threads)
{
  return std::make_shared<pucch_processor_pool_factory>(std::move(factory), nof_concurrent_threads);
}

std::shared_ptr<prach_generator_factory> srsran::create_prach_generator_factory_sw()
{
  return std::make_shared<prach_generator_factory_sw>();
}

std::shared_ptr<pucch_demodulator_factory>
srsran::create_pucch_demodulator_factory_sw(std::shared_ptr<channel_equalizer_factory>       equalizer_factory,
                                            std::shared_ptr<channel_modulation_factory>      demodulation_factory,
                                            std::shared_ptr<pseudo_random_generator_factory> prg_factory)
{
  return std::make_shared<pucch_demodulator_factory_sw>(
      std::move(equalizer_factory), std::move(demodulation_factory), std::move(prg_factory));
}

std::shared_ptr<pucch_detector_factory>
srsran::create_pucch_detector_factory_sw(std::shared_ptr<low_papr_sequence_collection_factory> lpcf,
                                         std::shared_ptr<pseudo_random_generator_factory>      prgf,
                                         std::shared_ptr<channel_equalizer_factory>            eqzrf)
{
  return std::make_shared<pucch_detector_factory_sw>(std::move(lpcf), std::move(prgf), std::move(eqzrf));
}

std::shared_ptr<ssb_processor_factory>
srsran::create_ssb_processor_factory_sw(ssb_processor_factory_sw_configuration& config)
{
  return std::make_shared<ssb_processor_factory_sw>(config);
}

std::shared_ptr<ssb_processor_factory>
srsran::create_ssb_processor_pool_factory(std::shared_ptr<ssb_processor_factory> processor_factory,
                                          unsigned                               nof_concurrent_threads)
{
  return std::make_shared<ssb_processor_pool_factory>(processor_factory, nof_concurrent_threads);
}

template <typename Func>
static std::chrono::nanoseconds time_execution(Func&& func)
{
  auto start = std::chrono::steady_clock::now();
  func();
  auto end = std::chrono::steady_clock::now();

  return std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
}

static bool is_broadcast_rnti(uint16_t rnti)
{
  return ((rnti < to_value(rnti_t::MIN_CRNTI)) || (rnti > to_value(rnti_t::MAX_CRNTI)));
}

namespace {

class logging_pdcch_processor_decorator : public pdcch_processor
{
public:
  logging_pdcch_processor_decorator(srslog::basic_logger&            logger_,
                                    bool                             enable_logging_broadcast_,
                                    std::unique_ptr<pdcch_processor> processor_) :
    logger(logger_), enable_logging_broadcast(enable_logging_broadcast_), processor(std::move(processor_))
  {
    srsran_assert(processor, "Invalid processor.");
  }

  void process(resource_grid_mapper& mapper, const pdu_t& pdu) override
  {
    const auto&& func = [this, &mapper, &pdu]() { processor->process(mapper, pdu); };

    if (!enable_logging_broadcast && is_broadcast_rnti(pdu.dci.rnti)) {
      func();
      return;
    }

    std::chrono::nanoseconds time_ns = time_execution(func);

    static_bit_buffer<pdcch_constants::MAX_DCI_PAYLOAD_SIZE> data(pdu.dci.payload.size());
    srsvec::zero(data.get_buffer());
    srsvec::bit_pack(data, pdu.dci.payload);

    if (logger.debug.enabled()) {
      // Detailed log information, including a list of all PDU fields.
      logger.debug(pdu.slot.sfn(),
                   pdu.slot.slot_index(),
                   data.get_buffer().data(),
                   divide_ceil(data.size(), 8),
                   "PDCCH: {:s} {}\n  {:n}\n  {}",
                   pdu,
                   time_ns,
                   pdu,
                   time_ns);
      return;
    }
    // Single line log entry.
    logger.info(pdu.slot.sfn(),
                pdu.slot.slot_index(),
                data.get_buffer().data(),
                divide_ceil(data.size(), 8),
                "PDCCH: {:s} {}",
                pdu,
                time_ns);
  }

private:
  srslog::basic_logger&            logger;
  bool                             enable_logging_broadcast;
  std::unique_ptr<pdcch_processor> processor;
};

class logging_pdsch_processor_decorator : public pdsch_processor, private pdsch_processor_notifier
{
public:
  logging_pdsch_processor_decorator(srslog::basic_logger&            logger_,
                                    bool                             enable_logging_broadcast_,
                                    std::unique_ptr<pdsch_processor> processor_) :
    logger(logger_), enable_logging_broadcast(enable_logging_broadcast_), processor(std::move(processor_))
  {
    srsran_assert(processor, "Invalid processor.");
  }

  void process(resource_grid_mapper&                                        mapper,
               pdsch_processor_notifier&                                    notifier_,
               static_vector<span<const uint8_t>, MAX_NOF_TRANSPORT_BLOCKS> data_,
               const pdu_t&                                                 pdu_) override
  {
    notifier = &notifier_;
    data     = data_.front();
    pdu      = pdu_;

    start = std::chrono::system_clock::now();
    processor->process(mapper, *this, data_, pdu);
  }

private:
  void on_finish_processing() override
  {
    // Finish time measurement.
    auto end = std::chrono::system_clock::now();

    // Only print if it is allowed.
    if (enable_logging_broadcast || !is_broadcast_rnti(pdu.rnti)) {
      // Get elapsed time.
      std::chrono::nanoseconds time_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);

      if (logger.debug.enabled()) {
        // Detailed log information, including a list of all PDU fields.
        logger.debug(pdu.slot.sfn(),
                     pdu.slot.slot_index(),
                     data.data(),
                     data.size(),
                     "PDSCH: {:s} tbs={} {}\n  {:n}\n",
                     pdu,
                     data.size(),
                     time_ns,
                     pdu);
      } else {
        // Single line log entry.
        logger.info(pdu.slot.sfn(),
                    pdu.slot.slot_index(),
                    data.data(),
                    data.size(),
                    "PDSCH: {:s} tbs={} {}",
                    pdu,
                    data.size(),
                    time_ns);
      }
      DL_scheduler::getInstance().PDSCH_update(time_ns.count() / 1000);
      TimestampLogger::getInstance().log_timestamp("PDSCH create time", 
          std::chrono::duration_cast<std::chrono::microseconds>(start.time_since_epoch()).count(), 
          "PDSCH finish time", 
          std::chrono::duration_cast<std::chrono::microseconds>(end.time_since_epoch()).count());
      TimestampLogger::getInstance().log_timestamp(fmt::format("{:s}", pdu) + " PDSCH time", time_ns.count(), data.size());
    }
    //fmt::print("logging: {}\n", pdu);

    // Verify the notifier is valid.
    srsran_assert(notifier != nullptr, "Detected PDSCH processor notified twice.");

    // Set notifier to invalid pointer before notification.
    pdsch_processor_notifier* notifier_ = notifier;
    notifier                            = nullptr;

    // Notify original callback. Processor will be available after returning.
    notifier_->on_finish_processing();
  }

  srslog::basic_logger&                              logger;
  bool                                               enable_logging_broadcast;
  std::unique_ptr<pdsch_processor>                   processor;
  pdsch_processor_notifier*                          notifier;
  span<const uint8_t>                                data;
  pdu_t                                              pdu;
  std::chrono::time_point<std::chrono::system_clock> start;
};

class logging_prach_detector_decorator : public prach_detector
{
public:
  logging_prach_detector_decorator(srslog::basic_logger&           logger_,
                                   bool                            log_all_opportunities_,
                                   std::unique_ptr<prach_detector> detector_) :
    logger(logger_), log_all_opportunities(log_all_opportunities_), detector(std::move(detector_))
  {
    srsran_assert(detector, "Invalid detector.");
  }

  prach_detection_result detect(const prach_buffer& input, const configuration& config) override
  {
    prach_detection_result result;
    const auto&&           func = [&]() { result = detector->detect(input, config); };

    std::chrono::nanoseconds time_ns = time_execution(func);

    if (log_all_opportunities || !result.preambles.empty()) {
      if (logger.debug.enabled()) {
        // Detailed log information, including a list of all PRACH config and result fields.
        logger.debug(config.slot.sfn(),
                     config.slot.slot_index(),
                     "PRACH: {:s} {:s} {}\n  {:n}\n  {:n}\n",
                     config,
                     result,
                     time_ns,
                     config,
                     result);
      } else {
        // Single line log entry.
        logger.info(config.slot.sfn(), config.slot.slot_index(), "PRACH: {:s} {:s} {}", config, result, time_ns);
      }
    }

    return result;
  }

private:
  srslog::basic_logger&           logger;
  bool                            log_all_opportunities;
  std::unique_ptr<prach_detector> detector;
};

class logging_pucch_processor_decorator : public pucch_processor
{
  template <typename Config>
  pucch_processor_result process_(const resource_grid_reader& grid, const Config& config)
  {
    pucch_processor_result result;

    std::chrono::nanoseconds time_ns = time_execution([&]() { result = processor->process(grid, config); });
    if (logger.debug.enabled()) {
      // Detailed log information, including a list of all PUCCH configuration and result fields.
      logger.debug(config.slot.sfn(),
                   config.slot.slot_index(),
                   "PUCCH: {:s} {:s} {}\n  {:n}\n  {:n}",
                   config,
                   result,
                   time_ns,
                   config,
                   result);
    } else {
      // Single line log entry.
      logger.info(config.slot.sfn(), config.slot.slot_index(), "PUCCH: {:s} {:s} {}", config, result, time_ns);
    }

    return result;
  }

public:
  logging_pucch_processor_decorator(srslog::basic_logger& logger_, std::unique_ptr<pucch_processor> processor_) :
    logger(logger_), processor(std::move(processor_))
  {
    srsran_assert(processor, "Invalid processor.");
  }

  pucch_processor_result process(const resource_grid_reader& grid, const format0_configuration& config) override
  {
    return process_(grid, config);
  }
  pucch_processor_result process(const resource_grid_reader& grid, const format1_configuration& config) override
  {
    return process_(grid, config);
  }
  pucch_processor_result process(const resource_grid_reader& grid, const format2_configuration& config) override
  {
    return process_(grid, config);
  }
  pucch_processor_result process(const resource_grid_reader& grid, const format3_configuration& config) override
  {
    return process_(grid, config);
  }
  pucch_processor_result process(const resource_grid_reader& grid, const format4_configuration& config) override
  {
    return process_(grid, config);
  }

private:
  srslog::basic_logger&            logger;
  std::unique_ptr<pucch_processor> processor;
};

class logging_ssb_processor_decorator : public ssb_processor
{
public:
  logging_ssb_processor_decorator(srslog::basic_logger& logger_, std::unique_ptr<ssb_processor> processor_) :
    logger(logger_), processor(std::move(processor_))
  {
    srsran_assert(processor, "Invalid processor.");
  }

  void process(resource_grid_writer& grid, const pdu_t& pdu) override
  {
    const auto&& func = [&]() { processor->process(grid, pdu); };

    std::chrono::nanoseconds time_ns = time_execution(func);

    static_bit_buffer<BCH_PAYLOAD_SIZE> data(pdu.bch_payload.size());
    srsvec::zero(data.get_buffer());
    srsvec::bit_pack(data, pdu.bch_payload);

    if (logger.debug.enabled()) {
      // Detailed log information, including a list of all SSB PDU fields.
      logger.debug(pdu.slot.sfn(),
                   pdu.slot.slot_index(),
                   data.get_buffer().data(),
                   divide_ceil(data.size(), 8),
                   "SSB: {:s} {}\n  {:n}\n  {}",
                   pdu,
                   time_ns,
                   pdu,
                   time_ns);

      return;
    }
    // Single line log entry.
    logger.info(data.get_buffer().data(), divide_ceil(data.size(), 8), "SSB: {:s} {}", pdu, time_ns);
  }

private:
  srslog::basic_logger&          logger;
  std::unique_ptr<ssb_processor> processor;
};

} // namespace

std::unique_ptr<pdcch_processor> pdcch_processor_factory::create(srslog::basic_logger& logger,
                                                                 bool                  enable_logging_broadcast)
{
  return std::make_unique<logging_pdcch_processor_decorator>(logger, enable_logging_broadcast, create());
}

std::unique_ptr<pdsch_processor> pdsch_processor_factory::create(srslog::basic_logger& logger,
                                                                 bool                  enable_logging_broadcast)
{
  return std::make_unique<logging_pdsch_processor_decorator>(logger, enable_logging_broadcast, create());
}

std::unique_ptr<prach_detector> prach_detector_factory::create(srslog::basic_logger& logger, bool log_all_opportunities)
{
  return std::make_unique<logging_prach_detector_decorator>(logger, log_all_opportunities, create());
}

std::unique_ptr<pucch_processor> pucch_processor_factory::create(srslog::basic_logger& logger)
{
  return std::make_unique<logging_pucch_processor_decorator>(logger, create());
}

std::unique_ptr<ssb_processor> ssb_processor_factory::create(srslog::basic_logger& logger)
{
  return std::make_unique<logging_ssb_processor_decorator>(logger, create());
}
