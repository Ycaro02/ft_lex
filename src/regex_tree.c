#include "../include/log.h"
#include "../include/regex_tree.h"


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
    node->op = OP_NONE;
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


char *get_operator_string(RegexOperator op) {
    switch (op) {
        case OP_NONE: return "NONE";
        case OP_STAR: return YELLOW"STAR (*)"RESET;
        case OP_PLUS: return CYAN"PLUS (+)"RESET;
        case OP_OPTIONAL: return PURPLE"OPTIONAL (?)"RESET;
        default: return "UNKNOWN";
    }
}

char *get_operator_display(RegexOperator op) {
    static char buff[128];

    bzero(buff, sizeof(buff));

    if (op == OP_NONE) return ("");

    sprintf(buff, ": %s", get_operator_string(op));
    return (buff);
}

/**
 * @brief Helper function to print the regex tree
 * @param r The root of the regex tree
 * @param prefix The prefix string for formatting
 * @param is_last Whether this node is the last child
 */
static void print_regex_node(RegexTreeNode* r, char* prefix, int is_last) {
    if (!r) return;
    
    printf("%s", prefix);
    printf("%s", is_last ? "└── " : "├── ");
    
    switch (r->type) {
        case REG_CHAR: 
            printf("CHAR('%c')%s\n", r->c, get_operator_display(r->op)); 
            break;
        case REG_CONCAT: 
            printf("CONCAT%s\n", get_operator_display(r->op)); 
            break;
        case REG_ALT: 
            printf("ALT (|)%s\n", get_operator_display(r->op)); 
            break;
        case REG_CLASS: 
            printf("CLASS [%s]%s\n", r->class ? r->class : "", get_operator_display(r->op)); 
            break;
        default: 
            printf("UNKNOWN\n"); 
            break;
    }
    
    /* Prepare prefix for children */
    char new_prefix[256] = {};
    snprintf(new_prefix, sizeof(new_prefix), "%s%s", prefix, is_last ? "    " : "│   ");
    
    if (r->left) {
        print_regex_node(r->left, new_prefix, r->right == NULL);
    }
    if (r->right) {
        print_regex_node(r->right, new_prefix, 1);
    }
}

/**
 * @brief Print the regex tree
 * @param r The root of the regex tree
 */
void print_regex_tree(RegexTreeNode* r) {
    if (!r) {
        printf("Empty tree\n");
        return;
    }
    printf("Regex Tree:\n");
    print_regex_node(r, "", 1);
    printf("\n");
}