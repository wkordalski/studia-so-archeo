#include "common.h"

const key_t museum_key1   = 321654668;
const key_t museum_key2   = 654165468;
const key_t bank_key1     = 656987466;
const key_t bank_key2     = 925554557;
const key_t company_key1  = 984984556;
const key_t company_key2  = 346511236;

const long bank_ip          = 1;
const long museum_ip        = 2;
// 3 .. (3 + #Firm - 1)   firmy
// (3 + #Firm) ...        workerzy
const long delegate_base_ip = 65798470;

const char ACT_NONE = 1;
const char ACT_OK = 2;
const char ACT_ERROR = 3;

const char ACT_BANK_WITH_MUSEUM_CONNECT_REQUEST = 10;

// idx (int), id (int) -> saldo (long long int)
const char ACT_BANK_GET_SALDO = 11;

const char ERR_ACCESS_DENIED = 2;
