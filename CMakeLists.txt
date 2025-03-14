idf_component_register(
    # As cmds are added, their associated  source must be added below with path relative to the project root dir
    SRCS 
        "src/at/core/at_cmd_formatter.c"
        "src/at/core/at_cmd_handler.c"
        "src/at/core/at_cmd_parser.c"
        "src/bg95/bg95_driver.c"
        "src/bg95/bg95_uart_interface.c"
        "src/bg95/bg95_uart_mock_interface.c" 
        "src/enum_utils.c"
        #### -- Commands --- ####
        "src/at/cmd/general/at_cmd_cfun.c"
        "src/at/cmd/mqtt/at_cmd_qmtcfg.c"
        "src/at/cmd/mqtt/at_cmd_qmtopen.c"
        "src/at/cmd/mqtt/at_cmd_qmtclose.c"
        "src/at/cmd/mqtt/at_cmd_qmtconn.c"
        "src/at/cmd/mqtt/at_cmd_qmtdisc.c"
        "src/at/cmd/network_service/at_cmd_csq.c"
        "src/at/cmd/network_service/at_cmd_cops.c"
        "src/at/cmd/packet_domain/at_cmd_cgdcont.c"
        "src/at/cmd/packet_domain/at_cmd_cgact.c"
        "src/at/cmd/packet_domain/at_cmd_cgpaddr.c"
        "src/at/cmd/sim_related/at_cmd_cpin.c"
    INCLUDE_DIRS 
        "include"
        "include/at"
        "include/at/cmd/general"
        # "include/at/cmd/hardware_related"
        "include/at/cmd/mqtt"
        "include/at/cmd/network_service"
        "include/at/cmd/packet_domain"
        "include/at/cmd/sim_related"
        "include/at/core"
        "include/bg95"
    REQUIRES 
        esp_driver_uart
)
