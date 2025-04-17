#include <librealsense2/rs.hpp>
#include <opencv2/opencv.hpp>
#include <iostream>

class RealSenseCamera {
private:
    rs2::pipeline pipe;

public:
    RealSenseCamera() {
        pipe.start();
    }

    rs2::frameset getFrames() {
        return pipe.wait_for_frames();
    }

    rs2::video_frame getColorFrame(const rs2::frameset& frames) {
        return frames.get_color_frame();
    }

    rs2::depth_frame getDepthFrame(const rs2::frameset& frames) {
        return frames.get_depth_frame();
    }
};

class ImageProcessor {
public:
    static cv::Mat convertToHSV(const cv::Mat& color_bgr) {
        cv::Mat hsv;
        cv::cvtColor(color_bgr, hsv, cv::COLOR_BGR2HSV);
        return hsv;
    }

    static cv::Mat createMask(const cv::Mat& hsv, const cv::Scalar& lower, const cv::Scalar& upper) {
        cv::Mat mask;
        cv::inRange(hsv, lower, upper, mask);
        return mask;
    }

    static void displayDepth(cv::Mat& frame, const rs2::depth_frame& depth_frame, int cx, int cy) {
        if (cx >= 0 && cx < depth_frame.get_width() && cy >= 0 && cy < depth_frame.get_height()) {
            float depth_value = depth_frame.get_distance(cx, cy);
            char depth_text[50];
            snprintf(depth_text, sizeof(depth_text), "Depth: %.2f m", depth_value);
            cv::putText(frame, depth_text, cv::Point(cx, cy - 10), cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(255, 255, 255), 2);
        }
    }

    static void drawCenterPoint(cv::Mat& frame, int cx, int cy) {
        cv::circle(frame, cv::Point(cx, cy), 5, cv::Scalar(255, 0, 0), -1);
    }

    static void resizeFrame(const cv::Mat& input, cv::Mat& output, const cv::Size& size) {
        cv::resize(input, output, size);
    }

    static cv::Point detectAndDrawObjects(cv::Mat& frame, const cv::Mat& mask, const cv::Scalar& boxColor = cv::Scalar(0, 255, 0)) {
        std::vector<std::vector<cv::Point>> contours;
        std::vector<cv::Vec4i> hierarchy;
        cv::findContours(mask, contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

        cv::Point objectCenter(-1, -1);
        double maxArea = 0.0;
        int maxIndex = -1;

        for (size_t i = 0; i < contours.size(); i++) {
            double area = cv::contourArea(contours[i]);
            if (area > 500 && area > maxArea) {
                maxArea = area;
                maxIndex = static_cast<int>(i);
            }
        }

        if (maxIndex >= 0) {
            cv::Rect boundingBox = cv::boundingRect(contours[maxIndex]);
            objectCenter = cv::Point(boundingBox.x + boundingBox.width / 2, boundingBox.y + boundingBox.height / 2);
            cv::rectangle(frame, boundingBox, boxColor, 2);
            cv::circle(frame, objectCenter, 5, cv::Scalar(0, 0, 255), -1);
            cv::line(frame, objectCenter, cv::Point(objectCenter.x, frame.rows - 1), cv::Scalar(0, 0, 255), 2);
        }

        return objectCenter;
    }

    static void detectAllObjects(cv::Mat& frame, const cv::Mat& mask, const cv::Scalar& boxColor = cv::Scalar(255, 0, 255)) {
        std::vector<std::vector<cv::Point>> contours;
        std::vector<cv::Vec4i> hierarchy;
        cv::findContours(mask, contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

        for (const auto& contour : contours) {
            double area = cv::contourArea(contour);
            if (area > 500) {
                cv::Rect boundingBox = cv::boundingRect(contour);
                cv::rectangle(frame, boundingBox, boxColor, 2);
            }
        }
    }
};

class ColorSegmenter {
private:
    int h_min_orange, s_min_orange, v_min_orange;
    int h_max_orange, s_max_orange, v_max_orange;
    int h_min_black, s_min_black, v_min_black;
    int h_max_black, s_max_black, v_max_black;
    RealSenseCamera camera;
    ImageProcessor processor;

public:
    ColorSegmenter()
        : h_min_orange(0), s_min_orange(100), v_min_orange(100),
          h_max_orange(25), s_max_orange(255), v_max_orange(255),
          h_min_black(0), s_min_black(0), v_min_black(0),
          h_max_black(180), s_max_black(255), v_max_black(50) {}

    void setupTrackbars() {
        cv::namedWindow("Trackbars", cv::WINDOW_AUTOSIZE);
        cv::createTrackbar("H Min Orange", "Trackbars", &h_min_orange, 179);
        cv::createTrackbar("H Max Orange", "Trackbars", &h_max_orange, 179);
        cv::createTrackbar("S Min Orange", "Trackbars", &s_min_orange, 255);
        cv::createTrackbar("S Max Orange", "Trackbars", &s_max_orange, 255);
        cv::createTrackbar("V Min Orange", "Trackbars", &v_min_orange, 255);
        cv::createTrackbar("V Max Orange", "Trackbars", &v_max_orange, 255);

        cv::createTrackbar("H Min Black", "Trackbars", &h_min_black, 179);
        cv::createTrackbar("H Max Black", "Trackbars", &h_max_black, 179);
        cv::createTrackbar("S Min Black", "Trackbars", &s_min_black, 255);
        cv::createTrackbar("S Max Black", "Trackbars", &s_max_black, 255);
        cv::createTrackbar("V Min Black", "Trackbars", &v_min_black, 255);
        cv::createTrackbar("V Max Black", "Trackbars", &v_max_black, 255);
    }

    void processFrames() {
        while (cv::waitKey(1) != 27) {
            rs2::frameset frames = camera.getFrames();
            rs2::video_frame color_frame = camera.getColorFrame(frames);
            rs2::depth_frame depth_frame = camera.getDepthFrame(frames);

            int w = color_frame.get_width();
            int h = color_frame.get_height();

            cv::Mat color_rgb(cv::Size(w, h), CV_8UC3, (void*)color_frame.get_data(), cv::Mat::AUTO_STEP);
            cv::Mat color_bgr;
            cv::cvtColor(color_rgb, color_bgr, cv::COLOR_RGB2BGR);

            cv::Mat hsv = processor.convertToHSV(color_bgr);
            cv::Mat orange_mask = processor.createMask(hsv, cv::Scalar(h_min_orange, s_min_orange, v_min_orange), cv::Scalar(h_max_orange, s_max_orange, v_max_orange));
            cv::Mat black_mask = processor.createMask(hsv, cv::Scalar(h_min_black, s_min_black, v_min_black), cv::Scalar(h_max_black, s_max_black, v_max_black));

            cv::Point orangeCenter = processor.detectAndDrawObjects(color_bgr, orange_mask, cv::Scalar(0, 255, 0));
            processor.detectAllObjects(color_bgr, black_mask, cv::Scalar(255, 0, 255));

            if (orangeCenter.x >= 0 && orangeCenter.y >= 0 &&
                orangeCenter.x < w && orangeCenter.y < h) {
                processor.displayDepth(color_bgr, depth_frame, orangeCenter.x, orangeCenter.y);
            }

            cv::Mat resized_color_bgr, resized_combined_mask;
            cv::Mat combined_mask;
            cv::bitwise_or(orange_mask, black_mask, combined_mask);

            processor.resizeFrame(color_bgr, resized_color_bgr, cv::Size(320, 240));
            processor.resizeFrame(combined_mask, resized_combined_mask, cv::Size(320, 240));

            cv::imshow("RealSense View", resized_color_bgr);
            cv::imshow("Segment View", resized_combined_mask);
        }
    }
};

int main() {
    ColorSegmenter segmenter;
    segmenter.setupTrackbars();
    segmenter.processFrames();
    return 0;
}