#include <iostream>
#include <list>
#include <string>

#include "io/camera.hpp"
#include "opencv2/opencv.hpp"
#include "tasks/yolo.hpp"
#include "tools/img_tools.hpp"

int main()
{
  // 初始化相机、yolo类
  try {
    std::cout << "开始初始化相机..." << std::endl;
    Camera cam;
    auto_aim::YOLO yolo("./configs/yolo.yaml");

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

      for (const auto & armor : armors) {
        // 四个关键点连成绿色闭合矩形
        tools::draw_points(img, armor.points, cv::Scalar(0, 255, 0), 2);

        // 颜色 编号
        const std::string label =
          auto_aim::COLORS[armor.color] + auto_aim::ARMOR_NAMES[armor.name];

         auto a = armor.points[0], b = armor.points[0];
        for (const auto & p : armor.points) {
          if (p.y < a.y) { b = a; a = p; } else if (p.y < b.y) { b = p; }
        }
        cv::Point2f top_point = (a.x < b.x) ? a : b;
        
        // 在装甲板上方显示颜色和编号
        tools::draw_text(
          img, label, {static_cast<int>(top_point.x), static_cast<int>(top_point.y) - 8},
          cv::Scalar(0, 0, 255), 2.0, 3);
      }

      // 显示图像
      cv::Mat display;
      cv::resize(img, display, cv::Size(640, 480));
      cv::imshow("img", display);

      // 按q or Esc退出
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
