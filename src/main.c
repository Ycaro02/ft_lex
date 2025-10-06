#include "../include/log.h"


typedef enum RegexType{
    REG_CHAR,           // simple char ex: 'a'
    // REG_ANY,            // '.' = any character
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

int len() {
    return strlen(input);
}

char peek() {
    return input[pos];
}

char next() {
    return input[pos++];
}

int end() {
    return input[pos] == '\0';
}

RegexTreeNode* parse_regex();
RegexTreeNode* parse_alt();
RegexTreeNode* parse_concat();
RegexTreeNode* parse_repeat();
RegexTreeNode* parse_atom();
RegexTreeNode* parse_class();


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

void RegexTreeNode_free(RegexTreeNode *node) {
    if (!node) return;
    RegexTreeNode_free(node->left);
    RegexTreeNode_free(node->right);
    free(node->str);
    free(node);
}

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

RegexTreeNode* parse_alt() {
    RegexTreeNode* left = parse_concat();

    while (peek() == '|') {
        next(); // skip '|'
        RegexTreeNode* right = parse_concat();
        left = RegexTreeNode_create(REG_ALT, left, right, NULL, 0);
    }

    return (left);
}

RegexTreeNode* parse_regex() {
    if (end()) return (NULL);


    return parse_alt();

}

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
    char new_prefix[256];
    snprintf(new_prefix, sizeof(new_prefix), "%s%s", prefix, is_last ? "    " : "│   ");
    
    // Print children
    int has_left = (r->left != NULL);
    int has_right = (r->right != NULL);
    
    if (has_left) {
        print_regex_tree(r->left, new_prefix, !has_right);
    }
    if (has_right) {
        print_regex_tree(r->right, new_prefix, 1);
    }
}

void print_regex(RegexTreeNode* r, int depth) {
    if (!r) {
        printf("Empty tree\n");
        return;
    }
    (void)depth;

    printf("Regex Tree:\n");
    print_regex_tree(r, "", 1);
    printf("\n");
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s <regex>\n", argv[0]);
        return 1;
    }

    input = argv[1];
    pos = 0;

    printf("Parsing regex: '%s'\n", input);
    printf("=====================================\n");
    
    RegexTreeNode* tree = parse_regex();
    
    if (tree) {
        print_regex(tree, 0);
        
        printf("Parsing completed successfully!\n");
        printf("Final position: %d/%d\n", pos, (int)strlen(input));
        
        RegexTreeNode_free(tree);
    } else {
        printf("Failed to parse regex!\n");
    }
    
    return 0;
}

