#include "reactor_fx.h"
#include <math.h>

ReactorFX::ReactorFX()
    : _strip(NUM_LEDS_TOTAL, PIN_LED_DATA, NEO_GRB + NEO_KHZ800),
      _lastFrameMs(0),
      _unibeamDurationMs(1800),
      _phase(0.0f),
      _nvsDirty(false),
      _lastStateChangeMs(0) {
    
    _state.mode = MODE_MK1_CLASSIC;
    _state.brightness = 180;
    _state.speed = 45;
    _state.coreColor = {160, 245, 255};
    _state.ringColor = {0, 170, 255};
    _state.unibeamActive = false;
    _state.unibeamStartMs = 0;
    _state.powerOn = true;
}

void ReactorFX::begin() {
    loadFromNVS();
    _strip.begin();
    for (uint8_t i = 0; i < NUM_LEDS_TOTAL; i++) {
        _pixelBuffer[i] = {0, 0, 0};
    }
    _strip.setBrightness(_state.brightness);
    _strip.clear();
    _strip.show();
    _lastFrameMs = millis();
}

void ReactorFX::loadFromNVS() {
    if (_prefs.begin(NVS_NAMESPACE, true)) {
        uint8_t savedMode = _prefs.getUChar("mode", (uint8_t)MODE_MK1_CLASSIC);
        if (savedMode >= MODE_COUNT) savedMode = (uint8_t)MODE_MK1_CLASSIC;
        _state.mode = (ReactorMode)savedMode;

        _state.brightness = _prefs.getUChar("bright", 180);
        _state.speed = _prefs.getUChar("speed", 45);
        _state.powerOn = _prefs.getBool("power", true);

        _state.coreColor.r = _prefs.getUChar("coreR", 160);
        _state.coreColor.g = _prefs.getUChar("coreG", 245);
        _state.coreColor.b = _prefs.getUChar("coreB", 255);

        _state.ringColor.r = _prefs.getUChar("ringR", 0);
        _state.ringColor.g = _prefs.getUChar("ringG", 170);
        _state.ringColor.b = _prefs.getUChar("ringB", 255);

        _prefs.end();
    }
}

void ReactorFX::commitToNVS() {
    if (_prefs.begin(NVS_NAMESPACE, false)) {
        _prefs.putUChar("mode", (uint8_t)_state.mode);
        _prefs.putUChar("bright", _state.brightness);
        _prefs.putUChar("speed", _state.speed);
        _prefs.putBool("power", _state.powerOn);

        _prefs.putUChar("coreR", _state.coreColor.r);
        _prefs.putUChar("coreG", _state.coreColor.g);
        _prefs.putUChar("coreB", _state.coreColor.b);

        _prefs.putUChar("ringR", _state.ringColor.r);
        _prefs.putUChar("ringG", _state.ringColor.g);
        _prefs.putUChar("ringB", _state.ringColor.b);

        _prefs.end();
        _nvsDirty = false;
    }
}

void ReactorFX::markDirty() {
    _nvsDirty = true;
    _lastStateChangeMs = millis();
}

void ReactorFX::update() {
    uint32_t now = millis();
    if (_nvsDirty && (now - _lastStateChangeMs >= NVS_SAVE_DELAY_MS)) {
        commitToNVS();
    }

    if (now - _lastFrameMs < FX_FRAME_PERIOD_MS) return;

    float dt = (now - _lastFrameMs) / 1000.0f;
    _lastFrameMs = now;

    float speedFactor = 0.4f + (_state.speed / 100.0f) * 4.6f;
    _phase += dt * speedFactor;
    if (_phase > 10000.0f) _phase -= 10000.0f;

    if (_state.unibeamActive) {
        uint32_t elapsed = now - _state.unibeamStartMs;
        if (elapsed >= _unibeamDurationMs) {
            _state.unibeamActive = false;
            _strip.setBrightness(_state.brightness);
        } else {
            renderUnibeam(elapsed);
            _strip.show();
            return;
        }
    }

    if (!_state.powerOn || _state.mode == MODE_OFF) {
        for (uint8_t i = 0; i < NUM_LEDS_TOTAL; i++) {
            setPixel(i, {0, 0, 0}, 0.0f);
        }
        _strip.show();
        return;
    }

    _strip.setBrightness(_state.brightness);

    switch (_state.mode) {
        case MODE_MK1_CLASSIC:   renderMk1Classic(dt); break;
        case MODE_TURBINE_SPIN:  renderTurbineSpin(dt); break;
        case MODE_DUAL_HELIX:    renderDualHelix(dt); break;
        case MODE_HEARTBEAT:     renderHeartbeat(dt); break;
        case MODE_OVERLOAD:      renderOverload(dt); break;
        case MODE_SYNTH_WAVE:    renderSynthWave(dt); break;
        case MODE_CUSTOM_STATIC: renderCustomStatic(); break;
        default:                 renderMk1Classic(dt); break;
    }

    _strip.show();
}

void ReactorFX::renderMk1Classic(float dt) {
    float breath = 0.675f + 0.325f * sinf(_phase * 1.8f);
    float corePulse = 0.88f + 0.12f * sinf(_phase * 5.0f);

    setPixel(LED_CORE_IDX, _state.coreColor, corePulse);
    for (uint8_t i = 0; i < NUM_RING_LEDS; i++) {
        float localWave = 0.85f + 0.15f * sinf(_phase * 2.0f + (i * PI / 4.0f));
        setPixel(LED_RING_START_IDX + i, _state.ringColor, breath * localWave);
    }
}

void ReactorFX::renderTurbineSpin(float dt) {
    float coreBeat = 0.75f + 0.25f * sinf(_phase * 8.0f);
    setPixel(LED_CORE_IDX, _state.coreColor, coreBeat);

    float headPos = fmodf(_phase * 2.2f, (float)NUM_RING_LEDS);
    for (uint8_t i = 0; i < NUM_RING_LEDS; i++) {
        float dist = headPos - (float)i;
        if (dist < 0.0f) dist += (float)NUM_RING_LEDS;

        float intensity = constrain(expf(-dist * 0.65f), 0.08f, 1.0f);
        RGBColor c = blend(_state.ringColor, _state.coreColor, powf(intensity, 3.0f));
        setPixel(LED_RING_START_IDX + i, c, intensity);
    }
}

void ReactorFX::renderDualHelix(float dt) {
    float corePulse = 0.8f + 0.2f * cosf(_phase * 4.0f);
    setPixel(LED_CORE_IDX, _state.coreColor, corePulse);

    for (uint8_t i = 0; i < NUM_RING_LEDS; i++) {
        float angle = (i * 2.0f * PI) / NUM_RING_LEDS;
        float wave1 = 0.5f + 0.5f * sinf(angle * 2.0f - _phase * 3.0f);
        float wave2 = 0.5f + 0.5f * sinf(angle * 2.0f + _phase * 3.0f);

        RGBColor c = blend(_state.ringColor, _state.coreColor, wave2 * 0.7f);
        float brightness = constrain(0.15f + 0.85f * powf(wave1, 2.0f), 0.1f, 1.0f);
        setPixel(LED_RING_START_IDX + i, c, brightness);
    }
}

void ReactorFX::renderHeartbeat(float dt) {
    float cycle = fmodf(_phase * 0.75f, 1.0f);
    float beat = 0.15f;

    if (cycle < 0.12f) {
        beat = 0.15f + 0.85f * sinf((cycle / 0.12f) * PI);
    } else if (cycle >= 0.18f && cycle < 0.32f) {
        beat = 0.15f + 0.65f * sinf(((cycle - 0.18f) / 0.14f) * PI);
    } else {
        beat = 0.15f + 0.10f * expf(-(cycle - 0.32f) * 5.0f);
    }

    setPixel(LED_CORE_IDX, _state.coreColor, constrain(beat * 1.15f, 0.2f, 1.0f));
    for (uint8_t i = 0; i < NUM_RING_LEDS; i++) {
        setPixel(LED_RING_START_IDX + i, _state.ringColor, beat * 0.85f);
    }
}

void ReactorFX::renderOverload(float dt) {
    uint8_t coreFlicker = random(160, 255);
    setPixel(LED_CORE_IDX, _state.coreColor, coreFlicker / 255.0f);

    for (uint8_t i = 0; i < NUM_RING_LEDS; i++) {
        if (random(0, 100) < 18) {
            setPixel(LED_RING_START_IDX + i, {255, 255, 255}, 1.0f);
        } else {
            setPixel(LED_RING_START_IDX + i, _state.ringColor, random(35, 100) / 100.0f);
        }
    }
}

void ReactorFX::renderSynthWave(float dt) {
    uint8_t baseHue = (uint8_t)(fmodf(_phase * 25.0f, 255.0f));
    setPixel(LED_CORE_IDX, wheel(baseHue + 64), 1.0f);

    for (uint8_t i = 0; i < NUM_RING_LEDS; i++) {
        setPixel(LED_RING_START_IDX + i, wheel(baseHue + (i * 256 / NUM_RING_LEDS)), 0.9f);
    }
}

void ReactorFX::renderCustomStatic() {
    setPixel(LED_CORE_IDX, _state.coreColor, 1.0f);
    for (uint8_t i = 0; i < NUM_RING_LEDS; i++) {
        setPixel(LED_RING_START_IDX + i, _state.ringColor, 1.0f);
    }
}

void ReactorFX::renderUnibeam(uint32_t elapsedMs) {
    _strip.setBrightness(255);
    float progress = (float)elapsedMs / (float)_unibeamDurationMs;
    
    if (progress < 0.25f) {
        float charge = progress / 0.25f;
        setPixel(LED_CORE_IDX, {255, 255, 255}, charge);
        for (uint8_t i = 0; i < NUM_RING_LEDS; i++) {
            setPixel(LED_RING_START_IDX + i, _state.coreColor, charge);
        }
    } else if (progress < 0.75f) {
        bool flash = ((elapsedMs / 35) % 2) == 0;
        RGBColor blast = flash ? RGBColor{255, 255, 255} : RGBColor{180, 245, 255};
        for (uint8_t i = 0; i < NUM_LEDS_TOTAL; i++) setPixel(i, blast, 1.0f);
    } else {
        float cool = 1.0f - ((progress - 0.75f) / 0.25f);
        for (uint8_t i = 0; i < NUM_LEDS_TOTAL; i++) setPixel(i, _state.coreColor, cool);
    }
}

void ReactorFX::setMode(ReactorMode mode) {
    if (mode < MODE_COUNT) {
        _state.mode = mode;
        if (mode != MODE_OFF) _state.powerOn = true;
        markDirty();
    }
}

void ReactorFX::setBrightness(uint8_t b) { _state.brightness = b; markDirty(); }
void ReactorFX::setSpeed(uint8_t s) { _state.speed = constrain(s, 1, 100); markDirty(); }
void ReactorFX::setCoreColor(uint8_t r, uint8_t g, uint8_t b) { _state.coreColor = {r, g, b}; markDirty(); }
void ReactorFX::setRingColor(uint8_t r, uint8_t g, uint8_t b) { _state.ringColor = {r, g, b}; markDirty(); }
void ReactorFX::setPower(bool on) { _state.powerOn = on; markDirty(); }

void ReactorFX::triggerUnibeam(uint16_t durationMs) {
    _unibeamDurationMs = durationMs;
    _state.unibeamStartMs = millis();
    _state.unibeamActive = true;
}

void ReactorFX::applyPreset(uint8_t presetId) {
    _state.powerOn = true;
    switch (presetId) {
        case 0: _state.coreColor = {175, 250, 255}; _state.ringColor = {0, 165, 255}; _state.mode = MODE_MK1_CLASSIC; _state.speed = 40; break;
        case 1: _state.coreColor = {255, 255, 255}; _state.ringColor = {0, 210, 255}; _state.mode = MODE_TURBINE_SPIN; _state.speed = 65; break;
        case 2: _state.coreColor = {255, 200, 30};  _state.ringColor = {255, 25, 0};   _state.mode = MODE_DUAL_HELIX;    _state.speed = 55; break;
        case 3: _state.coreColor = {255, 70, 0};    _state.ringColor = {220, 0, 15};   _state.mode = MODE_OVERLOAD;      _state.speed = 80; break;
        case 4: _state.coreColor = {80, 0, 255};    _state.ringColor = {15, 30, 180};  _state.mode = MODE_HEARTBEAT;     _state.speed = 35; break;
    }
    markDirty();
}

void ReactorFX::setPixel(uint8_t idx, RGBColor c, float scale) {
    if (idx >= NUM_LEDS_TOTAL) return;
    scale = constrain(scale, 0.0f, 1.0f);
    uint8_t r = (uint8_t)(c.r * scale);
    uint8_t g = (uint8_t)(c.g * scale);
    uint8_t b = (uint8_t)(c.b * scale);
    _pixelBuffer[idx] = {r, g, b};
    _strip.setPixelColor(idx, _strip.Color(r, g, b));
}

RGBColor ReactorFX::blend(RGBColor c1, RGBColor c2, float ratio) {
    ratio = constrain(ratio, 0.0f, 1.0f);
    return {(uint8_t)(c1.r + (c2.r - c1.r) * ratio), (uint8_t)(c1.g + (c2.g - c1.g) * ratio), (uint8_t)(c1.b + (c2.b - c1.b) * ratio)};
}

RGBColor ReactorFX::wheel(uint8_t pos) {
    pos = 255 - pos;
    if (pos < 85) return {(uint8_t)(255 - pos * 3), 0, (uint8_t)(pos * 3)};
    if (pos < 170) { pos -= 85; return {0, (uint8_t)(pos * 3), (uint8_t)(255 - pos * 3)}; }
    pos -= 170;
    return {(uint8_t)(pos * 3), (uint8_t)(255 - pos * 3), 0};
}

void ReactorFX::getPixelHexArray(String* outHexArray) const {
    char buf[8];
    uint8_t brightness = _state.unibeamActive ? 255 : _state.brightness;
    float masterScale = (!_state.powerOn && !_state.unibeamActive)
        ? 0.0f
        : (brightness == 255 ? 1.0f : (brightness + 1) / 256.0f);
    for (uint8_t i = 0; i < NUM_LEDS_TOTAL; i++) {
        uint8_t r = (uint8_t)(_pixelBuffer[i].r * masterScale);
        uint8_t g = (uint8_t)(_pixelBuffer[i].g * masterScale);
        uint8_t b = (uint8_t)(_pixelBuffer[i].b * masterScale);
        snprintf(buf, sizeof(buf), "#%02X%02X%02X", r, g, b);
        outHexArray[i] = String(buf);
    }
}
