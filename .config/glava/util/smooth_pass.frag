
uniform sampler1D tex;
uniform int sz;
uniform int w;

out vec4 fragment;
in vec4 gl_FragCoord;

#undef PRE_SMOOTHED_AUDIO
#define PRE_SMOOTHED_AUDIO 0

#include ":util/smooth.glsl"

void main() {
    fragment = vec4(smooth_audio(tex, sz, gl_FragCoord.x / w), 0, 0, 0);
}
