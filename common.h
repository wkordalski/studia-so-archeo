typedef int currency;
typedef int artefakt;
typedef artefakt kolekcja;

#include <sys/types.h>
#include <sys/ipc.h>

const key_t key_input = 321654668;
const key_t key_output = 654165468;

const long museum_ip = 65798468;
const long bank_ip = 65798469;
const long delegate_base_ip = 65798470;

const char ACT_OK = 1;
const char ACT_BANK_CONNECT = 10;