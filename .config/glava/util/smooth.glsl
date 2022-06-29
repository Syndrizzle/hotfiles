 
#ifndef _SMOOTH_GLSL /* include gaurd */
#define _SMOOTH_GLSL

#ifndef TWOPI
#define TWOPI 6.28318530718
#endif

#ifndef PI
#define PI 3.14159265359
#endif

#include "@smooth_parameters.glsl"
#include ":smooth_parameters.glsl"

/* window value t that resides in range [0, sz)*/
#define window(t, sz) (0.53836 - (0.46164 * cos(TWOPI * t / (sz - 1))))
/* this does nothing, but we keep it as an option for config */
#define linear(x) (x)
/* take value x that scales linearly between [0, 1) and return its sinusoidal curve */
#define sinusoidal(x) ((0.5 * sin((PI * (x)) - (PI / 2))) + 0.5)
/* take value x that scales linearly between [0, 1) and return its circlar curve */
#define circular(x) sqrt(1 - (((x) - 1) * ((x) - 1)))

#define average 0
#define maximum 1
#define hybrid 2

float scale_audio(float idx) {
    return -log((-(SAMPLE_RANGE) * idx) + 1) / (SAMPLE_SCALE);
}

float iscale_audio(float idx) {
    return -log((SAMPLE_RANGE) * idx) / (SAMPLE_SCALE);
}

/* Note: the SMOOTH_FACTOR macro is defined by GLava itself, from `#request setsmoothfactor`*/

float smooth_audio(in sampler1D tex, int tex_sz, highp float idx) {
    
    #if PRE_SMOOTHED_AUDIO < 1
    float
        smin = scale_audio(clamp(idx - SMOOTH_FACTOR, 0, 1)) * tex_sz,
        smax = scale_audio(clamp(idx + SMOOTH_FACTOR, 0, 1)) * tex_sz;
    float m = ((smax - smin) / 2.0F), s, w;
    float rm = smin + m; /* middle */
    
    #if SAMPLE_MODE == average
    float avg = 0, weight = 0;
    for (s = smin; s <= smax; s += 1.0F) {
        w = ROUND_FORMULA(clamp((m - abs(rm - s)) / m, 0, 1));
        weight += w;
        avg += texelFetch(tex, int(round(s)), 0).r * w;
    }
    avg /= weight;
    return avg;
    #elif SAMPLE_MODE == hybrid
    float vmax = 0, avg = 0, weight = 0, v;
    for (s = smin; s < smax; s += 1.0F) {
        w = ROUND_FORMULA(clamp((m - abs(rm - s)) / m, 0, 1));
        weight += w;
        v = texelFetch(tex, int(round(s)), 0).r * w;
        avg += v;
        if (vmax < v)
            vmax = v;
    }
    return (vmax * (1 - SAMPLE_HYBRID_WEIGHT)) + ((avg / weight) * SAMPLE_HYBRID_WEIGHT);
    #elif SAMPLE_MODE == maximum
    float vmax = 0, v;
    for (s = smin; s < smax; s += 1.0F) {
        w = texelFetch(tex, int(round(s)), 0).r * ROUND_FORMULA(clamp((m - abs(rm - s)) / m, 0, 1));
        if (vmax < w)
            vmax = w;
    }
    return vmax;
    #endif
    #else
    return texelFetch(tex, int(round(idx * tex_sz)), 0).r;
    #endif
}

/* Applies the audio smooth sampling function three times to the adjacent values */
float smooth_audio_adj(in sampler1D tex, int tex_sz, highp float idx, highp float pixel) {
    float
        al = smooth_audio(tex, tex_sz, max(idx - pixel, 0.0F)),
        am = smooth_audio(tex, tex_sz, idx),
        ar = smooth_audio(tex, tex_sz, min(idx + pixel, 1.0F));
    return (al + am + ar) / 3.0F;
}

#ifdef TWOPI
#undef TWOPI
#endif
#ifdef PI
#undef PI
#endif
#endif /* _SMOOTH_GLSL */
