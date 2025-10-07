#include "../include/log.h"
#include "../include/regex_tree.h"

typedef struct ClassDef {
    char charset[1024];
    s8   reverse_match;
} ClassDef;

s8 parse_class_exp(char *exp) {
    ClassDef class_def = { .charset = "", .reverse_match = 0 };
    int i = 0, j = 0;

    if (exp[i] == '^') {
        class_def.reverse_match = 1;
        i++;
    }

    int exp_len = strlen(exp);

    while (exp[i] != 0) {

        if (i + 2 < exp_len) {
            if (exp[i + 1] == '-' && exp[i + 2] && exp[i] < exp[i + 2]) {
                for (char c = exp[i]; c <= exp[i + 2]; c++) {
                    class_def.charset[j] = c;
                    j++;
                }
                i += 3;
            }
        } else {
            class_def.charset[j] = exp[i];
            j++;
            i++;

        }
    }

    INFO("CHARSET: %s\n", class_def.charset);
    return (1);

}


/**
 * @brief Match the input string against the regex tree
 * @param r The root of the regex tree
 * @param str The input string to match
 * @return pointer to the position in the string after matching, or NULL if no match
 */
char *match_regex(RegexTreeNode* r, char *str) {

    // INFO("Scanning '%s'\n", str);

    switch (r->type) {
        case REG_CHAR:
            DBG("Matching CHAR '%c' against '%c'\n", r->c, *str);
            if (*str == r->c) {
                DBG("Matched CHAR '%c'\n", r->c);
                return (str + 1);
            } 
            else if (*str != '\0' && r->c == '.') {
                DBG("Matched CHAR ANY '%c'\n", *str);
                return (str + 1);
            }
            return (NULL);
        case REG_CLASS: {
            // if (*str != '\0' && strchr(r->class, *str)) {
            //     return (str + 1);
            // }
            WARN("REG_CLASS matching not implemented yet\n");
            return (NULL);
        }
        case REG_CLASS_NEG: {
            WARN("REG_CLASS_NEG matching not implemented yet\n");
            // if (*str != '\0' && !strchr(r->class, *str)) {
            //     return (str + 1);
            // }
            return (NULL);
        }
        case REG_CONCAT: {
            DBG("Matching CONCAT against '%c'\n", *str);
            char *next_str = match_regex(r->left, str);
            if (next_str) {
                DBG("Matched CONCAT left part\n");
                return match_regex(r->right, next_str);
            }
            return (NULL);
        }
        case REG_ALT: {
            DBG("Matching ALT against '%c'\n", *str);
            char *next_str = match_regex(r->left, str);
            if (next_str) {
                DBG("Matched ALT left part\n");
                return (next_str);
            }
            return match_regex(r->right, str);
        }
        case REG_STAR: {
            DBG("Matching STAR against '%c'\n", *str);
            char *next_str = str;
            while ((next_str = match_regex(r->left, next_str)) != NULL) {
                DBG("Matched STAR\n");
                str = next_str;
            }
            return (str);
        }
        case REG_PLUS: {
            DBG("Matching PLUS against '%c'\n", *str);
            char *next_str = match_regex(r->left, str);
            if (!next_str) return (NULL);
            str = next_str;
            while ((next_str = match_regex(r->left, str)) != NULL) {
                DBG("Matched PLUS\n");
                str = next_str;
            }
            return (str);
        }
        case REG_OPTIONAL: {
            DBG("Matching OPTIONAL against '%c'\n", *str);
            char *next_str = match_regex(r->left, str);
            if (next_str) {
                DBG("Matched OPTIONAL\n");
                return (next_str);
            }
            return (str);
        }
        default: {
            ERR("Unknown regex node type\n");
            return (NULL);
        }
    }
    return (NULL);
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
    
    char *tmp = input;

    while (tmp && *tmp) {
        DBG("Attempting to match at: '%s'\n", tmp);
        
        char *old = tmp;
        char *res = match_regex(tree, tmp);
        
        if (res) {
            int len = res - old;
            char matched[BUFF_SIZE] = {0};
            strncpy(matched, old, len);

            INFO(PURPLE"Match Rule: %s\n"RESET, matched);
            tmp = res;
        } else {
            WARN("No match at: '%s'\n", tmp);
            tmp++;
        }
        DBG("Continuing match at: '%s'\n", tmp);
    }

    RegexTreeNode_free(tree);

    return (0);
}

