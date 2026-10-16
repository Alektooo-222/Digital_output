
#include "USB_CDC_Handlers.h"
#include "usb_cdc.h"
#include "do.h"
#include "usb_cdc_common.h"

#include "etl/string.h"
#include "etl/string_view.h"
#include "etl/string_utilities.h"
#include "etl/to_string.h"
#include "etl/format_spec.h"

#include <stdint.h>

void SET_CONFIG_OUTPUT(etl::string<MAX_SIZE_USB_CDC_MESSAGE> &cmd);
void GET_CONFIG_OUTPUT(etl::string<MAX_SIZE_USB_CDC_MESSAGE> &cmd);
void SET_FREQ(etl::string<MAX_SIZE_USB_CDC_MESSAGE> &cmd);
void GET_FREQ(etl::string<MAX_SIZE_USB_CDC_MESSAGE> &cmd);
void SET_STATE_OUTPUT(etl::string<MAX_SIZE_USB_CDC_MESSAGE> &cmd);
void GET_STATE_OUTPUT(etl::string<MAX_SIZE_USB_CDC_MESSAGE> &cmd);
void ENABLE_PULL_UP(etl::string<MAX_SIZE_USB_CDC_MESSAGE> &cmd);
void DISABLE_PULL_UP(etl::string<MAX_SIZE_USB_CDC_MESSAGE> &cmd);
void ENABLE_NV(etl::string<MAX_SIZE_USB_CDC_MESSAGE> &cmd);
void DISABLE_NV(etl::string<MAX_SIZE_USB_CDC_MESSAGE> &cmd);
void SET_ACTIVE_SENSOR(etl::string<MAX_SIZE_USB_CDC_MESSAGE> &cmd);
void SET_PASSIVE_SENSOR(etl::string<MAX_SIZE_USB_CDC_MESSAGE> &cmd);
void HELP(etl::string<MAX_SIZE_USB_CDC_MESSAGE> &cmd);

etl::string<MAX_SIZE_USB_CDC_MESSAGE> response_usb_cdc;

using HandlerFuncUsbCdc = void (*)(etl::string<MAX_SIZE_USB_CDC_MESSAGE> &cmd);

struct CommandHandlerUsbCdc
{
    const char *id;
    HandlerFuncUsbCdc handler;
};

CommandHandlerUsbCdc UsbCdcHandlers[NUMBER_OF_USB_CDC_HANDLERS] = {
    {"set_config_output", SET_CONFIG_OUTPUT},
    {"get_config_output", GET_CONFIG_OUTPUT},
    {"set_freq", SET_FREQ},
    {"get_freq", GET_FREQ},
    {"set_state_output", SET_STATE_OUTPUT},
    {"get_state_output", GET_STATE_OUTPUT},
    {"enable_pull_up", ENABLE_PULL_UP},
    {"disable_pull_up", DISABLE_PULL_UP},
    {"enable_nv", ENABLE_NV},
    {"disable_nv", DISABLE_NV},
    {"set_active_sensor", SET_ACTIVE_SENSOR},
    {"set_passive_sensor", SET_PASSIVE_SENSOR},
    {"help", HELP},
};

uint8_t str_inv_arg_usb_cdc[] = "Invalid Argument\n";
uint8_t str_wrong_arg_cnt_usb_cdc[] = "Wrong Argument Count\n";

enum class ParseResult
{
    Ok,
    InvalidCommand,
    InvalidArgument,
    WrongArgumentCount,
    Overflow
};

struct SetConfigOutputCommand
{
    DO::DO_Config_Output_Type output_type;
};

ParseResult inline parse_set_config_output(
    etl::string_view message,
    SetConfigOutputCommand &command)
{
    // Ищем пробелы и выделяем 2 токенов:
    //
    // set_config_output
    // output_type

    uint8_t number_of_tokens = 2U;
    etl::string_view tokens[number_of_tokens];

    size_t token_count = 0;
    size_t start = 0;

    for (size_t i = 0; i <= message.size(); ++i)
    {
        if (i == message.size() || message[i] == ' ' || message[i] == '\n')
        {
            if (i > start)
            {
                if (token_count >= number_of_tokens)
                    return ParseResult::WrongArgumentCount;

                tokens[token_count++] =
                    message.substr(start, i - start);
            }

            start = i + 1;
        }
    }

    if (token_count != number_of_tokens)
        return ParseResult::WrongArgumentCount;

    // Проверяем имя команды
    if (tokens[0] != "set_config_output")
        return ParseResult::InvalidCommand;

    if (tokens[1] == "discrete")
    {
        command.output_type = DO::DO_Config_Output_Type::DISCRETE;
        return ParseResult::Ok;
    }
    else if (tokens[1] == "freq")
    {
        command.output_type = DO::DO_Config_Output_Type::FREQ;
        return ParseResult::Ok;
    }
    else
    {
        return ParseResult::InvalidArgument;
    }

    return ParseResult::Ok;
}

void SET_CONFIG_OUTPUT(etl::string<MAX_SIZE_USB_CDC_MESSAGE> &cmd)
{
    SetConfigOutputCommand parsered_cmd = {};
    UsbCdcMessage msg = {};

    ParseResult state = parse_set_config_output(cmd, parsered_cmd);

    if (state == ParseResult::InvalidArgument)
    {
        msg.size = sizeof(str_inv_arg_usb_cdc);
        memcpy(msg.data, str_inv_arg_usb_cdc, msg.size);
        Usb_Cdc::send(&msg);
        return;
    }

    if (state == ParseResult::WrongArgumentCount)
    {
        msg.size = sizeof(str_wrong_arg_cnt_usb_cdc);
        memcpy(msg.data, str_wrong_arg_cnt_usb_cdc, msg.size);
        Usb_Cdc::send(&msg);
        return;
    }

    DO::disable_nv();
    DO::disable_pull_up();
    DO::set_config_output(parsered_cmd.output_type);

    if (parsered_cmd.output_type == DO::DO_Config_Output_Type::DISCRETE)
    {
        //DO::set_state_output(DO::get_state_output());

        response_usb_cdc += "OK. Output type is DISCRETE\n";
        msg.size = response_usb_cdc.size();
        memcpy(msg.data, response_usb_cdc.c_str(), msg.size);
        Usb_Cdc::send(&msg);
        response_usb_cdc.clear();

        return;
    }
    else if (parsered_cmd.output_type == DO::DO_Config_Output_Type::FREQ)
    {
        //DO::set_freq(DO::get_freq());

        response_usb_cdc += "OK. Output type is FREQ\n";
        msg.size = response_usb_cdc.size();
        memcpy(msg.data, response_usb_cdc.c_str(), msg.size);
        Usb_Cdc::send(&msg);
        response_usb_cdc.clear();

        return;
    }

    return;
}

void GET_CONFIG_OUTPUT(etl::string<MAX_SIZE_USB_CDC_MESSAGE> &cmd)
{
    DO::DO_Config_Output_Type output_type = DO::get_config_output();
    UsbCdcMessage msg = {};

    if (output_type == DO::DO_Config_Output_Type::DISCRETE)
    {
        response_usb_cdc += "OK. Output type is DISCRETE\n";
        msg.size = response_usb_cdc.size();
        memcpy(msg.data, response_usb_cdc.c_str(), msg.size);
        Usb_Cdc::send(&msg);
        response_usb_cdc.clear();
    }
    else if (output_type == DO::DO_Config_Output_Type::FREQ)
    {
        response_usb_cdc += "OK. Output type is FREQ\n";
        msg.size = response_usb_cdc.size();
        memcpy(msg.data, response_usb_cdc.c_str(), msg.size);
        Usb_Cdc::send(&msg);
        response_usb_cdc.clear();
    }

    return;
}

struct SetFreqCommand
{
    uint32_t freq;
};

static bool parse_uint32(etl::string_view str, uint32_t &value)
{
    if (str.empty())
        return false;

    uint32_t result = 0;

    for (char c : str)
    {
        if (c < '0' || c > '9')
            return false;

        uint32_t digit = static_cast<uint32_t>(c - '0');

        // Проверка переполнения uint32_t
        if (result > (UINT32_MAX - digit) / 10)
            return false;

        result = result * 10 + digit;
    }

    value = result;
    return true;
}

ParseResult inline parse_set_freq(
    etl::string_view message,
    SetFreqCommand &command)
{
    // Ищем пробелы и выделяем 5 токенов:
    //
    // set_freq
    // freq

    uint8_t number_of_tokens = 2U;
    etl::string_view tokens[number_of_tokens];

    size_t token_count = 0;
    size_t start = 0;

    for (size_t i = 0; i <= message.size(); ++i)
    {
        if (i == message.size() || message[i] == ' ' || message[i] == '\n')
        {
            if (i > start)
            {
                if (token_count >= 5)
                    return ParseResult::WrongArgumentCount;

                tokens[token_count++] =
                    message.substr(start, i - start);
            }

            start = i + 1;
        }
    }

    if (token_count != number_of_tokens)
        return ParseResult::WrongArgumentCount;

    // Проверяем имя команды
    if (tokens[0] != "set_freq")
        return ParseResult::InvalidCommand;

    if (!parse_uint32(tokens[1], command.freq))
        return ParseResult::InvalidArgument;

    return ParseResult::Ok;
}

void SET_FREQ(etl::string<MAX_SIZE_USB_CDC_MESSAGE> &cmd)
{
    SetFreqCommand parsered_cmd = {};
    UsbCdcMessage msg = {};

    ParseResult state = parse_set_freq(cmd, parsered_cmd);

    if (state == ParseResult::InvalidArgument)
    {
        msg.size = sizeof(str_inv_arg_usb_cdc);
        memcpy(msg.data, str_inv_arg_usb_cdc, msg.size);
        Usb_Cdc::send(&msg);
        return;
    }

    if (state == ParseResult::WrongArgumentCount)
    {
        msg.size = sizeof(str_wrong_arg_cnt_usb_cdc);
        memcpy(msg.data, str_wrong_arg_cnt_usb_cdc, msg.size);
        Usb_Cdc::send(&msg);
        return;
    }

    DO::set_freq(parsered_cmd.freq);

    response_usb_cdc += "OK. Set freq=";
    etl::to_string(parsered_cmd.freq, response_usb_cdc, true);
    response_usb_cdc += "\n";
    msg.size = response_usb_cdc.size();
    memcpy(msg.data, response_usb_cdc.c_str(), msg.size);
    Usb_Cdc::send(&msg);
    response_usb_cdc.clear();
    return;
}

void GET_FREQ(etl::string<MAX_SIZE_USB_CDC_MESSAGE> &cmd)
{
    UsbCdcMessage msg = {};
    uint16_t freq = DO::get_freq();

    response_usb_cdc += "Freq = ";
    etl::to_string(freq, response_usb_cdc, true);
    response_usb_cdc += "\n";
    msg.size = response_usb_cdc.size();
    memcpy(msg.data, response_usb_cdc.c_str(), msg.size);
    Usb_Cdc::send(&msg);
    response_usb_cdc.clear();

    return;
}

struct SetStateOutputCommand
{
    DO::DO_State_Output output_state;
};

ParseResult inline parse_set_state_output(
    etl::string_view message,
    SetStateOutputCommand &command)
{
    // Ищем пробелы и выделяем 2 токенов:
    //
    // set_state_output
    // output_state

    uint8_t number_of_tokens = 2U;
    etl::string_view tokens[number_of_tokens];

    size_t token_count = 0;
    size_t start = 0;

    for (size_t i = 0; i <= message.size(); ++i)
    {
        if (i == message.size() || message[i] == ' ' || message[i] == '\n')
        {
            if (i > start)
            {
                if (token_count >= number_of_tokens)
                    return ParseResult::WrongArgumentCount;

                tokens[token_count++] =
                    message.substr(start, i - start);
            }

            start = i + 1;
        }
    }

    if (token_count != number_of_tokens)
        return ParseResult::WrongArgumentCount;

    // Проверяем имя команды
    if (tokens[0] != "set_state_output")
        return ParseResult::InvalidCommand;

    if (tokens[1] == "high")
    {
        command.output_state = DO::DO_State_Output::HIGH;
        return ParseResult::Ok;
    }
    else if (tokens[1] == "low")
    {
        command.output_state = DO::DO_State_Output::LOW;
        return ParseResult::Ok;
    }
    else
    {
        return ParseResult::InvalidArgument;
    }

    return ParseResult::Ok;
}

void SET_STATE_OUTPUT(etl::string<MAX_SIZE_USB_CDC_MESSAGE> &cmd)
{
    SetStateOutputCommand parsered_cmd = {};
    UsbCdcMessage msg = {};

    ParseResult state = parse_set_state_output(cmd, parsered_cmd);

    if (state == ParseResult::InvalidArgument)
    {
        msg.size = sizeof(str_inv_arg_usb_cdc);
        memcpy(msg.data, str_inv_arg_usb_cdc, msg.size);
        Usb_Cdc::send(&msg);
        return;
    }

    if (state == ParseResult::WrongArgumentCount)
    {
        msg.size = sizeof(str_wrong_arg_cnt_usb_cdc);
        memcpy(msg.data, str_wrong_arg_cnt_usb_cdc, msg.size);
        Usb_Cdc::send(&msg);
        return;
    }

    DO::set_state_output(parsered_cmd.output_state);

    if (parsered_cmd.output_state == DO::DO_State_Output::HIGH)
    {
        response_usb_cdc += "OK. Set HIGH\n";
        msg.size = response_usb_cdc.size();
        memcpy(msg.data, response_usb_cdc.c_str(), msg.size);
        Usb_Cdc::send(&msg);
        response_usb_cdc.clear();
        return;
    }
    else if (parsered_cmd.output_state == DO::DO_State_Output::LOW)
    {
        response_usb_cdc += "OK. Set LOW\n";
        msg.size = response_usb_cdc.size();
        memcpy(msg.data, response_usb_cdc.c_str(), msg.size);
        Usb_Cdc::send(&msg);
        response_usb_cdc.clear();
        return;
    }

    return;
}

void GET_STATE_OUTPUT(etl::string<MAX_SIZE_USB_CDC_MESSAGE> &cmd)
{
    UsbCdcMessage msg = {};
    DO::DO_State_Output output_state = DO::get_state_output();

    if (output_state == DO::DO_State_Output::HIGH)
    {
        response_usb_cdc += "HIGH\n";
        msg.size = response_usb_cdc.size();
        memcpy(msg.data, response_usb_cdc.c_str(), msg.size);
        Usb_Cdc::send(&msg);
        response_usb_cdc.clear();
        return;
    }
    else if (output_state == DO::DO_State_Output::LOW)
    {
        response_usb_cdc += "LOW\n";
        msg.size = response_usb_cdc.size();
        memcpy(msg.data, response_usb_cdc.c_str(), msg.size);
        Usb_Cdc::send(&msg);
        response_usb_cdc.clear();
        return;
    }

    return;
}

void ENABLE_PULL_UP(etl::string<MAX_SIZE_USB_CDC_MESSAGE> &cmd)
{
    UsbCdcMessage msg = {};
    DO::enable_pull_up();

    response_usb_cdc += "OK\n";
    msg.size = response_usb_cdc.size();
    memcpy(msg.data, response_usb_cdc.c_str(), msg.size);
    Usb_Cdc::send(&msg);
    response_usb_cdc.clear();
    return;
}

void DISABLE_PULL_UP(etl::string<MAX_SIZE_USB_CDC_MESSAGE> &cmd)
{
    UsbCdcMessage msg = {};
    DO::disable_pull_up();

    response_usb_cdc += "OK\n";
    msg.size = response_usb_cdc.size();
    memcpy(msg.data, response_usb_cdc.c_str(), msg.size);
    Usb_Cdc::send(&msg);
    response_usb_cdc.clear();

    return;
}

void ENABLE_NV(etl::string<MAX_SIZE_USB_CDC_MESSAGE> &cmd)
{
    UsbCdcMessage msg = {};
    DO::enable_nv();

    response_usb_cdc += "OK\n";
    msg.size = response_usb_cdc.size();
    memcpy(msg.data, response_usb_cdc.c_str(), msg.size);
    Usb_Cdc::send(&msg);
    response_usb_cdc.clear();

    return;
}

void DISABLE_NV(etl::string<MAX_SIZE_USB_CDC_MESSAGE> &cmd)
{
    UsbCdcMessage msg = {};
    DO::disable_nv();

    response_usb_cdc += "OK\n";
    msg.size = response_usb_cdc.size();
    memcpy(msg.data, response_usb_cdc.c_str(), msg.size);
    Usb_Cdc::send(&msg);
    response_usb_cdc.clear();

    return;
}

void SET_ACTIVE_SENSOR(etl::string<MAX_SIZE_USB_CDC_MESSAGE> &cmd)
{
    UsbCdcMessage msg = {};
    DO::disable_pull_up();
    DO::disable_nv();

    response_usb_cdc += "OK\n";
    msg.size = response_usb_cdc.size();
    memcpy(msg.data, response_usb_cdc.c_str(), msg.size);
    Usb_Cdc::send(&msg);
    response_usb_cdc.clear();

    return;
}

void SET_PASSIVE_SENSOR(etl::string<MAX_SIZE_USB_CDC_MESSAGE> &cmd)
{
    UsbCdcMessage msg = {};
    DO::enable_pull_up();
    DO::enable_nv();

    response_usb_cdc += "OK\n";
    msg.size = response_usb_cdc.size();
    memcpy(msg.data, response_usb_cdc.c_str(), msg.size);
    Usb_Cdc::send(&msg);
    response_usb_cdc.clear();

    return;
}

void HELP(etl::string<MAX_SIZE_USB_CDC_MESSAGE> &cmd)
{
    UsbCdcMessage msg = {};

    for (uint8_t i = 0; i < NUMBER_OF_USB_CDC_HANDLERS - 1U; i++)
    {
        response_usb_cdc += UsbCdcHandlers[i].id;
        response_usb_cdc += "\n";

        msg.size = response_usb_cdc.size();
        memcpy(msg.data, response_usb_cdc.c_str(), msg.size);
        while (Usb_Cdc::send(&msg) != Status::OK)
            ;
        response_usb_cdc.clear();
    }

    return;
}

void UsbCdcCommmandHandler(UsbCdcMessage *usb_cdc_rx_message)
{
    etl::string<MAX_SIZE_USB_CDC_MESSAGE> str(reinterpret_cast<char *>(usb_cdc_rx_message->data), usb_cdc_rx_message->size);

    etl::string_view cmd_name;

    etl::to_lower_case(str);

    for (size_t i = 0; i <= str.size(); ++i)
    {
        if (i == str.size() || str[i] == ' ' || str[i] == '\n')
        {
            cmd_name = str.substr(0, i);
            break;
        }
    }

    for (uint8_t i = 0; i < NUMBER_OF_USB_CDC_HANDLERS; i++)
    {
        if (cmd_name.compare(reinterpret_cast<const char *>(UsbCdcHandlers[i].id)) == 0)
        {
            UsbCdcHandlers[i].handler(str);
            continue;
        }
    }
    return;
}
