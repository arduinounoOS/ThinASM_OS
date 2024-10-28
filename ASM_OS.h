#define MAX_TASKS 4

typedef struct {
  volatile uint8_t* sp;
  void (*function)();
  volatile uint8_t* stack;
  uint16_t stackSize;
  uint16_t highWaterMark; // Add high watermark to task structure
} Task;

uint8_t stack0[128];
uint8_t stack1[50];
uint8_t stack2[128];
uint8_t stack3[50];
uint8_t stack4[1];

const uint16_t stackSizes[5] = { sizeof(stack0), sizeof(stack1), sizeof(stack2), sizeof(stack3), sizeof(stack4) };
volatile uint8_t* stacks[5] = { stack0, stack1, stack2, stack3, stack4 };

volatile Task tasks[MAX_TASKS];
volatile uint8_t currentTask = 0;
volatile uint8_t taskCount = 0;

void createTask(void (*function)(), uint8_t taskIndex) {
  if (taskIndex < MAX_TASKS && taskCount < MAX_TASKS) {
    tasks[taskIndex].stack = stacks[taskIndex];
    tasks[taskIndex].function = function;
    tasks[taskIndex].stackSize = stackSizes[taskIndex];
    tasks[taskIndex].sp = tasks[taskIndex].stack + tasks[taskIndex].stackSize - 1;
    tasks[taskIndex].highWaterMark = tasks[taskIndex].stackSize; // Initialize high water mark

    memset((void*)tasks[taskIndex].stack, 0xAA, tasks[taskIndex].stackSize); // Fill stack with known pattern

    uint16_t pc = (uint16_t)function;
    *(tasks[taskIndex].sp--) = (uint8_t)(pc & 0xFF);
    *(tasks[taskIndex].sp--) = (uint8_t)(pc >> 8);
    *(tasks[taskIndex].sp--) = 0x00; // R0
    *(tasks[taskIndex].sp--) = 0x80; // SREG
    for (int i = 1; i <= 31; i++) {
        *(tasks[taskIndex].sp--) = 0x00;
    }
    taskCount++;
  } else {
    Serial.println(F("Task index out of bounds or maximum tasks reached"));
  }
}

ISR(TIMER1_COMPA_vect, ISR_NAKED) { 
  asm volatile (
    "push r0 \n"
    "in r0, __SREG__ \n"
    "push r0 \n"
    "push r1 \n"
    "push r2 \n"
    "push r3 \n"
    "push r4 \n"
    "push r5 \n"
    "push r6 \n"
    "push r7 \n"
    "push r8 \n"
    "push r9 \n"
    "push r10 \n"
    "push r11 \n"
    "push r12 \n"
    "push r13 \n"
    "push r14 \n"
    "push r15 \n"
    "push r16 \n"
    "push r17 \n"
    "push r18 \n"
    "push r19 \n"
    "push r20 \n"
    "push r21 \n"
    "push r22 \n"
    "push r23 \n"
    "push r24 \n"
    "push r25 \n"
    "push r26 \n"
    "push r27 \n"
    "push r28 \n"
    "push r29 \n"
    "push r30 \n"
    "push r31 \n"
  );

  tasks[currentTask].sp = (uint8_t*)SP; // Store current stack pointer
  currentTask = (currentTask + 1) % taskCount; // Switch to the next task
  SP = (uint16_t)tasks[currentTask].sp; // Load the new task's stack pointer

  asm volatile (
    "pop r31 \n"
    "pop r30 \n"
    "pop r29 \n"
    "pop r28 \n"
    "pop r27 \n"
    "pop r26 \n"
    "pop r25 \n"
    "pop r24 \n"
    "pop r23 \n"
    "pop r22 \n"
    "pop r21 \n"
    "pop r20 \n"
    "pop r19 \n"
    "pop r18 \n"
    "pop r17 \n"
    "pop r16 \n"
    "pop r15 \n"
    "pop r14 \n"
    "pop r13 \n"
    "pop r12 \n"
    "pop r11 \n"
    "pop r10 \n"
    "pop r9 \n"
    "pop r8 \n"
    "pop r7 \n"
    "pop r6 \n"
    "pop r5 \n"
    "pop r4 \n"
    "pop r3 \n"
    "pop r2 \n"
    "pop r1 \n"
    "pop r0 \n"
    "out __SREG__, r0 \n"
    "pop r0 \n"
  );

  reti(); // Return from interrupt
}

void StartOS() {
  SP = (uint16_t)tasks[currentTask].sp + 33; 

  TCNT1 = 0; // Initialize counter value
  TCCR1A = 0; // Normal mode
  TCCR1B = 0; // Ensure TCCR1B is clear
  TCCR1B |= (1 << WGM12); // Configure CTC mode
  TCCR1B |= (1 << CS11) | (1 << CS10); // Set prescaler to 64
  OCR1A = 125; // Set compare match value 250 for ~1ms at 16MHz
  TIMSK1 |= (1 << OCIE1A); // Enable Timer1 compare match interrupt
  reti();
}

void yield() {
  cli(); 
     TIFR1 |= (1 << OCF1A);
  sei(); 
}

#define PRINT_INTERVAL 5000  // Define the print interval as a macro

void stackMonitorTask() {
  static uint32_t lastPrintTime = 0;  // Keep track of the last print time with a static variable

  for (;;) {
   // uint32_t currentTime = millis();

     if ((millis() - lastPrintTime) >= PRINT_INTERVAL) {  // Use the macro here
      uint32_t totalSeconds = millis() / 1000; 

      // Print uptime in a single line
      Serial.print(F("\n***Monitor Task***\nUptime: "));
      Serial.print(totalSeconds / 3600);
      Serial.print(F("h "));
      Serial.print((totalSeconds % 3600) / 60);
      Serial.print(F("m "));
      Serial.print(totalSeconds % 60);
      Serial.println(F("s"));

      Serial.println(F("Stack high water marks:"));
      for (uint8_t i = 0; i < taskCount; i++) {
        uint16_t used = 0;

        for (uint16_t j = 0; j < tasks[i].stackSize; j++) {
          if (tasks[i].stack[j] != 0xAA) {
            used = j;
            break;
          }
        }
        tasks[i].highWaterMark = tasks[i].stackSize - used;

        // Direct calculation and print for each task
        Serial.print(F("Task "));
        Serial.print(i);
        Serial.print(F(": "));
        Serial.print(tasks[i].highWaterMark);  // Calculating directly instead of assigning to highWaterMark
        Serial.println(F(" bytes used"));
      }

      lastPrintTime += PRINT_INTERVAL;  // Update using the macro
      Serial.print(F("> "));
      yield();
    }
    yield();
  }
}