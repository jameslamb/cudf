#!/bin/bash
# Copyright (c) 2024, NVIDIA CORPORATION.

set -eou pipefail

# We will only fail these tests if the PR touches code in pylibcudf
# or cudf_polars itself.
# Note, the three dots mean we are doing diff between the merge-base
# of upstream and HEAD. So this is asking, "does _this branch_ touch
# files in cudf_polars/pylibcudf", rather than "are there changes
# between upstream and this branch which touch cudf_polars/pylibcudf"
# TODO: is the target branch exposed anywhere in an environment variable?
if [ -n "$(git diff --name-only origin/branch-24.08...HEAD -- python/cudf_polars/ python/cudf/cudf/_lib/pylibcudf/)" ];
then
    HAS_CHANGES=1
else
    HAS_CHANGES=0
fi

RAPIDS_PY_CUDA_SUFFIX="$(rapids-wheel-ctk-name-gen ${RAPIDS_CUDA_VERSION})"
RAPIDS_PY_WHEEL_NAME="libcudf_${RAPIDS_PY_CUDA_SUFFIX}" rapids-download-wheels-from-s3 cpp ./dist
RAPIDS_PY_WHEEL_NAME="cudf_${RAPIDS_PY_CUDA_SUFFIX}" rapids-download-wheels-from-s3 python ./dist
RAPIDS_PY_WHEEL_NAME="cudf_polars_${RAPIDS_PY_CUDA_SUFFIX}" RAPIDS_PY_WHEEL_PURE="1" rapids-download-wheels-from-s3 ./dist

# echo to expand wildcard before adding `[extra]` requires for pip
rapids-logger "Install cudf wheel"
python -m pip install "$(echo ./dist/libcudf_${RAPIDS_PY_CUDA_SUFFIX}*.whl)"
python -m pip install "$(echo ./dist/cudf_${RAPIDS_PY_CUDA_SUFFIX}*.whl)"

rapids-logger "Install cudf_polars"
python -m pip install --find-links $(pwd)/dist "$(echo ./dist/cudf_polars*.whl)[test]"

RESULTS_DIR=${RAPIDS_TESTS_DIR:-"$(mktemp -d)"}
RAPIDS_TESTS_DIR=${RAPIDS_TESTS_DIR:-"${RESULTS_DIR}/test-results"}/
mkdir -p "${RAPIDS_TESTS_DIR}"

rapids-logger "Run cudf_polars tests"

function set_exitcode()
{
    EXITCODE=$?
}
EXITCODE=0
trap set_exitcode ERR
set +e

./ci/run_cudf_polars_pytests.sh \
       --cov cudf_polars \
       --cov-fail-under=100 \
       --cov-config=./pyproject.toml \
       --junitxml="${RAPIDS_TESTS_DIR}/junit-cudf-polars.xml"

trap ERR
set -e

if [ ${EXITCODE} != 0 ]; then
    rapids-logger "Testing FAILED: exitcode ${EXITCODE}"
else
    rapids-logger "Testing PASSED"
fi

if [ ${HAS_CHANGES} == 1 ]; then
    exit ${EXITCODE}
else
    exit 0
fi