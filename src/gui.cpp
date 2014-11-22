#include "gui.hpp"

Gui::Gui() {
  /* Initialize SDL. */
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    LOG_SDL_DIE("SDL could not be initialized");
  }

  /* Set texture filtering to linear. */
  if(!SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1")) {
    std::cout << "Warning: Linear texture filtering not enabled!" << std::endl;
  }

  /* Create the main window. */
  window = SDL_CreateWindow(GAME_TITLE, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL);// | SDL_WINDOW_FULLSCREEN_DESKTOP);
  if (window == NULL) {
    LOG_SDL_DIE("SDL Window could not be created");
  }

  /* Create the renderer for the window. */
  windowRenderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_TARGETTEXTURE);
  if(windowRenderer == NULL) {
    LOG_SDL_DIE("Renderer could not be created");
  }

  /* Enable PNG and JPEG support from SDL2_image. */
  int imgFlags = IMG_INIT_PNG | IMG_INIT_JPG;
  if(!(IMG_Init(imgFlags) & imgFlags)) {
    LOG_SDL_DIE("SDL_image could not initialize");
  }

  /* Enable TTF support from SDL2_TTF. */
  if(TTF_Init() == -1) {
    LOG_SDL_DIE("SDL_ttf could not initialize");
  }
  else {
    windowFont = TTF_OpenFont(GAME_FONT_PATH, 60);
    if (windowFont == NULL) {
      LOG_SDL_DIE("Failed to load the font");
    }
  }
}

Gui::~Gui() {
  /* Destroy OpenGL. */
  SDL_GL_DeleteContext(glContext);
  delete game;
  game = NULL;

  /* Destroy textures. */
  SDL_DestroyTexture(backgroundTexture);
  backgroundTexture = NULL;

  /* Destroy the menu. */
  delete gameMenu;
  gameMenu = NULL;

  /* Destroy fonts. */
  TTF_CloseFont(windowFont);
  windowFont = NULL;

  /* Destroy renderer and window. */
  SDL_DestroyRenderer(windowRenderer);
  SDL_DestroyWindow(window);
  windowRenderer = NULL;
  window = NULL;

  IMG_Quit();
  TTF_Quit();
  SDL_Quit();
}

void Gui::gameLoop() {
  bool quit = false;
  SDL_Event e;

  /* Show the game splash screen. */
  renderSplashScreen(SPLASH_TEXTURE_PATH, GAME_SPLASH_TIMEOUT);

  /* Show the main menu. */
  backgroundTexture = loadTexture(windowRenderer, MENU_BACKGROUND_TEXTURE_PATH);
  gameMenu = new Menu(windowRenderer, MENU_LABEL_IN_COLOR, MENU_LABEL_OUT_COLOR, windowFont, SCREEN_WIDTH, SCREEN_HEIGHT, GAME_MENU_LABELS, backgroundTexture);

  while(!quit) {
    while(SDL_PollEvent(&e) != 0) {
      if (e.type == SDL_QUIT) {
        SDL_Log("SDL_QUIT event");
        quit = true;
        break;
      }
      else if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_RESIZED && context == CONTEXT_GAME) {
        unsigned width = e.window.data1;
        unsigned height = e.window.data2;
        SDL_Log("SDL_WINDOWEVENT: SDL_WINDOWEVENT_RESIZED: %d x %d", width, height);
        SDL_SetWindowSize(window, width, height);
        game->setWindowSize(width, height);
      }
      else if (e.type == SDL_KEYDOWN) {
        SDL_Log("SDL_KEYDOWN event: %s", SDL_GetKeyName(e.key.keysym.sym));
        if (context == CONTEXT_GAME && isMovementKey(e.key.keysym.sym)) {
          if (game->isLevelFinished()) {
            if (currentLevel == (GAME_MENU_LABELS.size() - 1)) {
              SDL_Log("Finished the last level (%d). Switching to CONTEXT_GAME_WON now.", currentLevel);
              context = CONTEXT_GAME_WON;
            }
            else {
              SDL_Log("Finished level %d", currentLevel);
              game->loadLevel(++currentLevel);
            }
          }
        }
        switch(e.key.keysym.sym) {
        case SDLK_ESCAPE:
        case SDLK_q:
          quit = true;
          break;
        case SDLK_s:
        case SDLK_DOWN:
          if (context == CONTEXT_MAIN_MENU) {
            gameMenu->nextIndex();
          }
          else if (context == CONTEXT_GAME){
            game->moveDownAction();
          }
          break;
        case SDLK_w:
        case SDLK_UP:
          if (context == CONTEXT_MAIN_MENU) {
            gameMenu->prevIndex();
          }
          else if (context == CONTEXT_GAME){
            game->moveUpAction();
          }
          break;
        case SDLK_a:
        case SDLK_LEFT:
          if (context == CONTEXT_GAME){
            game->moveLeftAction();
          }
          break;
        case SDLK_d:
        case SDLK_RIGHT:
          if (context == CONTEXT_GAME){
            game->moveRightAction();
          }
          break;
        case SDLK_r:
          if (context == CONTEXT_GAME) {
            SDL_Log("Level restarted");
            game->loadLevel(currentLevel);
          }
          break;
        case SDLK_u:
          if (context == CONTEXT_GAME) {
            game->undoAction();
          }
          break;
        case SDLK_n:
          // TODO: remove this later on.
          game->loadLevel(++currentLevel);
          break;
        case SDLK_RETURN:
          if (context == CONTEXT_MAIN_MENU) {
            unsigned index = gameMenu->getCurrentIndex();
            if (index == GAME_MENU_LABELS.size() - 1) {
              quit = true;
              break;
            }
            else {
              context = CONTEXT_GAME;
              if(!OPENGL_LOADED) {
                game = new Game(window, &glContext, SCREEN_WIDTH, SCREEN_HEIGHT);
                OPENGL_LOADED = true;
              }
              game->loadLevel(currentLevel = index + 1);
            }
          }
          break;
        }
      }
      else if (e.type == SDL_MOUSEMOTION) {
        if (context == CONTEXT_GAME) {
          int x, y; Uint32 mouseState = SDL_GetMouseState(&x, &y);
          if (mouseState & SDL_BUTTON(SDL_BUTTON_LEFT)) {
            SDL_Log("Mouse Button 1 (left) is being pressed and moved: %d, %d", x, y);
            game->setNewPosition(x, y);
          }
        }
      }
      else if (e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEBUTTONUP) {
        int x, y; Uint32 mouseState = SDL_GetMouseState(&x, &y);
        if (mouseState & SDL_BUTTON(SDL_BUTTON_LEFT)) {
          SDL_Log("Mouse Button 1 (left) pressed: %d, %d", x, y);
          if (context == CONTEXT_MAIN_MENU) {
            unsigned index = gameMenu->getCurrentIndex();
            if (index == GAME_MENU_LABELS.size() - 1) {
              quit = true;
              break;
            }
            else {
              context = CONTEXT_GAME;
              if(!OPENGL_LOADED) {
                game = new Game(window, &glContext, SCREEN_WIDTH, SCREEN_HEIGHT);
                OPENGL_LOADED = true;
              }
              currentLevel = index + 1;
              game->loadLevel(currentLevel);
            }
          }
          else if(context == CONTEXT_GAME) {
            game->setOldPosition(x, y);
          }
        }
      }
    }

    /* Actual rendering happens here. */
    if (context == CONTEXT_MAIN_MENU) {
      gameMenu->renderMainMenu();
    }
    else if (context == CONTEXT_GAME) {
      game->renderScene();
    }
    else if (context == CONTEXT_GAME_WON) {
      SDL_Log("Congratulations, you've won the game!");
      game->renderGameFinished();
      SDL_Delay(2 * GAME_SPLASH_TIMEOUT);
      quit = true;
      break;
    }
    /* Actual rendering ends here. */
  }
}

void Gui::renderSplashScreen(const char* path, unsigned timeout) const {
  SDL_Texture* splashTexture = loadTexture(windowRenderer, path);
  SDL_SetRenderDrawColor(windowRenderer, WINDOW_CLEAR_COLOR.r, WINDOW_CLEAR_COLOR.r, WINDOW_CLEAR_COLOR.b, WINDOW_CLEAR_COLOR.a);
  SDL_RenderClear(windowRenderer);
  SDL_RenderCopy(windowRenderer, splashTexture, NULL, NULL);
  SDL_RenderPresent(windowRenderer);
  SDL_Delay(timeout);
  SDL_DestroyTexture(splashTexture);
}

void Gui::renderSingleText(const char* text, unsigned timeout, SDL_Color color) const {
  SDL_Surface* surface = TTF_RenderText_Solid(windowFont, text, color);
  SDL_Texture* texture = SDL_CreateTextureFromSurface(windowRenderer, surface);
  SDL_Rect rect = {(SCREEN_WIDTH - std::min(surface->w, SCREEN_WIDTH))/2, (SCREEN_HEIGHT - surface->h)/2, std::min(surface->w, SCREEN_WIDTH), surface->h};

  SDL_SetRenderDrawColor(windowRenderer, WINDOW_CLEAR_COLOR.r, WINDOW_CLEAR_COLOR.r, WINDOW_CLEAR_COLOR.b, WINDOW_CLEAR_COLOR.a);
  SDL_RenderClear(windowRenderer);
  SDL_RenderCopy(windowRenderer, texture, NULL, &rect);
  SDL_RenderPresent(windowRenderer);
  SDL_Delay(timeout);

  SDL_DestroyTexture(texture);
  SDL_FreeSurface(surface);
}
