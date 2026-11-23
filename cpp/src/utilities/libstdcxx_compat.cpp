/*
 * SPDX-FileCopyrightText: Copyright (c) 2026, NVIDIA CORPORATION & AFFILIATES. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

// Compatibility shims for C++ runtime symbols that statically-linked dependencies need but that
// the libstdc++ libcudf is built against may not provide.
//
// libnvcomp_static.a is built with GCC 14 (Red Hat gcc-toolset-14), which emits calls to
// __cxa_call_terminate (CXXABI_1.3.15). libstdc++ < 14 doesn't define it, so linking the static
// archive into libcudf with an older toolchain leaves an unversioned undefined reference in
// libcudf.so, and every executable linked against libcudf.so then fails with
// "undefined reference to `__cxa_call_terminate'".
//
// The definitions here are hidden, so they only satisfy references from within libcudf.so and are
// never exported, and weak, so a real definition linked in statically takes precedence.
//
// This file is only compiled into libcudf when static nvCOMP is linked in (see CMakeLists.txt).

#include <exception>

#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE < 14

#include <cxxabi.h>

// Approximates libsupc++'s implementation (eh_call.cc in GCC 14). The only difference is that it
// calls the current global terminate handler instead of the handler captured when the exception
// was thrown. It only runs on a path that is already terminating the process.
extern "C" __attribute__((weak, visibility("hidden"), noreturn)) void __cxa_call_terminate(
  void* ue_header) noexcept
{
  if (ue_header != nullptr) { __cxxabiv1::__cxa_begin_catch(ue_header); }
  std::terminate();
}

#endif
