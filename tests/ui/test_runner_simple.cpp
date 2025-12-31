#include <SDL3/SDL.h>
#include <stdio.h>

int main(int argc, char** argv) {
    printf("=== SDL3 Simple Test ===\n");
    printf("SDL_Init() call...\n");
    
    bool result = SDL_Init(SDL_INIT_VIDEO);
    printf("SDL_Init() returned: %s\n", result ? "true (success)" : "false (failure)");
    
    if (!result) {
        const char* error = SDL_GetError();
        printf("ERROR: SDL_Init failed!\n");
        printf("SDL_GetError(): %s\n", error ? error : "(null)");
        return 1;
    }
    
    printf("SUCCESS: SDL3 initialized!\n");
    
    SDL_Quit();
    return 0;
}
