#include "mitchell_netravali_filter.h"

namespace MitchellNetravaliFilter {

/*
Mitchell-Netravali coefficients and anti-ringing behavior adapted from:
https://github.com/stenzek/duckstation/blob/master/data/resources/shaders/reshade/Shaders/interpolation/bicubic.fx

Bicubic multipass Shader
Copyright (C) 2011-2022 Hyllian - sergiogdb@gmail.com

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

const char* const FragmentShaderSource = R"GLSL(
#version 120
uniform sampler2D movieTexture;
uniform vec2 sourceSize;
uniform vec2 textureSize;

vec4 mitchellNetravaliWeights(float t) {
    const float b = 1.0 / 3.0;
    const float c = 1.0 / 3.0;
    float t2 = t * t;
    float t3 = t2 * t;
    return vec4(
        ((-b - 6.0*c)*t3 + (3.0*b + 12.0*c)*t2 + (-3.0*b - 6.0*c)*t + b) / 6.0,
        ((12.0 - 9.0*b - 6.0*c)*t3 + (-18.0 + 12.0*b + 6.0*c)*t2 + 6.0 - 2.0*b) / 6.0,
        (-(12.0 - 9.0*b - 6.0*c)*t3 + (18.0 - 15.0*b - 12.0*c)*t2 + (3.0*b + 6.0*c)*t + b) / 6.0,
        ((b + 6.0*c)*t3 - 6.0*c*t2) / 6.0);
}

vec4 sourcePixel(vec2 pixel) {
    pixel = clamp(pixel, vec2(0.0), sourceSize - vec2(1.0));
    return texture2D(movieTexture, (pixel + vec2(0.5)) / textureSize);
}

void main() {
    vec2 position = gl_TexCoord[0].st * textureSize - vec2(0.5);
    vec2 base = floor(position);
    vec2 fraction = position - base;
    vec4 wx = mitchellNetravaliWeights(fraction.x);
    vec4 wy = mitchellNetravaliWeights(fraction.y);

    vec4 center00 = sourcePixel(base + vec2(0.0, 0.0));
    vec4 center10 = sourcePixel(base + vec2(1.0, 0.0));
    vec4 center01 = sourcePixel(base + vec2(0.0, 1.0));
    vec4 center11 = sourcePixel(base + vec2(1.0, 1.0));

    vec4 row0 =
        sourcePixel(base + vec2(-1.0, -1.0)) * wx.x +
        sourcePixel(base + vec2( 0.0, -1.0)) * wx.y +
        sourcePixel(base + vec2( 1.0, -1.0)) * wx.z +
        sourcePixel(base + vec2( 2.0, -1.0)) * wx.w;
    vec4 row1 =
        sourcePixel(base + vec2(-1.0, 0.0)) * wx.x +
        center00 * wx.y + center10 * wx.z +
        sourcePixel(base + vec2(2.0, 0.0)) * wx.w;
    vec4 row2 =
        sourcePixel(base + vec2(-1.0, 1.0)) * wx.x +
        center01 * wx.y + center11 * wx.z +
        sourcePixel(base + vec2(2.0, 1.0)) * wx.w;
    vec4 row3 =
        sourcePixel(base + vec2(-1.0, 2.0)) * wx.x +
        sourcePixel(base + vec2( 0.0, 2.0)) * wx.y +
        sourcePixel(base + vec2( 1.0, 2.0)) * wx.z +
        sourcePixel(base + vec2( 2.0, 2.0)) * wx.w;

    vec4 color = row0 * wy.x + row1 * wy.y + row2 * wy.z + row3 * wy.w;
    vec3 minimumColor = min(min(center00.rgb, center10.rgb), min(center01.rgb, center11.rgb));
    vec3 maximumColor = max(max(center00.rgb, center10.rgb), max(center01.rgb, center11.rgb));
    color.rgb = clamp(color.rgb, minimumColor, maximumColor);
    gl_FragColor = vec4(color.rgb, 1.0);
}
)GLSL";

}
