/*
 *
 * Copyright 2013-2022 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "channel_equalizer_test_data.h"
#include "srsgnb/phy/upper/equalization/channel_equalizer.h"
#include "srsgnb/support/srsgnb_test.h"

using namespace srsgnb;

namespace {
// Temporary - to be removed when a real channel_equalizer implementation is provided.
class channel_equalizer_dummy : public channel_equalizer
{
public:
  // See interface for the documentation.
  void equalize(re_measurement<cf_t>&       mod_symbols,
                re_measurement<float>&      noise_vars,
                const re_measurement<cf_t>& ch_symbols,
                const channel_estimate&     ch_estimates,
                float /**/) override
  {
    // Only check that the dimensions of all inputs match.
    re_measurement_dimensions                     mod_symbols_size  = mod_symbols.size();
    re_measurement_dimensions                     noise_vars_size   = noise_vars.size();
    re_measurement_dimensions                     ch_symbols_size   = ch_symbols.size();
    channel_estimate::channel_estimate_dimensions ch_estimates_size = ch_estimates.size();
    TESTASSERT((mod_symbols_size.nof_prb == noise_vars_size.nof_prb) &&
                   (mod_symbols_size.nof_prb == ch_symbols_size.nof_prb) &&
                   (mod_symbols_size.nof_prb == ch_estimates_size.nof_prb),
               "Number of PRBs does not match.");
    TESTASSERT((mod_symbols_size.nof_symbols == noise_vars_size.nof_symbols) &&
                   (mod_symbols_size.nof_symbols == ch_symbols_size.nof_symbols) &&
                   (mod_symbols_size.nof_symbols == ch_estimates_size.nof_symbols),
               "Number of OFDM symbols does not match.");
    TESTASSERT((mod_symbols_size.nof_slices == noise_vars_size.nof_slices) &&
                   (mod_symbols_size.nof_slices == ch_estimates_size.nof_tx_layers),
               "Number of Tx layers does not match.");
    TESTASSERT(ch_symbols_size.nof_slices == ch_estimates_size.nof_rx_ports,
               "Number of Rx antenna ports does not match.");
  }
};

std::unique_ptr<channel_equalizer> create_ch_equalizer()
{
  return std::make_unique<channel_equalizer_dummy>();
}
} // namespace

void read_symbols(re_measurement<cf_t>& symbols, const re_measurement_exploded& syms_expl)
{
  re_measurement_dimensions re_dims;
  re_dims.nof_prb     = syms_expl.nof_prb;
  re_dims.nof_symbols = syms_expl.nof_symbols;
  re_dims.nof_slices  = syms_expl.nof_slices;

  symbols.resize(re_dims);
  const std::vector<cf_t> all_syms     = syms_expl.measurements.read();
  unsigned                slice_length = re_dims.nof_symbols * re_dims.nof_prb * NRE;
  TESTASSERT_EQ(slice_length * re_dims.nof_slices, all_syms.size(), "Wrong number of symbols.");

  span<const cf_t> all_syms_span(all_syms);
  for (unsigned i_slice = 0, skip = 0; i_slice != re_dims.nof_slices; ++i_slice) {
    symbols.set_slice(all_syms_span.subspan(skip, slice_length), i_slice);
    skip += slice_length;
  }
}

void read_ch_estimates(channel_estimate& ch_est, const ch_estimates_exploded& ch_est_expl)
{
  channel_estimate::channel_estimate_dimensions ch_dims;
  ch_dims.nof_prb       = ch_est_expl.nof_prb;
  ch_dims.nof_symbols   = ch_est_expl.nof_symbols;
  ch_dims.nof_rx_ports  = ch_est_expl.nof_rx_ports;
  ch_dims.nof_tx_layers = ch_est_expl.nof_tx_layers;

  ch_est.resize(ch_dims);
  std::vector<cf_t> all_ests     = ch_est_expl.estimates.read();
  unsigned          slice_length = ch_dims.nof_symbols * ch_dims.nof_prb * NRE;
  unsigned          nof_paths    = ch_dims.nof_tx_layers * ch_dims.nof_rx_ports;
  TESTASSERT_EQ(slice_length * nof_paths, all_ests.size(), "Wrong number of channel estimates.");

  span<const cf_t> all_ests_span(all_ests);
  unsigned         skip = 0;
  for (unsigned i_rx = 0; i_rx != ch_dims.nof_rx_ports; ++i_rx) {
    for (unsigned i_tx = 0; i_tx != ch_dims.nof_tx_layers; ++i_tx) {
      span<cf_t> path = ch_est.get_path_ch_estimate(i_rx, i_tx);
      srsvec::copy(path, all_ests_span.subspan(skip, slice_length));
      skip += slice_length;

      ch_est.set_noise_variance(ch_est_expl.noise_var, i_rx, i_tx);
    }
  }
}

int main()
{
  std::unique_ptr<channel_equalizer> test_equalizer = create_ch_equalizer();
  re_measurement<cf_t>               test_rx_symbols;
  re_measurement<cf_t>               test_eq_symbols_expected;
  re_measurement<cf_t>               test_eq_symbols_actual;
  re_measurement<float>              test_noise_vars;
  channel_estimate                   test_ch_estimate;
  for (const auto& t_case : channel_equalizer_test_data) {
    read_symbols(test_rx_symbols, t_case.received_symbols);
    read_symbols(test_eq_symbols_expected, t_case.equalized_symbols);
    read_ch_estimates(test_ch_estimate, t_case.ch_estimates);
    test_noise_vars.resize(test_eq_symbols_expected.size());
    test_eq_symbols_actual.resize(test_eq_symbols_expected.size());

    test_equalizer->equalize(
        test_eq_symbols_actual, test_noise_vars, test_rx_symbols, test_ch_estimate, t_case.scaling);
  }
}