#include "../include/log.h"


typedef enum RegexType{
    REG_CHAR,           // simple char ex: 'a'
    REG_ANY,            // '.' = any character
    REG_CLASS,          // [abc] or [a-z]
    REG_CLASS_NEG,      // [^abc] or [^a-z]
    REG_CONCAT,         // AB
    REG_ALT,            // A|B
    REG_STAR,           // A*
    REG_PLUS,           // A+
    REG_OPTIONAL,       // A?
} RegexType;


typedef struct RegexTreeNode_s {
    RegexType               type;             // type of the node
    struct RegexTreeNode_s  *left;            // left child
    struct RegexTreeNode_s  *right;           // right child
    char                    *str;             // for character classes like [0-9]
    char                    c;                // for single characters
} RegexTreeNode;


const char  *input;
int         pos = 0;


/**
 * @brief Get the length of the input string
 * @return Length of the input string
 */
int len() {
    return (strlen(input));
}

/**
 * @brief Peek at the current character in the input without consuming it
 * @return The current character
 */
char peek() {
    return (input[pos]);
}


/**
 * @brief Consume and return the next character in the input
 * @return The next character
 */
char next() {
    return (input[pos++]);
}

/**
 * @brief Check if we've reached the end of the input
 * @return 1 if at end, 0 otherwise
 */
int end() {
    return (input[pos] == '\0');
}

RegexTreeNode* parse_regex();
RegexTreeNode* parse_alt();
RegexTreeNode* parse_concat();
RegexTreeNode* parse_repeat();
RegexTreeNode* parse_atom();
RegexTreeNode* parse_class();

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
    node->str = NULL;
    if (str) {
        node->str = malloc(strlen(str) + 1);
        if (!node->str) {
            ERR("Memory allocation failed\n");
            free(node);
            return (NULL);
        }
        strcpy(node->str, str);
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
    free(node->str);
    free(node);
}

/**
 * @brief Parse an atom: either a character class, a parenthesized expression, or a single character
 * @return The root of the atom subtree
 */
RegexTreeNode* parse_atom() {
    // First try to parse a character class
    RegexTreeNode* class_node = parse_class();
    if (class_node) {
        return class_node;
    }
    
    // Then try to parse parentheses
    if (peek() == '(') {
        next(); // skip '('
        RegexTreeNode* r = parse_regex();
        if (peek() == ')') next(); // skip ')'
        return r;
    } else {
        char c = next();
        return RegexTreeNode_create(REG_CHAR, NULL, NULL, NULL, c);
    }
}

/**
 * @brief Parse a character class like [abc] or [^abc]
 * @return The root of the character class subtree
 */
RegexTreeNode* parse_class() {
    if (peek() == '[') {
        next(); // skip '['
        int neg = 0;
        if (peek() == '^') {
            neg = 1;
            next(); // skip '^'
        }

        char buffer[256] = {0};
        int idx = 0;

        if (len() >= 255) {
            ERR("Character class too long\n");
            return (NULL);
        }

        while (!end() && peek() != ']') {
            buffer[idx++] = next();
        }
        buffer[idx] = '\0';

        if (peek() == ']') next(); // skip ']'

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
RegexTreeNode* parse_repeat() {
    RegexTreeNode* atom = parse_atom();
    char c = peek();

    if (c == '*') {
        next();
        return RegexTreeNode_create(REG_STAR, atom, NULL, NULL, 0);
    } else if (c == '+') {
        next();
        return RegexTreeNode_create(REG_PLUS, atom, NULL, NULL, 0);
    } else if (c == '?') {
        next();
        return RegexTreeNode_create(REG_OPTIONAL, atom, NULL, NULL, 0);
    }

    return (atom);
}

/**
 * @brief Parse concatenation (AB)
 * @return The root of the concatenation subtree
 */
RegexTreeNode* parse_concat() {
    RegexTreeNode* left = parse_repeat();

    while (!end()) {
        char c = peek();
        if (c == ')' || c == '|') break; // end concatenation on ')' or '|'

        RegexTreeNode* right = parse_repeat();
        left = RegexTreeNode_create(REG_CONCAT, left, right, NULL, 0);
    }

    return (left);
}

/**
 * @brief Parse alternation (A|B)
 * @return The root of the alternation subtree
 */
RegexTreeNode* parse_alt() {
    RegexTreeNode* left = parse_concat();

    while (peek() == '|') {
        next(); // skip '|'
        RegexTreeNode* right = parse_concat();
        left = RegexTreeNode_create(REG_ALT, left, right, NULL, 0);
    }

    return (left);
}

/**
 * @brief Parse the full regex
 * @return The root of the regex tree
 */
RegexTreeNode* parse_regex() {
    if (end()) return (NULL);

    if (len() == 1 && peek() == '.') {
        return RegexTreeNode_create(REG_ANY, NULL, NULL, NULL, 0);
    }

    return (parse_alt());
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
            printf("CLASS [%s]\n", r->str ? r->str : ""); 
            break;
        case REG_CLASS_NEG: 
            printf("CLASS_NEG [^%s]\n", r->str ? r->str : ""); 
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

    INFO("Scanning '%s'\n", str);

    switch (r->type) {
        case REG_CHAR:
            DBG("Matching CHAR '%c' against '%c'\n", r->c, *str);
            if (*str == r->c) {
                INFO("Matched CHAR '%c'\n", r->c);
                return (str + 1);
            }
            return (NULL);
        case REG_ANY:
            DBG("Matching ANY against '%c'\n", *str);
            if (*str != '\0') {
                INFO("Matched ANY '%c'\n", *str);
                return (str + 1);
            }
            return (NULL);
        case REG_CLASS: {
            // if (*str != '\0' && strchr(r->str, *str)) {
            //     return (str + 1);
            // }
            WARN("REG_CLASS matching not implemented yet\n");
            return (NULL);
        }
        case REG_CLASS_NEG: {
            WARN("REG_CLASS_NEG matching not implemented yet\n");
            // if (*str != '\0' && !strchr(r->str, *str)) {
            //     return (str + 1);
            // }
            return (NULL);
        }
        case REG_CONCAT: {
            DBG("Matching CONCAT against '%c'\n", *str);
            char *next_str = match_regex(r->left, str);
            if (next_str) {
                INFO("Matched CONCAT left part\n");
                return match_regex(r->right, next_str);
            }
            return (NULL);
        }
        case REG_ALT: {
            DBG("Matching ALT against '%c'\n", *str);
            char *next_str = match_regex(r->left, str);
            if (next_str) {
                INFO("Matched ALT left part\n");
                return (next_str);
            }
            return match_regex(r->right, str);
        }
        case REG_STAR: {
            DBG("Matching STAR against '%c'\n", *str);
            char *next_str = str;
            while ((next_str = match_regex(r->left, next_str)) != NULL) {
                INFO("Matched STAR\n");
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
                INFO("Matched PLUS\n");
                str = next_str;
            }
            return (str);
        }
        case REG_OPTIONAL: {
            DBG("Matching OPTIONAL against '%c'\n", *str);
            char *next_str = match_regex(r->left, str);
            if (next_str) {
                INFO("Matched OPTIONAL\n");
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


    char *str = argv[2];

    input = argv[1];
    pos = 0;

    INFO("Parsing regex: '%s'\n", input);
    INFO("=====================================\n");
    
    RegexTreeNode *tree = parse_regex();
    
    if (tree) {
        print_regex(tree);
        INFO("Parsing completed successfully!\n");
        INFO("Final position: %d/%d\n", pos, (int)strlen(input));
        INFO("=====================================\n");
        
    } else {
        INFO("Failed to parse regex!\n");
    }
    
    char *tmp = str;

    while (tmp && *tmp) {
        INFO("Attempting to match at: '%s'\n", tmp);
        
        char *old = tmp;
        char *res = match_regex(tree, tmp);
        
        if (res) {
            INFO(PURPLE"Matched up to: '%s'\n"RESET, old);
            tmp = res;
        } else {
            WARN("No match at: '%s'\n", tmp);
            tmp++;
        }
        INFO("Continuing match at: '%s'\n", tmp);
    }

    RegexTreeNode_free(tree);

    return (0);
}

