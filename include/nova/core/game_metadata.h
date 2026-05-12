#pragma once

#include <string>
#include <optional>

namespace nova {

struct GameMetadata {
    std::string name;
    std::string title;
    std::string version;
    std::string author;
    
    std::string entry_scene;
    std::string scripts_path = "scripts";
    std::string assets_path = "assets";
    std::string base_bg_path = "bg/";
    std::string base_sprite_path = "sprites/";
    std::string base_audio_path = "audio/";
    
    std::string default_font = "sans-serif";
    int default_font_size = 24;
    int default_text_speed = 50;
    
    bool valid = false;
    
    static GameMetadata from_front_matter(const std::string& yaml_content);
    static GameMetadata from_project_file(const std::string& yaml_content);
    std::string to_yaml() const;
};

} // namespace nova
