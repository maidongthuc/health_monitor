#ifndef SERIAL_HANDLER_H
#define SERIAL_HANDLER_H

#include "Arduino.h"
#include "ArduinoJson.h"

class SerialHandler {
    public:
        SerialHandler();
        void syncData(JsonObject data);
    private:

};

#endif