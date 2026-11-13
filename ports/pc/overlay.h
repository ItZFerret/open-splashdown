// The port's settings overlay (ports/pc/overlay.cpp), drawn over the game's
// picture by the presenter (ports/pc/video.cpp). Main thread only.
#ifndef PORT_OVERLAY_H
#define PORT_OVERLAY_H

union SDL_Event;
struct SDL_Renderer;
struct SDL_Window;

// An SDL event: true when the overlay used it (F1 or the gamepad's guide
// button, or both sticks pressed, open and close it; while open it takes
// the keyboard, the gamepads and the mouse).
bool port_overlay_event(const SDL_Event &e, SDL_Window *window);
bool port_overlay_is_open(void);
// True once after a setting changed: the presenter applies them.
bool port_overlay_take_changed(void);
// Draw what is due over the picture in a ww x wh pixel window: the frame
// counter, the "F1: settings" note of the first seconds, the menu.
void port_overlay_draw(SDL_Renderer *r, int ww, int wh, double fps);
// True while something is drawn that changes without a new game frame.
bool port_overlay_animating(void);

#endif
