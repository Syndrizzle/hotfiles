in vec4 gl_FragCoord;

#request uniform "screen" screen
uniform ivec2 screen;

#request uniform "audio_sz" audio_sz
uniform int audio_sz;

#include "@bars.glsl"
#include ":bars.glsl"

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
#include ":util/smooth.glsl"

#define TWOPI 6.28318530718
#define PI 3.14159265359

void main() {

    #if MIRROR_YX == 0
    #define AREA_WIDTH screen.x
    #define AREA_HEIGHT screen.y
    #define AREA_X gl_FragCoord.x
    #define AREA_Y gl_FragCoord.y
    #else
    #define AREA_WIDTH screen.y
    #define AREA_HEIGHT screen.x
    #define AREA_X gl_FragCoord.y
    #define AREA_Y gl_FragCoord.x
    #endif
    
    float dx = (AREA_X - (AREA_WIDTH / 2));
    #if FLIP == 0
    float d = AREA_Y;
    #else
    float d = AREA_HEIGHT - AREA_Y;
    #endif
    float section = BAR_WIDTH + BAR_GAP;    /* size of section for each bar (including gap) */
    float center =  section / 2.0F;         /* half section, distance to center             */
    float m = abs(mod(dx, section));        /* position in section                          */
    float md = m - center;                  /* position in section from center line         */
    float nbars = floor((AREA_WIDTH * 0.5F) / section) * 2;
    float p, s;
    if (md < ceil(float(BAR_WIDTH) / 2) && md >= -floor(float(BAR_WIDTH) / 2)) {  /* if not in gap */
        s = dx / section;
        p = (sign(s) == 1.0 ? ceil(s) : floor(s)) / float(nbars / 2); /* position, (-1.0F, 1.0F))     */
        p += sign(p) * ((0.5F + center) / AREA_WIDTH);    /* index center of bar position */
        /* Apply smooth function and index texture */
        #define smooth_f(tex, p) smooth_audio(tex, audio_sz, p)
        float v;
        /* ignore out of bounds values */
        if (p > 1.0F || p < -1.0F) {
            fragment = vec4(0, 0, 0, 0);
            return;
        }
        /* handle user options and store result of indexing in 'v' */
        if (p > 0.0F) {
            #if DIRECTION == 1
            p = 1.0F - p;
            #endif
            #if INVERT > 0
            v = smooth_f(audio_l, p);
            #else
            v = smooth_f(audio_r, p);
            #endif
        } else {
            p = abs(p);
            #if DIRECTION == 1
            p = 1.0F - p;
            #endif
            #if INVERT > 0
            v = smooth_f(audio_r, p);
            #else
            v = smooth_f(audio_l, p);
            #endif
        }
        #undef smooth_f

        v *= AMPLIFY;                    /* amplify result                                  */
        if (d < v - BAR_OUTLINE_WIDTH) { /* if within range of the reported frequency, draw */
            #if BAR_OUTLINE_WIDTH > 0
            if (md < (BAR_WIDTH / 2) - BAR_OUTLINE_WIDTH)
                fragment = COLOR;
            else
                fragment = BAR_OUTLINE;
            #else
            fragment = COLOR;
            #endif
            return;
        }
        
        #if BAR_OUTLINE_WIDTH > 0
        if (d <= v) {
            fragment = BAR_OUTLINE;
            return;
        }
        #endif
    }
    fragment = vec4(0, 0, 0, 0); /* default frag color */
}
