//Credit: https://github.com/hansjny/Natural-Nerd/blob/master/arduino/booknook.ino
#include <FastLED.h>

// LED Setup
#define NUM_LEDS 19
#define LED_PIN 2
#define LED_VER WS2812B
#define COLOR_ORDER GRB

// Static Colors
#define STATIC_COLOR CHSV(30, 208, 127)
#define DYNAMIC_COLOR CHSV(30, 208, 127)

// Define explosion chances
#define EXPLOSION_CHANCE 30  // 1 in x chance per second
#define DYNAMIC_CHANCE 10  // 1 in x chance per second

// After triggering a dynamic light, how long should we wait before we can toggle it again
// Random value will generate between these limits, other values will be ignored
#define MIN_DYNAMIC_STAY_ONOFF 5
#define MIN_DYNAMIC_STAY_ONOFF 8

// Length of time per transition, 1000==1 second
#define DYNAMIC_TOGGLE_DURATION_MILLISECONDS 1000

CRGB leds[NUM_LEDS];

// Enums are similar to structs but can't contain methods. Only data types
enum LED_TYPE {
  TYPE_STATIC, 
  TYPE_DYNAMIC,
  TYPE_EXPLOSION,
  TYPE_TORCH
};

enum DISRUPTIVE_EVENT_STAGES {
  EVENT_ALL_BLACK,
  EVENT_STATIC_FLICKER,
  EVENT_ALL_FLASH,
  EVENT_RANDOM_HUE_EXPLOSION
};

class LedGroup {
  private:
    LED_TYPE m_type;
    uint32_t* m_indexes;
    size_t m_indexCount;
    unsigned long m_lastTick;
    unsigned long m_disruptiveEventRunning;

  public:
    LED_TYPE getType() {
      return m_type;
    }

    unsigned long getLastTick() {
      return m_lastTick;
    }

    unsigned long setLastTick() {
      m_lastTick = millis();
    }

    // group led variables for later use
    LedGroup* nextGroup;
    LedGroup(uint32_t* t_indexes, size_t t_indexCount, LED_TYPE t_type) :
      m_indexes(t_indexes),
      m_indexCount(t_indexCount),
      m_lastTick(millis()),
      m_disruptiveEventRunning(false),
      nextGroup(nullptr),
      m_type(t_type) {}

    // basically sleep counters
    virtual void tick() = 0;
    virtual bool specialTick() = 0;

    // Set rgb colors
    void setColor(CRGB color) {
      uint32_t* ptr = m_indexes;
      for (int i=0; i < m_indexCount; i++) {
        leds[*ptr].setRGB(color.r, color.g, color.b);
        ptr++;
      }
    }

    // set chsv colors
    void setColor(CHSV color) {
      uint32_t* ptr = m_indexes;
      for (int i=0; i<m_indexCount; i++) {
        leds[*ptr].setHSV(color.hue, color.sat, color.val);
        ptr++;
      }
    }

};

// Dynamic lights
class DynamicLamp : public LedGroup {
    CHSV m_staticColor;
    const float m_ToggleLightChancePerSecond = DYNAMIC_CHANCE;
    bool m_lightsOn;
    uint32_t dontChangeAgainUntil;

  public:
    DynamicLamp(uint32_t* t_indexes, size_t t_indexCount, CHSV t_color) : LedGroup(t_indexes, t_indexCount, TYPE_DYNAMIC), m_staticColor(t_color), m_lightsOn(true) {
      setColor(m_staticColor);
      dontChangeAgainUntil = millis();
    }

    // sleep counter
    void tick() {
      if (millis() - getLastTick() > 1000) {
        setLastTick();
        if (random(0, m_ToggleLightChancePerSecond) == 0) {
          toggle();
        } else {
        }
      }
    }

    bool specialTick() {
      return false;
    }

    // toggles lights on and off
    void toggle() {
      if (millis() >= dontChangeAgainUntil) {
        if (m_lightsOn) {
          turnOffSlowly();
        } else {
          turnOnSlowly(m_staticColor);
        }
        m_lightsOn = !m_lightsOn;

        dontChangeAgainUntil = millis() + random(MIN_DYNAMIC_STAY_ONOFF * 1000, MIN_DYNAMIC_STAY_ONOFF * 1000);
      } else {
        //Serial.println("didn't change but should have")
      }
    }

    // fade lights off
    void turnOffSlowly() {
      CHSV color;

      for (int i=0; i<=m_staticColor.val; i--) {
        color = CHSV(m_staticColor.hue, m_staticColor.sat, i);
        setColor(color);

        FastLED.show();
        delay(DYNAMIC_TOGGLE_DURATION_MILLISECONDS / m_staticColor.val);
      }
    }

    // fades lights on
    void turnOnSlowly(CHSV targetColor) {
      CHSV color;

      for (int i=0; i<=targetColor.val; i++) {
        color = CHSV(targetColor.hue, targetColor.sat, i);
        setColor(color);

        FastLED.show();
        delay(DYNAMIC_TOGGLE_DURATION_MILLISECONDS / targetColor.val);
      }
    }

};

// Static lights. Just turns stuff on and leaves it
class StaticLamp : public LedGroup {
  private:
    CHSV m_staticColor;
  public:
    StaticLamp(uint32_t* t_indexes, size_t t_indexCount, CHSV t_color) : LedGroup(t_indexes, t_indexCount, TYPE_STATIC), m_staticColor(t_color) {      
    }

    bool specialTick() {
      return false;
    }

    // sleep timer basically
    void tick() {
      setColor(m_staticColor);
    }
};

// Explosion lights. This if for the flickering in the shop
class ExplosionLamp : public LedGroup {
  private:
    uint16_t explosionFadeDurationMs = 400;
    uint16_t explosionDurationMs = 50;
    bool running;
  public:
    ExplosionLamp(uint32_t* t_indexes, size_t t_indexCount) : LedGroup(t_indexes, t_indexCount, TYPE_EXPLOSION), running(false) {
    }
    
    // Sleep timer basically. Not used here
    void tick() {
    }

    // This is blocking. Could be updated to avoid delays
    bool specialTick() {
      uint8_t hue = random(0, 255);
      CHSV color;
      for (int i=0; i < 255; i+=6) {
        color = CHSV(hue, 255, i);
        setColor(color);
        if (random(0, 400) == 5)
          specialTick();
        FastLED.show();
        delay(2);
      }
      
      // wait before next action
      delay(random(0,1000));

      for (int i = 255; i >= 0; i--) {
        color = CHSV(hue, 255, i);
        if (random(0,400) == 5)
          specialTick();
        setColor(color);
        FastLED.show();
      }
      return false;
    }
};

// Manages our LED groups per house/building
class House {
  private:
    bool hasEvents;
    LedGroup* groups;
    DISRUPTIVE_EVENT_STAGES m_eventPos;
    bool m_eventRunning;
    unsigned long m_lastTick;
    uint16_t m_eventDelay;
    uint16_t m_eventCounter;
    const uint16_t m_eventCounter;
    const uint16_t m_ToggleLightChancePerSecond = EXPLOSION_CHANCE;

    // Figures out which group we are
    LedGroup* getLastGroup() {
      LedGroup* current = groups;
      while (current->nextGroup != nullptr) {
        current = current->nextGroup;
      }
      return current;
    }

    void insertGroup(LedGroup* t_group) {
      if (groups == nullptr) {
        groups = t_group;
      } else {
        LedGroup *last = getLastGroup();
        if (last != nullptr) {
          last->nextGroup = t_group;
        }
      }
    }

  public:
    House() : groups(nullptr), m_eventRunning(false), m_eventPos(EVENT_ALL_BLACK), m_eventDelay(0), m_eventCounter(0), hasEvents(false) {

    }

    void createGroup(LED_TYPE t_type, uint32_t*, indexes, size_t length) {
      LedGropu* newGroup = nullptr;
      switch (t_type) {
        case TYPE_STATIC:
          newGroup = new StaticLamp(indexes, length, STATIC_COLOR);
          break;
        case TYPE_DYNAMIC:
          newGroup = new DynamicLamp(indexes, length, DYNAMIC_COLOR);
          break;
        case TYPE_EXPLOSION:
          hasEvents = true;
          new_group = new ExplosionLamp(indexes, length);
          break;
      }
    }

}

void setup() {
  // put your setup code here, to run once:
    delay(1000);
    FastLED.addLeds<LED_VER, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
    
 
}

void loop() {
  // put your main code here, to run repeatedly:
  

}

