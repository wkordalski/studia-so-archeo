#ifndef _ARCHEO_COMMON_H_
#define _ARCHEO_COMMON_H_

typedef int currency;
typedef int artefakt;
typedef artefakt kolekcja;

#include <sys/types.h>
#include <sys/ipc.h>

extern const key_t museum_key1;
extern const key_t museum_key2;
extern const key_t bank_key1;
extern const key_t bank_key2;
extern const key_t company_key1;
extern const key_t company_key2;

extern const long bank_ip;
extern const long museum_ip;
extern const long company_base_ip;
// 3 .. (3 + #Firm - 1)   firmy
// (3 + #Firm) ...        workerzy
extern const long delegate_base_ip;

//
// GENERAL
//

extern const char ACT_NONE;
extern const char ACT_OK;
extern const char ACT_ERROR;

//
// BANK
//

// idx (int), id (int) -> saldo (long long int)
extern const char ACT_BANK_GET_SALDO;

//
// MUSEUM
//

// () -> number of companies (int)
extern const char ACT_MUSEUM_WITH_BANK_CONNECT_REQUEST;

// l (int), p(int), t(int), b(int) -> result (int[l..p][t..b])
extern const char ACT_MUSEUM_GET_ESTIMATIONS;

//
// COMPANY
//

// () -> (all data goes here)
extern const char ACT_COMPANY_REGISTER;



extern const char ERR_ACCESS_DENIED;
extern const char ERR_WRONG_MESSAGE;
extern const char ERR_INVALID_ARGUMENT;


#endif /* end of include guard: _ARCHEO_COMMON_H_ */
