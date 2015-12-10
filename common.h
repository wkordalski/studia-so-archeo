typedef int currency;
typedef int artefakt;
typedef artefakt kolekcja;

#include <sys/types.h>
#include <sys/ipc.h>

const key_t museum_key1 = 321654668;
const key_t museum_key2 = 654165468;
const key_t bank_key1   = 656987466;
const key_t bank_key2   = 925554557;

const long museum_ip        = 65798468;
const long bank_ip          = 65798469;
const long delegate_base_ip = 65798470;

const char ACT_NONE = 1;
const char ACT_OK = 2;

const char ACT_BANK_WITH_MUSEUM_CONNECT_REQUEST = 10;
