// Copyright (c) 2011, Marin Saric <marin.saric@gmail.com>
// All rights reserved.
//
// This file is a part of LibHand. LibHand is open-source software. You can
// redistribute it and/or modify it under the terms of the LibHand
// license. The LibHand license is the BSD license with an added clause that
// requires academic citation. You should have received a copy of the
// LibHand license (the license.txt file) along with LibHand. If not, see
// <http://www.libhand.org/>

// PoseDesigner

# include "pose_designer.h"

# include <iostream>
# include <stdexcept>
# include <string>

# include "boost/math/constants/constants.hpp"
# include "opencv2/opencv.hpp"
# include "opencv2/highgui/highgui.hpp"

# include "file_dialog.h"
# include "hog_cell_rectangles.h"
# include "hog_descriptor.h"
# include "hog_utils.h"
# include "image_to_hog_calculator.h"
# include "image_utils.h"
# include "printfstring.h"
# include "simple_slider.h"
# include "text_printer.h"

//test
# include <iostream>
# include <fstream>
#include <stdlib.h>     /* srand, rand */
#include <time.h>       /* time */
#include <vector>
#include <limits>

extern int file_num;
extern bool save_all;
cv::Point topleft,bright;

namespace libhand {

PoseDesigner::PoseDesigner()
  : argc_(0),
    argv_(NULL),
    is_setup_(false),
    win_render_("Pose Designer - Main Window"),
    win_camera_("Camera Window"),
    update_slider_fn_(this, &PoseDesigner::UpdateSlider),
    cam_sketch_(kCamWinHeight, kCamWinWidth, CV_8UC3),
    render_help_(false),
    render_hog_(false),
    warning_status_(0),
    warning_key_(-1),
    save_d(0),
    save_b(0),
    save_c(0),
    auto_cg(0),
    //bound_box(0),
    crop(0),
    pose_file_("") {
}

void PoseDesigner::Setup(int argc, char **argv) {
  argc_ = argc;
  argv_ = argv;

  string scene_file = GetSceneFileName();
  if (scene_file == "") {
    throw runtime_error("No scene file chosen!");
  }

  scene_spec_ = SceneSpec(scene_file);

  hand_renderer_.Setup(kRenderWinWidth, kRenderWinHeight);
  hand_renderer_.LoadScene(scene_spec_);

  hand_pose_.reset(new FullHandPose(scene_spec_.num_bones()));

  cv::namedWindow(win_render_, CV_WINDOW_AUTOSIZE);
  cvMoveWindow(win_render_.c_str(), kGUILeftX, kGUITopY);

  sl_bend_.reset(MakeAngleSlider("bend", win_render_));
  sl_side_.reset(MakeAngleSlider("side", win_render_));
  sl_twist_.reset(MakeAngleSlider("twist", win_render_));

  sl_joint_no_.reset(new SimpleSlider("joint_no", win_render_,
                                      0, hand_pose_->num_joints() - 1,
                                      hand_pose_->num_joints() - 1,
                                      &update_slider_fn_));

  cv::namedWindow(win_camera_, CV_WINDOW_AUTOSIZE);
  cvMoveWindow(win_camera_.c_str(),
               kGUILeftX + kRenderWinWidth + kGUIXSpace,
               kGUITopY);

  sl_theta_.reset(MakeAngleSlider("theta", win_camera_));
  sl_phi_.reset(MakeAngleSlider("phi", win_camera_));
  sl_tilt_.reset(MakeAngleSlider("tilt", win_camera_));
  sl_dist_mult_.reset(new SimpleSlider("distance", win_camera_,
                                       0.1, 5., 1000, &update_slider_fn_));

  ZeroBend(); ZeroSide(); ZeroTwist();
  ResetCamera();

  is_setup_ = true;
}

double randfrom(double min, double max) 
{
    //srand((unsigned) time(NULL));
    double range = (max - min); 
    double div = RAND_MAX / range;
    return min + (rand() / div);
}

void PoseDesigner::Run() {
  if (!is_setup_) {
    throw runtime_error("Setup() not called for PoseDesigner");
  }

  RenderEverything();

  for (;;) {
    char cmd = cv::waitKey();

    if (!ProcessKey(cmd)) break;
    //lx
    if (auto_cg) 
    {
     auto_cg=0;
     srand((unsigned) time(NULL));
     double current_theta=camera_spec_.theta;
     double current_phi=camera_spec_.phi;
     double current_tilt=camera_spec_.tilt;
     std::vector<double> current_bend;
     std::vector<double> current_side;
     std::vector<double> current_twist;
     current_bend.resize(hand_pose_->num_joints());
     current_side.resize(hand_pose_->num_joints());
     current_twist.resize(hand_pose_->num_joints());
     for(int ij=0; ij<hand_pose_->num_joints();ij++)
     {
       current_bend[ij]=(double)hand_pose_->bend(ij);
       current_side[ij]=(double)hand_pose_->side(ij);
       current_twist[ij]=(double)hand_pose_->twist(ij);
     }
     for(int i=0; i<1000;i++)
     {
      double random_r=randfrom(1,1.8);
      camera_spec_.r = ( hand_renderer_.initial_cam_distance()
                       * random_r );
      double random_theta=randfrom(-0.5,0.5);
      camera_spec_.theta = current_theta + random_theta;
      double random_phi=randfrom(-0.5,0.5);
      camera_spec_.phi=current_phi + random_phi;
      double random_tilt=randfrom(-0.5,0.5);
      camera_spec_.tilt=current_tilt + random_tilt;
      for (int j=0;j<hand_pose_->num_joints();j++)
      { 
        double random_bend = randfrom(-0.2,0.2);
        double random_side = randfrom(-0.07,0.07);
        double random_twist = randfrom(-0.05,0.05);
        hand_pose_->bend(j)= (float)current_bend[j]+(float)random_bend;
        hand_pose_->side(j)= (float)current_side[j]+(float)random_side;
        hand_pose_->twist(j)=(float)current_twist[j]+(float)random_twist;
      }
      //cout<< "the random r is "<< random_r <<endl;
      //cout<< "the random theta is "<< random_theta <<endl;
      //cout<< "the random phi is "<< random_phi <<endl;
      //cout<< "the random tilt is "<< random_tilt <<endl;
      if(save_all)
      {
      save_d=1;
      save_b=1;
      save_c=1;
      }
      RenderEverything();
      //cv::waitKey(100);
      file_num++;
     }

    }
    else
    {
      RenderEverything();
    }

    
  }
}

SimpleSlider *PoseDesigner::MakeAngleSlider(const string &name,
                                            const string &win_name) {
  SimpleSlider *slider = new SimpleSlider(name, win_name,
                                          -math::constants::pi<float>(),
                                          math::constants::pi<float>(),
                                          720,
                                          &update_slider_fn_);
  slider->set_val(0);
  return slider;
}

void PoseDesigner::UpdateSlider(SimpleSlider *slider) {
  // Some releases of OpenCV have a nasty habit of calling UpdateSlider
  // immediately...
  if (!is_setup_) return;

  if (slider->name() == "bend") {
    hand_pose_->bend(cur_joint()) = slider->val();
  } else if (slider->name() == "side") {
    hand_pose_->side(cur_joint()) = slider->val();
  } else if (slider->name() == "twist") {
    hand_pose_->twist(cur_joint()) = slider->val();
  } else if (slider->name() == "joint_no") {
    UpdateBSTSliders();
  } else if (slider->name() == "theta") {
    camera_spec_.theta = sl_theta_->val();
  } else if (slider->name() == "phi") {
    camera_spec_.phi = sl_phi_->val();
  } else if (slider->name() == "tilt") {
    camera_spec_.tilt = sl_tilt_->val();
  } else if (slider->name() == "distance") {
    camera_spec_.r = ( hand_renderer_.initial_cam_distance()
                       * sl_dist_mult_->val() );
  }

  hand_pose_->SetRotMatrix(camera_spec_);

  RenderEverything();
}

bool PoseDesigner::WarnKey(char cmd) {
  warning_key_ = cmd; ++warning_status_;
  return warning_status_ == 2;
}

bool PoseDesigner::ProcessKey(char cmd) {
  if (cmd == -1) return true;
  
  if (warning_status_ && warning_key_ != -1 && (cmd != warning_key_)) {
    warning_status_ = 0;
    warning_key_ = -1;
    return true;
  }

  switch(cmd) {
  case 27:
  case 'q':
    if (WarnKey(cmd)) return false;
    break;
  case 'h': render_help_ = !render_help_; break;
  //case 'd': render_hog_ = !render_hog_; break;
  case 'b': ZeroBend(); break;
  case 's': ZeroSide(); break;
  case 't': ZeroTwist(); break;
  case 'a': JointUp(); break;
  case 'z': JointDown(); break;
  case 'c':
    if (WarnKey(cmd)) ZeroJoints();
    break;

  case 'p':
    break;
  case 'w': SavePose(); break;
  case 'r': LoadPose(); break;

  case '1': ResetCamera(); break;
  case 'x':save_d=!save_d;break;
  case 'y':save_b=!save_b;break;
  case 'i':save_c=!save_c;break;
  case 'u':auto_cg=!auto_cg;break;
  case 'o':crop=!crop;break;
  //case 'o':bound_box=!bound_box;break;
  }

  if (warning_status_ >= 2) {
    warning_status_ = 0;
    warning_key_ = -1;
  }

  return true;
}

void PoseDesigner::UpdateBSTSliders() {
  sl_bend_->set_val(hand_pose_->bend(cur_joint()));
  sl_side_->set_val(hand_pose_->side(cur_joint()));
  sl_twist_->set_val(hand_pose_->twist(cur_joint()));
}

void PoseDesigner::ZeroBend() {
  hand_pose_->bend(cur_joint()) = 0;
  sl_bend_->set_val(0);
}

void PoseDesigner::ZeroSide() {
  hand_pose_->side(cur_joint()) = 0;
  sl_side_->set_val(0);
}

void PoseDesigner::ZeroTwist() {
  hand_pose_->twist(cur_joint()) = 0;
  sl_twist_->set_val(0);
}

void PoseDesigner::JointUp() {
  int next_joint = cur_joint() + 1;
  if (next_joint >= hand_pose_->num_joints()) return;

  sl_joint_no_->set_raw_val(next_joint);
  UpdateBSTSliders();
}

void PoseDesigner::JointDown() {
  int prev_joint = cur_joint() - 1;
  if (prev_joint < 0) return;

  sl_joint_no_->set_raw_val(prev_joint);
  UpdateBSTSliders();
}

void PoseDesigner::ResetCamera() {
  camera_spec_ = HandCameraSpec(hand_renderer_.initial_cam_distance());
  sl_theta_->set_val(0.);
  sl_phi_->set_val(0.);
  sl_tilt_->set_val(0.);
  sl_dist_mult_->set_val(1.0);
  hand_pose_->SetRotMatrix(camera_spec_);
}

void PoseDesigner::RenderEverything() {
  // Render the hand
  hand_renderer_.set_camera_spec(camera_spec_);
  hand_renderer_.SetHandPose(*hand_pose_);
  hand_renderer_.RenderHand();
  
  display_ = hand_renderer_.pixel_buffer_cv().clone();
  //lx
  depth_v = hand_renderer_.depth_buffer_cv().clone();
  if(crop==1)
  {
    HandRenderer::JointPositionMap j_map;
    hand_renderer_.walk_bones(j_map);
    HandRenderer::JointPositionMap::iterator pos;
    int tempnum=0;
    cv::Point center;
    double box=160;  
    for (pos = j_map.begin(); pos != j_map.end(); ++pos) {
        if(tempnum==26)
        {
          center.x= (int)pos->second[0];
          center.y= (int)pos->second[1];
        }
        tempnum++;
      }
    topleft.x=center.x-box;
    topleft.y=center.y-box;
    bright.x=center.x+box;
    bright.y=center.y+box;
   }

  if(save_c==1)
  {
    save_c=0;
    ostringstream ss;
    ss << "/media/data_cifs/lu/Synthetic/Segment/seg_" << file_num << ".jpg";
    string imagename = ss.str();
    if(crop==0)
    {
      cv::imwrite(imagename,display_);
    }
    else
    {
      cv::Rect handbox(topleft,bright);
      cv::Mat cropseg=display_(handbox);
      cv::imwrite(imagename,cropseg);
    }
    
  }
  

  if (render_hog_) RenderHog();

  DisplayJointAngle();

  TextPrinter printer_rt_(display_,
                          kRenderWinWidth - 10,
                          kRenderWinHeight - 10);
  printer_rt_.SetRightAlign();
  printer_rt_.Print("Press 'h' for help");

  if (render_help_) DisplayHelp();

  if (warning_status_ == 1) {
    switch(warning_key_) {
    case 'c': DisplayClearWarning(); break;
    case 27:
    case 'q': DisplayQuitWarning(); break;
    default: break;
    }
  }
  //lx
  if(save_d==1)
  {
   save_d=0;
   SaveDepth(depth_v,hand_renderer_.hand_camera_distance());

  }
  if(save_b==1)
  {
    save_b=0;
    hand_renderer_.get_camera_info();
    HandRenderer::JointPositionMap j_map;
    hand_renderer_.walk_bones(j_map);
    std::ofstream bonefile;
    ostringstream ss;
    ss << "/media/data_cifs/lu/Synthetic/Joints/bone_" << file_num << ".txt";
    string filename = ss.str();
 
    bonefile.open(filename.c_str());

    HandRenderer::JointPositionMap::iterator pos;
    for (pos = j_map.begin(); pos != j_map.end(); ++pos) {
        bonefile<<pos->second[0]<<",";
        bonefile<<pos->second[1]<<",";
        bonefile<<pos->second[2]<<",";
        bonefile<<pos->second[3]<<",";
        bonefile<<pos->second[4]<<",";
        bonefile<<pos->second[5]<<",";
    }
    bonefile<<0;
    bonefile.close();
  }
  DrawCameraSketch();
  if(crop==1)
  { 
    cv::rectangle(display_,topleft,bright,cv::Scalar(255,255,255),1,8);
    cv::imshow(win_render_,display_);
  }
  else
  {
    cv::imshow(win_render_, display_);
  }
  
  cv::imshow(win_camera_, cam_sketch_);
}

void PoseDesigner::DisplayJointAngle() {
  int n = cur_joint();
  float b = hand_pose_->bend(n),
    s = hand_pose_->side(n),
    t = hand_pose_->twist(n);

  float pi = math::constants::pi<float>();
  float deg_b = b * 180. / pi, deg_s = s * 180. / pi, deg_t = t * 180. / pi;

  TextPrinter printer_(display_, 10, kRenderWinHeight - 50);

  printer_.PrintF("Joint #%d: %s", cur_joint(),
                  scene_spec_.bone_name(cur_joint()).c_str());
  printer_.PrintF("Degrees: b: %+6.1f s: %+6.1f t: %+6.1f",
                  deg_b, deg_s, deg_t);
  printer_.PrintF("Radians: b: %+6.4f s: %+6.4f t: %+6.4f", b, s, t);
}

void PoseDesigner::DisplayHelp() {
  int center_x = kRenderWinWidth / 2, center_y = kRenderWinHeight / 2;
  int rect_w = ((double) kRenderWinWidth * 0.8),
    rect_h = ((double) kRenderWinHeight * 0.8);

  cv::Point rect_ul(center_x - rect_w / 2, center_y - rect_h / 2);
  cv::Point rect_br(center_x + rect_w / 2, center_y + rect_h / 2);

  cv::rectangle(display_, rect_ul, rect_br, cv::Scalar(40, 0, 0), CV_FILLED);

  TextPrinter printer_(display_, rect_ul.x + 10, rect_ul.y + 10);

  printer_.Print("Pose Designer - v1.0\n"
                 "\n"
                 "Keyboard Shortcuts:\n"
                 "  h - This help screen (press again to dismiss)\n"
                 "  ESC or q - Quit\n"
                 "\n"
                 "  r - Read a pose file\n"
                 "  w - Write a pose file\n"
                 "\n"
                 //"  d - Toggle HOG descriptor view\n"
                 //"\n"
                 "  b / s / t - Reset (b)end or (s)ide or (t)wist to 0\n"
                 "  c - clear all angles back to zero\n"
                 "\n"
                 "  a / z - Select previous/next joint"
                 "\n"
                 "  1 - Reset the camera to original view"
                 "\n"//lx
                 "  x - Save the depth of current pose\n"
                 "  y - Save the bone information of current pose\n"
                 "  i - Save the color image of current pose\n"
                 "\n"//lx
                 "  u - Render 1000 gestures according to the current pose and save them\n"
                 "  o - crop the hand by box\n"
                 );
}

void PoseDesigner::DisplayClearWarning() {
  DisplayWarning("WARNING! All angles will be cleared!",
                 "Press 'c' to continue, any other key to cancel");
}

void PoseDesigner::DisplayQuitWarning() {
  DisplayWarning("WARNING! All unsaved data will be lost",
                 "Press q or ESC to quit, any other key to cancel");
}

void PoseDesigner::DisplayWarning(const string &line1, const string &line2) {
  int center_x = kRenderWinWidth / 2, center_y = kRenderWinHeight / 2;
  int rect_w = ((double) kRenderWinWidth * 0.8),
    rect_h = 50;

  cv::Point rect_ul(center_x - rect_w / 2, center_y - rect_h / 2);
  cv::Point rect_br(center_x + rect_w / 2, center_y + rect_h / 2);

  cv::rectangle(display_, rect_ul, rect_br, cv::Scalar(0, 0, 80), CV_FILLED);

  TextPrinter printer_(display_, rect_ul.x + 10, rect_ul.y + 15);

  printer_.SetThickness(2);
  printer_.Print(line1 + "\n");
  printer_.SetThickness(1);
  printer_.Print(line2);
}

void PoseDesigner::ZeroJoints() {
  hand_pose_->ClearJoints();
  ZeroBend(); ZeroSide(); ZeroTwist();
}

void PoseDesigner::RenderHog() {
  cv::Mat image_mask =
    ImageUtils::MaskFromNonZero(hand_renderer_.pixel_buffer_cv());

  cv::Rect roi_box = ImageUtils::FindBoundingBox(image_mask);
  cv::Mat roi_image(hand_renderer_.pixel_buffer_cv(), roi_box);
  cv::Mat roi_mask(image_mask, roi_box);

  cv::Rect hog_rect(kRenderWinWidth - kHogDispWidth,
                    0,
                    kHogDispWidth, kHogDispHeight);
  cv::Mat roi_display(display_, hog_rect);

  HogDescriptor hog_desc(8,8,8);

  hog_calc_.CalcHog(roi_image, roi_mask, &hog_desc);
  HogUtils::RenderHog(roi_image, hog_desc, roi_display);
}

void PoseDesigner::DrawCameraSketch() {
  const int sw = kCamWinWidth, sh = kCamWinHeight;

  float cx = sw / 2., cy = sh / 2.;

  HandCameraSpec h_cam(camera_spec_);
  float sp_r = 0.4 * (float) sw;
  h_cam.r = sp_r;

  float s_x = 0, s_y = 0, s_z = 0;
  h_cam.GetPosition(&s_x, &s_y, &s_z);

  float persp_f = sp_r * 1.2;
  float z_min = -sp_r;
  float z_max = -z_min;
  float z_range = z_max - z_min;
  float scale_f = (z_max - s_y + persp_f) / (z_range + persp_f);
  float y_2d = cy - (s_z * scale_f);
  float x_2d = cx + (s_x * scale_f);

  cv::rectangle(cam_sketch_, cv::Point(0, 0), cv::Point(sw, sh),
                cv::Scalar(40, 0, 0), CV_FILLED);
  
  cv::rectangle(cam_sketch_,
                cv::Point(x_2d - 3 * scale_f, y_2d - 3 * scale_f),
                cv::Point(x_2d + 3 * scale_f, y_2d + 3 * scale_f),
                cv::Scalar(0, 0, 200), CV_FILLED);
}

void PoseDesigner::LoadPose() {
  FileDialog dialog;

  dialog.SetTitle("Choose a pose file to load");

  dialog.AddExtension(FileExtension("YAML file", 2, ".yml", ".yaml"));
  dialog.AddExtension(FileExtension("All files", 1, "*"));

  dialog.SetDefaultExtension(".yml");

  string pose_file = dialog.Open();
  if (pose_file == "") return;

  try {
    hand_pose_->Load(pose_file, scene_spec_);
    camera_spec_.SetFromHandPose(*hand_pose_);
    pose_file_ = pose_file;
  } catch (const std::exception &e) {
    cout << "Exception caught! What:" << e.what() << endl;
  }
}

void PoseDesigner::SavePose() {
  FileDialog dialog;

  dialog.SetTitle("Save a pose file");

  dialog.AddExtension(FileExtension("YAML file", 2, ".yml", ".yaml"));
  dialog.AddExtension(FileExtension("All files", 1, "*"));

  dialog.SetDefaultFile(pose_file_);
  dialog.SetDefaultExtension(".yml");

  string pose_file = dialog.Save();
  if (pose_file == "") return;

  try {
    hand_pose_->Save(pose_file, scene_spec_); 
    pose_file_ = pose_file;
  } catch (const std::exception &e) {
    cout << "Exception caught! What:" << e.what() << endl;
  }
}

void PoseDesigner::SaveDepth(cv::Mat depth_v,float hc_d){
   cv::Mat cropd;
   if(crop==1)
   {
     cv::Rect handbox(topleft,bright);
     cropd=depth_v(handbox);
   }
   else
   {
     cropd=depth_v;
   }
   std::ofstream depthfile;
   ostringstream ss;

   ss << "/media/data_cifs/lu/Synthetic/Depth/depth_" << file_num << ".txt";
   //ss << "../../../test/cropdep_" << 0 << ".txt";
   string filename = ss.str();
   depthfile.open(filename.c_str());

depthfile << cropd.rows;
depthfile <<",";
depthfile << cropd.cols;
depthfile <<",";
   for(int i=0;i<cropd.rows;i++)
   {
     for(int j=0;j<cropd.cols;j++)
     {
        float dvalue = cropd.at<float>(i,j);
        depthfile << dvalue;
        depthfile <<",";
     }
   }
   depthfile << depth_v.rows;
   depthfile <<",";
   depthfile << depth_v.cols;
   depthfile <<",";
   depthfile << topleft.x;
   depthfile <<",";
   depthfile << topleft.y;
   depthfile <<",";
   depthfile << bright.x;
   depthfile <<",";
   depthfile << bright.y;
   depthfile.close();
}

string PoseDesigner::GetSceneFileName() {
  FileDialog dialog;

  dialog.SetTitle("Choose a scene spec file");

  dialog.AddExtension(FileExtension("YAML file", 2, ".yml", ".yaml"));
  dialog.AddExtension(FileExtension("All files", 1, "*"));

  dialog.SetDefaultName("scene_spec.yml");
  dialog.SetDefaultExtension(".yml");

  return dialog.Open();
}

}  // namespace libhand
