#include "MonomeSerialDevice.h"
#include <Arduino.h>

// Grid event methods
bool MonomeEventQueue::gridEventAvailable()
{
    return gridEventCount > 0;
}

MonomeGridEvent MonomeEventQueue::readGridEvent()
{
    if (gridEventCount > 0)
    {
        MonomeGridEvent event = gridEvents[gridFirstEvent];
        gridFirstEvent = (gridFirstEvent + 1) % MAXEVENTCOUNT;
        gridEventCount--;
        return event;
    }
    return emptyGridEvent;
}

MonomeGridEvent MonomeEventQueue::sendGridKey()
{
    // This probably should return an event, but the implementation isn't clear
    // Return empty event for now
    return emptyGridEvent;
}

// Arc event methods
bool MonomeEventQueue::arcEventAvailable()
{
    return arcEventCount > 0;
}

MonomeArcEvent MonomeEventQueue::readArcEvent()
{
    if (arcEventCount > 0)
    {
        MonomeArcEvent event = arcEvents[arcFirstEvent];
        arcFirstEvent = (arcFirstEvent + 1) % MAXEVENTCOUNT;
        arcEventCount--;
        return event;
    }
    return emptyArcEvent;
}

MonomeArcEvent MonomeEventQueue::sendArcDelta()
{
    // Return empty event for now
    return emptyArcEvent;
}

MonomeArcEvent MonomeEventQueue::sendArcKey()
{
    // Return empty event for now
    return emptyArcEvent;
}

// Event handling methods
void MonomeEventQueue::addGridEvent(uint8_t x, uint8_t y, uint8_t pressed)
{
    if (gridEventCount < MAXEVENTCOUNT)
    {
        int nextPos = (gridFirstEvent + gridEventCount) % MAXEVENTCOUNT;
        gridEvents[nextPos].x = x;
        gridEvents[nextPos].y = y;
        gridEvents[nextPos].pressed = pressed;
        gridEventCount++;
    }
}

void MonomeEventQueue::sendGridKey(uint8_t x, uint8_t y, uint8_t pressed)
{
    // Send grid key event directly to serial
    Serial.write((uint8_t)(pressed ? 0x21 : 0x20));
    Serial.write(x);
    Serial.write(y);
    Serial.write(pressed);
}

void MonomeEventQueue::addArcEvent(uint8_t index, int8_t delta)
{
    if (arcEventCount < MAXEVENTCOUNT)
    {
        int nextPos = (arcFirstEvent + arcEventCount) % MAXEVENTCOUNT;
        arcEvents[nextPos].index = index;
        arcEvents[nextPos].delta = delta;
        arcEventCount++;
    }
}

void MonomeEventQueue::sendArcDelta(uint8_t index, int8_t delta)
{
    // Send arc delta event directly to serial
    Serial.write((uint8_t)0x50);
    Serial.write(index);
    Serial.write(delta);
}

void MonomeEventQueue::sendArcKey(uint8_t index, uint8_t pressed)
{
    // Send arc key event directly to serial
    Serial.write((uint8_t)(pressed ? 0x52 : 0x51));
    Serial.write(index);
    Serial.write(pressed);
}

void MonomeEventQueue::sendTiltEvent(uint8_t n, uint8_t xh, uint8_t xl, uint8_t yh, uint8_t yl, uint8_t zh, uint8_t zl)
{
    // Send tilt event directly to serial
    Serial.write((uint8_t)0x81);
    Serial.write(n);
    Serial.write(xh);
    Serial.write(xl);
    Serial.write(yh);
    Serial.write(yl);
    Serial.write(zh);
    Serial.write(zl);
}