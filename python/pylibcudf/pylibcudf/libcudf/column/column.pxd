# Copyright (c) 2020-2024, NVIDIA CORPORATION.

from libcpp cimport bool
from libcpp.memory cimport unique_ptr
from libcpp.vector cimport vector
from pylibcudf.libcudf.column.column_view cimport (
    column_view,
    mutable_column_view,
)
from pylibcudf.libcudf.types cimport data_type, size_type

from rmm._lib.device_buffer cimport device_buffer


cdef extern from "cudf/column/column.hpp" namespace "cudf" nogil:
    cdef cppclass column_contents "cudf::column::contents":
        unique_ptr[device_buffer] data
        unique_ptr[device_buffer] null_mask
        vector[unique_ptr[column]] children

    cdef cppclass column:
        column() except +
        column(const column& other) except +

        column(column_view view) except +

        size_type size() except +
        size_type null_count() except +
        bool has_nulls() except +
        data_type type() except +
        column_view view() except +
        mutable_column_view mutable_view() except +
        column_contents release() except +
