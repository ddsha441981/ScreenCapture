# ScreenCapture

ScreenCapture is a lightweight cross-platform C++ application for
capturing arbitrary rectangular screen regions on X11-based systems
(mainly Linux). It provides an interactive overlay to select a screen
area and then outputs a high-quality PNG screenshot using the
stb_image_write library.

## Features

-   Interactive rectangle selection overlay
-   Captures selected screen area
-   Saves screenshots as PNG in the default pictures directory
-   Minimal dependencies: Uses X11, Xfixes, and stb_image_write
-   Easy integration for Linux-based desktop environments

## Build Instructions

1.  **Install dependencies:**

``` bash
sudo apt-get install libx11-dev libxfixes-dev libxrender-dev libxi-dev mesa-common-dev
```

2.  **Build project:**

``` bash
mkdir build && cd build
cmake ..
make
```

3.  **Run the tool:**

``` bash
./area_capture_tool
```

## Usage

-   Click and drag to select the area on screen
-   Release mouse to capture and save the screenshot
-   Press ESC to cancel

## Screenshot Location

Screenshots are saved in the `${XDG_PICTURES_DIR}/Screenshots`
directory, or fallback to `${HOME}/Pictures/Screenshots`.

## Project Structure

    ScreenCapture
    ├── CMakeLists.txt
    ├── rectangle_capture.cpp
    ├── rectangle_capture.h
    ├── stb_image_write.h
    └── stb_image_write_impl.cpp

## Development

-   Tested on Linux (Ubuntu 22.04+)
-   Uses modern C++17
-   Dependency: X11, Xfixes libraries for screen capturing
-   Uses stb_image_write for efficient PNG output

## License

Open-source under MIT License

## Author

Developed and maintained by Deendayal Kumawat
