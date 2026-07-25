/*
 * misc.c
 *
 *  Created on: Jul 23, 2026
 *      Author: YarikSherb
 */


#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "../microsh/src/include/microsh/microsh.h"
#include "../hw_driver/hw_driver.h"

#define _STM32_DEMO_VER             "1.0"

#define _ENDLINE_SEQ                MICRORL_CFG_END_LINE

/* Definition commands word */
#define _CMD_HELP                   "help"
#define _CMD_CLEAR                  "clear"
#define _CMD_SERNUM                 "sernum"
#define _CMD_READ_DATA 				"rdm"
#define _CMD_WRITE_DATA 			"wdm"
#define _CMD_LOGOUT                 "logout"
#define _CMD_WRITE_FLASH 			"wrflash"
#define _CMD_JUMP 					"jump"

/* Arguments for set/clear */
#define _SCMD_RD                    "?"

#define _NUM_OF_CMD                 6
#define _NUM_OF_SETCLEAR_SCMD       2

#define MAX_READ_MEMORY_SIZE 		1024 * 256
#define START_FLASH_MEMORY 			0x08000000U
#define END_FLASH_MEMORY 			0x08040000U
#define START_RAM_MEMORY 			0x20000000U
#define END_RAM_MEMORY 				0x20010000U
/* Available  commands */
char *keyword[] =
{ _CMD_HELP,
_CMD_CLEAR,
_CMD_SERNUM,
_CMD_READ_DATA,
_CMD_WRITE_DATA,
_CMD_LOGOUT,
_CMD_WRITE_FLASH };

/* 'read/save' command argements */
char* read_save_key[] = {_SCMD_RD};

/* Array for comletion */
char* compl_word[_NUM_OF_CMD + 1];

/* Variable changeable with commands */
uint32_t device_sn = 0;

static void *misc_handler = {0};

static int help_cmd(microsh_t* msh, int argc, const char* const *argv);
static int clear_screen_cmd(microsh_t* msh, int argc, const char* const *argv);
static int sernum_cmd(microsh_t* msh, int argc, const char* const *argv);
static int read_memory_cmd(microsh_t* msh, int argc, const char* const *argv);
static int write_memory_cmd(microsh_t* msh, int argc, const char* const *argv);
static int write_flash_cmd(microsh_t *msh,int argc, const char *const *argv);

#if MICROSH_CFG_CONSOLE_SESSIONS
static int logout_cmd(microsh_t* msh, int argc, const char* const *argv);
#endif /* MICROSH_CFG_CONSOLE_SESSIONS */

/**
 * \brief           Init STM32F4 platform
 */
void init(void *handler) {

	misc_handler = handler;
	hw_init(&misc_handler);
}
#if MICROSH_CFG_CONSOLE_SESSIONS
/**
 * \brief           Register commands that may be used in authorization process
 * \param[in]       msh: \ref microsh_t working instance
 * \return          \ref microshOK on success, member of \ref microshr_t otherwise
 */
microshr_t register_auth_commands(microsh_t* msh) {
    microshr_t result = microshOK;

    result |= microsh_cmd_register(msh, 1, _CMD_HELP,   help_cmd,         NULL);

    return result;
}
#endif /* MICROSH_CFG_CONSOLE_SESSIONS */

/**
 * \brief           Register all commands used by shell
 * \param[in]       msh: \ref microsh_t working instance
 * \return          \ref microshOK on success, member of \ref microshr_t otherwise
 */
microshr_t register_all_commands(microsh_t* msh) {
    microshr_t result = microshOK;

    result |= microsh_cmd_register(msh, 1, _CMD_HELP,   help_cmd,         NULL);
    result |= microsh_cmd_register(msh, 1, _CMD_CLEAR,  clear_screen_cmd, NULL);
    result |= microsh_cmd_register(msh, 2, _CMD_SERNUM, sernum_cmd,       NULL);
    result |= microsh_cmd_register(msh, 4, _CMD_READ_DATA, read_memory_cmd,NULL);
    result |= microsh_cmd_register(msh,3,_CMD_WRITE_DATA,write_memory_cmd,NULL);
    result |= microsh_cmd_register(msh,4,_CMD_WRITE_FLASH,write_flash_cmd,NULL);
    #if MICROSH_CFG_CONSOLE_SESSIONS
    result |= microsh_cmd_register(msh, 1, _CMD_LOGOUT, logout_cmd,       NULL);

#endif /* MICROSH_CFG_CONSOLE_SESSIONS */

    return result;
}

/**
 * \brief           Print string to IO stream
 * \param[in]       str: Output string
 * \return          The number of characters that would have been written,
 *                      not counting the terminating null character.
 */
static int print(const char* str) {
    uint32_t i = 0;

    while (str[i] != 0) {
        while (!IsActiveFlag_TX(misc_handler)) {}
    	transmit_data_byte(misc_handler, str[i++]);
    }

    return i;
}

/**
 * \brief           Print to IO stream callback for MicroRL library
 * \param[in]       mrl: \ref microrl_t working instance
 * \param[in]       str: Output string
 * \return          The number of characters that would have been written,
 *                      not counting the terminating null character.
 */
int microrl_print(microrl_t* mrl, const char* str) {
    MICROSH_UNUSED(mrl);
    return print(str);
}

/**
 * \brief           Get char user pressed
 * \return          Input character
 */
char get_char(void) {

	while (!IsActiveFlag_RX(misc_handler)){}
	return recive_data_byte(misc_handler);

}

/**
 * \brief           Makes `unsigned 32-bit` value from ascii char array
 * \param[in]       str: Input string with value to convert
 * \param[out]      val: `unsigned 32-bit` data to be converted
 */
static void str_to_u32(char* str, uint32_t* val) {
    uint32_t temp = 0;

    for (uint8_t i = 0; str[i] >= 0x30 && str[i] <= 0x39; ++i) {
        temp = temp + (str[i] & 0x0F);
        temp = temp * 10;
    }
    temp = temp / 10;

    *val = temp;
}

/**
 * \brief           Makes ascii char array from `unsigned 32-bit` value
 * \param[in]       val: `unsigned 32-bit` data to be converted
 * \param[out]      str: Minimum `11-bytes` long array to write value to
 */
static void u32_to_str(uint32_t* val, char* str) {
    uint32_t v = *val;
    size_t s = 0;
    char t;

    size_t n;
    for (n = 0; v > 0; v /= 10) {
        str[s + n++] = "0123456789"[v % 10];
    }

    /* Reverse a string */
    for (size_t i = 0; i < n / 2; ++i) {
        t = str[s + i];
        str[s + i] = str[s + n - i - 1];
        str[s + n - i - 1] = t;
    }

    if (val == NULL) {
        str[n++] = '0';  /* Handle special case */
    }
}

/**
 * \brief           SERNUM ? command callback
 */
static void read_sernum(void) {
    char sn_str[11] = {0};
    uint32_t sn = device_sn;
    u32_to_str(&sn, sn_str);

    print("\tS/N ");
    print(sn_str);
    print(_ENDLINE_SEQ);
}

/**
 * \brief           SERNUM VALUE command callback
 * \param[in]       str_val: New serial number value
 */
static void set_sernum(char* str_val) {
    uint32_t sn = 0;

    str_to_u32(str_val, &sn);
    if (sn != 0) {
        device_sn = sn;

        print("\tset S/N ");
        print(str_val);
        print(_ENDLINE_SEQ);
        return;
    }

    print("\tS/N not set"_ENDLINE_SEQ);
}


/**
 * \brief           HELP command execution
 * \param[in]       msh: \ref microsh_t working instance
 * \param[in]       argc: argument count
 * \param[in]       argv: pointer array to token string
 * \return          \ref microshEXEC_OK on success, member of
 *                      \ref microsh_execr_t enumeration otherwise
 */
int help_cmd(microsh_t* msh, int argc, const char* const *argv) {
    MICRORL_UNUSED(msh);
    MICRORL_UNUSED(argc);
    MICRORL_UNUSED(argv);

    print("MicroSH library DEMO v");
    print(_STM32_DEMO_VER);
    print(_ENDLINE_SEQ);

    print("Use TAB key for completion"_ENDLINE_SEQ);
#if MICROSH_CFG_CONSOLE_SESSIONS
    if (!msh->session.status.flags.logged_in) {
        print(_ENDLINE_SEQ"You must log in to one of the sessions."_ENDLINE_SEQ);
        print("After authorization, session commands will be available."_ENDLINE_SEQ);
        print("Different commands may be available for different sessions."_ENDLINE_SEQ);
    } else {
#endif /* MICROSH_CFG_CONSOLE_SESSIONS */
    	print("List of commands:" _ENDLINE_SEQ);

    	print("\thelp                - show command list" _ENDLINE_SEQ);
    	print("\tclear               - clear terminal screen" _ENDLINE_SEQ);

    	print("\tsernum ?            - read serial number value" _ENDLINE_SEQ);
    	print("\tsernum VALUE        - set serial number value" _ENDLINE_SEQ);

    	print("\trdm ADDRESS LENGTH  - read memory dump" _ENDLINE_SEQ);
    	print("\twdm ADDRESS VALUE   - write 32-bit value to memory" _ENDLINE_SEQ);
    	print("\twrflash RAM_ADDR FLASH_ADDR VALUE   - write 3 value to memory" _ENDLINE_SEQ);

    	print("\tlogout              - end an authorized session" _ENDLINE_SEQ);
#if MICROSH_CFG_CONSOLE_SESSIONS
    }
#endif /* MICROSH_CFG_CONSOLE_SESSIONS */

    return microshEXEC_OK;
}

/**
 * \brief           CLEAR command execution
 * \param[in]       msh: \ref microsh_t working instance
 * \param[in]       argc: argument count
 * \param[in]       argv: pointer array to token string
 * \return          \ref microshEXEC_OK on success, member of
 *                      \ref microsh_execr_t enumeration otherwise
 */
int clear_screen_cmd(microsh_t* msh, int argc, const char* const *argv) {
    MICRORL_UNUSED(msh);
    MICRORL_UNUSED(argc);
    MICRORL_UNUSED(argv);

    print("\033[2J");    /* ESC seq for clear entire screen */
    print("\033[H");     /* ESC seq for move cursor at left-top corner */

    return microshEXEC_OK;
}

/**
 * \brief           SERNUM command execution
 * \param[in]       msh: \ref microsh_t working instance
 * \param[in]       argc: argument count
 * \param[in]       argv: pointer array to token string
 * \return          \ref microshEXEC_OK on success, member of
 *                      \ref microsh_execr_t enumeration otherwise
 */
int sernum_cmd(microsh_t* msh, int argc, const char* const *argv) {
    MICRORL_UNUSED(msh);
    MICRORL_UNUSED(argc);
    MICRORL_UNUSED(argv);

    int i = 0;

    if (++i < argc) {
        if (strcmp(argv[i], _SCMD_RD) == 0) {
            read_sernum();
        } else {
            set_sernum((char*)argv[i]);
        }
    } else {
        print("Read or specify serial number"_ENDLINE_SEQ);
        return microshEXEC_ERROR;
    }

    return microshEXEC_OK;
}

typedef enum
{
    MEM_FMT_U8,
    MEM_FMT_U16,
    MEM_FMT_U32
} mem_format_t;

static void print_memory_line(uint32_t address)
{
    char buffer[16];

    snprintf(buffer,
             sizeof(buffer),
             "\r\n%08lX: ",
             (unsigned long)address);

    print(buffer);
}

static void print_memory_value(uint32_t value, mem_format_t format)
{
    char buffer[16];

    switch (format)
    {
        case MEM_FMT_U8:
            snprintf(buffer, sizeof(buffer), "%02X ", (uint8_t)value);
            break;

        case MEM_FMT_U16:
            snprintf(buffer, sizeof(buffer), "%04X ", (uint16_t)value);
            break;

        case MEM_FMT_U32:
            snprintf(buffer, sizeof(buffer), "%08lX ", (unsigned long)value);
            break;
    }

    print(buffer);
}

int read_memory_cmd(microsh_t* msh, int argc, const char* const *argv)
{
    MICRORL_UNUSED(msh);

    mem_format_t format = MEM_FMT_U8;

    if ((argc != 3) && (argc != 4))
    {
        print("Usage: rdm ADDRESS LENGTH [u8|u16|u32]" _ENDLINE_SEQ);
        return microshEXEC_OK;
    }

    uint32_t address = strtoul(argv[1], NULL, 0);
    uint32_t length  = strtoul(argv[2], NULL, 0);

    if (argc == 4)
    {
        if (!strcmp(argv[3], "u8"))
            format = MEM_FMT_U8;
        else if (!strcmp(argv[3], "u16"))
            format = MEM_FMT_U16;
        else if (!strcmp(argv[3], "u32"))
            format = MEM_FMT_U32;
        else
        {
            print("Unknown format. Use u8/u16/u32" _ENDLINE_SEQ);
            return microshEXEC_OK;
        }
    }

    if ((length == 0U) || (length > MAX_READ_MEMORY_SIZE))
    {
        print("Invalid length" _ENDLINE_SEQ);
        return microshEXEC_OK;
    }

    uint32_t element_size;
    uint32_t elements_per_line;

    switch (format)
    {
        case MEM_FMT_U8:
            element_size = 1;
            elements_per_line = 16;
            break;

        case MEM_FMT_U16:
            if (address & 1U)
            {
                print("Address must be 2-byte aligned" _ENDLINE_SEQ);
                return microshEXEC_OK;
            }
            element_size = 2;
            elements_per_line = 8;
            break;

        default:
            if (address & 3U)
            {
                print("Address must be 4-byte aligned" _ENDLINE_SEQ);
                return microshEXEC_OK;
            }
            element_size = 4;
            elements_per_line = 4;
            break;
    }

    char buffer[64];

    snprintf(buffer,
             sizeof(buffer),
             "Read memory 0x%08lX (%lu bytes)",
             (unsigned long)address,
             (unsigned long)length);

    print(buffer);

    uint32_t count = length / element_size;

    for (uint32_t i = 0; i < count; i++)
    {
        if ((i % elements_per_line) == 0U)
        {
            print_memory_line(address + i * element_size);
        }

        switch (format)
        {
            case MEM_FMT_U8:
                print_memory_value(((volatile const uint8_t *)address)[i], format);
                break;

            case MEM_FMT_U16:
                print_memory_value(((volatile const uint16_t *)address)[i], format);
                break;

            case MEM_FMT_U32:
                print_memory_value(((volatile const uint32_t *)address)[i], format);
                break;
        }
    }

    print(_ENDLINE_SEQ);

    return microshEXEC_OK;
}

int write_memory_cmd(microsh_t* msh, int argc, const char* const *argv)
{
    MICRORL_UNUSED(msh);


    if (argc != 3)
    {
        print("Usage: wdm ADDRESS VALUE" _ENDLINE_SEQ);
        return microshEXEC_OK;
    }


    uint32_t address = strtoul(argv[1], NULL, 0);
    uint32_t value   = strtoul(argv[2], NULL, 0);


    volatile uint32_t *ptr = (volatile uint32_t *)address;


    print("Write memory" _ENDLINE_SEQ);


    *ptr = value;


    char buffer[64];

    snprintf(buffer,
             sizeof(buffer),
             "0x%08lX <= 0x%08lX" _ENDLINE_SEQ,
             address,
             value);

    print(buffer);


    return microshEXEC_OK;
}

int write_flash_cmd(microsh_t *msh,
                    int argc,
                    const char *const *argv)
{
    MICRORL_UNUSED(msh);

    if (argc != 4)
    {
        print("Usage: wrflash RAM_ADDR FLASH_ADDR SIZE" _ENDLINE_SEQ);
        return microshEXEC_OK;
    }

    uint32_t ram_addr   = strtoul(argv[1], NULL, 0);
    uint32_t flash_addr = strtoul(argv[2], NULL, 0);
    uint32_t size       = strtoul(argv[3], NULL, 0);

    if (size == 0U)
    {
        print("Invalid size" _ENDLINE_SEQ);
        return microshEXEC_OK;
    }

    /* Проверка выравнивания */
    if ((ram_addr & 0x3U) || (flash_addr & 0x3U) || (size & 0x3U))
    {
        print("Address and size must be 4-byte aligned" _ENDLINE_SEQ);
        return microshEXEC_OK;
    }

    /* Проверка диапазона RAM */
    if ((ram_addr < START_RAM_MEMORY) ||
        ((ram_addr + size) > END_RAM_MEMORY))
    {
        print("RAM address out of range" _ENDLINE_SEQ);
        return microshEXEC_OK;
    }

    /* Проверка диапазона Flash */
    if ((flash_addr < START_FLASH_MEMORY) ||
        ((flash_addr + size) > END_FLASH_MEMORY))   /* STM32F401RC = 256 KB */
    {
        print("Flash address out of range" _ENDLINE_SEQ);
        return microshEXEC_OK;
    }

    unsigned int status =
        Flash_WriteBuffer(flash_addr,
                          (const void *)ram_addr,
                          size);

    if (status == 0)
    {
        print("Write OK" _ENDLINE_SEQ);
    }
    else
    {
        print("Write FAILED" _ENDLINE_SEQ);
    }

    return microshEXEC_OK;
}

#if MICROSH_CFG_CONSOLE_SESSIONS
/**
 * \brief           LOGOUT command execution
 * \param[in]       msh: \ref microsh_t working instance
 * \param[in]       argc: argument count
 * \param[in]       argv: pointer array to token string
 * \return          \ref microshEXEC_OK on success, member of
 *                      \ref microsh_execr_t enumeration otherwise
 */
int logout_cmd(microsh_t* msh, int argc, const char* const *argv) {
    MICRORL_UNUSED(argc);
    MICRORL_UNUSED(argv);

    microsh_session_logout(msh);
    microsh_cmd_unregister_all(msh);
    print("Logged out"_ENDLINE_SEQ);

    return microshEXEC_OK;
}
#endif /* MICROSH_CFG_CONSOLE_SESSIONS */

#if MICRORL_CFG_USE_COMPLETE || __DOXYGEN__
/**
 * \brief           Completion callback for MicroRL library
 * \param[in,out]   mrl: \ref microrl_t working instance
 * \param[in]       argc: argument count
 * \param[in]       argv: pointer array to token string
 * \return          NULL-terminated string, contain complite variant split by 'Whitespace'
 */
char** complet(microrl_t* mrl, int argc, const char* const *argv) {
    MICRORL_UNUSED(mrl);
    int j = 0;

    compl_word[0] = NULL;

    /* If there is token in cmdline */
    if (argc == 1) {
        /* Get last entered token */
        char* bit = (char*)argv[argc - 1];
        /* Iterate through our available token and match it */
        for (int i = 0; i < _NUM_OF_CMD; ++i) {
            /* If token is matched (text is part of our token starting from 0 char) */
            if (strstr(keyword[i], bit) == keyword[i]) {
                /* Add it to completion set */
                compl_word[j++] = keyword[i];
            }
        }
    }  else if ((argc > 1) && (strcmp(argv[0], _CMD_SERNUM) == 0)) {   /* If command needs subcommands */
        /* Iterate through subcommand */
        for (int i = 0; i < _NUM_OF_SETCLEAR_SCMD; ++i) {
            if (strstr(read_save_key[i], argv[argc - 1]) == read_save_key[i]) {
                compl_word[j++] = read_save_key[i];
            }
        }
    } else {    /* If there is no token in cmdline, just print all available token */
        for (; j < _NUM_OF_CMD; ++j) {
            compl_word[j] = keyword[j];
        }
    }

    /* Note! Last ptr in array always must be NULL!!! */
    compl_word[j] = NULL;

    /* Return set of variants */
    return compl_word;
}
#endif /* MICRORL_CFG_USE_COMPLETE || __DOXYGEN__ */

#if MICRORL_CFG_USE_CTRL_C || __DOXYGEN__
/**
 * \brief           Ctrl+C terminal signal function
 * \param[in]       mrl: \ref microrl_t working instance
 */
void sigint(microrl_t* mrl) {
    microrl_print(mrl, "^C is caught!"_ENDLINE_SEQ);
}
#endif /* MICRORL_CFG_USE_CTRL_C || __DOXYGEN__ */

