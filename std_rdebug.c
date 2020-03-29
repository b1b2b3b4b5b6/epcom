#include "std_rdebug.h"

#define PRINT_BUF_SIZE 500
#define CONSOLE_TASK_SIZE 8192
#define CONSOLE_TASK_PRI (ESP_TASK_MAIN_PRIO + 3)
#define STD_LOCAL_LOG_LEVEL STD_LOG_DEBUG

int STD_GLOBAL_LOG_LEVEL = STD_LOG_VERBOSE;
static bool g_control = false;

void set_global_log_level(int level)
{
    STD_GLOBAL_LOG_LEVEL = level;
}

static void initialize_console()
{
    /* Disable buffering on stdin and stdout */
    setvbuf(stdin, NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);

    /* Minicom, screen, idf_monitor send CR when ENTER key is pressed */
    esp_vfs_dev_uart_set_rx_line_endings(ESP_LINE_ENDINGS_CR);
    /* Move the caret to the beginning of the next line on '\n' */
    esp_vfs_dev_uart_set_tx_line_endings(ESP_LINE_ENDINGS_CRLF);

    /* Configure UART. Note that REF_TICK is used so that the baud rate remains
     * correct while APB frequency is changing in light sleep mode.
     */
    const uart_config_t uart_config = {
            .baud_rate = CONFIG_CONSOLE_UART_BAUDRATE,
            .data_bits = UART_DATA_8_BITS,
            .parity = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .use_ref_tick = true
    };
    ESP_ERROR_CHECK( uart_param_config(CONFIG_CONSOLE_UART_NUM, &uart_config) );

    /* Install UART driver for interrupt-driven reads and writes */
    ESP_ERROR_CHECK( uart_driver_install(CONFIG_CONSOLE_UART_NUM,
            256, 0, 0, NULL, 0) );

    /* Tell VFS to use UART driver */
    esp_vfs_dev_uart_use_driver(CONFIG_CONSOLE_UART_NUM);

    /* Initialize the console */
    esp_console_config_t console_config = {
            .max_cmdline_args = 8,
            .max_cmdline_length = 256,
            .hint_color = atoi(LOG_COLOR_CYAN)
    };
    ESP_ERROR_CHECK( esp_console_init(&console_config) );

    /* Configure linenoise line completion library */
    /* Enable multiline editing. If not set, long commands will scroll within
     * single line.
     */
    linenoiseSetMultiLine(1);

    /* Tell linenoise where to get command completions and hints */
    linenoiseSetCompletionCallback(&esp_console_get_completion);
    linenoiseSetHintsCallback((linenoiseHintsCallback*) &esp_console_get_hint);

    /* Set command history size */
    linenoiseHistorySetMaxLen(100);

}

static struct {
    struct arg_str *mac;
    struct arg_end *end;
} set_mac_args;

static int set_mac(int argc, char** argv)
{
    uint8_t mac[6];
    int nerrors = arg_parse(argc, argv, (void**) &set_mac_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, set_mac_args.end, argv[0]);
        return 1;
    }
    PORT_LOGD("mac : %s\r\n", set_mac_args.mac->sval[0]);
    
    if(str2mac(mac, set_mac_args.mac->sval[0]) == NULL)
    {
        printf("input mac error\r\n");
        return 0;
    }

    if(rdebug_set_mac(mac) == 0)
        printf("target mac connect\r\n");
    else
        printf("target mac can not connect\r\n");

    return 0;
}

static void register_set_mac()
{
    set_mac_args.mac = arg_str0("m", "mac", "<mac>", "target mac");
    set_mac_args.end = arg_end(20);

    const esp_console_cmd_t cmd = {
        .command = "set_mac",
        .help = "set a target mac",
        .hint = NULL,
        .func = &set_mac,
        .argtable = &set_mac_args
    };

    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd));
}



static struct {
    struct arg_str *mac;
    struct arg_end *end;
} unset_mac_args;

static int unset_mac(int argc, char** argv)
{
    uint8_t mac[6];
    int nerrors = arg_parse(argc, argv, (void**) &unset_mac_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, unset_mac_args.end, argv[0]);
        return 1;
    }
    PORT_LOGD("mac : %s\r\n", unset_mac_args.mac->sval[0]);
    
    if(str2mac(mac, unset_mac_args.mac->sval[0]) == NULL)
    {
        printf("input mac error\r\n");
        return 0;
    }

    if(rdebug_unset_mac(mac) == 0)
        printf("target mac unset\r\n");
    else
        printf("target mac can not unset\r\n");

    return 0;
}

static void register_unset_mac()
{
    set_mac_args.mac = arg_str0("m", "mac", "<mac>", "target mac");
    set_mac_args.end = arg_end(20);

    const esp_console_cmd_t cmd = {
        .command = "unset_mac",
        .help = "unset a target mac",
        .hint = NULL,
        .func = &unset_mac,
        .argtable = &unset_mac_args
    };

    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd));
}


static struct {
    struct arg_int *level;
    struct arg_end *end;
}set_log_args;

static int set_log(int argc, char** argv) 
{
    int nerrors = arg_parse(argc, argv, (void**) &set_log_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, set_log_args.end, argv[0]);
        return 1;
    }

    PORT_LOGD("level : %d\r\n", set_log_args.level->ival[0]);

    int level = set_log_args.level->ival[0];
    if(level >6)
    {
        printf("level must <= 6\r\n");
        return 0;
    }

    if(rdebug_set_log(level) != 0)
        printf("set fail\r\n");

    return 0;
}

static void register_set_log()
{
    set_log_args.level = arg_int0("l", "level", "<level>", "log level, 1M,2E,3W,4I,5D,6V");
    set_log_args.end = arg_end(20);

    const esp_console_cmd_t cmd = {
        .command = "set_log",
        .help = "set log",
        .hint = NULL,
        .func = &set_log,
        .argtable = &set_log_args,
    };

    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));    
}



static void register_system()
{
    register_set_mac();
    //register_unset_mac();
    register_set_log();
}

void std_rdebug_console_run_task(void *arg)
{
    
    initialize_console();
    esp_console_register_help_command();
    register_system();
    /* Prompt to be printed before each line.
     * This can be customized, made dynamic, etc.
     */
    const char* prompt = LOG_COLOR_I "esp32> " LOG_RESET_COLOR;

    printf("\n"
           "This is an debug console for MCB.\n"
           "Type 'help' to get the list of commands.\n"
           "Press TAB when typing command name to auto-complete.\n");

    /* Figure out if the terminal supports escape sequences */
    int probe_status = linenoiseProbe();
    if (probe_status) { /* zero indicates success */
        printf("\n"
               "Your terminal application does not support escape sequences.\n"
               "Line editing and history features are disabled.\n"
               "On Windows, try using Putty instead.\n");
        linenoiseSetDumbMode(1);
        prompt = "esp32> ";
    }
    
    /* Main loop */
    while(true) {
        /* Get a line using linenoise.
         * The line is returned when ENTER is pressed.
         */
        char* line = linenoise(prompt);
        if (line == NULL) { /* Ignore empty lines */
            continue;
        }
        /* Add the command to the history */
        linenoiseHistoryAdd(line);

        /* Try to run the command */
        int ret;
        esp_err_t err = esp_console_run(line, &ret);
        if (err == ESP_ERR_NOT_FOUND) {
            printf("Unrecognized command\n");
        } else if (err == ESP_ERR_INVALID_ARG) {
            // command was empty
        } else if (err == ESP_OK && ret != ESP_OK) { 
            printf("Command returned non-zero error code: 0x%x (%s)\n", ret, esp_err_to_name(err));
        } else if (err != ESP_OK) {
            printf("Internal error: %s\n", esp_err_to_name(err));
        }
        /* linenoise allocates line buffer on the heap, so need to free it */
        linenoiseFree(line);
    }
}

void std_rdebug_cmd_register(const esp_console_cmd_t *join_cmd)
{
    ESP_ERROR_CHECK( esp_console_cmd_register(join_cmd) );
}

 void IRAM_ATTR std_rdebug_log(int level, char *file, int line, char *format, ...)
{
    static char buf[PRINT_BUF_SIZE];
    memset(buf, 0, PRINT_BUF_SIZE);
    
    va_list vArgList; 
    va_start (vArgList, format);
    file = strrchr(file, '/') + 1;

    switch(level)
    {
        case STD_LOG_MARK:
            sprintf(buf, "\033[34m [%d] %s[%d]| ", esp_log_timestamp(), file, line);
            break;

        case STD_LOG_ERROR:
            sprintf(buf, "\033[31m [%d] %s[%d]| ", esp_log_timestamp(), file, line);
            break;

        case STD_LOG_WARN:
            sprintf(buf, "\033[33m [%d] %s[%d]| ", esp_log_timestamp(), file, line);
            break;

        case STD_LOG_INFO:
            sprintf(buf, "\033[32m [%d] %s[%d]| ", esp_log_timestamp(), file, line);
            break;

        case STD_LOG_DEBUG:
            sprintf(buf, "\033[37m [%d] %s[%d]| ",esp_log_timestamp(), file, line);
            break;

        case STD_LOG_VERBOSE:
            sprintf(buf, "\033[37m [%d] %s[%d]| ", esp_log_timestamp(), file, line);
            break;

        default:
            PORT_END("udefined error");
            break;
    }
    vsprintf(buf + strlen(buf), format, vArgList);
    va_end(vArgList); 

    if(g_control)
        printf(buf);
    else
    {
        if(target_print((uint8_t*)buf, strlen(buf) + 1) != 0)
            printf(buf);
    }
}

void std_rdebug_init(bool control)
{
    std_espnow_init();
    g_control = control;
    if(control)
    {   
        xTaskCreate(std_rdebug_console_run_task, "console task", CONSOLE_TASK_SIZE, NULL, CONSOLE_TASK_PRI, NULL);
    }
    else    
        target_create_receive_debug_task();

    PORT_LOGI("std rdebug init success");

}
