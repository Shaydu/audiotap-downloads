/*{
    "ISFVSN": "2",
    "DESCRIPTION": "Spectrum bars with adjustable colour",
    "CREDIT": "AudioTap example",
    "CATEGORIES": ["Audio Reactive"],
    "INPUTS": [
        {
            "NAME": "spectrum",
            "TYPE": "audioFFT",
            "MAX": 64
        },
        {
            "NAME": "barColor",
            "TYPE": "color",
            "LABEL": "Bar colour",
            "DEFAULT": [0.88, 0.55, 0.85, 1.0]
        },
        {
            "NAME": "smoothing",
            "TYPE": "float",
            "LABEL": "Smoothing",
            "MIN": 0.0,
            "MAX": 0.95,
            "DEFAULT": 0.6
        }
    ],
    "PASSES": [
        {
            "TARGET": "smoothed",
            "PERSISTENT": true,
            "FLOAT": true,
            "WIDTH": "64",
            "HEIGHT": "1"
        },
        {}
    ]
}*/

// Pass 0 keeps a smoothed copy of the 64-band spectrum in a persistent
// buffer. Pass 1 draws one bar per band from that copy.

const float BANDS = 64.0;

void main() {
    if (PASSINDEX == 0) {
        float x = isf_FragNormCoord.x;
        float now = IMG_NORM_PIXEL(spectrum, vec2(x, 0.5)).r;
        float before = IMG_NORM_PIXEL(smoothed, vec2(x, 0.5)).r;
        float level = mix(now, before, smoothing);
        gl_FragColor = vec4(level, level, level, 1.0);
    } else {
        vec2 uv = isf_FragNormCoord;
        float band = floor(uv.x * BANDS);
        float level = IMG_NORM_PIXEL(smoothed, vec2((band + 0.5) / BANDS, 0.5)).r;

        // A gap between bars: each bar fills 80% of its column.
        float inBar = step(fract(uv.x * BANDS), 0.8);
        float lit = step(uv.y, level) * inBar;

        // Brighter towards the top of each bar.
        float shade = 0.55 + 0.45 * clamp(uv.y / max(level, 0.001), 0.0, 1.0);
        vec3 background = vec3(0.04, 0.035, 0.06);
        vec3 color = mix(background, barColor.rgb * shade, lit);
        gl_FragColor = vec4(color, 1.0);
    }
}
