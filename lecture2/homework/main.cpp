#include "io/camera.hpp"
#include "tasks/yolo.hpp"
#include "opencv2/opencv.hpp"
#include "tools/img_tools.hpp"



int main()
{
    // 初始化相机、yolo类
    try {
        std::cout << "开始初始化相机..." << std::endl;
        Camera cam; // 如果失败，cam的构造函数会抛出异常，跳到 catch
         auto_aim::YOLO yolo("./configs/yolo.yaml");  // 加载 assets/yolov5.xml，失败会抛异常

         cv::Mat img;
         int frame_count = 0;
         while (1) {
            // 调用相机读取图像
             if (!cam.read(img)) {
                 std::cerr << "读取图像失败！" << std::endl;
                 continue;
             }

             // 调用yolo识别装甲板
             std::list<auto_aim::Armor> armors = yolo.detect(img, frame_count++);

            for (const auto& armor : armors) {

                 // 绘制装甲板边界框
                  tools::draw_points(img, armor.points, cv::Scalar(0, 255, 0), 2);
                 // 绘制装甲板关键点
                 for (const auto& point : armor.points) {
                     cv::circle(img, point, 3, cv::Scalar(0, 0, 255), -1);
                 }
             }


             // 显示图像
            cv::Mat display;
            cv::resize(img, display, cv::Size(640, 480));
            cv::imshow("img", display);


             // 按键检测，ESC 退出
    int key = cv::waitKey(1);
    if (key == 'q' || key == 27) break;
    }
  } catch (const std::exception & e) {
    std::cerr << "初始化失败: " << e.what() << std::endl;
    return -1;
  }

  std::cout << "程序正常退出" << std::endl;
    return 0;
}
