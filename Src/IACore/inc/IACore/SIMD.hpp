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

#pragma once

#include <IACore/PCH.hpp>

#if defined(__clang__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wunused-parameter"
#  pragma GCC diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#endif

#include <hwy/highway.h>

#if defined(__clang__)
#  pragma GCC diagnostic pop
#endif

namespace IACore
{
  namespace hn = hwy::HWY_NAMESPACE;

#if HWY_TARGET == HWY_SCALAR
#  pragma message("Warning: Configuration mismatch. IACore is being compiled for SCALAR SIMD (Slow)")
#endif

  class alignas(16) IntVec4
  {
public:
    IntVec4() = default;

    inline explicit IntVec4(const u32 s);
    inline explicit IntVec4(const u32 *values);
    inline explicit IntVec4(const u32 a, const u32 b, const u32 c, const u32 d);

    inline auto operator+(Ref<IntVec4> other) const -> IntVec4;
    inline auto operator-(Ref<IntVec4> other) const -> IntVec4;
    inline auto operator*(Ref<IntVec4> other) const -> IntVec4;

    inline auto operator&(Ref<IntVec4> other) const -> IntVec4;
    inline auto operator|(Ref<IntVec4> other) const -> IntVec4;
    inline auto operator^(Ref<IntVec4> other) const -> IntVec4;
    inline auto operator~() const -> IntVec4;

    inline auto operator<<(const u32 amount) const -> IntVec4;
    inline auto operator>>(const u32 amount) const -> IntVec4;

    [[nodiscard]] inline auto sat_add(Ref<IntVec4> other) const -> IntVec4;
    [[nodiscard]] inline auto sat_sub(Ref<IntVec4> other) const -> IntVec4;

    [[nodiscard]] inline auto clamp(const u32 min, const u32 max) const -> IntVec4;

    [[nodiscard]] inline auto mult_add(Ref<IntVec4> multiplier, Ref<IntVec4> addend) const -> IntVec4;

    inline auto store(Mut<u32 *> values) -> void;
    static inline auto load(const u32 *values) -> IntVec4;

private:
    using Tag = hn::FixedTag<u32, 4>;

    Mut<hn::Vec<Tag>> m_data;

    inline explicit IntVec4(const hn::Vec<Tag> v) : m_data(v)
    {
    }
  };

  class alignas(16) FloatVec4
  {
public:
    FloatVec4() = default;

    inline explicit FloatVec4(const f32 s);
    inline explicit FloatVec4(const f32 *values);
    inline explicit FloatVec4(const f32 a, const f32 b, const f32 c, const f32 d);

    inline auto operator+(Ref<FloatVec4> other) const -> FloatVec4;
    inline auto operator-(Ref<FloatVec4> other) const -> FloatVec4;
    inline auto operator*(Ref<FloatVec4> other) const -> FloatVec4;
    inline auto operator/(Ref<FloatVec4> other) const -> FloatVec4;

    [[nodiscard]] inline auto clamp(const f32 min, const f32 max) const -> FloatVec4;

    [[nodiscard]] inline auto abs() const -> FloatVec4;
    [[nodiscard]] inline auto sqrt() const -> FloatVec4;
    [[nodiscard]] inline auto rsqrt() const -> FloatVec4;
    [[nodiscard]] inline auto normalize() const -> FloatVec4;

    [[nodiscard]] inline auto dot(Ref<FloatVec4> other) const -> f32;

    [[nodiscard]] inline auto mult_add(Ref<FloatVec4> multiplier, Ref<FloatVec4> addend) const -> FloatVec4;

    inline auto store(Mut<f32 *> values) -> void;
    static inline auto load(const f32 *values) -> FloatVec4;

private:
    using Tag = hn::FixedTag<f32, 4>;

    Mut<hn::Vec<Tag>> m_data;

    inline explicit FloatVec4(const hn::Vec<Tag> v) : m_data(v)
    {
    }
  };
} // namespace IACore

namespace IACore
{
  IntVec4::IntVec4(const u32 s)
  {
    const Tag d;
    m_data = hn::Set(d, s);
  }

  IntVec4::IntVec4(const u32 *values)
  {
    const Tag data;
    m_data = hn::Load(data, values);
  }

  IntVec4::IntVec4(const u32 a, const u32 b, const u32 c, const u32 d)
  {
    const Tag data;
    alignas(16) Mut<Array<u32, 4>> values = {a, b, c, d};
    m_data = hn::Load(data, values.data());
  }

  auto IntVec4::operator+(Ref<IntVec4> other) const -> IntVec4
  {
    return IntVec4(hn::Add(m_data, other.m_data));
  }

  auto IntVec4::operator-(Ref<IntVec4> other) const -> IntVec4
  {
    return IntVec4(hn::Sub(m_data, other.m_data));
  }

  auto IntVec4::operator*(Ref<IntVec4> other) const -> IntVec4
  {
    return IntVec4(hn::Mul(m_data, other.m_data));
  }

  auto IntVec4::operator&(Ref<IntVec4> other) const -> IntVec4
  {
    return IntVec4(hn::And(m_data, other.m_data));
  }

  auto IntVec4::operator|(Ref<IntVec4> other) const -> IntVec4
  {
    return IntVec4(hn::Or(m_data, other.m_data));
  }

  auto IntVec4::operator^(Ref<IntVec4> other) const -> IntVec4
  {
    return IntVec4(hn::Xor(m_data, other.m_data));
  }

  auto IntVec4::operator~() const -> IntVec4
  {
    return IntVec4(hn::Not(m_data));
  }

  auto IntVec4::operator<<(const u32 amount) const -> IntVec4
  {
    return IntVec4(hn::ShiftLeftSame(m_data, amount));
  }

  auto IntVec4::operator>>(const u32 amount) const -> IntVec4
  {
    return IntVec4(hn::ShiftRightSame(m_data, amount));
  }

  auto IntVec4::mult_add(Ref<IntVec4> multiplier, Ref<IntVec4> addend) const -> IntVec4
  {
    return IntVec4(hn::MulAdd(m_data, multiplier.m_data, addend.m_data));
  }

  auto IntVec4::sat_add(Ref<IntVec4> other) const -> IntVec4
  {
    return IntVec4(hn::SaturatedAdd(m_data, other.m_data));
  }

  auto IntVec4::sat_sub(Ref<IntVec4> other) const -> IntVec4
  {
    return IntVec4(hn::SaturatedSub(m_data, other.m_data));
  }

  auto IntVec4::clamp(const u32 min, const u32 max) const -> IntVec4
  {
    const Tag d;
    const hn::Vec<Tag> v_min = hn::Set(d, min);
    const hn::Vec<Tag> v_max = hn::Set(d, max);
    return IntVec4(hn::Min(hn::Max(m_data, v_min), v_max));
  }

  auto IntVec4::store(Mut<u32 *> values) -> void
  {
    const Tag d;
    hn::Store(m_data, d, values);
  }

  auto IntVec4::load(const u32 *values) -> IntVec4
  {
    const Tag d;
    return IntVec4(hn::Load(d, values));
  }
} // namespace IACore

namespace IACore
{
  FloatVec4::FloatVec4(const f32 s)
  {
    const Tag d;
    m_data = hn::Set(d, s);
  }

  FloatVec4::FloatVec4(const f32 *values)
  {
    const Tag d;
    m_data = hn::Load(d, values);
  }

  FloatVec4::FloatVec4(const f32 a, const f32 b, const f32 c, const f32 d)
  {
    const Tag data;
    alignas(16) Mut<Array<f32, 4>> temp = {a, b, c, d};
    m_data = hn::Load(data, temp.data());
  }

  auto FloatVec4::operator+(Ref<FloatVec4> other) const -> FloatVec4
  {
    return FloatVec4(hn::Add(m_data, other.m_data));
  }

  auto FloatVec4::operator-(Ref<FloatVec4> other) const -> FloatVec4
  {
    return FloatVec4(hn::Sub(m_data, other.m_data));
  }

  auto FloatVec4::operator*(Ref<FloatVec4> other) const -> FloatVec4
  {
    return FloatVec4(hn::Mul(m_data, other.m_data));
  }

  auto FloatVec4::operator/(Ref<FloatVec4> other) const -> FloatVec4
  {
    return FloatVec4(hn::Div(m_data, other.m_data));
  }

  auto FloatVec4::mult_add(Ref<FloatVec4> multiplier, Ref<FloatVec4> addend) const -> FloatVec4
  {
    return FloatVec4(hn::MulAdd(m_data, multiplier.m_data, addend.m_data));
  }

  auto FloatVec4::clamp(const f32 min, const f32 max) const -> FloatVec4
  {
    const Tag d;
    const hn::Vec<Tag> v_min = hn::Set(d, min);
    const hn::Vec<Tag> v_max = hn::Set(d, max);
    return FloatVec4(hn::Min(hn::Max(m_data, v_min), v_max));
  }

  auto FloatVec4::sqrt() const -> FloatVec4
  {
    return FloatVec4(hn::Sqrt(m_data));
  }

  auto FloatVec4::rsqrt() const -> FloatVec4
  {
    return FloatVec4(hn::ApproximateReciprocalSqrt(m_data));
  }

  auto FloatVec4::abs() const -> FloatVec4
  {
    return FloatVec4(hn::Abs(m_data));
  }

  auto FloatVec4::dot(Ref<FloatVec4> other) const -> f32
  {
    const Tag d;
    const hn::Vec<Tag> v_mul = hn::Mul(m_data, other.m_data);
    return hn::ReduceSum(d, v_mul);
  }

  auto FloatVec4::normalize() const -> FloatVec4
  {
    const Tag d;
    const hn::Vec<Tag> v_mul = hn::Mul(m_data, m_data);
    const hn::Vec<Tag> v_len_sq = hn::SumOfLanes(d, v_mul);
    const hn::Vec<Tag> v_inv_len = hn::ApproximateReciprocalSqrt(v_len_sq);
    return FloatVec4(hn::Mul(m_data, v_inv_len));
  }

  auto FloatVec4::store(Mut<f32 *> values) -> void
  {
    const Tag d;
    hn::Store(m_data, d, values);
  }

  auto FloatVec4::load(const f32 *values) -> FloatVec4
  {
    const Tag d;
    return FloatVec4(hn::Load(d, values));
  }
} // namespace IACore