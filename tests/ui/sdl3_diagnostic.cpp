#include <SDL3/SDL.h>
#include <stdio.h>

int main(int argc, char** argv) {
    printf("=== SDL3 Diagnostic Tool ===\n\n");
    
    printf("Step 1: Calling SDL_Init...\n");
    bool result = SDL_Init(SDL_INIT_VIDEO);
    
    if (result) {
        printf("SUCCESS: SDL_Init returned true\n");
        
        // 获取视频驱动信息
        const char* driver = SDL_GetCurrentVideoDriver();
        printf("Video driver: %s\n", driver ? driver : "(null)");
        
        // 列出所有显示器
        int display_count = 0;
        SDL_DisplayID* displays = SDL_GetDisplays(&display_count);
        if (displays) {
            printf("Found %d display(s):\n", display_count);
            for (int i = 0; i < display_count; i++) {
                const char* name = SDL_GetDisplayName(displays[i]);
                printf("  Display %d: %s\n", i, name ? name : "(null)");
            }
            SDL_free(displays);
        }
        
        SDL_Quit();
        printf("\nSDL3 is working correctly!\n");
        return 0;
    } else {
        printf("FAILED: SDL_Init returned false\n");
        
        const char* error = SDL_GetError();
        printf("SDL_GetError() returned: %s\n", error ? error : "(empty string)");
        
        printf("\nPossible causes:\n");
        printf("1. Missing Windows runtime dependencies\n");
        printf("2. Static library linking issue\n");
        printf("3. Platform-specific initialization failure\n");
        
        return 1;
    }
}
