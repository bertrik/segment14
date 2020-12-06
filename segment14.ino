#include <Arduino.h>
#include <Wire.h>

#include "cmdproc.h"
#include "editline.h"

#include "ht16k33.h"

#define printf Serial.printf

#define PIN_SCL D1
#define PIN_SDA D2

static char line[128];
static HT16K33 ht;

static void show_help(const cmd_t * cmds)
{
    for (const cmd_t * cmd = cmds; cmd->cmd != NULL; cmd++) {
        printf("%10s: %s\n", cmd->name, cmd->help);
    }
}

static int do_help(int argc, char *argv[]);

static int do_power(int argc, char *argv[])
{
    if (argc < 2) {
        return CMD_ARG;
    }
    bool power = atoi(argv[1]);

    if (power) {
        ht.displayOn();
    } else {
        ht.displayOff();
    }

    return CMD_OK;
}

static int do_seg(int argc, char *argv[])
{
    if (argc < 2) {
        return CMD_ARG;
    }
    uint16_t val = strtoul(argv[1], NULL, 16);
    uint8_t digit = (argc > 2) ? atoi(argv[2]) : 0;

    printf("Writing '0x%X' to digit %d...\n", val, digit);
    int pos = digit * 2;
    ht.setDisplayRaw(pos++, (val >> 0) & 0xFF);
    ht.setDisplayRaw(pos++, (val >> 8) & 0xFF);
    ht.sendLed();

    return CMD_OK;
}

static const struct font_t {
    char c;
    uint16_t code;
} font[] = {
    { 'a', 0x00F7 },
    { 'b', 0x128F },
    { 'c', 0x0039 },
    { 'd', 0x120F },
    { 'e', 0x00F9 },
    { 'f', 0x00F1 },
    { 'g', 0x00BD },
    { 'h', 0x00F6 },
    { 'i', 0x1209 },
    { 'j', 0x001E },
    { 'k', 0x2470 },
    { 'l', 0x0038 },
    { 'm', 0x0536 },
    { 'n', 0x2136 },
    { 'o', 0x003F },
    { 'p', 0x00F3 },
    { 'q', 0x203F },
    { 'r', 0x20F3 },
    { 's', 0x018D },
    { 't', 0x1201 },
    { 'u', 0x003E },
    { 'v', 0x0C30 },
    { 'w', 0x2836 },
    { 'x', 0x2D00 },
    { 'y', 0x1500 },
    { 'z', 0x0C09 },
    { '0', 0x0C3F },
    { '1', 0x0406 },
    { '2', 0x00DB },
    { '3', 0x008F },
    { '4', 0x00E6 },
    { '5', 0x00ED },
    { '6', 0x00FD },
    { '7', 0x1401 },
    { '8', 0x00FF },
    { '9', 0x00E7 },
    { '\0', 0x00 }              // terminator
};

static int do_char(int argc, char *argv[])
{
    if (argc < 2) {
        return CMD_ARG;
    }
    char c = argv[1][0];
    uint8_t digit = (argc > 2) ? atoi(argv[2]) : 0;

    for (const struct font_t * p = font; p->code != 0; p++) {
        if (p->c == c) {
            int idx = digit * 2;
            uint16_t val = p->code;
            ht.setDisplayRaw(idx++, (val >> 0) & 0xFF);
            ht.setDisplayRaw(idx++, (val >> 8) & 0xFF);
            ht.sendLed();
            return CMD_OK;
        }
    }

    return -1;
}

const cmd_t commands[] = {
    { "help", do_help, "Show help" },
    { "power", do_power, "<0|1> Set power" },
    { "seg", do_seg, "[value] [pos] Set segment value at position pos" },
    { "char", do_char, "[char] [pos] Write character 'char' at position 'pos' (0-3)" },
    { NULL, NULL, NULL }
};

static int do_help(int argc, char *argv[])
{
    show_help(commands);
    return CMD_OK;
}

void setup(void)
{
    Serial.begin(115200);
    EditInit(line, sizeof(line));

    Wire.begin(PIN_SDA, PIN_SCL);
    ht.begin(0);
}

void loop(void)
{
    // parse command line
    bool haveLine = false;
    if (Serial.available()) {
        char c;
        haveLine = EditLine(Serial.read(), &c);
        Serial.write(c);
    }
    if (haveLine) {
        int result = cmd_process(commands, line);
        switch (result) {
        case CMD_OK:
            printf("OK\n");
            break;
        case CMD_NO_CMD:
            break;
        case CMD_UNKNOWN:
            printf("Unknown command, available commands:\n");
            show_help(commands);
            break;
        default:
            printf("%d\n", result);
            break;
        }
        printf(">");
    }
}
