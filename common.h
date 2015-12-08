typedef int currency;
typedef int artefakt;
typedef artefakt kolekcja;

const int museum_action_want_talk = 1;

typedef struct {
  long type;
  int action;   // must be 1
} museum_want_talk;
