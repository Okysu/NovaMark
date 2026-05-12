#include "nova/core/game_metadata.h"
#include <sstream>
#include <algorithm>

namespace nova {

static std::string trim(std::string s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

static std::string unquote(const std::string& s) {
    if (s.size() >= 2 && s.front() == '"' && s.back() == '"') {
        return s.substr(1, s.size() - 2);
    }
    if (s.size() >= 2 && s.front() == '\'' && s.back() == '\'') {
        return s.substr(1, s.size() - 2);
    }
    return s;
}

GameMetadata GameMetadata::from_front_matter(const std::string& yaml_content) {
    GameMetadata meta;
    meta.valid = true;
    
    std::istringstream stream(yaml_content);
    std::string line;
    
    while (std::getline(stream, line)) {
        size_t colon = line.find(':');
        if (colon == std::string::npos) continue;
        
        std::string key = trim(line.substr(0, colon));
        std::string value = unquote(trim(line.substr(colon + 1)));
        
        if (key == "title") meta.title = value;
        else if (key == "name") meta.name = value;
        else if (key == "version") meta.version = value;
        else if (key == "author") meta.author = value;
        else if (key == "entry_scene") meta.entry_scene = value;
        else if (key == "default_font") meta.default_font = value;
        else if (key == "default_font_size") meta.default_font_size = std::stoi(value);
        else if (key == "default_text_speed") meta.default_text_speed = std::stoi(value);
        else if (key == "base_bg_path") meta.base_bg_path = value;
        else if (key == "base_sprite_path") meta.base_sprite_path = value;
        else if (key == "base_audio_path") meta.base_audio_path = value;
    }
    
    if (meta.name.empty() && !meta.title.empty()) {
        meta.name = meta.title;
    }
    
    return meta;
}

GameMetadata GameMetadata::from_project_file(const std::string& yaml_content) {
    GameMetadata meta;
    meta.valid = true;
    
    std::istringstream stream(yaml_content);
    std::string line;
    
    while (std::getline(stream, line)) {
        size_t colon = line.find(':');
        if (colon == std::string::npos) continue;
        
        std::string key = trim(line.substr(0, colon));
        std::string value = unquote(trim(line.substr(colon + 1)));
        
        if (key == "name") meta.name = value;
        else if (key == "title") meta.title = value;
        else if (key == "version") meta.version = value;
        else if (key == "author") meta.author = value;
        else if (key == "entry_scene") meta.entry_scene = value;
        else if (key == "scripts_path") meta.scripts_path = value;
        else if (key == "assets_path") meta.assets_path = value;
        else if (key == "base_bg_path") meta.base_bg_path = value;
        else if (key == "base_sprite_path") meta.base_sprite_path = value;
        else if (key == "base_audio_path") meta.base_audio_path = value;
        else if (key == "default_font") meta.default_font = value;
        else if (key == "default_font_size") meta.default_font_size = std::stoi(value);
        else if (key == "default_text_speed") meta.default_text_speed = std::stoi(value);
    }
    
    if (meta.title.empty() && !meta.name.empty()) {
        meta.title = meta.name;
    }
    
    return meta;
}

std::string GameMetadata::to_yaml() const {
    std::ostringstream out;
    out << "name: " << name << "\n";
    if (!title.empty() && title != name) {
        out << "title: " << title << "\n";
    }
    if (!version.empty()) {
        out << "version: " << version << "\n";
    }
    if (!author.empty()) {
        out << "author: " << author << "\n";
    }
    if (!entry_scene.empty()) {
        out << "entry_scene: " << entry_scene << "\n";
    }
    out << "scripts_path: " << scripts_path << "\n";
    out << "assets_path: " << assets_path << "\n";
    out << "base_bg_path: " << base_bg_path << "\n";
    out << "base_sprite_path: " << base_sprite_path << "\n";
    out << "base_audio_path: " << base_audio_path << "\n";
    out << "default_font: " << default_font << "\n";
    out << "default_font_size: " << default_font_size << "\n";
    out << "default_text_speed: " << default_text_speed << "\n";
    return out.str();
}

} // namespace nova
