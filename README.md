# BG95-M2 Driver

This is a driver for the Quectel BG95-M2  RF transceiver that is capable of allowing an external microcontroller to transmit and recieve data using most application layer communication protocols ontop of modern cellular infrastructure (using NB-IoT and LTE Cat-M1 networks). 

The following is a breakout down the driver:

The driver features a primary interface / API that higher level code and users will use, and it is built upon the follow architectural components:

1. An AT command formatter - this creates an AT command string by taking variables and input provided, and placing it within a defined format given the command and the specified type of the command.

2. An AT command response parser - this extracts the parameters and information associated with a response to an AT command, again depending on formats defined for the particular command and its type that was sent and resulted in this response. 

3. An AT command handler - this is repsonsible for handling communication of AT commands with the cell module, and handles sending and receiving them. 

## How it works:

Each AT command defines its own parser and formatter functions for its associated types. The formatter and parser use a function pointer so that the parse and format functions pointed to in each command type definition are used.

The cmd handler uses the format and parsing fxns associated with a command and its defined type, to format the cmd, send it the bg95 via UART interface, and then waits for a response. 

The response is read using the UART interface read in chunks, where received bytes are appended to a temp buffer which, after each loop, is checked as to whether it contains valid response indicators (which depend on the command its type that was sent to the BG95). If no valid response indicator is received after a timeout time (again defined by the command) then it is considered the command failed.

If the commmand expects a 'data response' (something other than the standard OK/ERROR) then the cmd handler calls the associated parsing function, passing it the raw respons string after it is finished being read from the UART interface.

### Project directory structure 

```
/
├── include/                       # Header files
│   ├── at/                        # AT command interface headers
│   │   ├── at_cmds.h              # General AT command definitions
│   │   ├── cmd/                   # Command-specific headers by category
│   │   │   ├── general/           # General modem commands
│   │   │   ├── hardware_related/  # Hardware-specific commands
│   │   │   ├── mqtt/              # MQTT protocol commands
│   │   │   ├── network_service/   # Network registration/operator commands
│   │   │   ├── packet_domain/     # PDP context and PS attachment commands
│   │   │   └── sim_related/       # SIM card management commands
│   │   └── core/                  # Core AT command infrastructure
│   ├── bg95/                      # BG95-specific module interface
│   └── enum_utils.h               # Utilities for enum-string conversion
│
└── src/                           # Implementation files
    ├── at/                        # AT command implementations
    │   ├── cmd/                   # Command-specific implementations by category
    │   │   ├── general/           # Implementation of general commands
    │   │   ├── hardware_related/  # Implementation of hardware commands
    │   │   ├── mqtt/              # Implementation of MQTT commands
    │   │   ├── network_service/   # Implementation of network service commands
    │   │   ├── packet_domain/     # Implementation of packet domain commands
    │   │   └── sim_related/       # Implementation of SIM-related commands
    │   └── core/                  # Core AT command infrastructure implementation
    ├── bg95/                      # BG95-specific module implementation
    └── enum_utils.c               # Implementation of enum utilities
```

## Core Components

### 1. AT Command Structure (`at_cmd_structure.h`)
- Defines fundamental types and structures for AT commands
- Includes command type enum (TEST, READ, WRITE, EXECUTE)
- Provides generic response handling structures
- Establishes the base command definition format

### 2. AT Command source files 
- Contains command-specific enums and data structures
- Contains definition of parsing and formatting fxns associated with that command and its available command types 

### 3. Command Handler (`at_cmd_handler.h`, `at_cmd_handler.c`)
- Manages UART communication with the BG95 module
- Handles command timeouts and retries
- Provides thread-safe command execution
- Coordinates command formatting and response parsing

### 4. Command Formatter (`at_cmd_formatter.h`, `at_cmd_formatter.c`)
- Creates properly formatted AT command strings
- Handles parameter insertion into command templates
- Ensures correct command syntax and termination
- Validates command parameters before formatting

### 5. Response Parser (`at_cmd_parser.h`, `at_cmd_parser.c`)
- Extracts data from AT command responses
- Maps raw responses to structured data types
- Handles optional and variable response fields
- Provides error detection and reporting

### 6. Public API (`bg95_driver.h`, `bg95_driver.c`)
- Offers high-level interface for module operations
- Abstracts AT command complexity from users
- Provides type-safe function calls
- Implements common cellular operations


## Steps to connect to a cell network:

1. Check module and SIM card status 
```
AT+CPIN?        # Check if SIM is ready
AT+QCSQ          # Check signal quality  (NOTE! - Quectel said that 'AT+CSQ' is for 2G only, and for LTE and NB-IoT we should use 'AT+QCSQ')
```

2. Configure network settings 
```
AT+COPS=?       # List available operators
AT+COPS=1,2,"<operator_code>"    # Select operator (use specific operator code)
```

3. Configure PDP context for data connection 
```
AT+CGDCONT=1,"IP","<your_apn>"   # Define PDP context with your carrier's APN
AT+CGACT=1,1                      # Activate PDP context
AT+CGPADDR=1                      # Verify IP address assignment
```

4. Check network registration
```
AT+CEREG?       # Check (LTE or NB-IoT) network registration status (NOTE! - 'AT+CREG' is for 2G)
```

5. Ready to use higher level application layer communication protocol such  as MQTT or HTTP ....


## Testing  

TODO...


## Usage 

TODO..
