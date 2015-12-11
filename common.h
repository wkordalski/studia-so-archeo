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
// 3 .. (3 + #Firm - 1)   firmy
// (3 + #Firm) ...        workerzy
extern const long delegate_base_ip;

extern const char ACT_NONE;
extern const char ACT_OK;
extern const char ACT_ERROR;

extern const char ACT_BANK_WITH_MUSEUM_CONNECT_REQUEST;

// idx (int), id (int) -> saldo (long long int)
extern const char ACT_BANK_GET_SALDO;

extern const char ERR_ACCESS_DENIED;


#endif /* end of include guard: _ARCHEO_COMMON_H_ */
