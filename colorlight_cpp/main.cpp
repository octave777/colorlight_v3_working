#include "ColorLightController.hpp"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <unistd.h>
#include <opencv2/opencv.hpp>
#include <ft2build.h>
#include FT_FREETYPE_H

struct Config {
    std::string interface = "enp0s6";
    int width = 320;
    int height = 240;
    std::string font_path = "/usr/share/fonts/truetype/nanum/NanumGothic.ttf";
    int font_size = 150;
    cv::Scalar text_color = cv::Scalar(255, 0, 0); // BGR
    cv::Scalar bg_color = cv::Scalar(0, 0, 0);       // BGR
    std::string color_order = "BGR";
    std::string initial_text = "Hello!";
};

Config loadConfig(const std::string& filename) {
    Config config;
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "설정 파일(" << filename << ")을 열 수 없어 기본값을 사용합니다." << std::endl;
        return config;
    }

    std::string line;
    while (std::getline(file, line)) {
        size_t sep = line.find('=');
        if (sep == std::string::npos) continue;
        std::string key = line.substr(0, sep);
        std::string val = line.substr(sep + 1);

        if (key == "interface") config.interface = val;
        else if (key == "width") config.width = std::stoi(val);
        else if (key == "height") config.height = std::stoi(val);
        else if (key == "font_path") config.font_path = val;
        else if (key == "font_size") config.font_size = std::stoi(val);
        else if (key == "color_order") config.color_order = val;
        else if (key == "initial_text") config.initial_text = val;
        else if (key == "text_color" || key == "bg_color") {
            int r, g, b;
            if (sscanf(val.c_str(), "%d,%d,%d", &r, &g, &b) == 3) {
                if (key == "text_color") config.text_color = cv::Scalar(b, g, r); // BGR
                else config.bg_color = cv::Scalar(b, g, r);
            }
        }
    }
    return config;
}

cv::Mat createTextImage(const std::string& text, int width, int height, int font_size, cv::Scalar text_color, cv::Scalar bg_color, const std::string& font_path) {
    cv::Mat img(height, width, CV_8UC3, bg_color);
    
    FT_Library ft;
    if (FT_Init_FreeType(&ft)) {
        std::cerr << "FreeType 초기화 실패" << std::endl;
        return img;
    }

    FT_Face face;
    if (FT_New_Face(ft, font_path.c_str(), 0, &face)) {
        std::cerr << "폰트 로드 실패: " << font_path << std::endl;
        FT_Done_FreeType(ft);
        return img;
    }

    FT_Set_Pixel_Sizes(face, 0, font_size);

    // 텍스트 전체 너비 계산
    int total_width = 0;
    for (char c : text) {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) continue;
        total_width += face->glyph->advance.x >> 6;
    }

    // 중앙 정렬을 위한 시작 위치
    int x = (width - total_width) / 2;
    int y = (height + font_size / 2) / 2; // 대략적인 세로 중앙

    for (char c : text) {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) continue;
        
        FT_GlyphSlot slot = face->glyph;
        int gx = x + slot->bitmap_left;
        int gy = y - slot->bitmap_top;

        for (int r = 0; r < slot->bitmap.rows; ++r) {
            for (int c_idx = 0; c_idx < slot->bitmap.width; ++c_idx) {
                int py = gy + r;
                int px = gx + c_idx;
                if (px >= 0 && px < width && py >= 0 && py < height) {
                    uint8_t alpha = slot->bitmap.buffer[r * slot->bitmap.width + c_idx];
                    if (alpha > 0) {
                        cv::Vec3b& pixel = img.at<cv::Vec3b>(py, px);
                        for (int k = 0; k < 3; ++k) {
                            pixel[k] = (uint8_t)((pixel[k] * (255 - alpha) + text_color[k] * alpha) / 255);
                        }
                    }
                }
            }
        }
        x += slot->advance.x >> 6;
    }

    FT_Done_Face(face);
    FT_Done_FreeType(ft);
    return img;
}

int main(int argc, char** argv) {
    Config config = loadConfig("config.txt");
    
    // 명령줄 인자가 있으면 덮어쓰기
    if (argc > 1) config.interface = argv[1];
    if (argc > 2) config.initial_text = argv[2];

    ColorLightController controller(config.interface, config.width, config.height, config.color_order);
    
    std::cout << "--- ColorLight v2 C++ 설정 (from config.txt) ---" << std::endl;
    std::cout << "인터페이스: " << config.interface << std::endl;
    std::cout << "해상도: " << config.width << "x" << config.height << std::endl;
    std::cout << "폰트: " << config.font_path << std::endl;
    std::cout << "초기 텍스트: '" << config.initial_text << "'" << std::endl;
    std::cout << "-----------------------------------------------" << std::endl;

    cv::Mat img = createTextImage(config.initial_text, config.width, config.height, config.font_size, config.text_color, config.bg_color, config.font_path);
    controller.outputFrame(img);
    std::cout << "초기 화면 전송 완료." << std::endl;

    std::string current_text = config.initial_text;
    std::string buffer = "";

    while (true) {
        std::cout << "\n현재 표시 중: '" << current_text << "'" << std::endl;
        std::cout << "출력할 새로운 텍스트 입력: ";
        std::string new_text;
        if (!std::getline(std::cin, new_text)) break;

        if (!new_text.empty()) {
            buffer = new_text;
            
            std::string display_text;
            if (buffer.length() > 3) {
                display_text = buffer.substr(buffer.length() - 3);
            } else {
                display_text = buffer;
            }

            if (buffer.find('+') != std::string::npos) {
                display_text = " ";
            }

            current_text = display_text;
            cv::Mat new_img = createTextImage(current_text, config.width, config.height, config.font_size, config.text_color, config.bg_color, config.font_path);
            controller.outputFrame(new_img);
            std::cout << "텍스트가 '" << current_text << "'(으)로 갱신 및 전송되었습니다." << std::endl;
        } else {
            std::cout << "입력이 없어 이전 상태를 유지합니다." << std::endl;
        }
    }

    return 0;
}
