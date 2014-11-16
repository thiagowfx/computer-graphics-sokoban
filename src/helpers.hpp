#ifndef _HELPERS_H_
#define _HELPERS_H_

#include <iostream>
#include "SDL.h"
#include "SDL_image.h"

void sdldie(const char*);
bool rectCollision(SDL_Rect rect1, SDL_Rect rect2);
SDL_Texture* loadTexture(SDL_Renderer*, const char* path);

#endif // _HELPERS_H_