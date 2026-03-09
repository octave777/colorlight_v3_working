#include "ColorLightController.hpp"
#include <iostream>
#include <cmath>
#include <cstring>
#include <unistd.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>

ColorLightController::ColorLightController(const std::string& interface, int width, int height, const std::string& color_order)
    : interface_(interface), width_(width), height_(height), color_order_(color_order) {
    
    updateGammaTable();

    // Create raw socket
    sock_ = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (sock_ < 0) {
        perror("socket");
        exit(1);
    }

    // Get interface index
    struct ifreq ifr;
    std::memset(&ifr, 0, sizeof(ifr));
    std::strncpy(ifr.ifr_name, interface_.c_str(), IFNAMSIZ - 1);
    if (ioctl(sock_, SIOCGIFINDEX, &ifr) < 0) {
        perror("SIOCGIFINDEX");
        exit(1);
    }

    // Set up socket address
    std::memset(&socket_address_, 0, sizeof(socket_address_));
    socket_address_.sll_family = AF_PACKET;
    socket_address_.sll_ifindex = ifr.ifr_ifindex;
    socket_address_.sll_halen = ETH_ALEN;
    std::memcpy(socket_address_.sll_addr, CL_DEST_MAC, ETH_ALEN);
}

ColorLightController::~ColorLightController() {
    if (sock_ >= 0) {
        close(sock_);
    }
}

void ColorLightController::updateGammaTable() {
    for (int i = 0; i < 256; ++i) {
        float f = 255.0f * std::pow(i / 255.0f, gamma_);
        gamma_table_[i] = static_cast<uint8_t>(std::max(0.0f, std::min(255.0f, std::round(f))));
    }
}

void ColorLightController::setBrightness(int brightness) {
    brightness_ = std::max(0, std::min(100, brightness));
}

void ColorLightController::setGamma(float gamma) {
    gamma_ = gamma;
    updateGammaTable();
}

void ColorLightController::setFirmwareVersion(int version) {
    firmware_version_ = version;
}

std::vector<uint8_t> ColorLightController::convertColorOrder(uint8_t r, uint8_t g, uint8_t b) {
    if (color_order_ == "BGR") return {b, g, r};
    if (color_order_ == "RGB") return {r, g, b};
    if (color_order_ == "GRB") return {g, r, b};
    if (color_order_ == "GBR") return {g, b, r};
    if (color_order_ == "RBG") return {r, b, g};
    if (color_order_ == "BRG") return {b, r, g};
    return {r, g, b};
}

void ColorLightController::buildEtherHeader(uint8_t packet_type, uint8_t first_byte, uint8_t* header) {
    std::memcpy(header, CL_DEST_MAC, 6);
    std::memcpy(header + 6, CL_SRC_MAC, 6);
    header[12] = packet_type;
    header[13] = first_byte;
}

void ColorLightController::sendBrightness() {
    uint8_t b_val = static_cast<uint8_t>(2.55 * brightness_);
    uint8_t buffer[14 + 64];
    buildEtherHeader(CL_BRIG_PACKET_TYPE, b_val, buffer);
    
    std::memset(buffer + 14, 0, 64);
    buffer[14] = b_val;
    buffer[15] = b_val;
    buffer[16] = 0xFF;

    sendto(sock_, buffer, sizeof(buffer), 0, (struct sockaddr*)&socket_address_, sizeof(socket_address_));
    if (firmware_version_ >= 13) {
        sendto(sock_, buffer, sizeof(buffer), 0, (struct sockaddr*)&socket_address_, sizeof(socket_address_));
    }
}

void ColorLightController::sendPixelData(const cv::Mat& image) {
    for (int y = 0; y < height_; ++y) {
        for (int x_offset = 0; x_offset < width_; x_offset += CL_MAX_PIXL_PER_PACKET) {
            int pixel_count = std::min(CL_MAX_PIXL_PER_PACKET, width_ - x_offset);
            
            uint8_t row_msb = (y >> 8) & 0xFF;
            uint8_t row_lsb = y & 0xFF;
            uint8_t off_msb = (x_offset >> 8) & 0xFF;
            uint8_t off_lsb = x_offset & 0xFF;
            uint8_t cnt_msb = (pixel_count >> 8) & 0xFF;
            uint8_t cnt_lsb = pixel_count & 0xFF;

            std::vector<uint8_t> payload;
            payload.push_back(row_lsb);
            payload.push_back(off_msb);
            payload.push_back(off_lsb);
            payload.push_back(cnt_msb);
            payload.push_back(cnt_lsb);
            payload.push_back(0x08);
            payload.push_back(0x00);

            for (int x = 0; x < pixel_count; ++x) {
                cv::Vec3b p;
                if (y < image.rows && (x_offset + x) < image.cols) {
                    p = image.at<cv::Vec3b>(y, x_offset + x);
                } else {
                    p = cv::Vec3b(0, 0, 0);
                }
                
                uint8_t r = gamma_table_[p[2]]; // Mat is BGR by default
                uint8_t g = gamma_table_[p[1]];
                uint8_t b = gamma_table_[p[0]];
                
                std::vector<uint8_t> colors = convertColorOrder(r, g, b);
                payload.insert(payload.end(), colors.begin(), colors.end());
            }

            uint8_t buffer[14 + payload.size()];
            buildEtherHeader(CL_PIXL_PACKET_TYPE, row_msb, buffer);
            std::memcpy(buffer + 14, payload.data(), payload.size());

            sendto(sock_, buffer, sizeof(buffer), 0, (struct sockaddr*)&socket_address_, sizeof(socket_address_));
            if (firmware_version_ >= 13) {
                 sendto(sock_, buffer, sizeof(buffer), 0, (struct sockaddr*)&socket_address_, sizeof(socket_address_));
            }
        }
    }
}

void ColorLightController::sendSync() {
    uint8_t buffer[14 + 98];
    buildEtherHeader(CL_SYNC_PACKET_TYPE, 0xFF, buffer);
    uint8_t* payload = buffer + 14;
    std::memset(payload, 0, 98);
    payload[1] = 0xFF; payload[2] = 0xFF; payload[3] = 0xFF; payload[4] = 0xFF;
    payload[12] = 0x01;
    payload[17] = 0x01;
    payload[24] = 0xFF; payload[25] = 0xFF; payload[26] = 0xFD;
    
    uint8_t b_val = static_cast<uint8_t>(2.55 * brightness_);
    payload[31] = b_val;

    sendto(sock_, buffer, sizeof(buffer), 0, (struct sockaddr*)&socket_address_, sizeof(socket_address_));
    if (firmware_version_ >= 13) {
        sendto(sock_, buffer, sizeof(buffer), 0, (struct sockaddr*)&socket_address_, sizeof(socket_address_));
    }
}

void ColorLightController::outputFrame(const cv::Mat& image) {
    if (brightness_ != last_brightness_) {
        sendBrightness();
        last_brightness_ = brightness_;
    }
    sendPixelData(image);
    sendSync();
}

void ColorLightController::detectAndPrintConfig() {
    std::cout << "--- Colorlight 카드 감지 중 (인터페이스: " << interface_ << ") ---" << std::endl;
    // ... Simplified or omitted for now as it needs sniff logic which is complex in C++ without libpcap
    std::cout << "카드 감지 기능은 C++ 버전에서 간략화되었습니다." << std::endl;
}
