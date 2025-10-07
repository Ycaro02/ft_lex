#include "../include/log.h"


#define BUFF_SIZE (1024*1024)

typedef enum RegexType{
    REG_CHAR,           /* simple char ex: 'a' */
    REG_CLASS,          /* [abc] or [a-z] or [0-9] */
    REG_CLASS_NEG,      /* [^abc] or [^a-z] or [^0-9] */
    REG_CONCAT,         /* AB */
    REG_ALT,            /* A|B */
    REG_STAR,           /* A* */
    REG_PLUS,           /* A+ */
    REG_OPTIONAL,       /* A? */
} RegexType;


typedef struct RegexTreeNode_s {
    RegexType               type;             /* type of the node */
    struct RegexTreeNode_s  *left;            /* left child */
    struct RegexTreeNode_s  *right;           /* right child */
    char                    *class;           /* for character classes like [0-9] */
    char                    c;                /* for single characters */
} RegexTreeNode;


typedef struct String {
    char *str;
    int  pos;
    int  len;
} String;

/**
 * @brief Get the length of the input string
 * @return Length of the input string
 */
int len(String *s) {
    return (s->len);
}

/**
 * @brief Peek at the current character in the input without consuming it
 * @return The current character
 */
char peek(String *s) {
    return (s->str[s->pos]);
}


/**
 * @brief Consume and return the next character in the input
 * @return The next character
 */
char next(String *s) {
    return (s->str[s->pos++]);
}

/**
 * @brief Check if we've reached the end of the input
 * @return 1 if at end, 0 otherwise
 */
int end(String *s) {
    return (s->str[s->pos] == '\0');
}

RegexTreeNode* parse_regex(String *s);
RegexTreeNode* parse_alt(String *s);
RegexTreeNode* parse_concat(String *s);
RegexTreeNode* parse_repeat(String *s);
RegexTreeNode* parse_atom(String *s);
RegexTreeNode* parse_class(String *s);

/**
 * @brief Create a new regex tree node
 * @param type The type of the node
 * @param left The left child
 * @param right The right child
 * @param str The string for character classes
 * @param c The character for single character nodes
 * @return Pointer to the newly created node
 */
RegexTreeNode* RegexTreeNode_create(RegexType type, RegexTreeNode *left, RegexTreeNode *right, char *str, char c) {
    RegexTreeNode *node = malloc(sizeof(RegexTreeNode));
    if (!node) {
        ERR("Memory allocation failed\n");
        return (NULL);
    }
    node->type = type;
    node->left = left;
    node->right = right;
    node->class = NULL;
    if (str) {
        node->class = malloc(strlen(str) + 1);
        if (!node->class) {
            ERR("Memory allocation failed\n");
            free(node);
            return (NULL);
        }
        strcpy(node->class, str);
    }
    node->c = c;
    return (node);
}

/**
 * @brief Free the regex tree
 * @param node The root of the regex tree
 */
void RegexTreeNode_free(RegexTreeNode *node) {
    if (!node) return;
    RegexTreeNode_free(node->left);
    RegexTreeNode_free(node->right);
    free(node->class);
    free(node);
}

/**
 * @brief Parse an atom: either a character class, a parenthesized expression, or a single character
 * @return The root of the atom subtree
 */
RegexTreeNode* parse_atom(String *s) {
    // First try to parse a character class
    RegexTreeNode* class_node = parse_class(s);
    if (class_node) {
        return class_node;
    }
    
    // Then try to parse parentheses
    if (peek(s) == '(') {
        next(s); // skip '('
        RegexTreeNode* r = parse_regex(s);
        if (peek(s) == ')') next(s); // skip ')'
        return r;
    } else {
        char c = next(s);
        return RegexTreeNode_create(REG_CHAR, NULL, NULL, NULL, c);
    }
}

/**
 * @brief Parse a character class like [abc] or [^abc]
 * @return The root of the character class subtree
 */
RegexTreeNode* parse_class(String *s) {
    if (peek(s) == '[') {
        next(s); // skip '['
        int neg = 0;
        if (peek(s) == '^') {
            neg = 1;
            next(s); // skip '^'
        }

        char buffer[256] = {0};
        int idx = 0;

        if (len(s) >= 255) {
            ERR("Character class too long\n");
            return (NULL);
        }

        while (!end(s) && peek(s) != ']') {
            buffer[idx++] = next(s);
        }
        buffer[idx] = '\0';

        if (peek(s) == ']') next(s); // skip ']'

        if (neg) {
            return RegexTreeNode_create(REG_CLASS_NEG, NULL, NULL, buffer, 0);
        } else {
            return RegexTreeNode_create(REG_CLASS, NULL, NULL, buffer, 0);
        }
    }

    return (NULL);
}

/**
 * @brief Parse repetition operators (*, +, ?)
 * @return The root of the repetition subtree
 */
RegexTreeNode* parse_repeat(String *s) {
    RegexTreeNode* atom = parse_atom(s);
    char c = peek(s);

    if (c == '*') {
        next(s);
        return RegexTreeNode_create(REG_STAR, atom, NULL, NULL, 0);
    } else if (c == '+') {
        next(s);
        return RegexTreeNode_create(REG_PLUS, atom, NULL, NULL, 0);
    } else if (c == '?') {
        next(s);
        return RegexTreeNode_create(REG_OPTIONAL, atom, NULL, NULL, 0);
    }
    return (atom);
}

/**
 * @brief Parse concatenation (AB)
 * @return The root of the concatenation subtree
 */
RegexTreeNode* parse_concat(String *s) {
    RegexTreeNode* left = parse_repeat(s);

    while (!end(s)) {
        char c = peek(s);
        if (c == ')' || c == '|') break; // end concatenation on ')' or '|'

        RegexTreeNode* right = parse_repeat(s);
        left = RegexTreeNode_create(REG_CONCAT, left, right, NULL, 0);
    }

    return (left);
}

/**
 * @brief Parse alternation (A|B)
 * @return The root of the alternation subtree
 */
RegexTreeNode* parse_alt(String *s) {
    RegexTreeNode* left = parse_concat(s);

    while (peek(s) == '|') {
        next(s); // skip '|'
        RegexTreeNode* right = parse_concat(s);
        left = RegexTreeNode_create(REG_ALT, left, right, NULL, 0);
    }

    return (left);
}

/**
 * @brief Parse the full regex
 * @return The root of the regex tree
 */
RegexTreeNode* parse_regex(String *s) {
    if (end(s)) return (NULL);

    return (parse_alt(s));
}


/**
 * @brief Helper function to print the regex tree
 * @param r The root of the regex tree
 * @param prefix The prefix string for formatting
 * @param is_last Whether this node is the last child
 */
void print_regex_tree(RegexTreeNode* r, char* prefix, int is_last) {
    if (!r) return;
    
    printf("%s", prefix);
    printf("%s", is_last ? "└── " : "├── ");
    
    switch (r->type) {
        case REG_CHAR: 
            printf("CHAR('%c')\n", r->c); 
            break;
        case REG_CONCAT: 
            printf("CONCAT\n"); 
            break;
        case REG_ALT: 
            printf("ALT (|)\n"); 
            break;
        case REG_STAR: 
            printf("STAR (*)\n"); 
            break;
        case REG_PLUS: 
            printf("PLUS (+)\n"); 
            break;
        case REG_OPTIONAL: 
            printf("OPTIONAL (?)\n"); 
            break;
        case REG_CLASS: 
            printf("CLASS [%s]\n", r->class ? r->class : ""); 
            break;
        case REG_CLASS_NEG: 
            printf("CLASS_NEG [^%s]\n", r->class ? r->class : ""); 
            break;
        default: 
            printf("UNKNOWN\n"); 
            break;
    }
    
    // Prepare prefix for children
    char new_prefix[256] = {};
    snprintf(new_prefix, sizeof(new_prefix), "%s%s", prefix, is_last ? "    " : "│   ");
    
    if (r->left) {
        print_regex_tree(r->left, new_prefix, r->right == NULL);
    }
    if (r->right) {
        print_regex_tree(r->right, new_prefix, 1);
    }
}

/**
 * @brief Print the regex tree
 * @param r The root of the regex tree
 */
void print_regex(RegexTreeNode* r) {
    if (!r) {
        printf("Empty tree\n");
        return;
    }
    printf("Regex Tree:\n");
    print_regex_tree(r, "", 1);
    printf("\n");
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
        print_regex(tree);
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

