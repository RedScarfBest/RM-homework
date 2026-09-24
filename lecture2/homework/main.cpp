#include "io/camera.hpp"
#include "tasks/yolo.hpp"
#include "opencv2/opencv.hpp"
#include "tools/img_tools.hpp"

#include <chrono>//

int main()
{
    // 初始化相机、yolo类
    
    // while (1) {
        // 调用相机读取图像


        // 调用yolo识别装甲板



        // 显示图像
        // cv::resize(img, img , cv::Size(640, 480));
        // cv::imshow("img", img);
        // if (cv::waitKey(0) == 'q') {
        //     // break;
        // }
    // }
try {
        std::cout << "开始初始化相机..." << std::endl;
        Camera cam; // 如果失败，这里会抛出异常，跳到 catch
        std::cout << "相机初始化成功！开始读取图像，按 ESC 退出。" << std::endl;

        cv::Mat frame;
        int frame_count = 0;
        auto start_time = std::chrono::high_resolution_clock::now();

        while (true) {
            // 读取一帧
            if (cam.read(frame)) {
                // 显示图像
                cv::imshow("Camera Test", frame);
                frame_count++;

                // 每 30 帧计算一次帧率
                if (frame_count % 30 == 0) {
                    auto end_time = std::chrono::high_resolution_clock::now();
                    double elapsed = std::chrono::duration<double>(end_time - start_time).count();
                    std::cout << "当前帧率: " << 30.0 / elapsed << " FPS" << std::endl;
                    start_time = end_time;
                }
            } else {
                std::cerr << "读取图像失败！" << std::endl;
            }

            // 按键检测，ESC 退出（ASCII码27）
            if (cv::waitKey(1) == 27) {
                break;
            }
        }
    } 
    catch (const std::exception& e) {
        // 捕获构造函数中抛出的异常
        std::cerr << "相机发生异常: " << e.what() << std::endl;
        return -1;
    }

    std::cout << "程序正常退出，相机资源即将释放..." << std::endl;
    return 0;
}