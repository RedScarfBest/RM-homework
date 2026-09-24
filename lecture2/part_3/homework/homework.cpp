#include <iostream>
#include <opencv2/opencv.hpp>

// ======================= 作业 =======================
// 1. 读取 ../assets/demo.jpg
// 2. 使用 cvtColor 把图像转为灰度图（颜色空间：BGR2GRAY）
// 3. 使用 imwrite 把灰度图保存为 gray.jpg
// 4. 在灰度图上用 circle 画一个圆，标记你要"瞄准"的位置
// 5. 显示灰度图，按任意键退出
// ====================================================

int main()
{
    std::cout << "1. 程序开始" << std::endl;
    
    // 假设这里是你的读取代码
    cv::Mat img = cv::imread("assets/demo.jpg"); // 注意路径！
    
    if (img.empty()) {
        std::cout << "2. 错误：图像读取失败！路径不对" << std::endl;
        return -1;
    }
    std::cout << "3. 图像读取成功，准备转换" << std::endl;
    
    cv::Mat gray_img;
    cv::cvtColor(img, gray_img, cv::COLOR_BGR2GRAY);
    
    cv::circle(gray_img, cv::Point(gray_img.cols / 2, gray_img.rows / 2), 50, 
    cv::Scalar(255), -1); // 在图像中心画一个半径为50的白色圆圈
    
    if(!cv::imwrite("gray.jpg", gray_img)) {
        std::cout << "imwrite失败" << std::endl;
    }
    
    std::cout << "4. 准备弹出窗口" << std::endl;
    cv::imshow("Aiming Point", gray_img);
    
    std::cout << "5. 窗口已弹出，等待按键中..." << std::endl;
    cv::waitKey(0);
    
    std::cout << "6. 按键被按下，程序结束" << std::endl;
    return 0;
}
