// Main window for the convert_geotiff GUI: a form mirroring the CLI flags,
// plus a progress bar and log pane. Runs the conversion on a worker thread
// so the UI stays responsive.

#ifndef CONVERT_GEOTIFF_GUI_CONVERT_WINDOW_HPP
#define CONVERT_GEOTIFF_GUI_CONVERT_WINDOW_HPP

#include <FL/Fl_Button.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Float_Input.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Int_Input.H>
#include <FL/Fl_Output.H>
#include <FL/Fl_Progress.H>
#include <FL/Fl_Text_Buffer.H>
#include <FL/Fl_Text_Display.H>

#include <atomic>
#include <functional>
#include <string>
#include <thread>

class ConvertWindow : public Fl_Double_Window {
public:
  ConvertWindow(int w, int h, const char *title);
  ~ConvertWindow() override;

private:
  // Widgets
  Fl_Choice *direction_; // 0 = GeoTIFF -> geogrid, 1 = geogrid -> GeoTIFF
  Fl_Output *input_file_;
  Fl_Button *browse_input_;
  Fl_Output *output_dir_;      // forward mode: output directory
  Fl_Button *browse_output_;
  Fl_Output *output_tiff_;     // reverse mode: output GeoTIFF path
  Fl_Button *browse_output_tiff_;
  Fl_Check_Button *categorical_;
  Fl_Int_Input *categories_;
  Fl_Int_Input *border_width_;
  Fl_Int_Input *tile_size_;
  Fl_Choice *word_size_;
  Fl_Check_Button *unsigned_;
  Fl_Float_Input *scale_;
  Fl_Float_Input *missing_;
  Fl_Input *units_;
  Fl_Input *description_;
  Fl_Button *convert_button_;
  Fl_Progress *progress_;
  Fl_Text_Buffer *log_buffer_;
  Fl_Text_Display *log_display_;

  std::thread worker_;
  std::atomic<bool> running_{false};

  static void browse_input_cb(Fl_Widget *, void *);
  static void browse_output_cb(Fl_Widget *, void *);
  static void browse_output_tiff_cb(Fl_Widget *, void *);
  static void convert_cb(Fl_Widget *, void *);
  static void categorical_cb(Fl_Widget *, void *);
  static void direction_cb(Fl_Widget *, void *);

  void browse_input();
  void browse_output();
  void browse_output_tiff();
  void update_direction();
  bool is_reverse() const;
  void start_conversion();
  // Spawns the worker thread running `job`, wiring up the common
  // start/finish UI state, error logging, and alert-on-failure behavior
  // shared by both conversion directions. `job` runs on the worker thread
  // and should throw on failure.
  void run_worker(std::function<void()> job);
  void set_running(bool running);
  void log_line(const std::string &line);
};

#endif
