// Nondeterministic finite automaton (NFA) implementation

#include <stdbool.h>

#include "include/nfa.h"
#include "include/log.h"

// -------------------- Main --------------------
int main(int argc, char **argv) {

    set_log_level(L_DEBUG);

    // 'a.*b' hardcoded NFA for testing
    NFAState* s0 = create_state(0,0); // First state for 'a.*b'
    NFAState* s1 = create_state(1,0);  
    NFAState* s2 = create_state(2,0);
    NFAState* s3 = create_state(3,0);
    NFAState* s4 = create_state(4,0);
    NFAState* s5 = create_state(5,1); // final state

    add_transition(s0,'a',s1);   // S0 -- 'a' --> S1
    add_transition(s1,0,s2);     // S1 -- epsilon --> S2
    add_transition(s2,'.',s3);   // S2 -- '.' --> S3
    add_transition(s2,0,s3);     // S2 -- epsilon --> S3
    add_transition(s3,'.',s3);   // S3 -- '.' --> S3
    add_transition(s3,0,s4);     // S3 -- epsilon --> S4
    add_transition(s4,'b',s5);   // S4 -- 'b' --> S5 (FINAL)

    print_states_set("NFA States from s0", &(StateSet){ .states = (NFAState*[]){s0,s1,s2,s3,s4,s5}, .count=6 });

    char **input_array = argv;

    for(int i=1;i<argc;i++) {
        INFO("===== Test %d: '%s' =====", i+1, input_array[i]);
        bool matched = match_nfa(s0, input_array[i]);
        INFO("Result: '%s' => %s", input_array[i], matched?"MATCH":"NO MATCH");
    }


    free_state(s0);
    free_state(s1);
    free_state(s2);
    free_state(s3);
    free_state(s4);
    free_state(s5);

    return 0;
}


