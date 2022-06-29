
layout(pixel_center_integer) in vec4 gl_FragCoord;

#request uniform "screen" screen
uniform ivec2 screen; /* screen dimensions */

#request uniform "audio_sz" audio_sz
uniform int audio_sz;

/* When we transform our audio, we need to go through the following steps: 
   
   transform -> "window"
       First, apply a window function to taper off the ends of the spectrum, helping
       avoid artifacts in the FFT output.
   
   transform -> "fft"
       Apply the Fast Fourier Transform algorithm to separate raw audio data (waves)
       into their respective spectrums.
   
   transform -> "fft"
       As part of the FFT process, we return spectrum magnitude on a log(n) scale,
       as this is how the (decibel) dB scale functions.
       
   transform -> "gravity"
       To help make our data more pleasing to look at, we apply our data received over
       time to a buffer, taking the max of either the existing value in the buffer or
       the data from the input. We then reduce the data by the 'gravity step', and
       return the storage buffer.
       
       This makes frequent and abrupt changes in frequency less distracting, and keeps
       short frequency responses on the screen longer.
       
   transform -> "avg"
       As a final step, we take the average of several data frames (specified by
       'setavgframes') and return the result to further help smooth the resulting
       animation. In order to mitigate abrupt changes to the average, the values
       at each end of the average buffer can be weighted less with a window function
       (the same window function used at the start of this step!). It can be disabled
       with 'setavgwindow'.
*/

#include ":util/smooth.glsl"
#include "@graph.glsl"
#include ":graph.glsl"

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

/* distance from center */
#define CDIST (abs((screen.x / 2) - gl_FragCoord.x) / screen.x)
/* distance from sides (far) */
#define FDIST (min(gl_FragCoord.x, screen.x - gl_FragCoord.x) / screen.x)

#if DIRECTION < 0
#define LEFT_IDX (gl_FragCoord.x)
#define RIGHT_IDX (-gl_FragCoord.x + screen.x)
/* distance from base frequencies */
#define BDIST FDIST
/* distance from high frequencies */
#define HDIST CDIST
#else
#define LEFT_IDX (half_w - gl_FragCoord.x)
#define RIGHT_IDX (gl_FragCoord.x - half_w)
#define BDIST CDIST
#define HDIST FDIST
#endif

#define TWOPI 6.28318530718

float half_w;
float middle;
highp float pixel = 1.0F / float(screen.x);

float get_line_height(in sampler1D tex, float idx) {
    float s = smooth_audio_adj(tex, audio_sz, idx / half_w, pixel);
    /* scale the data upwards so we can see it */
    s *= VSCALE;
    /* clamp far ends of the screen down to make the ends of the graph smoother */

    float fact = clamp((abs((screen.x / 2) - gl_FragCoord.x) / screen.x) * 48, 0.0F, 1.0F);
    #if JOIN_CHANNELS > 0
    fact = -2 * pow(fact, 3) + 3 * pow(fact, 2);    /* To avoid spikes */
    s = fact * s + (1 - fact) * middle;
    #else
    s *= fact;
    #endif

    s *= clamp((min(gl_FragCoord.x, screen.x - gl_FragCoord.x) / screen.x) * 48, 0.0F, 1.0F);

    return s;
}

void render_side(in sampler1D tex, float idx) {
    float s = get_line_height(tex, idx);

    /* and finally set fragment color if we are in range */
    #if INVERT > 0
    float pos = float(screen.y) - gl_FragCoord.y;
    #else
    float pos = gl_FragCoord.y;
    #endif
    if (pos + 1.5 <= s) {
        fragment = COLOR;
    } else {
        fragment = vec4(0, 0, 0, 0);
    }
}

void main() {
    half_w = (screen.x / 2);

    middle = VSCALE * (smooth_audio_adj(audio_l, audio_sz, 1, pixel) + smooth_audio_adj(audio_r, audio_sz, 0, pixel)) / 2;

    if (gl_FragCoord.x < half_w) {
        render_side(audio_l, LEFT_IDX);
    } else {
        render_side(audio_r, RIGHT_IDX);
    }
}
