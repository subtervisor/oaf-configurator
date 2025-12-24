#include <3ds.h>
#include <citro3d.h>
#include <tex3ds.h>

#include <cmath>
#include <cstdint>
#include <cstring>
#include <format>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <tuple>
#include <vector>

#include "ini.h"

// Enable Npi Assert to write Errors to file
#define IMGUI_IMPL_CITRO3D_USE_NPI_ASSERT
#define IMGUI_IMPL_CIR_USE_NPI_ASSERT

#include "imgui.h"
#include "imgui_impl_citro3d.h"
#include "imgui_impl_ctr.h"

const auto CLEAR_COLOR = 0x1a2529FF;
const auto SCREEN_WIDTH = 400.0f;
const auto SCREEN_HEIGHT = 480.0f;
const auto TRANSFER_SCALING = GX_TRANSFER_SCALE_NO;
const auto FB_SCALE = 1.0f;
const auto FB_WIDTH = SCREEN_WIDTH * FB_SCALE;
const auto FB_HEIGHT = SCREEN_HEIGHT * FB_SCALE;

const auto DISPLAY_TRANSFER_FLAGS =
    GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) |
    GX_TRANSFER_RAW_COPY(0) | GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) |
    GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) |
    GX_TRANSFER_SCALING(TRANSFER_SCALING);

C3D_RenderTarget* Top;
C3D_RenderTarget* Bottom;

#define rev_void(x) reinterpret_cast<void*>(x)
#define is_new3ds() (modelId == 2 || modelId == 4)

uint8_t modelId = 0xff;
const std::filesystem::path iniPath{"sdmc:/3ds/open_agb_firm/config.ini"};
const std::filesystem::path iniDir{"sdmc:/3ds/open_agb_firm"};

std::tuple<bool, ini::File> loadIni() {
  try {
    auto ini = ini::open(iniPath);
    return {true, ini};
  } catch (...) {
    auto ini = ini::File();
    return {false, ini};
  }
}

std::tuple<bool, ini::File> loadAndValidateIniWithDefaults() {
  auto [iniLoaded, ini] = loadIni();
  if (ini.has_section("general")) {
    try {
      auto backlight = ini["general"].get<int>("backlight");
      if (is_new3ds()) {
        if (backlight < 16 || backlight > 142) {
          ini["general"].set<int>("backlight", 64);
        }
      } else {
        if (backlight < 20 || backlight > 117) {
          ini["general"].set<int>("backlight", 64);
        }
      }
    } catch (...) {
      ini["general"].set<int>("backlight", 64);
    }
    try {
      auto backlightSteps = ini["general"].get<int>("backlightSteps");
      if (backlightSteps < 1 || backlightSteps > 128) {
        ini["general"].set<int>("backlightSteps", 5);
      }
    } catch (...) {
      ini["general"].set<int>("backlightSteps", 5);
    }
    try {
      ini["general"].get<bool>("directBoot");
    } catch (...) {
      ini["general"].set<bool>("directBoot", false);
    }
    try {
      ini["general"].get<bool>("useGbaDb");
    } catch (...) {
      ini["general"].set<bool>("useGbaDb", true);
    }
    try {
      ini["general"].get<bool>("useSavesFolder");
    } catch (...) {
      ini["general"].set<bool>("useSavesFolder", true);
    }
  } else {
    ini.add_section("general");
    ini["general"].set<int>("backlight", 64);
    ini["general"].set<int>("backlightSteps", 5);
    ini["general"].set<bool>("directBoot", false);
    ini["general"].set<bool>("useGbaDb", true);
    ini["general"].set<bool>("useSavesFolder", true);
  }
  if (ini.has_section("video")) {
    try {
      auto scaler = ini["video"].get<std::string>("scaler");
      if (scaler != "none" && scaler != "bilinear" && scaler != "matrix") {
        ini["video"].set<std::string>("scaler", "matrix");
      }
    } catch (...) {
      ini["video"].set<std::string>("scaler", "matrix");
    }
    try {
      auto colorProfile = ini["video"].get<std::string>("colorProfile");
      if (colorProfile != "none" && colorProfile != "gba" &&
          colorProfile != "gb_micro" && colorProfile != "gba_sp101" &&
          colorProfile != "nds" && colorProfile != "ds_lite" &&
          colorProfile != "nso" && colorProfile != "vba" &&
          colorProfile != "identity") {
        ini["video"].set<std::string>("colorProfile", "none");
      }
    } catch (...) {
      ini["video"].set<std::string>("colorProfile", "none");
    }
    try {
      auto contrast = ini["video"].get<float>("contrast");
      if (std::isnan(contrast) || contrast < 0.0 || contrast > 1.0) {
        ini["video"].set<float>("contrast", 1.0);
      }
    } catch (...) {
      ini["video"].set<float>("contrast", 1.0);
    }
    try {
      auto brightness = ini["video"].get<float>("brightness");
      if (std::isnan(brightness) || brightness < 0.0 || brightness > 1.0) {
        ini["video"].set<float>("brightness", 0.0);
      }
    } catch (...) {
      ini["video"].set<float>("brightness", 0.0);
    }
    try {
      auto saturation = ini["video"].get<float>("saturation");
      if (std::isnan(saturation) || saturation < 0.0 || saturation > 1.0) {
        ini["video"].set<float>("saturation", 0.0);
      }
    } catch (...) {
      ini["video"].set<float>("saturation", 1.0);
    }
  } else {
    ini.add_section("video");
    ini["video"].set<std::string>("scaler", "matrix");
    ini["video"].set<std::string>("colorProfile", "none");
    ini["video"].set<float>("contrast", 1.0);
    ini["video"].set<float>("brightness", 0.0);
    ini["video"].set<float>("saturation", 1.0);
  }
  if (ini.has_section("audio")) {
    try {
      auto audioOut = ini["audio"].get<std::string>("audioOut");
      if (audioOut != "auto" && audioOut != "speakers" &&
          audioOut != "headphones") {
        ini["audio"].set<std::string>("audioOut", "auto");
      }
    } catch (...) {
      ini["audio"].set<std::string>("audioOut", "auto");
    }
    try {
      auto volume = ini["audio"].get<int>("volume");
      if (volume < -128 || volume > 127 || (volume > -20 && volume < 49)) {
        ini["audio"].set<int>("volume", 127);
      }
    } catch (...) {
      ini["audio"].set<int>("volume", 127);
    }
  } else {
    ini.add_section("audio");
    ini["audio"].set<std::string>("audioOut", "auto");
    ini["audio"].set<int>("volume", 127);
  }

  if (ini.has_section("advanced")) {
    try {
      ini["advanced"].get<bool>("saveOverride");
    } catch (...) {
      ini["advanced"].set<bool>("saveOverride", false);
    }
    try {
      auto defaultSave = ini["advanced"].get<std::string>("defaultSave");
      if (defaultSave != "eeprom_8k" && defaultSave != "rom_256m_eeprom_8k" &&
          defaultSave != "eeprom_64k" && defaultSave != "rom_256m_eeprom_64k" &&
          defaultSave != "flash_512k_atmel_rtc" &&
          defaultSave != "flash_512k_atmel" &&
          defaultSave != "flash_512k_sst_rtc" &&
          defaultSave != "flash_512k_sst" &&
          defaultSave != "flash_512k_panasonic_rtc" &&
          defaultSave != "flash_512k_panasonic" &&
          defaultSave != "flash_1m_macronix_rtc" &&
          defaultSave != "flash_1m_macronix" &&
          defaultSave != "flash_1m_sanyo_rtc" &&
          defaultSave != "flash_1m_sanyo" && defaultSave != "sram_256k" &&
          defaultSave != "none") {
        ini["advanced"].set<std::string>("defaultSave", "sram_256k");
      }
    } catch (...) {
      ini["advanced"].set<std::string>("defaultSave", "sram_256k");
    }
  } else {
    ini.add_section("advanced");
    ini["advanced"].set<bool>("saveOverride", false);
    ini["advanced"].set<std::string>("defaultSave", "sram_256k");
  }
  return {iniLoaded, ini};
}
std::vector<std::string> models = {"Old 3DS - CTR",    "Old 3DS XL - SPR",
                                   "New 3DS - KTR",    "Old 2DS - FTR",
                                   "New 3DS XL - RED", "New 2DS XL - JAN"};

void consolePanic(const std::string& message) {
  consoleInit(GFX_TOP, NULL);
  const auto panicMessage =
      std::format("\n\n{}ERROR:{} {}\n\n  Press START to exit.", CONSOLE_RED,
                  CONSOLE_RESET, message);
  puts(panicMessage.c_str());

  while (aptMainLoop()) {
    gspWaitForVBlank();
    hidScanInput();
    if (hidKeysDown() & KEY_START) break;
  }
  exit(0);
}

void exitHandler() {
  cfguExit();
  fsExit();
  gfxExit();
}

static const char* scalerOptionsRaw[] = {"none", "bilinear", "matrix"};
static const char* profileOptionsRaw[] = {"none",      "gba", "gb_micro",
                                          "gba_sp101", "nds", "ds_lite",
                                          "nso",       "vba", "identity"};
static const char* audioOutOptionsRaw[] = {"auto", "speakers", "headphones"};
static const char* saveTypeOptionsRaw[] = {
    "eeprom_8k",
    "rom_256m_eeprom_8k",
    "eeprom_64k",
    "rom_256m_eeprom_64k",
    "flash_512k_atmel_rtc",
    "flash_512k_atmel",
    "flash_512k_sst_rtc",
    "flash_512k_sst",
    "flash_512k_panasonic_rtc",
    "flash_512k_panasonic",
    "flash_1m_macronix_rtc",
    "flash_1m_macronix",
    "flash_1m_sanyo_rtc",
    "flash_1m_sanyo",
    "sram_256k",
    "none"  // None
};

static const char* scalerOptionsPretty[] = {"None", "Bilinear", "Matrix"};
static const char* profileOptionsPretty[] = {"None",
                                             "Game Boy Advance",
                                             "Game Boy Micro",
                                             "Game Boy Advance SP",
                                             "Nintendo DS",
                                             "Nintendo DS Lite",
                                             "Nintendo Switch Online",
                                             "Visual Boy Advance",
                                             "No Conversion"};
static const char* audioOutOptionsPretty[] = {
    "Automatic",
    "3DS Speakers",
    "Headphones",
};
static const char* saveTypeOptionsPretty[] = {"EEPROM (8k)",
                                              "EEPROM (8k, 256M ROM)",
                                              "EEPROM (64k)",
                                              "EEPROM (64k, 256M ROM)",
                                              "Flash (Atmel/512k/RTC)",
                                              "Flash (Atmel/512k)",
                                              "Flash (SST/512k/RTC)",
                                              "Flash (SST/512k)",
                                              "Flash (Panasonic/512k/RTC)",
                                              "Flash (Panasonic/512k)",
                                              "Flash (Macronix/1M/RTC)",
                                              "Flash (Macronix/1M)",
                                              "Flash (Sanyo/1M/RTC)",
                                              "Flash (Sanyo/1M)",
                                              "SRAM (256k)",
                                              "None"};

int main() {
  gfxInitDefault();
  romfsInit();
  fsInit();
  cfguInit();
  atexit(exitHandler);
  if (R_FAILED(CFGU_GetSystemModel(std::addressof(modelId))) ||
      modelId >= models.size()) {
    consolePanic("Could not load model or model ID invalid.");
  }

  if (modelId == 3 || modelId == 5) {
    consolePanic("2DS is not supported.");
  }

  auto [iniLoaded, iniFile] = loadAndValidateIniWithDefaults();

  C3D_Init(2 * C3D_DEFAULT_CMDBUF_SIZE);

  // create top screen render target
  Top = C3D_RenderTargetCreate(FB_HEIGHT * 0.5f, FB_WIDTH, GPU_RB_RGBA8,
                               GPU_RB_DEPTH24_STENCIL8);
  C3D_RenderTargetSetOutput(Top, GFX_TOP, GFX_LEFT, DISPLAY_TRANSFER_FLAGS);

  // create bottom screen render target
  Bottom = C3D_RenderTargetCreate(FB_HEIGHT * 0.5f, FB_WIDTH * 0.8f,
                                  GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
  C3D_RenderTargetSetOutput(Bottom, GFX_BOTTOM, GFX_LEFT,
                            DISPLAY_TRANSFER_FLAGS);

  ImGui::CreateContext();
  ImGui::StyleColorsDark();
  auto& io = ImGui::GetIO();
  auto& style = ImGui::GetStyle();
  style.ScaleAllSizes(0.5f);
  io.IniFilename = nullptr;
  io.AddMouseSourceEvent(ImGuiMouseSource_TouchScreen);
  ImGui_ImplCtr_Init();
  ImGui_ImplCitro3D_Init();

  static const char* scalerOptionsRaw[] = {"none", "bilinear", "matrix"};
  static const char* profileOptionsRaw[] = {"none",      "gba", "gb_micro",
                                            "gba_sp101", "nds", "ds_lite",
                                            "nso",       "vba", "identity"};
  static const char* audioOutOptionsRaw[] = {"auto", "speakers", "headphones"};

  static const char* scalerOptionsPretty[] = {"None", "Bilinear", "Matrix"};
  static const char* profileOptionsPretty[] = {"None",
                                               "Game Boy Advance",
                                               "Game Boy Micro",
                                               "Game Boy Advance SP",
                                               "Nintendo DS",
                                               "Nintendo DS Lite",
                                               "Nintendo Switch Online",
                                               "Visual Boy Advance",
                                               "No Conversion"};
  static const char* audioOutOptionsPretty[] = {
      "Automatic",
      "3DS Speakers",
      "Headphones",
  };

  int scalerIndex = 2;
  auto currentScaler = iniFile["video"].get<std::string>("scaler");
  for (int i = 0; i < IM_ARRAYSIZE(scalerOptionsRaw); i++) {
    if (currentScaler == scalerOptionsRaw[i]) {
      scalerIndex = i;
    }
  }
  int profileIndex = 0;
  auto currentProfile = iniFile["video"].get<std::string>("colorProfile");
  for (int i = 0; i < IM_ARRAYSIZE(profileOptionsRaw); i++) {
    if (currentProfile == profileOptionsRaw[i]) {
      profileIndex = i;
    }
  }
  int audioOutIndex = 0;
  auto currentAudioOut = iniFile["audio"].get<std::string>("audioOut");
  for (int i = 0; i < IM_ARRAYSIZE(audioOutOptionsRaw); i++) {
    if (currentAudioOut == audioOutOptionsRaw[i]) {
      audioOutIndex = i;
    }
  }
  int saveTypeIndex = 0;
  auto currentSaveType = iniFile["advanced"].get<std::string>("defaultSave");
  for (int i = 0; i < IM_ARRAYSIZE(saveTypeOptionsRaw); i++) {
    if (currentSaveType == saveTypeOptionsRaw[i]) {
      saveTypeIndex = i;
    }
  }

  int volume = iniFile["audio"].get<int>("volume");
  bool sliderControlled = volume > -20;
  if (sliderControlled) {
    volume = 127;
    iniFile["audio"].set<int>("volume", volume);
  }

  std::vector<std::string> logLines{
      std::format("Model: {}", models[modelId]),
      std::string(iniLoaded ? "Loaded configuration from SD."
                            : "No configuration found, creating one.")};

  while (aptMainLoop()) {
    hidScanInput();
    if (hidKeysDown() & KEY_START) exit(0);
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
    C3D_FrameDrawOn(Top);
    C3D_RenderTargetClear(Top, C3D_CLEAR_ALL, CLEAR_COLOR, 0);
    C3D_FrameDrawOn(Bottom);
    C3D_RenderTargetClear(Bottom, C3D_CLEAR_ALL, CLEAR_COLOR, 0);
    // setup display metrics
    io.DisplaySize = ImVec2(SCREEN_WIDTH, SCREEN_HEIGHT);
    io.DisplayFramebufferScale = ImVec2(FB_SCALE, FB_SCALE);
    auto const width = io.DisplaySize.x;
    auto const height = io.DisplaySize.y;
    ImGui_ImplCitro3D_NewFrame();
    ImGui_ImplCtr_NewFrame();
    ImGui::NewFrame();
    auto halfHeight = height * 0.5f;

    auto topOffset = ImVec2(0, 0);
    auto topSize = ImVec2(width, halfHeight);
    ImGui::SetNextWindowPos(topOffset, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(topSize);
    ImGui::Begin("Logs", nullptr,
                 ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoResize);
    for (const auto& logLine : logLines) {
      ImGui::Text(logLine.c_str());
    }
    // auto-scroll if scroll bar is at end
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
      ImGui::SetScrollHereY(1.0f);
    ImGui::End();

    auto bottomOffset = ImVec2(width * 0.1f, halfHeight);
    auto bottomSize = ImVec2(width * 0.8f, halfHeight);
    ImGui::SetNextWindowPos(bottomOffset, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(bottomSize);
    ImGui::Begin("open_agb_firm configurator", nullptr,
                 ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoCollapse |
                     ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);
    if (ImGui::BeginMenuBar()) {
      if (ImGui::BeginMenu("Menu \xee\x80\x83")) {
        if (ImGui::MenuItem("Save")) {
          try {
            std::filesystem::create_directories(iniDir);
            iniFile.write(iniPath);
            logLines.push_back("Configuration saved.");
          } catch (const std::exception& e) {
            logLines.push_back(std::format("Error while saving: {}", e.what()));
          }
        }
        if (ImGui::MenuItem("Close")) {
          exit(0);
        }
        ImGui::EndMenu();
      }
      ImGui::EndMenuBar();
    }

    ImGui::Text("General Settings");
    ImGui::Separator();
    int backlight = iniFile["general"].get<int>("backlight");
    if (ImGui::SliderInt("Backlight", &backlight, is_new3ds() ? 16 : 20,
                         is_new3ds() ? 142 : 117, "%d",
                         ImGuiSliderFlags_AlwaysClamp)) {
      iniFile["general"].set<int>("backlight", backlight);
    }
    int backlightSteps = iniFile["general"].get<int>("backlightSteps");
    if (ImGui::SliderInt("Backlight Steps", &backlightSteps, 1, 128, "%d",
                         ImGuiSliderFlags_AlwaysClamp)) {
      iniFile["general"].set<int>("backlightSteps", backlightSteps);
    }

    bool directBoot = iniFile["general"].get<bool>("directBoot");
    if (ImGui::Checkbox("Direct Boot", &directBoot)) {
      iniFile["general"].set<bool>("directBoot", directBoot);
    }

    bool useGbaDb = iniFile["general"].get<bool>("useGbaDb");
    if (ImGui::Checkbox("Use GBA DB", &useGbaDb)) {
      iniFile["general"].set<bool>("useGbaDb", useGbaDb);
    }

    bool useSavesFolder = iniFile["general"].get<bool>("useSavesFolder");
    if (ImGui::Checkbox("Use Saves Folder", &useSavesFolder)) {
      iniFile["general"].set<bool>("useSavesFolder", useSavesFolder);
    }

    ImGui::Separator();
    ImGui::Text("Video");
    ImGui::Separator();

    if (ImGui::Combo("Video Scaler", &scalerIndex, scalerOptionsPretty,
                     IM_ARRAYSIZE(scalerOptionsPretty))) {
      iniFile["video"].set<std::string>(
          "scaler", std::string(scalerOptionsRaw[scalerIndex]));
    }

    if (ImGui::Combo("Color Profile", &profileIndex, profileOptionsPretty,
                     IM_ARRAYSIZE(profileOptionsPretty))) {
      iniFile["video"].set<std::string>(
          "colorProfile", std::string(profileOptionsRaw[profileIndex]));
    }

    float contrast = iniFile["video"].get<float>("contrast");
    if (ImGui::SliderFloat("Contrast", &contrast, 0.0, 1.0, "%.3f",
                           ImGuiSliderFlags_AlwaysClamp)) {
      iniFile["video"].set<float>("contrast", contrast);
    }

    float brightness = iniFile["video"].get<float>("brightness");
    if (ImGui::SliderFloat("Brightness", &brightness, 0.0, 1.0, "%.3f",
                           ImGuiSliderFlags_AlwaysClamp)) {
      iniFile["video"].set<float>("brightness", brightness);
    }

    float saturation = iniFile["video"].get<float>("saturation");
    if (ImGui::SliderFloat("Saturation", &saturation, 0.0, 1.0, "%.3f",
                           ImGuiSliderFlags_AlwaysClamp)) {
      iniFile["video"].set<float>("saturation", saturation);
    }

    ImGui::Separator();
    ImGui::Text("Audio");
    ImGui::Separator();

    if (ImGui::Combo("Audio Output", &audioOutIndex, audioOutOptionsPretty,
                     IM_ARRAYSIZE(audioOutOptionsPretty))) {
      iniFile["audio"].set<std::string>(
          "audioOut", std::string(audioOutOptionsRaw[audioOutIndex]));
    }

    ImGui::BeginDisabled(sliderControlled);
    if (ImGui::SliderInt("Volume", &volume, sliderControlled ? volume : -128,
                         sliderControlled ? volume : -20,
                         sliderControlled ? "Slider Controlled" : "%d",
                         ImGuiSliderFlags_AlwaysClamp)) {
      iniFile["audio"].set<int>("volume", volume);
    }
    ImGui::EndDisabled();

    if (ImGui::Checkbox("Control volume with slider", &sliderControlled)) {
      if (sliderControlled) {
        volume = 127;
      } else {
        volume = -20;
      }
      iniFile["audio"].set<int>("volume", volume);
    }

    ImGui::Separator();
    ImGui::Text("Advanced");
    ImGui::Separator();

    bool saveOverride = iniFile["advanced"].get<bool>("saveOverride");
    if (ImGui::Checkbox("Use save type override menu on ROM load",
                        &saveOverride)) {
      iniFile["advanced"].set<bool>("saveOverride", saveOverride);
    }

    if (ImGui::Combo("Auto Save Type", &saveTypeIndex, saveTypeOptionsPretty,
                     IM_ARRAYSIZE(saveTypeOptionsPretty))) {
      iniFile["advanced"].set<std::string>(
          "defaultSave", std::string(saveTypeOptionsRaw[saveTypeIndex]));
    }

    ImGui::End();

    ImGui::Render();

    ImGui_ImplCitro3D_RenderDrawData(ImGui::GetDrawData(), rev_void(Top),
                                     rev_void(Bottom));
    C3D_FrameEnd(0);
  }
  ImGui_ImplCitro3D_Shutdown();
  ImGui_ImplCtr_Shutdown();
  C3D_Fini();
  return 0;
}
