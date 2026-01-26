// IACore-OSS; The Core Library for All IA Open Source Projects
// Copyright (C) 2026 IAS (ias@iasoft.dev)
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <IACore/IATest.hpp>
#include <IACore/SIMD.hpp>

using namespace IACore;

IAT_BEGIN_BLOCK(Core, FloatVec4)

auto test_float_arithmetic() -> bool
{
  FloatVec4 v1(10.0f, 20.0f, 30.0f, 40.0f);
  FloatVec4 v2(2.0f, 4.0f, 5.0f, 8.0f);

  alignas(16) f32 res[4];

  (v1 / v2).store(res);
  IAT_CHECK_APPROX(res[0], 5.0f);
  IAT_CHECK_APPROX(res[3], 5.0f);

  (v1 * v2).store(res);
  IAT_CHECK_APPROX(res[0], 20.0f);

  (v1 + v2).store(res);
  IAT_CHECK_APPROX(res[0], 12.0f);

  return true;
}

auto test_math_helpers() -> bool
{
  alignas(16) f32 res[4];

  FloatVec4 v_sq(4.0f, 9.0f, 16.0f, 25.0f);
  v_sq.sqrt().store(res);
  IAT_CHECK_APPROX(res[0], 2.0f);
  IAT_CHECK_APPROX(res[3], 5.0f);

  FloatVec4 v_neg(-1.0f, -5.0f, 10.0f, -0.0f);
  v_neg.abs().store(res);
  IAT_CHECK_APPROX(res[0], 1.0f);
  IAT_CHECK_APPROX(res[2], 10.0f);

  FloatVec4 v_clamp(-100.0f, 0.0f, 50.0f, 200.0f);
  v_clamp.clamp(0.0f, 100.0f).store(res);
  IAT_CHECK_APPROX(res[0], 0.0f);
  IAT_CHECK_APPROX(res[2], 50.0f);
  IAT_CHECK_APPROX(res[3], 100.0f);

  return true;
}

auto test_approx_math() -> bool
{
  alignas(16) f32 res[4];
  FloatVec4 v(16.0f, 25.0f, 100.0f, 1.0f);

  v.rsqrt().store(res);

  IAT_CHECK_APPROX(res[0], 0.25f);
  IAT_CHECK_APPROX(res[2], 0.1f);

  return true;
}

auto test_linear_algebra() -> bool
{
  FloatVec4 v1(1.0f, 2.0f, 3.0f, 4.0f);
  FloatVec4 v2(1.0f, 0.0f, 1.0f, 0.0f);

  f32 dot = v1.dot(v2);
  IAT_CHECK_APPROX(dot, 4.0f);

  FloatVec4 v_norm(10.0f, 0.0f, 0.0f, 0.0f);
  alignas(16) f32 res[4];

  v_norm.normalize().store(res);
  IAT_CHECK_APPROX(res[0], 1.0f);
  IAT_CHECK_APPROX(res[1], 0.0f);

  return true;
}

IAT_BEGIN_TEST_LIST()
IAT_ADD_TEST(test_float_arithmetic);
IAT_ADD_TEST(test_math_helpers);
IAT_ADD_TEST(test_approx_math);
IAT_ADD_TEST(test_linear_algebra);
IAT_END_TEST_LIST()

IAT_END_BLOCK()

IAT_REGISTER_ENTRY(Core, FloatVec4)