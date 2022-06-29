in vec4 gl_FragCoord;

#request uniform "screen" screen
uniform ivec2 screen;

#request uniform "audio_sz" audio_sz
uniform int audio_sz;

#include ":util/smooth.glsl"
#include "@radial.glsl"
#include ":radial.glsl"

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

#define TWOPI 6.28318530718
#define PI 3.14159265359

void main() {
    
    #if USE_ALPHA > 0
    #define APPLY_FRAG(f, c) f = vec4(f.rgb * f.a + c.rgb * (1 - clamp(f.a, 0, 1)), max(c.a, f.a))
    fragment = #00000000;
    #else
    #define APPLY_FRAG(f, c) f = c
    #endif
    
    /* To handle jagged edges, we alias in the shader by using alpha layer blending.
       Alpha layer blending is only applied when `xroot` transparency is enabled. */
    
    float /* translate (x, y) to use (0, 0) as the center of the screen */
        dx = gl_FragCoord.x - (screen.x / 2) + CENTER_OFFSET_X,
        dy = gl_FragCoord.y - (screen.y / 2) + CENTER_OFFSET_Y;
    float theta = atan(dy, dx); /* fragment angle with the center of the screen as the origin */
    float d = sqrt((dx * dx) + (dy * dy)); /* distance */
    if (d > C_RADIUS - (float(C_LINE) / 2.0F) && d < C_RADIUS + (float(C_LINE) / 2.0F)) {
        APPLY_FRAG(fragment, OUTLINE);
        #if USE_ALPHA > 0
        fragment.a *= clamp(((C_LINE / 2) - abs(C_RADIUS - d)) * C_ALIAS_FACTOR, 0, 1);
        #else
        return; /* return immediately if there is no alpha blending available */
        #endif
    }
    if (d > C_RADIUS) {
        const float section = (TWOPI / NBARS);         /* range (radians) for each bar */
        const float center = ((TWOPI / NBARS) / 2.0F); /* center line angle */
        float m = mod(theta, section);                 /* position in section (radians) */
        float ym = d * sin(center - m);                /* distance from center line (cartesian coords) */
        if (abs(ym) < BAR_WIDTH / 2) {                 /* if within width, draw audio */
            float idx = theta + ROTATE;                /* position (radians) in texture */
            float dir = mod(abs(idx), TWOPI);          /* absolute position, [0, 2pi) */
            if (dir > PI)
                idx = -sign(idx) * (TWOPI - dir);      /* Re-correct position values to [-pi, pi) */
            if (INVERT > 0)
                idx = -idx;                            /* Invert if needed */
            float pos = int(abs(idx) / section) / float(NBARS / 2);        /* bar position, [0, 1) */
            #define smooth_f(tex) smooth_audio(tex, audio_sz, pos)         /* smooth function format */
            float v;
            if (idx > 0) v = smooth_f(audio_l);                            /* left buffer */
            else         v = smooth_f(audio_r);                            /* right buffer */
            v *= AMPLIFY;                                                  /* amplify */
            #undef smooth_f
            /* offset to fragment distance from inner circle */
            #if USE_ALPHA > 0
            #define ALIAS_FACTOR (((BAR_WIDTH / 2) - abs(ym)) * BAR_ALIAS_FACTOR)
            d -= C_RADIUS; /* start bar overlapping the inner circle for blending */
            #else
            #define ALIAS_FACTOR 1
            d -= C_RADIUS + (float(C_LINE) / 2.0F); /* start bar after circle */
            #endif
            if (d <= v - BAR_OUTLINE_WIDTH) {
                vec4 r;
                #if BAR_OUTLINE_WIDTH > 0
                if (abs(ym) < (BAR_WIDTH / 2) - BAR_OUTLINE_WIDTH)
                    r = COLOR;
                else
                    r = BAR_OUTLINE;
                #else
                r = COLOR;
                #endif
                #if USE_ALPHA > 0
                r.a *= ALIAS_FACTOR;
                #endif
                APPLY_FRAG(fragment, r);
                return;
            }
            #if BAR_OUTLINE_WIDTH > 0
            if (d <= v) {
                #if USE_ALPHA > 0
                vec4 r = BAR_OUTLINE;
                r.a *= ALIAS_FACTOR;
                APPLY_FRAG(fragment, r);
                #else
                APPLY_FRAG(fragment, BAR_OUTLINE);
                #endif
                return;
            }
            #endif
        }
    }
    fragment = APPLY_FRAG(fragment, vec4(0, 0, 0, 0)); /* default frag color */
}
