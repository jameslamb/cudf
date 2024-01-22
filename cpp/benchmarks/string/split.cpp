/*
 * Copyright (c) 2021-2024, NVIDIA CORPORATION.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <benchmarks/common/generate_input.hpp>

#include <cudf_test/column_wrapper.hpp>

#include <cudf/scalar/scalar.hpp>
#include <cudf/strings/split/split.hpp>
#include <cudf/strings/strings_column_view.hpp>
#include <cudf/utilities/default_stream.hpp>

#include <nvbench/nvbench.cuh>

static void bench_split(nvbench::state& state)
{
  auto const num_rows  = static_cast<cudf::size_type>(state.get_int64("num_rows"));
  auto const row_width = static_cast<cudf::size_type>(state.get_int64("row_width"));
  auto const stype     = state.get_string("type");

  if (static_cast<std::size_t>(num_rows) * static_cast<std::size_t>(row_width) >=
      static_cast<std::size_t>(std::numeric_limits<cudf::size_type>::max())) {
    state.skip("Skip benchmarks greater than size_type limit");
  }

  data_profile const profile = data_profile_builder().distribution(
    cudf::type_id::STRING, distribution_id::NORMAL, 0, row_width);
  auto const column = create_random_column(cudf::type_id::STRING, row_count{num_rows}, profile);
  cudf::strings_column_view input(column->view());
  cudf::string_scalar target("+");

  state.set_cuda_stream(nvbench::make_cuda_stream_view(cudf::get_default_stream().value()));
  // gather some throughput statistics as well
  auto chars_size = input.chars_size(cudf::get_default_stream());
  state.add_element_count(chars_size, "chars_size");            // number of bytes;
  state.add_global_memory_reads<nvbench::int8_t>(chars_size);   // all bytes are read;
  state.add_global_memory_writes<nvbench::int8_t>(chars_size);  // all bytes are written

  if (stype == "split") {
    state.exec(nvbench::exec_tag::sync,
               [&](nvbench::launch& launch) { cudf::strings::split(input, target); });
  } else if (stype == "split_ws") {
    state.exec(nvbench::exec_tag::sync,
               [&](nvbench::launch& launch) { cudf::strings::split(input); });
  } else if (stype == "record") {
    state.exec(nvbench::exec_tag::sync,
               [&](nvbench::launch& launch) { cudf::strings::split_record(input, target); });
  } else {
    state.exec(nvbench::exec_tag::sync,
               [&](nvbench::launch& launch) { cudf::strings::split_record(input); });
  }
}

NVBENCH_BENCH(bench_split)
  .set_name("split")
  .add_int64_axis("row_width", {32, 64, 128, 256, 512, 1024, 2048})
  .add_int64_axis("num_rows", {4096, 32768, 262144, 2097152, 16777216})
  .add_string_axis("type", {"split", "split_ws", "record", "record_ws"});
