#include <Arduino.h>
#include "ASM_OS.h"
#include <EEPROM.h>
#include <avr/pgmspace.h>


volatile uint32_t ledDelay = 500;     // Default LED blink delay in milliseconds

const char helpText[] PROGMEM = 
"Available commands:\n"
"  status          - Shows the system status.\n"
"  help            - Lists all available commands.\n"
"  setdelay <ms>   - Sets the LED blink delay in milliseconds.\n"
"  stack           - Shows the stack high water mark for each task.\n"
"  list            - Lists all files in EEPROM.\n"
"  read <index>    - Reads a file from EEPROM.\n"
"  write <index> <data> - Writes data to a file in EEPROM.\n"
"  delete <index>  - Deletes a file from EEPROM.\n";

const char* str;

void serialShellTask() {
    #define COMMAND_BUFFER_SIZE 17
    char commandBuffer[COMMAND_BUFFER_SIZE];
    uint8_t bufferIndex = 0;

    Serial.print(F("> "));

    for (;;) {
        if (Serial.available() > 0) {
            char incomingChar = Serial.read();
            Serial.write(incomingChar);

            if (incomingChar == '\n' || incomingChar == '\r') {
                commandBuffer[bufferIndex] = '\0';
                if (strcmp(commandBuffer, "status") == 0) {
                    Serial.println(F("System is running."));
                } else if (strcmp(commandBuffer, "help") == 0) {
                    char c;
                    str = helpText;
                    while ((c = pgm_read_byte(str++)) != 0) {
                        Serial.print(c);
                    }
                } else if (strncmp(commandBuffer, "setdelay ", 9) == 0) {
                    long delayValue = atol(&commandBuffer[9]);
                    if (delayValue >= 10) {
                        ledDelay = delayValue;
                        Serial.print(F("LED delay set to "));
                        Serial.print(ledDelay);
                        Serial.println(F(" milliseconds."));
                    } else {
                        Serial.println(F("Error: delay must be at least 10 milliseconds."));
                    }
                } else if (strcmp(commandBuffer, "stack") == 0) {
                    for (uint8_t i = 0; i < taskCount; i++) {
                        Serial.print(F("Task "));
                        Serial.print(i);
                        Serial.print(F(": "));
                        Serial.print(tasks[i].highWaterMark); // Directly access the high watermark
                        Serial.println(F(" bytes used"));
                    }
                } else {
                    Serial.println(F("Unknown command. Type 'help' for a list of commands."));
                }
                bufferIndex = 0;
                Serial.print(F("> "));
            } else if (bufferIndex < COMMAND_BUFFER_SIZE - 1) {
                commandBuffer[bufferIndex++] = incomingChar;
            } else {
                Serial.println(F("Error: command buffer overflow."));
                bufferIndex = 0;
                Serial.print(F("> "));
            }
            yield();
        }
        yield();
    }
}

#define LED_PIN 13 // Onboard LED pin on Arduino Uno
void Task2() {
    pinMode(LED_PIN, OUTPUT);
    static uint32_t lastToggleTime = 0;
    static bool ledState = LOW;

    for (;;) {
        if ((millis() - lastToggleTime) >= ledDelay) {
            ledState = !ledState;
            digitalWrite(LED_PIN, ledState);
            lastToggleTime = millis();
            yield();
        }
        yield(); // yield is always inside FOR LOOP!
    }
}

#include <avr/io.h> // Ensure correct header is included for AVR macros

void setup() {
    cli();
    Serial.begin(115200);
    Serial.println(F("\nSerial begun")); // Print the boot message

    createTask(serialShellTask, 0);
    createTask(Task2, 1);
    createTask(stackMonitorTask, 2); // Add the new task for stack monitoring
    createTask(Task2, 3); // Add the new task for stack monitoring
    //createTask(stackMonitorTask, 4); // Add the new task for stack monitoring

    Serial.println(F("Starting kernel")); // Print the boot message
    StartOS();

    for (;;) {
        // Main loop
    }
}

void loop () { for (;;){}}
