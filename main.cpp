#include <ApplicationServices/ApplicationServices.h>
#include <cstdlib>
#include <iostream>
#include <netinet/in.h>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

// --- Mac system controls ---
void runAppleScript(const std::string &script) {
  std::string cmd = "osascript -e '" + script + "'";
  system(cmd.c_str());
}

void volumeUp() {
  runAppleScript(
      "set volume output volume (output volume of (get volume settings) + 10)");
}

void volumeDown() {
  runAppleScript(
      "set volume output volume (output volume of (get volume settings) - 10)");
}

void pressKey(const std::string &key) {
  runAppleScript("tell application \"System Events\" to key code " + key);
}

void spotifyPlayPause() {
  runAppleScript("tell application \"Spotify\" to playpause");
}

void spotifyNext() {
  runAppleScript("tell application \"Spotify\" to next track");
}

void moveMouse(float norm_x, float norm_y) {
  // Get screen size
  CGRect screen = CGDisplayBounds(CGMainDisplayID());
  float screen_w = screen.size.width;
  float screen_h = screen.size.height;

  // Map normalised coords to screen coords
  float screen_x = norm_x * screen_w;
  float screen_y = norm_y * screen_h;

  CGPoint point = CGPointMake(screen_x, screen_y);
  CGEventRef event = CGEventCreateMouseEvent(NULL, kCGEventMouseMoved, point,
                                             kCGMouseButtonLeft);
  CGEventPost(kCGHIDEventTap, event);
  CFRelease(event);
}

// --- Gesture handler ---
void handleGesture(const std::string &raw) {
  // Split by ':'
  std::vector<std::string> parts;
  std::stringstream ss(raw);
  std::string token;
  while (std::getline(ss, token, ':')) {
    parts.push_back(token);
  }

  std::string gesture = parts[0];
  float x = parts.size() > 1 ? std::stof(parts[1]) : 0.0f;
  float y = parts.size() > 2 ? std::stof(parts[2]) : 0.0f;

  std::cout << "Received: " << gesture << " (" << x << ", " << y << ")"
            << std::endl;

  if (gesture == "POINTING")
    moveMouse(x, y);
  else if (gesture == "THUMBS_UP")
    volumeUp();
  else if (gesture == "FIST")
    volumeDown();
  else if (gesture == "OPEN_HAND")
    spotifyPlayPause();
  else if (gesture == "PEACE")
    spotifyNext();
}

int main() {
  int server_fd, client_fd;
  struct sockaddr_in address;
  int opt = 1;
  int addrlen = sizeof(address);
  char buffer[1024] = {0};

  // Create socket
  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(65432);

  // Bind and listen
  bind(server_fd, (struct sockaddr *)&address, sizeof(address));
  listen(server_fd, 3);

  std::cout << "C++ server listening on port 65432..." << std::endl;

  // Accept connection from Python
  client_fd =
      accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
  std::cout << "Python client connected!" << std::endl;

  // Main loop — receive gestures
  std::string leftover = "";

  while (true) {
    memset(buffer, 0, sizeof(buffer));
    int bytes = recv(client_fd, buffer, sizeof(buffer), 0);

    if (bytes <= 0) {
      std::cout << "Python disconnected" << std::endl;
      break;
    }

    // Parse newline-delimited messages
    std::string data = leftover + std::string(buffer, bytes);
    leftover = "";

    size_t pos;
    while ((pos = data.find('\n')) != std::string::npos) {
      std::string gesture = data.substr(0, pos);
      data.erase(0, pos + 1);
      if (!gesture.empty())
        handleGesture(gesture);
    }

    leftover = data;
  }

  close(client_fd);
  close(server_fd);
  return 0;
}
