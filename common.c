#include "common.h"

const key_t museum_key1   = 321654668;
const key_t museum_key2   = 654165468;
const key_t bank_key1     = 656987466;
const key_t bank_key2     = 925554557;
const key_t company_key1  = 984984556;
const key_t company_key2  = 346511236;

const long bank_ip          = 1;
const long museum_ip        = 2;
const long company_base_ip  = 1000000000;
// 3 .. (3 + #Firm - 1)   firmy
// (3 + #Firm) ...        workerzy
const long delegate_base_ip = 1500000000;

// GENERAL

const char ACT_NONE = 1;
const char ACT_OK = 2;
const char ACT_ERROR = 3;
const char ACT_REJECT = 4;

// BANK

const char ACT_BANK_GET_SALDO = 0x11;
const char ACT_BANK_CHANGE_SALDO = 0x15;
const char ACT_BANK_COMPANY_EXIT = 0x12;
const char ACT_BANK_EXIT_ALL_COMPANIES = 0x13;
const char ACT_BANK_EXIT = 0x14;

// MUSEUM

const char ACT_MUSEUM_WITH_BANK_CONNECT_REQUEST = 0x20;
const char ACT_MUSEUM_GET_ESTIMATIONS = 0x21;
const char ACT_MUSEUM_COMPANY_EXIT = 0x22;
const char ACT_MUSEUM_BUY_TERRAIN = 0x23;
const char ACT_MUSEUM_REPORT_INIT = 0x24;
const char ACT_MUSEUM_SEND_REPORT_PART = 0x25;

// COMPANY
const char ACT_COMPANY_TO_BANK_CONNECT_REQUEST = 0x30;
// ...

// DELEGATE
const char ACT_DELEGATE_GIVE_WORK = 0x40;
const char ACT_DELEGATE_CHECK_WORK = 0x41;

const char ERR_ACCESS_DENIED = 2;
const char ERR_WRONG_MESSAGE = 3;
const char ERR_INVALID_ARGUMENT = 4;