#pragma once
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <Preferences.h>
#include "config.h"

enum ReactorMode : uint8_t {
    MODE_MK1_CLASSIC = 0,     // Steady Core + subtle Breathing Outer Ring
    MODE_TURBINE_SPIN = 1,    // High-speed plasma turbine rotation + Core pulse
    MODE_DUAL_HELIX = 2,      // Opposite spinning dual nodes on outer ring
    MODE_HEARTBEAT = 3,       // Stark cardiac rhythm (Lub-Dub)
    MODE_OVERLOAD = 4,        // Unstable plasma flicker & random arc flashes
    MODE_SYNTH_WAVE = 5,      // Smooth radial color gradient shift
    MODE_CUSTOM_STATIC = 6,   // Independent static Core & Ring colors
    MODE_OFF = 7,             // Standby Mode
    MODE_COUNT = 8
};

struct RGBColor {
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

struct ReactorState {
    ReactorMode mode;
    uint8_t brightness;       // 0 - 255 master brightness
    uint8_t speed;            // 1 - 100 animation speed scalar
    RGBColor coreColor;       // Primary color for D3 (Center)
    RGBColor ringColor;       // Primary color for D4..D11 (Outer Ring)
    bool unibeamActive;
    uint32_t unibeamStartMs;
    bool powerOn;
};

class ReactorFX {
public:
    ReactorFX();
    void begin();
    void update();

    void setMode(ReactorMode mode);
    void setBrightness(uint8_t b);
    void setSpeed(uint8_t s);
    void setCoreColor(uint8_t r, uint8_t g, uint8_t b);
    void setRingColor(uint8_t r, uint8_t g, uint8_t b);
    void setPower(bool on);
    void triggerUnibeam(uint16_t durationMs = 1800);
    void applyPreset(uint8_t presetId);

    const ReactorState& getState() const { return _state; }
    void getPixelHexArray(String* outHexArray) const;

private:
    Adafruit_NeoPixel _strip;
    Preferences _prefs;
    ReactorState _state;
    uint32_t _lastFrameMs;
    uint16_t _unibeamDurationMs;
    float _phase;

    bool _nvsDirty;
    uint32_t _lastStateChangeMs;
    RGBColor _pixelBuffer[NUM_LEDS_TOTAL];

    void loadFromNVS();
    void commitToNVS();
    void markDirty();

    void renderMk1Classic(float dt);
    void renderTurbineSpin(float dt);
    void renderDualHelix(float dt);
    void renderHeartbeat(float dt);
    void renderOverload(float dt);
    void renderSynthWave(float dt);
    void renderCustomStatic();
    void renderUnibeam(uint32_t elapsedMs);

    void setPixel(uint8_t idx, RGBColor c, float scale = 1.0f);
    RGBColor blend(RGBColor c1, RGBColor c2, float ratio);
    RGBColor wheel(uint8_t pos);
};
