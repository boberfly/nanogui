/*
    nanogui/vec_types.h -- Vector types definition

    NanoGUI was developed by Wenzel Jakob <wenzel.jakob@epfl.ch>.
    The widget drawing code is based on the NanoVG demo application
    by Mikko Mononen.

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE.txt file.
*/
/** \file */

#pragma once

#include <nanogui/common.h>

#if defined(NANOGUI_USE_ENOKI)
    #include <enoki/array.h>

NAMESPACE_BEGIN(nanogui)
    /* Import some common Enoki types */
    using Vector2f = enoki::Array<float, 2>;
    using Vector3f = enoki::Array<float, 3>;
    using Vector4f = enoki::Array<float, 4>;
    using Vector2i = enoki::Array<int32_t, 2>;
    using Vector3i = enoki::Array<int32_t, 3>;
    using Vector4i = enoki::Array<int32_t, 4>;
NAMESPACE_END(nanogui)

#else
    #include <array>

NAMESPACE_BEGIN(nanogui)

    template<typename T>
    class Vector2 {
    private:
        std::array<T, 2> m_values;

    public:
        Vector2() {
            m_values = {0,0};
        }
        Vector2(T x, T y) {
            m_values = {x,y};
        }

        T const & x() const {
            return m_values[0];
        }
        T & x() {
            return m_values[0];
        }
        T const & y() const {
            return m_values[1];
        }
        T & y() {
            return m_values[1];
        }
        T const & operator[](int i) const {
            return m_values[i];
        }
        T & operator[](int i) {
            return m_values[i];
        }
        Vector2<T> operator+(Vector2<T> const & other) const {
            return Vector2<T>(x()+other.x(), y()+other.y());
        }
        Vector2<T> operator-(Vector2<T> const & other) const {
            return Vector2<T>(x()-other.x(), y()-other.y());
        }
    };

    using Vector2f = Vector2<float>;
    using Vector2i = Vector2<int32_t>;

NAMESPACE_END(nanogui)

#endif
