#include "convert_window.hpp"

#include "convert_geotiff/convert.hpp"
#include "convert_geotiff/convert_back.hpp"
#include "convert_geotiff/geogrid_index.hpp"

#include <FL/Fl.H>
#include <FL/Fl_Native_File_Chooser.H>
#include <FL/fl_ask.H>

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <string>

ConvertWindow::ConvertWindow(int w, int h, const char *title) : Fl_Double_Window(w, h, title) {
  const int label_x = 160;
  const int field_w = 300;
  const int browse_w = 80;
  int y = 10;

  direction_ = new Fl_Choice(label_x, y, 220, 25, "Direction:");
  direction_->add("GeoTIFF -> geogrid");
  direction_->add("geogrid -> GeoTIFF");
  direction_->value(0);
  direction_->callback(direction_cb, this);
  y += 35;

  input_file_ = new Fl_Output(label_x, y, field_w, 25, "GeoTIFF file:");
  browse_input_ = new Fl_Button(label_x + field_w + 10, y, browse_w, 25, "Browse...");
  browse_input_->callback(browse_input_cb, this);
  y += 35;

  output_dir_ = new Fl_Output(label_x, y, field_w, 25, "Output directory:");
  browse_output_ = new Fl_Button(label_x + field_w + 10, y, browse_w, 25, "Browse...");
  browse_output_->callback(browse_output_cb, this);
  output_tiff_ = new Fl_Output(label_x, y, field_w, 25, "Output GeoTIFF:");
  browse_output_tiff_ = new Fl_Button(label_x + field_w + 10, y, browse_w, 25, "Browse...");
  browse_output_tiff_->callback(browse_output_tiff_cb, this);
  output_tiff_->hide();
  browse_output_tiff_->hide();
  y += 35;

  categorical_ = new Fl_Check_Button(label_x, y, 160, 25, "Categorical data");
  categorical_->callback(categorical_cb, this);
  categories_ = new Fl_Int_Input(label_x + 280, y, 80, 25, "Categories:");
  categories_->align(FL_ALIGN_LEFT);
  categories_->value("10");
  categories_->deactivate();
  y += 35;

  border_width_ = new Fl_Int_Input(label_x, y, 80, 25, "Border width:");
  border_width_->value("3");
  y += 35;

  tile_size_ = new Fl_Int_Input(label_x, y, 80, 25, "Tile size:");
  tile_size_->value("100");
  y += 35;

  word_size_ = new Fl_Choice(label_x, y, 80, 25, "Word size (bytes):");
  word_size_->add("1");
  word_size_->add("2");
  word_size_->add("4");
  word_size_->value(1); // "2"
  y += 35;

  unsigned_ = new Fl_Check_Button(label_x, y, 200, 25, "Unsigned data");
  y += 35;

  scale_ = new Fl_Float_Input(label_x, y, 100, 25, "Scale factor:");
  scale_->value("1.0");
  y += 35;

  missing_ = new Fl_Float_Input(label_x, y, 100, 25, "Missing value:");
  missing_->value("0.0");
  y += 35;

  units_ = new Fl_Input(label_x, y, field_w, 25, "Units:");
  units_->value("NO UNITS");
  y += 35;

  description_ = new Fl_Input(label_x, y, field_w, 25, "Description:");
  description_->value("NO DESCRIPTION");
  y += 45;

  convert_button_ = new Fl_Button(label_x, y, 120, 30, "Convert");
  convert_button_->callback(convert_cb, this);
  y += 40;

  progress_ = new Fl_Progress(10, y, w - 20, 25);
  progress_->minimum(0);
  progress_->maximum(1);
  progress_->value(0);
  y += 35;

  log_buffer_ = new Fl_Text_Buffer();
  log_display_ = new Fl_Text_Display(10, y, w - 20, h - y - 10);
  log_display_->buffer(log_buffer_);

  end();
}

ConvertWindow::~ConvertWindow() {
  if (worker_.joinable()) worker_.join();
  log_display_->buffer(nullptr);
  delete log_buffer_;
}

void ConvertWindow::browse_input_cb(Fl_Widget *, void *data) {
  static_cast<ConvertWindow *>(data)->browse_input();
}

void ConvertWindow::browse_output_cb(Fl_Widget *, void *data) {
  static_cast<ConvertWindow *>(data)->browse_output();
}

void ConvertWindow::browse_output_tiff_cb(Fl_Widget *, void *data) {
  static_cast<ConvertWindow *>(data)->browse_output_tiff();
}

void ConvertWindow::direction_cb(Fl_Widget *, void *data) {
  static_cast<ConvertWindow *>(data)->update_direction();
}

void ConvertWindow::convert_cb(Fl_Widget *, void *data) {
  static_cast<ConvertWindow *>(data)->start_conversion();
}

void ConvertWindow::categorical_cb(Fl_Widget *, void *data) {
  auto *self = static_cast<ConvertWindow *>(data);
  if (self->categorical_->value())
    self->categories_->activate();
  else
    self->categories_->deactivate();
}

bool ConvertWindow::is_reverse() const { return direction_->value() == 1; }

void ConvertWindow::browse_input() {
  if (is_reverse()) {
    Fl_Native_File_Chooser chooser(Fl_Native_File_Chooser::BROWSE_DIRECTORY);
    chooser.title("Choose a geogrid directory");
    if (chooser.show() == 0) {
      input_file_->value(chooser.filename());
    }
    return;
  }
  Fl_Native_File_Chooser chooser(Fl_Native_File_Chooser::BROWSE_FILE);
  chooser.title("Choose a GeoTIFF file");
  chooser.filter("GeoTIFF Files\t*.{tif,tiff}");
  if (chooser.show() == 0) {
    input_file_->value(chooser.filename());
  }
}

void ConvertWindow::browse_output() {
  // BROWSE_SAVE_DIRECTORY, not BROWSE_DIRECTORY: on Linux, FLTK's GTK
  // backend (the default driver whenever GTK libs are present, which is
  // effectively always) maps BROWSE_DIRECTORY to
  // GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER and never calls
  // gtk_file_chooser_set_create_folders() for it -- the NEW_FOLDER option
  // below is simply never consulted on that path. BROWSE_SAVE_DIRECTORY
  // maps to GTK_FILE_CHOOSER_ACTION_CREATE_FOLDER, which does enable
  // folder creation, and is the type every other backend (Zenity,
  // kdialog, FLTK's own dialog, macOS, Windows) implements this same way.
  Fl_Native_File_Chooser chooser(Fl_Native_File_Chooser::BROWSE_SAVE_DIRECTORY);
  chooser.title("Choose an output directory");
  chooser.options(Fl_Native_File_Chooser::NEW_FOLDER);
  if (chooser.show() == 0) {
    output_dir_->value(chooser.filename());
  }
}

void ConvertWindow::browse_output_tiff() {
  Fl_Native_File_Chooser chooser(Fl_Native_File_Chooser::BROWSE_SAVE_FILE);
  chooser.title("Choose the output GeoTIFF path");
  chooser.filter("GeoTIFF Files\t*.{tif,tiff}");
  chooser.options(Fl_Native_File_Chooser::SAVEAS_CONFIRM | Fl_Native_File_Chooser::NEW_FOLDER);
  if (chooser.show() == 0) {
    output_tiff_->value(chooser.filename());
  }
}

void ConvertWindow::update_direction() {
  const bool reverse = is_reverse();

  input_file_->label(reverse ? "Geogrid directory:" : "GeoTIFF file:");
  input_file_->value("");

  if (reverse) {
    output_dir_->hide();
    browse_output_->hide();
    output_tiff_->show();
    browse_output_tiff_->show();
  } else {
    output_tiff_->hide();
    browse_output_tiff_->hide();
    output_dir_->show();
    browse_output_->show();
  }
  output_dir_->value("");
  output_tiff_->value("");

  // The forward-only options are meaningless for the reverse direction --
  // everything it needs is already in the geogrid index file.
  Fl_Widget *forward_only[] = {categorical_, categories_, border_width_, tile_size_,
                                word_size_,   unsigned_,   scale_,        missing_,
                                units_,       description_};
  for (Fl_Widget *w : forward_only) {
    if (reverse)
      w->deactivate();
    else
      w->activate();
  }
  if (!reverse && !categorical_->value()) categories_->deactivate();

  redraw();
}

void ConvertWindow::set_running(bool running) {
  running_ = running;
  if (running) {
    convert_button_->deactivate();
    browse_input_->deactivate();
    browse_output_->deactivate();
    browse_output_tiff_->deactivate();
    direction_->deactivate();
  } else {
    convert_button_->activate();
    browse_input_->activate();
    browse_output_->activate();
    browse_output_tiff_->activate();
    direction_->activate();
  }
}

void ConvertWindow::log_line(const std::string &line) {
  log_buffer_->append(line.c_str());
  log_buffer_->append("\n");
  log_display_->scroll(log_buffer_->count_lines(0, log_buffer_->length()), 0);
}

void ConvertWindow::start_conversion() {
  if (running_) return;

  if (is_reverse()) {
    const std::string geogrid_dir = input_file_->value() ? input_file_->value() : "";
    const std::string output_tiff = output_tiff_->value() ? output_tiff_->value() : "";
    if (geogrid_dir.empty()) {
      fl_alert("Please choose a geogrid directory to convert.");
      return;
    }
    if (output_tiff.empty()) {
      fl_alert("Please choose an output GeoTIFF path.");
      return;
    }

    progress_->copy_label("");
    run_worker([geogrid_dir, output_tiff]() { convert_geotiff::convert_back(geogrid_dir, output_tiff); });
    return;
  }

  const std::string filename = input_file_->value() ? input_file_->value() : "";
  const std::string output_dir = output_dir_->value() ? output_dir_->value() : "";
  if (filename.empty()) {
    fl_alert("Please choose a GeoTIFF file to convert.");
    return;
  }
  if (output_dir.empty()) {
    fl_alert("Please choose an output directory.");
    return;
  }

  convert_geotiff::ConversionOptions opts;
  opts.border_width = std::atoi(border_width_->value());
  opts.tile_size = std::atoi(tile_size_->value());
  const char *word_sizes[] = {"1", "2", "4"};
  opts.word_size = std::atoi(word_sizes[word_size_->value()]);
  opts.isigned = unsigned_->value() == 0;
  opts.scale = static_cast<float>(std::atof(scale_->value()));
  opts.missing = static_cast<float>(std::atof(missing_->value()));
  opts.units = std::string("\"") + units_->value() + "\"";
  opts.description = std::string("\"") + description_->value() + "\"";
  opts.categorical_range = categorical_->value() ? std::atoi(categories_->value()) : 0;

  if (opts.border_width < 0) {
    fl_alert("Border width must be >= 0.");
    return;
  }
  if (opts.tile_size <= 0 || opts.tile_size > 99999) {
    fl_alert("Tile size must be between 1 and 99999.");
    return;
  }
  if (opts.scale == 0.f) {
    fl_alert("Scale factor must not be 0.");
    return;
  }
  if (categorical_->value() && opts.categorical_range <= 0) {
    fl_alert("Number of categories must be > 0.");
    return;
  }

  ConvertWindow *self = this;
  run_worker([self, filename, output_dir, opts]() {
    std::filesystem::current_path(output_dir);
    convert_geotiff::convert(filename, opts, [self](int tx, int ty, int nxt, int nyt) {
      const int done = ty * nxt + tx + 1;
      const int total = nxt * nyt;
      Fl::lock();
      self->progress_->maximum(static_cast<double>(total));
      self->progress_->value(static_cast<double>(done));
      char buf[64];
      std::snprintf(buf, sizeof(buf), "%d/%d", done, total);
      self->progress_->copy_label(buf);
      Fl::unlock();
      Fl::awake();
    });
  });
}

void ConvertWindow::run_worker(std::function<void()> job) {
  if (worker_.joinable()) worker_.join();

  log_buffer_->text("");
  progress_->value(0);
  progress_->maximum(1);
  set_running(true);

  ConvertWindow *self = this;
  worker_ = std::thread([self, job = std::move(job)]() {
    std::string error;
    bool has_error = false;
    try {
      job();
    } catch (const std::exception &e) {
      error = e.what();
      has_error = true;
    }

    Fl::lock();
    self->set_running(false);
    if (has_error) {
      self->log_line("ERROR: " + error);
    } else {
      self->progress_->value(self->progress_->maximum());
      self->log_line("Conversion complete.");
    }
    Fl::unlock();
    Fl::awake();

    if (has_error) {
      // fl_alert must run on the main thread; awake a dedicated handler.
      auto *msg = new std::string(error);
      Fl::awake(
          [](void *d) {
            auto *s = static_cast<std::string *>(d);
            fl_alert("%s", s->c_str());
            delete s;
          },
          msg);
    }
  });
}
