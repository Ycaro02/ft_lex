#include "../include/log.h"
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


/* Global state ID counter */
int *get_state_id_counter() {
    static int state_id = 0;
    return (&state_id);
}

/**
 * @brief Match the input string against the regex tree
 * @param r The root of the regex tree
 * @param str The input string to match
 * @return pointer to the position in the string after matching, or NULL if no match
 */
char *match_regex(RegexTreeNode* r, char *str) {

    char *next = NULL;

    if (!r) return (str); // Empty regex matches empty string

    if (!str) return (NULL); // No string to match against


    switch (r->type) {
        case REG_CHAR:
            DBG("Matching CHAR '%c' against '%c'\n", r->c, *str);
            if (*str == r->c || (*str != '\0' && r->c == '.')) {
                DBG("Matched CHAR '%c'\n", r->c);
                next = str + 1;
            } else {
                DBG("Failed to match CHAR '%c'\n", r->c);
            }
            break;
        case REG_CLASS: {
            WARN("REG_CLASS matching not implemented yet\n");
            return (NULL);
        }
        case REG_CONCAT: {
            DBG("Matching CONCAT against '%c'\n", *str);
            next = match_regex(r->left, str);
            if (next) {
                DBG("Matched CONCAT left part\n");
                next = match_regex(r->right, next);
            }
            break;
        }
        case REG_ALT: {
            DBG("Matching ALT against '%c'\n", *str);
            next = match_regex(r->left, str);
            if (!next) {
                next = match_regex(r->right, str);
            }
            break;
        }
        default: {
            ERR("Unknown regex node type\n");
            return (NULL);
        }
    }

    switch (r->op) {
        case OP_NONE:
            return (next);
        case OP_STAR: {
            char *temp = next;
            while (temp) {
                str = temp;
                temp = match_regex(r, str);
                if (temp == str) {
                    return (str); // Prevent infinite loop
                }

            }
            return (str);
        }
        case OP_PLUS: {
            if (!next) return (NULL);
            char *temp = next;
            while (temp) {
                str = temp;
                temp = match_regex(r, str);
            }
            return (str);
        }
        case OP_OPTIONAL: {
            DBG("Matching OPTIONAL Str: '%s', r->c = '%c'\n", str, r->c);
            return (next ? next : str);
        }
        
        default:
            break;
    }

    return (next);
}

void match_regex_by_tree(RegexTreeNode *tree, char *input) {
    char *tmp = input;
    char *old = NULL;
    char *res = NULL;

    while (tmp && *tmp) {
        DBG("Attempting to match at: '%s'\n", tmp);
        
        old = tmp;
        res = match_regex(tree, tmp);
        

        if (res) {
            int len = res - old;
            if (len <= 0) {
                WARN("Zero-length match, advancing by one to avoid infinite loop\n");
                res = old + 1;
                tmp++;
                continue;
            }
            char matched[BUFF_SIZE] = {0};
            strncpy(matched, old, len);
            INFO("=====================================\n");
            INFO(PURPLE"Match Rule: %s\n"RESET, matched);
            INFO("=====================================\n");
            tmp = res;
        } else {
            DBG("No match at: '%s'\n", tmp);
            tmp++;
        }
        DBG("Continuing match at: '%s'\n", tmp);
    }

}

int main(int argc, char* argv[]) {
    
    set_log_level(L_INFO);

    // (void)argc;
    // parse_class_exp(argv[1]);

    if (argc < 3) {
        INFO("Usage: %s <regex> <str_to_parse>\n", argv[0]);
        return (1);
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
    
    if (tree) {
        print_regex_tree(tree);
        INFO("Parsing completed successfully!\n");
        INFO("Final position: %d/%d\n", s.pos, (int)strlen(s.str));
        INFO("=====================================\n");
        
    } else {
        ERR("Failed to parse regex!\n");
    }

    (void)input;

    // set_log_level(L_DEBUG);
    NFAFragment frag = thompson_from_tree(tree);
    
    // print_nfa_state(frag.start);
    
    // Create unique global final state
    NFAState *final = create_state(g_state_id++, 1); // mark as final

    // Connect all outputs to this final state
    for (int i = 0; i < frag.out->count; i++) {
        add_transition(frag.out->states[i], 0, final); // ε transition
        DBG("Connecting state s%d to final state s%d\n", frag.out->states[i]->id, final->id);
        // print_nfa_state(frag.out->states[i]);
    }

    // print_states_set("Fragment output states", frag.out);

    match_nfa_anywhere(frag.start, input);
    // match_regex_by_tree(tree, input);
    RegexTreeNode_free(tree);

    return (0);
}

