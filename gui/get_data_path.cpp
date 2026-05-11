#include <SDL2/SDL.h>
#include <iostream>
#include <string>


std::string author_name = "Oedada";
std::string project_name = "wifisync";

int main(){
    char* char_path = SDL_GetPrefPath(author_name.c_str(), project_name.c_str());
    SDL_free(char_path);
    return 0;
}
