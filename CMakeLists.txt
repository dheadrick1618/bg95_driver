idf_component_register(
    # As cmds are added, their associated  source must be added below with path relative to the project root dir
    SRCS 
        "src/at/core/at_cmd_formatter.c"
        "src/at/core/at_cmd_handler.c"
        "src/at/core/at_cmd_parser.c"
        "src/bg95/bg95_driver.c"
        "src/bg95/bg95_uart_interface.c"
        "src/bg95/bg95_uart_mock_interface.c" 
        #### -- Commands --- ####
        "src/at/cmd/general"
        "src/at/cmd/mqtt"
        "src/at/cmd/network_service"
        "src/at/cmd/packet_domain"
        "src/at/cmd/sim_related"
    INCLUDE_DIRS 
        "include"
        "include/at"
        "include/at/cmd"
        "include/at/core"
        "include/bg95"
    REQUIRES 
        esp_driver_uart
)
