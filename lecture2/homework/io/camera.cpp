#include "camera.hpp"

#include <stdexcept>
#include <string>

Camera::Camera() : handle_(nullptr), is_grabbing_(false)
{
  // 打开相机
  MV_CC_DEVICE_INFO_LIST device_list = {0};
  int ret = MV_CC_EnumDevices(MV_USB_DEVICE, &device_list);
  if (ret != MV_OK) {
    throw std::runtime_error("相机枚举失败，错误码: " + std::to_string(ret));
  }
  if (device_list.nDeviceNum == 0) {
    throw std::runtime_error("未找到相机设备");
  }

  ret = MV_CC_CreateHandle(&handle_, device_list.pDeviceInfo[0]);
  if (ret != MV_OK) {
    handle_ = nullptr;
    throw std::runtime_error("创建相机句柄失败，错误码: " + std::to_string(ret));
  }

  ret = MV_CC_OpenDevice(handle_);
  if (ret != MV_OK) {
    MV_CC_DestroyHandle(handle_);
    handle_ = nullptr;
    throw std::runtime_error("打开相机失败，错误码: " + std::to_string(ret));
  }

  if (!setDefaultParams_()) {
    MV_CC_CloseDevice(handle_);
    MV_CC_DestroyHandle(handle_);
    handle_ = nullptr;
    throw std::runtime_error("设置相机默认参数失败");
  }

  ret = MV_CC_StartGrabbing(handle_);
  if (ret != MV_OK) {
    MV_CC_CloseDevice(handle_);
    MV_CC_DestroyHandle(handle_);
    handle_ = nullptr;
    throw std::runtime_error("开始取流失败，错误码: " + std::to_string(ret));
  }
  is_grabbing_ = true;
}

Camera::~Camera()
{
  if (is_grabbing_) {
    MV_CC_StopGrabbing(handle_);
  }
  if (handle_) {
    MV_CC_CloseDevice(handle_);
    MV_CC_DestroyHandle(handle_);
  }
}

bool Camera::read(cv::Mat & img)
{
  if (!handle_ || !is_grabbing_) {
    return false;
  }

  MV_FRAME_OUT raw = {0};
  int ret = MV_CC_GetImageBuffer(handle_, &raw, 100);
  if (ret != MV_OK) {
    return false;  // 读取图像失败
  }

  try {
    img = convertFrame_(raw);
  } catch (const std::exception & e) {
    std::cerr << "图像转换失败: " << e.what() << std::endl;
    MV_CC_FreeImageBuffer(handle_, &raw);  // 释放图像缓冲
    return false;
  }

  MV_CC_FreeImageBuffer(handle_, &raw);  // 释放图像缓冲区
  return !img.empty();
}

bool Camera::setDefaultParams_()
{
  MV_CC_SetEnumValue(handle_, "BalanceWhiteAuto", MV_BALANCEWHITE_AUTO_CONTINUOUS);
  MV_CC_SetEnumValue(handle_, "ExposureAuto", MV_EXPOSURE_AUTO_MODE_OFF);
  MV_CC_SetEnumValue(handle_, "GainAuto", MV_GAIN_MODE_OFF);
  MV_CC_SetFloatValue(handle_, "ExposureTime", 5000.0f);  // 设置曝光时间为5ms
  MV_CC_SetFloatValue(handle_, "Gain", 16.9f);            // 设置增益为16.9
  MV_CC_SetFrameRate(handle_, 60.0f);

  return true;
}

cv::Mat Camera::convertFrame_(MV_FRAME_OUT & raw)
{
  cv::Mat img(cv::Size(raw.stFrameInfo.nWidth, raw.stFrameInfo.nHeight), CV_8U, raw.pBufAddr);
  cv::Mat out_img;
  auto pixel_type = raw.stFrameInfo.enPixelType;

  const static std::unordered_map<MvGvspPixelType, cv::ColorConversionCodes> type_map = {
    {PixelType_Gvsp_BayerGR8, cv::COLOR_BayerGR2BGR},
    {PixelType_Gvsp_BayerRG8, cv::COLOR_BayerRG2BGR},
    {PixelType_Gvsp_BayerGB8, cv::COLOR_BayerGB2BGR},
    {PixelType_Gvsp_BayerBG8, cv::COLOR_BayerBG2BGR}};

  auto it = type_map.find(pixel_type);
  if (it != type_map.end()) {
    cv::cvtColor(img, out_img, it->second);
  } else {
    out_img = img.clone();
  }
  return out_img;
}