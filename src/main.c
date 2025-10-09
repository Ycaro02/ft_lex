#include "../include/log.h"
#include "../include/bitmap.h"
#include "../include/nfa.h"

// typedef struct ClassDef {
//     char charset[1024];
//     s8   reverse_match;
// } ClassDef;

// s8 parse_class_exp(char *exp) {
//     ClassDef class_def = { .charset = "", .reverse_match = 0 };
//     int i = 0, j = 0;

//     if (exp[i] == '^') {
//         class_def.reverse_match = 1;
//         i++;
//     }

//     int exp_len = strlen(exp);

//     while (exp[i] != 0) {

//         if (i + 2 < exp_len) {
//             if (exp[i + 1] == '-' && exp[i + 2] && exp[i] < exp[i + 2]) {
//                 for (char c = exp[i]; c <= exp[i + 2]; c++) {
//                     class_def.charset[j] = c;
//                     j++;
//                 }
//                 i += 3;
//             }
//         } else {
//             class_def.charset[j] = exp[i];
//             j++;
//             i++;

//         }
//     }

//     INFO("CHARSET: %s\n", class_def.charset);
//     return (1);

// }


int main(int argc, char* argv[]) {
    set_log_level(L_INFO);
    
    if (argc < 3) {
        INFO("Usage: %s <regex> <str_to_parse>\n", argv[0]);
        return 1;
    }
    
    char *input = argv[2];
    String s = {
        .str = argv[1],
        .pos = 0,
        .len = strlen(argv[1])
    };
    
    INFO("Parsing regex: '%s'\n", s.str);
    INFO("=====================================\n");
    
    RegexTreeNode *tree = parse_regex(&s);
    
    if (!tree) {
        ERR("Failed to parse regex!\n");
        return (1);
    }
    
    print_regex_tree(tree);
    INFO("Parsing completed successfully!\n");
    INFO("Final position: %d/%d\n", s.pos, (int)strlen(s.str));
    INFO("=====================================\n");
    
    nfa_init(DEFAULT_NFA_CAPACITY);
    
    NFAFragment frag = thompson_from_tree(tree);
    
    nfa_finalize(&frag);
    
    print_nfa_tree();
    INFO("=====================================\n");
    

    print_nfa();
    INFO("=====================================\n");

    INFO("Matching input: '%s'\n", input);
    match_nfa_anywhere(input);
    
    nfa_free();
    RegexTreeNode_free(tree);
    
    return (0);
}