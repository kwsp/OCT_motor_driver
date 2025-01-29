#include <Arduino.h>
#include <TimerOne.h>

const char *NAME_VERSION = "OCT Motor Driver v0.2.0";

// Pin definitions
const int STEP_PIN = 6;      // 3D motor step pin
const int DIR_PIN = 5;       // 3D motor direction pin
const int ROTARY_EN_PIN = 8; // Rotary motor enable pin

// Variables
const int togglePin = STEP_PIN;
volatile bool toggleState = false;
bool timerRunning = false;
unsigned long timerPeriod = 5e5;
volatile int counter = 0;

void toggleFunc() {
  if (timerRunning) {
    toggleState = !toggleState;
    digitalWrite(LED_BUILTIN, toggleState); // Update the LED state
    digitalWrite(togglePin, toggleState);   // Update the target pin state
    counter += toggleState;
  } else {
    toggleState = false;
    digitalWrite(LED_BUILTIN, toggleState); // Update the LED state
    digitalWrite(togglePin, toggleState);   // Update the target pin state
  }
}

const char *HELP = R"(
Enter a command:

Commands related to the 3D motor (square wave and direction)
1. 'r' to start the timer
2. 's' to stop the timer
3. 'f<value>' to set frequency in Hz (e.g., 'f1000' for 1 Hz)
4. 'p<value>' to set period in us (e.g., 'p1000' for 1 ms)
5. 'd<value>' to set direction (e.g., 'd1' or 'd0' for high or low)

Commands related to the rotation motor (on off)
6. 'm0' to enable rotation or 'm1' to disable rotation

)";

const char *INVALID_COMMAND_MSG = "Invalid command. Try 'r', 's', 'f<value>', "
                                  "'p<value>', 'd<value>', or 'm'";

void setup() {
  // Initialize pins
  pinMode(LED_BUILTIN, OUTPUT);

  // Pins for 3D motor control
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);

  // Pins for rotary motor control
  pinMode(ROTARY_EN_PIN, OUTPUT);
  digitalWrite(ROTARY_EN_PIN, HIGH);

  // Initialize serial comms
  Serial.begin(115200);
  while (!Serial) {
    delay(100);
  }

  // Initialize timer
  Timer1.initialize(timerPeriod);
  Timer1.attachInterrupt(toggleFunc); // Attach the interrupt service routine

  // Print version
  Serial.println(NAME_VERSION);

  // Print instructions to the Serial Monitor
  Serial.println(HELP);
  Serial.println();
}

void processCommand(char *command);

/*
Square wave
0 to 5 V

400 us period
50% duty cycle
*/

void loop() {
  // Check for incoming serial data
  if (Serial.available() > 0) {
    static char inputBuffer[32];
    static size_t inputIdx = 0;

    // Read the incoming byte
    char incomingByte = Serial.read();

    // Check for the end of the command
    if (incomingByte == '\n' || incomingByte == '\r') {
      if (inputIdx > 0) {
        inputBuffer[inputIdx] = '\0'; // Null-terminate the string
        processCommand(inputBuffer);
        inputIdx = 0;
      }
    } else if (inputIdx < sizeof(inputBuffer) - 1) {
      inputBuffer[inputIdx++] = incomingByte;
    }
  }
}

// Function to process serial commands
void processCommand(char *command) {
  char *trimmedCommand = strtok(command, "\r\n");

  // Parse the commands
  if (trimmedCommand == nullptr) {
    Serial.println("Error: Empty command");

  } else if (trimmedCommand[0] == 'r') {
    // Start the timer
    timerRunning = true;

    Serial.println("Running");
  } else if (trimmedCommand[0] == 's') {
    // Stop the timer
    timerRunning = false;

    Serial.println("Stopped");

  } else if (trimmedCommand[0] == 'm') {
    // Rotary motor enable command
    if (strlen(trimmedCommand) == 2) {
      const uint8_t enablePinVal = trimmedCommand[1] == '0' ? LOW : HIGH;
      digitalWrite(ROTARY_EN_PIN, enablePinVal);

      Serial.print("Rotary enable set to ");
      Serial.println(enablePinVal);
    } else {
      Serial.println("Error: rotary enable must be m0 or m1");
    }

  } else if (trimmedCommand[0] == 'f') {
    // Set frequency command
    unsigned long frequency = atol(trimmedCommand + 1);
    if (frequency > 0) {
      timerPeriod = 1e6 / frequency; // Convert to period in us
      Timer1.setPeriod(timerPeriod);

      Serial.print("Frequency set to ");
      Serial.print(frequency);
      Serial.println(" Hz");
    } else {
      Serial.println("Error: frequency must be positive");
    }
  } else if (trimmedCommand[0] == 'p') {
    // Set period command
    unsigned long period = atol(trimmedCommand + 1);
    if (period > 0) {
      timerPeriod = period;
      Timer1.setPeriod(timerPeriod);

      Serial.print("Period set to ");
      Serial.print(timerPeriod);
      Serial.println(" us");
    } else {
      Serial.println("Error: frequency must be positive");
    }
  } else if (trimmedCommand[0] == 'd') {
    // Set direction command
    if (strlen(trimmedCommand) == 2) {
      const uint8_t dir = trimmedCommand[1] == '0' ? LOW : HIGH;
      digitalWrite(DIR_PIN, dir);

      Serial.print("Set direction pin to ");
      Serial.println(dir);
    } else {
      Serial.println("Error: direction value must be 0 or 1");
    }
  } else {
    // Invalid command
    Serial.println(INVALID_COMMAND_MSG);
    Serial.print("Received: ");
    Serial.println(command);
  }
  Serial.println();
}
