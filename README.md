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

### Project directory structure 

```
TODO - waiting until project is more mature incase things change
```

## Core Components

### 1. AT Command Structure (`at_cmd_structure.h`)
- Defines fundamental types and structures for AT commands
- Includes command type enum (TEST, READ, WRITE, EXECUTE)
- Provides generic response handling structures
- Establishes the base command definition format

<!-- ### 2. AT Commands (`at_cmds.h`, `at_cmds.c`) -->
<!-- - Contains command-specific enums and data structures -->
<!-- - Defines individual AT command implementations -->
<!-- - Includes command validation rules -->
<!-- - Maps commands to their response parsers -->

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
AT+CSQ          # Check signal quality
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
AT+CREG?        # Check network registration status
AT+CEREG?       # Check EPS (LTE) network registration status
```

5. Ready to use higher level application layer communication protocol such  as MQTT or HTTP ....


## Testing  

TODO...


## Usage 

TODO..
