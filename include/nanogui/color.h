/*
    NanoGUI was developed by Wenzel Jakob <wenzel.jakob@epfl.ch>.
    The widget drawing code is based on the NanoVG demo application
    by Mikko Mononen.

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE.txt file.
*/
/**
 * \file nanogui/combobox.h
 *
 * \brief Simple combo box widget based on a popup button.
 */

#pragma once

#include <nanogui/vec_types.h>
#include <nanogui/opengl.h>

NAMESPACE_BEGIN(nanogui)

/**
 * \class Color color.h nanogui/color.h
 *
 * \brief Stores an RGBA floating point color value.
 */
class Color {
private:
    std::array<float, 4> m_rgba;

public:
    /// Default constructor: represents black (``r, g, b, a = 0``)
    Color() : m_rgba{0, 0, 0, 0} { }

    /**
     * Creates the Color ``(intensity, intensity, intensity, alpha)``.
     *
     * \param intensity
     * The value to be used for red, green, and blue.
     *
     * \param alpha
     * The alpha component of the color.
     */
    Color(float intensity, float alpha)
        : Color(intensity, intensity, intensity, alpha) { }

    /**
     * Creates the Color ``(intensity, intensity, intensity, alpha) / 255.0``.
     * Values are casted to floats before division.
     *
     * \param intensity
     * The value to be used for red, green, and blue, will be divided by ``255.0``.
     *
     * \param alpha
     * The alpha component of the color, will be divided by ``255.0``.
     */
    Color(int intensity, int alpha)
        : Color(intensity, intensity, intensity, alpha) { }

    /**
     * Explicit constructor: creates the Color ``(r, g, b, a)``.
     *
     * \param r
     * The red component of the color.
     *
     * \param g
     * The green component of the color.
     *
     * \param b
     * The blue component of the color.
     *
     * \param a
     * The alpha component of the color.
     */
    Color(float r, float g, float b, float a) : m_rgba{r, g, b, a} { }

    /**
     * Explicit constructor: creates the Color ``(r, g, b, a) / 255.0``.
     * Values are casted to floats before division.
     *
     * \param r
     * The red component of the color, will be divided by ``255.0``.
     *
     * \param g
     * The green component of the color, will be divided by ``255.0``.
     *
     * \param b
     * The blue component of the color, will be divided by ``255.0``.
     *
     * \param a
     * The alpha component of the color, will be divided by ``255.0``.
     */
    Color(int r, int g, int b, int a) : m_rgba{(float) r / 255.f, (float) g / 255.f, (float) b / 255.f, (float) a / 255.f} { }

    /// Return a reference to the red channel
    float &r() { return m_rgba[0]; }
    /// Return a reference to the red channel (const version)
    const float &r() const { return m_rgba[0]; }
    /// Return a reference to the green channel
    float &g() { return m_rgba[1]; }
    /// Return a reference to the green channel (const version)
    const float &g() const { return m_rgba[1]; }
    /// Return a reference to the blue channel
    float &b() { return m_rgba[2]; }
    /// Return a reference to the blue channel (const version)
    const float &b() const { return m_rgba[2]; }
    /// Return a reference to the alpha channel
    float &a() { return m_rgba[3]; }
    /// Return a reference to the alpha channel (const version)
    const float &a() const { return m_rgba[3]; }

    /**
     * Computes the luminance as ``l = 0.299r + 0.587g + 0.144b + 0.0a``.  If
     * the luminance is less than 0.5, white is returned.  If the luminance is
     * greater than or equal to 0.5, black is returned.  Both returns will have
     * an alpha component of 1.0.
     */
    Color contrasting_color() const {
        float luminance = 0.299f * r() + 0.587f * g() + 0.144f * b();
        return Color(luminance < 0.5f ? 1.f : 0.f, 1.f);
    }

    /// Allows for conversion between this Color and NanoVG's representation.
    operator const NVGcolor &() const {
        return reinterpret_cast<const NVGcolor &>(*m_rgba.data());
    }
};

NAMESPACE_END(nanogui)
