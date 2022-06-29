layout(pixel_center_integer) in vec4 gl_FragCoord;

#request uniform "screen" screen
uniform ivec2 screen;

#request uniform "audio_sz" audio_sz
uniform int audio_sz;

#include ":util/smooth.glsl"
#include "@circle.glsl"
#include ":circle.glsl"

#request uniform "audio_l" audio_l
#request transform audio_l "window"
#request transform audio_l "fft"
#request transform audio_l "gravity"
#request transform audio_l "avg"
uniform sampler1D audio_l;

#request uniform "audio_r" audio_r
#request transform audio_r "window"
#request transform audio_r "fft"
#request transform audio_r "gravity"
#request transform audio_r "avg"
uniform sampler1D audio_r;

out vec4 fragment;

/* #define TWOPI 6.28318530718 */
#define TWOPI 6.28318530718
#define PI 3.14159265359

/* This shader is based on radial.glsl, refer to it for more commentary */

float apply_smooth(float theta) {
    float idx = theta + ROTATE;
    float dir = mod(abs(idx), TWOPI);
    if (dir > (PI))
        idx = -sign(idx) * (TWOPI - dir);
    if (INVERT > 0)
        idx = -idx;
    
    float pos = abs(idx) / ((PI / 3.25) + 0.001F);
    #define smooth_f(tex) smooth_audio(tex, audio_sz, pos)
    float v;
    if (idx > 0) v = smooth_f(audio_l) ;
    else         v = smooth_f(audio_r);
    v *= AMPLIFY;      
    #undef smooth_f
    return v;
}

void main() {
    fragment = vec4(0, 0, 0, 0);
    float
        dx = gl_FragCoord.x - (screen.x / 2),
        dy = gl_FragCoord.y - (screen.y / 2);
    float theta = atan(dy, dx);
    float d = sqrt((dx * dx) + (dy * dy));
    float adv = (1.0F / d) * (C_LINE);
    float
        adj0 = (theta + adv),
        adj1 = (theta - adv);
    d -= C_RADIUS;
    if (d >= -(float(C_LINE) / 2.0F)) {
        float v = apply_smooth(theta);
        
        adj0 = apply_smooth(adj0) - v;
        adj1 = apply_smooth(adj1) - v;
        
        float
            dmax = max(adj0, adj1),
            dmin = min(adj0, adj1);
        
        d -= v;
        #if C_FILL > 0
        #define BOUNDS (d < (float(C_LINE) / 2.0F))
        #else
        #define BOUNDS (d > -(float(C_LINE) / 2.0F) && d < (float(C_LINE) / 2.0F)) || (d <= dmax && d >= dmin)
        #endif
        if (BOUNDS)  {
            fragment = OUTLINE;
        }
    }
}
