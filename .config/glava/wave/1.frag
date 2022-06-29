
layout(pixel_center_integer) in vec4 gl_FragCoord;

#request uniform "screen" screen
uniform ivec2     screen; /* screen dimensions */

#request uniform "audio_l" audio_l
#request transform audio_l "window"
#request transform audio_l "wrange"
uniform sampler1D audio_l;

out vec4 fragment;

#include "@wave.glsl"
#include ":wave.glsl"

#define index(offset) ((texture(audio_l, (gl_FragCoord.x + offset) / screen.x).r - 0.5) * AMPLIFY) + 0.5F

void main() {
    float
        os   = index(0),
        adj0 = index(-1),
        adj1 = index(1);
    float
        s0 = adj0 - os,
        s1 = adj1 - os;
    float
        dmax = max(s0, s1),
        dmin = min(s0, s1);
    
    float s = (os + (screen.y * 0.5F) - 0.5F); /* center to screen coords */
    float diff = gl_FragCoord.y - s;
    if (abs(diff) < clamp(abs(s - (screen.y * 0.5)) * 6, MIN_THICKNESS, MAX_THICKNESS)
        || (diff <= dmax && diff >= dmin)) {
        fragment = BASE_COLOR + (abs((screen.y * 0.5F) - s) * 0.02);
    } else {
        fragment = vec4(0, 0, 0, 0);
    }
}
