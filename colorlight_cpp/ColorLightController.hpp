#ifndef COLORLIGHT_CONTROLLER_HPP
#define COLORLIGHT_CONTROLLER_HPP

#include <string>
#include <vector>
#include <cstdint>
#include <net/if.h>
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <opencv2/opencv.hpp>

class ColorLightController {
public:
    ColorLightController(const std::string& interface, int width = 320, int height = 240, const std::string& color_order = "BGR");
    ~ColorLightController();

    void setBrightness(int brightness);
    void setGamma(float gamma);
    void setFirmwareVersion(int version);

    void outputFrame(const cv::Mat& image);
    void detectAndPrintConfig();

private:
    void updateGammaTable();
    std::vector<uint8_t> convertColorOrder(uint8_t r, uint8_t g, uint8_t b);
    void buildEtherHeader(uint8_t packet_type, uint8_t first_byte, uint8_t* header);
    
    void sendBrightness();
    void sendPixelData(const cv::Mat& image);
    void sendSync();

    std::string interface_;
    int width_;
    int height_;
    std::string color_order_;
    int brightness_ = 100;
    float gamma_ = 2.2f;
    int firmware_version_ = 0;
    uint8_t gamma_table_[256];
    int last_brightness_ = -1;

    int sock_ = -1;
    struct sockaddr_ll socket_address_;

    const uint8_t CL_SYNC_PACKET_TYPE = 0x01;
    const uint8_t CL_BRIG_PACKET_TYPE = 0x0A;
    const uint8_t CL_PIXL_PACKET_TYPE = 0x55;
    const int CL_MAX_PIXL_PER_PACKET = 400;
    const uint8_t CL_DEST_MAC[6] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
    const uint8_t CL_SRC_MAC[6] = {0x21, 0x22, 0x33, 0x44, 0x55, 0x66};
};

#endif // COLORLIGHT_CONTROLLER_HPP
