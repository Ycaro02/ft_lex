#include "../include/log.h"
#include "../include/regex_tree.h"


void char_bitmap_display(Bitmap *b) {
    INFO("Character Bitmap: ");
    for (u32 i = 0; i < BITMAP_SIZE(b->size) ; i++) {
        if (bitmap_is_set(b, i)) {
            if (i >= 32 && i <= 126) {
                printf("%c ", (char)i);
            } else {
                printf("%d ", i);
            }
        }
    }
    printf("\n");
}

ClassDef *init_class() {
    ClassDef *class = malloc(sizeof(ClassDef));
    if (!class) {
        ERR("Failed to allocate memory for ClassDef\n");
        return (0);
    }
    class->reverse_match = 0;
    bitmap_init(&class->char_bitmap, 4); // 4 * 64 = 256 bits for ASCII
    return (class);
}

void free_class(ClassDef *class) {
    if (class) {
        free(class->char_bitmap.bits);
        free(class);
    }
}

ClassDef *class_exp_to_bitmap(char *exp) {
    ClassDef *class = init_class();
    if (!class) {
        ERR("Failed to initialize class definition\n");
        return (NULL);
    }


    INFO("Parsing class expression: '%s'\n", exp);

    int i = 0;

    // if (exp[i] == '[') {
    //     i++;
    // } else {
    //     ERR("Invalid class expression, missing '[' at the start\n");
    //     free_class(class);
    //     return (NULL);
    // }

    if (exp[i] == '^') {
        class->reverse_match = 1;
        i++;
    }

    int exp_len = strlen(exp);

    while (exp[i] != 0) {
        INFO("Processing exp[%d] = '%c'\n", i, exp[i]);

        if (i + 2 < exp_len && (exp[i + 1] == '-' && exp[i + 2] != ']' && exp[i] != '\0')) {
            if (exp[i] > exp[i + 2]) {
                ERR("Invalid range in class expression: '%c-%c'\n", exp[i], exp[i + 2]);
                free_class(class);
                return (NULL);
            }
            for (char c = exp[i]; c <= exp[i + 2]; c++) {
                DBG("Adding char '%c' to class\n", c);
                bitmap_set(&class->char_bitmap, (u32)c);
            }
            i += 3;
        } else {
            if (exp[i] == ']' && exp[i + 1] == 0) {
                break;
            }
            DBG("Else case adding char '%c' to class\n", exp[i]);
            bitmap_set(&class->char_bitmap, (u32)exp[i]);
            i++;
        }
    }

    char_bitmap_display(&class->char_bitmap);
    INFO("Reverse match: %s\n", class->reverse_match ? "true" : "false");
    return (class);
}


char *class_to_string(ClassDef *class) {
    static char buff[512];
    bzero(buff, sizeof(buff));

    if (!class) {
        snprintf(buff, sizeof(buff), "NULL");
        return (buff);
    }

    if (class->reverse_match) {
        snprintf(buff + strlen(buff), sizeof(buff) - strlen(buff), "^");
    }

    // snprintf(buff + strlen(buff), sizeof(buff) - strlen(buff), "[");

    for (u32 i = 0; i < BITMAP_SIZE(class->char_bitmap.size); i++) {
        if (bitmap_is_set(&class->char_bitmap, i)) {
            if (i >= 32 && i <= 126) {
                snprintf(buff + strlen(buff), sizeof(buff) - strlen(buff), "%c", (char)i);
            } else {
                snprintf(buff + strlen(buff), sizeof(buff) - strlen(buff), "<%d>", i);
            }
        }
    }

    // snprintf(buff + strlen(buff), sizeof(buff) - strlen(buff), "]");

    return (buff);
}

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
        node->class = class_exp_to_bitmap(str);
        if (!node->class) {
            ERR("Memory allocation failed\n");
            free(node);
            return (NULL);
        }
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
    free_class(node->class);
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
            printf("CLASS [%s]%s\n", r->class ? class_to_string(r->class) : "", get_operator_display(r->op)); 
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