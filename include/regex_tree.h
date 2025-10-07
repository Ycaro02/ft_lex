#ifndef REGEX_TREE_H
#define REGEX_TREE_H

#include "string_handler.h"

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


/* regex_tree.c */
RegexTreeNode   *RegexTreeNode_create(RegexType type, RegexTreeNode *left, RegexTreeNode *right, char *str, char c);
void            RegexTreeNode_free(RegexTreeNode *root);
void            print_regex_tree(RegexTreeNode* r);

/* regex_parser.c */
RegexTreeNode   *parse_regex(String *s);

#endif /* REGEX_TREE_H */