//Credit: https://github.com/hansjny/Natural-Nerd/blob/master/arduino/booknook.ino
#include <FastLED.h>

// LED Setup
#define NUM_LEDS 20
#define LED_PIN 2
#define LED_VER WS2812B
#define COLOR_ORDER GRB

// Static Colors
#define STATIC_COLOR CHSV(30, 208, 127)
#define DYNAMIC_COLOR CHSV(30, 208, 127)
#define MURDER_COLOR CHSV(95, 255, 127)

// Define explosion chances
#define EXPLOSION_CHANCE 20  // 1 in x chance per second
#define DYNAMIC_CHANCE 10  // 1 in x chance per second
#define MURDER_CHANCE 10 // 1 in x chance per second

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
  TYPE_TORCH,
  TYPE_MURDER
};

enum DISRUPTIVE_EVENT_STAGES {
  EVENT_ALL_BLACK,
  EVENT_STATIC_FLICKER,
  EVENT_ALL_FLASH,
  EVENT_RANDOM_HUE_EXPLOSION,
  EVENT_MURDER_BLACK,
  EVENT_RANDOM_MURDER
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

      for (int i = m_staticColor.val; i>=0; i--) {
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

// Class for Avade Kedavera in Knockturn Alley
// Reusing a bunch of the explosion lamp stuff
// Will basically stay off til it gets cast
class MurderLamp : public LedGroup {
  private:
    uint16_t explosionFadeDurationMs = 400;
    uint16_t explosionDurationMs = 50;
    bool running;
  public:
    MurderLamp(uint32_t* t_indexes, size_t t_indexCount) : LedGroup(t_indexes, t_indexCount, TYPE_MURDER), running(false) {
    }

    void tick() {
    }

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

    // Creates a gropu to manage and assigns lights in them
    void createGroup(LED_TYPE t_type, uint32_t* indexes, size_t length) {
      LedGroup* newGroup = nullptr;
      switch (t_type) {
        case TYPE_STATIC:
          newGroup = new StaticLamp(indexes, length, STATIC_COLOR);
          break;
        case TYPE_DYNAMIC:
          newGroup = new DynamicLamp(indexes, length, DYNAMIC_COLOR);
          break;
        case TYPE_EXPLOSION:
          hasEvents = true;
          newGroup = new ExplosionLamp(indexes, length);
          break;
        case TYPE_MURDER:
          hasEvents = true;
          newGroup = new MurderLamp(indexes, length);
          break;
      }
      if (newGroup != nullptr) {
        insertGroup(newGroup);
      }
    }

    // Sets all the colors in a group
    void setAllColor(CHSV color) {
      LedGroup* current = groups;
      while (current != nullptr) {
        current->setColor(color);
        current = current->nextGroup;
      }
    }

    bool normalTick() {
      LedGroup* current = groups;
      while (current != nullptr) {
        current->tick();
        current = current->nextGroup;
      }
      return true;
    }

    bool specialTick(LED_TYPE type) {
      LedGroup* current = groups;
      while (current != nullptr) {
        if (current->getType() == type) {
          current->specialTick();
        }
        current = current->nextGroup;
      }
    }

    void eventTick() {
      if(millis() - m_lastTick > m_eventDelay) {
        m_lastTick = millis();
        
        switch (m_eventPos) {
          case EVENT_ALL_BLACK:
            m_eventDelay = 500;
            setAllColor(CHSV(0, 0, 0));
            m_eventPos = EVENT_STATIC_FLICKER;
            break;

          // This is blocking. Need to change to timers not delays.
          case EVENT_STATIC_FLICKER:
            for (int i=0; i<random(3,6); i++) {
              setAllColor(STATIC_COLOR);
              FastLED.show();
              delay(random(0, 250));
              setAllColor(CHSV(0, 0, 0));
              FastLED.show();
              delay(random(0, 450));
            }
            setAllColor(CHSV(0, 0, 0));
            delay(2000);
            m_eventPos = EVENT_RANDOM_HUE_EXPLOSION;
            break;

          case EVENT_RANDOM_HUE_EXPLOSION:
            m_lastTick = millis();
            specialTick(TYPE_EXPLOSION);
            resetEvent();
            break;

          case EVENT_RANDOM_MURDER:
            m_lastTick = millis();
            specialTick(TYPE_MURDER);
            resetEvent();
            break;
        }
      }
    }

     void murderTick() {
      if(millis() - m_lastTick > m_eventDelay) {
        m_lastTick = millis();
        
        switch (m_eventPos) {
          case EVENT_ALL_BLACK:
            m_eventDelay = 500;
            setAllColor(CHSV(0, 0, 0));
            m_eventPos = EVENT_STATIC_FLICKER;
            break;

          // This is blocking. Need to change to timers not delays.
          case EVENT_STATIC_FLICKER:
            for (int i=0; i<random(3,6); i++) {
              setAllColor(MURDER_COLOR);
              FastLED.show();
              delay(random(0, 250));
              setAllColor(CHSV(0, 0, 0));
              FastLED.show();
              delay(random(0, 450));
            }
            setAllColor(CHSV(0, 0, 0));
            delay(2000);
            m_eventPos = EVENT_RANDOM_HUE_EXPLOSION;
            break;

          case EVENT_RANDOM_HUE_EXPLOSION:
            m_lastTick = millis();
            specialTick(TYPE_EXPLOSION);
            resetEvent();
            break;

          case EVENT_RANDOM_MURDER:
            m_lastTick = millis();
            specialTick(TYPE_MURDER);
            resetEvent();
            break;
        }
      }
     }

    // Fades to black 
    void resetEvent() {
      m_eventRunning = false;
      m_eventPos = EVENT_ALL_BLACK;
    }

    bool isEventRunning() {
      if (!hasEvents)
        return false;

      if (!m_eventRunning) {
        if (millis() - m_lastTick > 1000) {
          m_lastTick = millis();

          if (random(0, m_ToggleLightChancePerSecond) == 0) {
            m_eventRunning = true;
          }

        }
      }
      return m_eventRunning;
    }

    void tick() {
      if (isEventRunning() == false) {
        normalTick();
      } else {
        eventTick();
      }
    }

    void mtick() {
      if (isEventRunning() == false) {
        normalTick();
      } else {
        murderTick();
      }
    }
};

// Defines how many houses we will be using
// Ollivanders, Scribbulous, Quality Quidditch Supplies
#define NUM_HOUSES 4
//#define NUM_HOUSES 1
House* houses[NUM_HOUSES];

// Defines which leds go into which house
// Ollivanders
// LEDs 0-6
/*
uint32_t house1_floor[] = {0, 2, 4, 6}; 
uint32_t house1_room1[] = {3};
uint32_t house1_explosion[] = {1, 5, 0, 6};
*/
uint32_t house1_floor[] = {0, 2, 3, 5}; 
uint32_t house1_room1[] = {6};
uint32_t house1_room2[] = {7};
uint32_t house1_explosion[] = {1, 4, 0, 5};

// Quality Quidditch Supplies
// LEDs 8-14
uint32_t house2_floor[] = {8, 9, 10};
uint32_t house2_room1[] = {11};
uint32_t house2_room2[] = {12};
uint32_t house2_room3[] = {13};
uint32_t house2_room4[] = {14};

// Quality Quidditch Supplies
// LEDs 16-19
uint32_t house3_floor[] = {16, 17};
uint32_t house3_room1[] = {18};
uint32_t house3_room2[] = {19};

// 
// LEDs 15
uint32_t alley[] = {15};

void createGroup() {

}

void setup() {
  // put your setup code here, to run once:
  randomSeed(analogRead(0));
  Serial.begin(9600);
  while (!Serial);

  //FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  //FastLED.addLeds<LED_VER, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);

  FastLED.addLeds<NEOPIXEL, LED_PIN>(leds, NUM_LEDS);
  // Which house and which lamps are which (static/dynamic/explosion)
  // House 0 == Ollivanders
  houses[0] = new House();
  houses[0]->createGroup(TYPE_STATIC, &house1_floor[0], (size_t)(sizeof(house1_floor) / sizeof(house1_floor[0])));
  houses[0]->createGroup(TYPE_DYNAMIC, &house1_room1[0], (size_t)(sizeof(house1_room1) / sizeof(house1_room1[0])));
  houses[0]->createGroup(TYPE_DYNAMIC, &house1_room2[0], (size_t)(sizeof(house1_room2) / sizeof(house1_room2[0])));
  houses[0]->createGroup(TYPE_EXPLOSION, &house1_explosion[0], (size_t)(sizeof(house1_explosion) / sizeof(house1_explosion[0])));


  // House 1
  // Quality Quidditch Supplies
  houses[1] = new House();
  houses[1]->createGroup(TYPE_STATIC, &house2_floor[0], (size_t)(sizeof(house2_floor) / sizeof(house2_floor[0])));
  houses[1]->createGroup(TYPE_DYNAMIC, &house2_room1[0], (size_t)(sizeof(house2_room1) / sizeof(house2_room1[0])));
  houses[1]->createGroup(TYPE_DYNAMIC, &house2_room2[0], (size_t)(sizeof(house2_room2) / sizeof(house2_room2[0])));
  houses[1]->createGroup(TYPE_DYNAMIC, &house2_room3[0], (size_t)(sizeof(house2_room3) / sizeof(house2_room3[0])));
  houses[1]->createGroup(TYPE_DYNAMIC, &house2_room4[0], (size_t)(sizeof(house2_room4) / sizeof(house2_room4[0])));
 
  // House 2
  // Scribbulous
  houses[2] = new House();
  houses[2]->createGroup(TYPE_STATIC, &house3_floor[0], (size_t)(sizeof(house3_floor) / sizeof(house3_floor[0])));
  houses[2]->createGroup(TYPE_DYNAMIC, &house3_room1[0], (size_t)(sizeof(house3_room1) / sizeof(house3_room1[0])));
  houses[2]->createGroup(TYPE_DYNAMIC, &house3_room2[0], (size_t)(sizeof(house3_room2) / sizeof(house3_room2[0])));

  // House 3 == Knockturn Alley
  // Custom call to mtick instead of tick
  houses[3] = new House();
  //houses[3]->createGroup(TYPE_STATIC, &streetlight[0], (size_t)(sizeof(streetlight) / sizeof(streetlight[0])));
  houses[3]->createGroup(TYPE_MURDER, &alley[0], (size_t)(sizeof(alley) / sizeof(alley[0])));

}

void loop() {
  // put your main code here, to run repeatedly:
  // Lights up each individual house
  for (int i=0; i<NUM_HOUSES-1; i++) {
    houses[i]->tick();
  }
  houses[3]->mtick();
  FastLED.show();
  delay(30);
}

