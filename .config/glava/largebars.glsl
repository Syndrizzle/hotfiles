
/* Center line thickness (pixels) */
#define C_LINE 1
/* Width (in pixels) of each bar */
#define BAR_WIDTH 15
/* Width (in pixels) of each bar gap */
#define BAR_GAP 15
/* Outline color */
#define BAR_OUTLINE #b4befe
/* Outline width (in pixels, set to 0 to disable outline drawing) */
#define BAR_OUTLINE_WIDTH 4
/* Amplify magnitude of the results each bar displays */
#define AMPLIFY 600
/* Whether the current settings use the alpha channel;
   enabling this is required for alpha to function
   correctly on X11 with `"native"` transparency. */
#define USE_ALPHA 0
/* How strong the gradient changes */
#define GRADIENT_POWER 60
/* Bar color changes with height */
#define GRADIENT (d / GRADIENT_POWER + 1)
/* Bar color */
#define COLOR (#cba6f7 * GRADIENT)
/* Direction that the bars are facing, 0 for inward, 1 for outward */
#define DIRECTION 0
/* Whether to switch left/right audio buffers */
#define INVERT 0
/* Whether to flip the output vertically */
#define FLIP 0
/* Whether to mirror output along `Y = X`, causing output to render on the left side of the window */
/* Use with `FLIP 1` to render on the right side */
#define MIRROR_YX 0
